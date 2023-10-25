/*
    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resourcebase.h"
#include "agentbase_p.h"

#include "akonadifull-version.h"
#include "collectiondeletejob.h"
#include "collectionsync_p.h"
#include "relationsync.h"
#include "resourceadaptor.h"
#include "resourcescheduler_p.h"
#include "tagsync.h"
#include "tracerinterface.h"
#include <QDBusConnection>

#include "changerecorder.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "collectionmodifyjob.h"
#include "favoritecollectionattribute.h"
#include "invalidatecachejob_p.h"
#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "itemmodifyjob_p.h"
#include "monitor_p.h"
#include "recursivemover_p.h"
#include "resourceselectjob_p.h"
#include "servermanager_p.h"
#include "session.h"
#include "specialcollectionattribute.h"
#include "tagmodifyjob.h"

#include "akonadiagentbase_debug.h"

#include <cstdlib>
#include <iterator>
#include <shared/akranges.h>

#include <KAboutData>
#include <KLocalizedString>

#include <QApplication>
#include <QHash>
#include <QTimer>

using namespace Akonadi;
using namespace AkRanges;
using namespace std::chrono_literals;
class Akonadi::ResourceBasePrivate : public AgentBasePrivate
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dfaure")

public:
    explicit ResourceBasePrivate(ResourceBase *parent)
        : AgentBasePrivate(parent)
        , scheduler(nullptr)
        , mItemSyncer(nullptr)
        , mItemTransactionMode(ItemSync::SingleTransaction)
        , mItemMergeMode(ItemSync::RIDMerge)
        , mCollectionSyncer(nullptr)
        , mTagSyncer(nullptr)
        , mRelationSyncer(nullptr)
        , mHierarchicalRid(false)
        , mUnemittedProgress(0)
        , mAutomaticProgressReporting(true)
        , mDisableAutomaticItemDeliveryDone(false)
        , mItemSyncBatchSize(10)
        , mCurrentCollectionFetchJob(nullptr)
        , mScheduleAttributeSyncBeforeCollectionSync(false)
    {
        Internal::setClientType(Internal::Resource);
        mStatusMessage = defaultReadyMessage();
        mProgressEmissionCompressor.setInterval(1000);
        mProgressEmissionCompressor.setSingleShot(true);
        // HACK: skip local changes of the EntityDisplayAttribute by default. Remove this for KDE5 and adjust resource implementations accordingly.
        mKeepLocalCollectionChanges << "ENTITYDISPLAY";
    }

    ~ResourceBasePrivate() override = default;

    Q_DECLARE_PUBLIC(ResourceBase)

    void delayedInit() override
    {
        const QString serviceId = ServerManager::agentServiceName(ServerManager::Resource, mId);
        if (!QDBusConnection::sessionBus().registerService(serviceId)) {
            QString reason = QDBusConnection::sessionBus().lastError().message();
            if (reason.isEmpty()) {
                reason = QStringLiteral("this service is probably running already.");
            }
            qCCritical(AKONADIAGENTBASE_LOG) << "Unable to register service" << serviceId << "at D-Bus:" << reason;

            if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
                QCoreApplication::instance()->exit(1);
            }
        } else {
            AgentBasePrivate::delayedInit();
        }
    }

    void changeProcessed() override
    {
        if (m_recursiveMover) {
            m_recursiveMover->changeProcessed();
            QTimer::singleShot(0s, m_recursiveMover.data(), &RecursiveMover::replayNext);
            return;
        }

        mChangeRecorder->changeProcessed();
        if (!mChangeRecorder->isEmpty()) {
            scheduler->scheduleChangeReplay();
        }
        scheduler->taskDone();
    }

    void slotAbortRequested();

    void slotDeliveryDone(KJob *job);
    void slotCollectionSyncDone(KJob *job);
    void slotLocalListDone(KJob *job);
    void slotSynchronizeCollection(const Collection &col);
    void slotItemRetrievalCollectionFetchDone(KJob *job);
    void slotCollectionListDone(KJob *job);
    void slotSynchronizeCollectionAttributes(const Collection &col);
    void slotCollectionListForAttributesDone(KJob *job);
    void slotCollectionAttributesSyncDone(KJob *job);
    void slotSynchronizeTags();
    void slotSynchronizeRelations();
    void slotAttributeRetrievalCollectionFetchDone(KJob *job);

    void slotItemSyncDone(KJob *job);

    void slotPercent(KJob *job, quint64 percent);
    void slotDelayedEmitProgress();
    void slotDeleteResourceCollection();
    void slotDeleteResourceCollectionDone(KJob *job);
    void slotCollectionDeletionDone(KJob *job);

    void slotInvalidateCache(const Akonadi::Collection &collection);

    void slotPrepareItemRetrieval(const Akonadi::Item &item);
    void slotPrepareItemRetrievalResult(KJob *job);

    void slotPrepareItemsRetrieval(const QList<Akonadi::Item> &item);
    void slotPrepareItemsRetrievalResult(KJob *job);

    void changeCommittedResult(KJob *job);

    void slotRecursiveMoveReplay(RecursiveMover *mover);
    void slotRecursiveMoveReplayResult(KJob *job);

    void slotTagSyncDone(KJob *job);
    void slotRelationSyncDone(KJob *job);

    void slotSessionReconnected()
    {
        Q_Q(ResourceBase);

        new ResourceSelectJob(q->identifier());
    }

    void createItemSyncInstanceIfMissing()
    {
        Q_Q(ResourceBase);
        Q_ASSERT_X(scheduler->currentTask().type == ResourceScheduler::SyncCollection,
                   "createItemSyncInstance",
                   "Calling items retrieval methods although no item retrieval is in progress");
        if (!mItemSyncer) {
            mItemSyncer = new ItemSync(q->currentCollection());
            mItemSyncer->setTransactionMode(mItemTransactionMode);
            mItemSyncer->setBatchSize(mItemSyncBatchSize);
            mItemSyncer->setMergeMode(mItemMergeMode);
            mItemSyncer->setDisableAutomaticDeliveryDone(mDisableAutomaticItemDeliveryDone);
            mItemSyncer->setProperty("collection", QVariant::fromValue(q->currentCollection()));
            connect(mItemSyncer, &KJob::percentChanged, this,
                    &ResourceBasePrivate::slotPercent); // NOLINT(google-runtime-int): ulong comes from KJob

            connect(mItemSyncer, &KJob::result, this, &ResourceBasePrivate::slotItemSyncDone);
            connect(mItemSyncer, &ItemSync::readyForNextBatch, q, &ResourceBase::retrieveNextItemSyncBatch);
        }
        Q_ASSERT(mItemSyncer);
    }

public Q_SLOTS:
    // Dump the state of the scheduler
    Q_SCRIPTABLE QString dumpToString() const
    {
        Q_Q(const ResourceBase);
        return scheduler->dumpToString() + QLatin1Char('\n') + q->dumpResourceToString();
    }

    Q_SCRIPTABLE void dump()
    {
        scheduler->dump();
    }

    Q_SCRIPTABLE void clear()
    {
        scheduler->clear();
    }

protected Q_SLOTS:
    // reimplementations from AgentbBasePrivate, containing sanity checks that only apply to resources
    // such as making sure that RIDs are present as well as translations of cross-resource moves
    // TODO: we could possibly add recovery code for no-RID notifications by re-enquing those to the change recorder
    // as the corresponding Add notifications, although that contains a risk of endless fail/retry loops

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemAdded(item, collection);
    }

    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) override
    {
        if (item.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemChanged(item, partIdentifiers);
    }

    void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags) override
    {
        if (addedFlags.isEmpty() && removedFlags.isEmpty()) {
            changeProcessed();
            return;
        }

        const Item::List validItems = filterValidItems(items);
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsFlagsChanged(validItems, addedFlags, removedFlags);
    }

    void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags) override
    {
        if (addedTags.isEmpty() && removedTags.isEmpty()) {
            changeProcessed();
            return;
        }

        const Item::List validItems = filterValidItems(items);
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsTagsChanged(validItems, addedTags, removedTags);
    }

    // TODO move the move translation code from AgentBasePrivate here, it's wrong for agents
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &destination) override
    {
        if (item.remoteId().isEmpty() || destination.remoteId().isEmpty() || destination == source) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemMoved(item, source, destination);
    }

    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &source, const Akonadi::Collection &destination) override
    {
        if (destination.remoteId().isEmpty() || destination == source) {
            changeProcessed();
            return;
        }

        const Item::List validItems = filterValidItems(items);
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsMoved(validItems, source, destination);
    }

    void itemRemoved(const Akonadi::Item &item) override
    {
        if (item.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemRemoved(item);
    }

    void itemsRemoved(const Akonadi::Item::List &items) override
    {
        const Item::List validItems = filterValidItems(items);
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsRemoved(validItems);
    }

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override
    {
        if (parent.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionAdded(collection, parent);
    }

    void collectionChanged(const Akonadi::Collection &collection) override
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionChanged(collection);
    }

    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &partIdentifiers) override
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionChanged(collection, partIdentifiers);
    }

    void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &destination) override
    {
        // unknown destination or source == destination means we can't do/don't have to do anything
        if (destination.remoteId().isEmpty() || source == destination) {
            changeProcessed();
            return;
        }

        // inter-resource moves, requires we know which resources the source and destination are in though
        if (!source.resource().isEmpty() && !destination.resource().isEmpty() && source.resource() != destination.resource()) {
            if (source.resource() == q_ptr->identifier()) { // moved away from us
                AgentBasePrivate::collectionRemoved(collection);
            } else if (destination.resource() == q_ptr->identifier()) { // moved to us
                scheduler->taskDone(); // stop change replay for now
                auto mover = new RecursiveMover(this);
                mover->setCollection(collection, destination);
                scheduler->scheduleMoveReplay(collection, mover);
            }
            return;
        }

        // intra-resource move, requires the moved collection to have a valid id though
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }

        // intra-resource move, ie. something we can handle internally
        AgentBasePrivate::collectionMoved(collection, source, destination);
    }

    void collectionRemoved(const Akonadi::Collection &collection) override
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionRemoved(collection);
    }

    void tagAdded(const Akonadi::Tag &tag) override
    {
        if (!tag.isValid()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::tagAdded(tag);
    }

    void tagChanged(const Akonadi::Tag &tag) override
    {
        if (tag.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::tagChanged(tag);
    }

    void tagRemoved(const Akonadi::Tag &tag) override
    {
        if (tag.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::tagRemoved(tag);
    }

private:
    static Item::List filterValidItems(Item::List items)
    {
        items.erase(std::remove_if(items.begin(),
                                   items.end(),
                                   [](const auto &item) {
                                       return item.remoteId().isEmpty();
                                   }),
                    items.end());
        return items;
    }

public:
    // synchronize states
    Collection currentCollection;

    ResourceScheduler *scheduler = nullptr;
    ItemSync *mItemSyncer = nullptr;
    ItemSync::TransactionMode mItemTransactionMode;
    ItemSync::MergeMode mItemMergeMode;
    CollectionSync *mCollectionSyncer = nullptr;
    TagSync *mTagSyncer = nullptr;
    RelationSync *mRelationSyncer = nullptr;
    bool mHierarchicalRid;
    QTimer mProgressEmissionCompressor;
    int mUnemittedProgress;
    QMap<Akonadi::Collection::Id, QVariantMap> mUnemittedAdvancedStatus;
    bool mAutomaticProgressReporting;
    bool mDisableAutomaticItemDeliveryDone;
    QPointer<RecursiveMover> m_recursiveMover;
    int mItemSyncBatchSize;
    QSet<QByteArray> mKeepLocalCollectionChanges;
    KJob *mCurrentCollectionFetchJob = nullptr;
    bool mScheduleAttributeSyncBeforeCollectionSync;
};

ResourceBase::ResourceBase(const QString &id)
    : AgentBase(new ResourceBasePrivate(this), id)
{
    Q_D(ResourceBase);

    qDBusRegisterMetaType<QByteArrayList>();

    new Akonadi__ResourceAdaptor(this);

    d->scheduler = new ResourceScheduler(this);

    d->mChangeRecorder->setChangeRecordingEnabled(true);
    d->mChangeRecorder->setCollectionMoveTranslationEnabled(false); // we deal with this ourselves
    connect(d->mChangeRecorder, &ChangeRecorder::changesAdded, d->scheduler, &ResourceScheduler::scheduleChangeReplay);

    d->mChangeRecorder->setResourceMonitored(d->mId.toLatin1());
    d->mChangeRecorder->fetchCollection(true);

    connect(d->scheduler, &ResourceScheduler::executeFullSync, this, &ResourceBase::retrieveCollections);
    connect(d->scheduler, &ResourceScheduler::executeCollectionTreeSync, this, &ResourceBase::retrieveCollections);
    connect(d->scheduler, &ResourceScheduler::executeCollectionSync, d, &ResourceBasePrivate::slotSynchronizeCollection);
    connect(d->scheduler, &ResourceScheduler::executeCollectionAttributesSync, d, &ResourceBasePrivate::slotSynchronizeCollectionAttributes);
    connect(d->scheduler, &ResourceScheduler::executeTagSync, d, &ResourceBasePrivate::slotSynchronizeTags);
    connect(d->scheduler, &ResourceScheduler::executeRelationSync, d, &ResourceBasePrivate::slotSynchronizeRelations);
    connect(d->scheduler, &ResourceScheduler::executeItemFetch, d, &ResourceBasePrivate::slotPrepareItemRetrieval);
    connect(d->scheduler, &ResourceScheduler::executeItemsFetch, d, &ResourceBasePrivate::slotPrepareItemsRetrieval);
    connect(d->scheduler, &ResourceScheduler::executeResourceCollectionDeletion, d, &ResourceBasePrivate::slotDeleteResourceCollection);
    connect(d->scheduler, &ResourceScheduler::executeCacheInvalidation, d, &ResourceBasePrivate::slotInvalidateCache);
    connect(d->scheduler, &ResourceScheduler::status, this, qOverload<int, const QString &>(&ResourceBase::status));
    connect(d->scheduler, &ResourceScheduler::executeChangeReplay, d->mChangeRecorder, &ChangeRecorder::replayNext);
    connect(d->scheduler, &ResourceScheduler::executeRecursiveMoveReplay, d, &ResourceBasePrivate::slotRecursiveMoveReplay);
    connect(d->scheduler, &ResourceScheduler::fullSyncComplete, this, &ResourceBase::synchronized);
    connect(d->scheduler, &ResourceScheduler::collectionTreeSyncComplete, this, &ResourceBase::collectionTreeSynchronized);
    connect(d->mChangeRecorder, &ChangeRecorder::nothingToReplay, d->scheduler, &ResourceScheduler::taskDone);
    connect(d->mChangeRecorder, &Monitor::collectionRemoved, d->scheduler, &ResourceScheduler::collectionRemoved);
    connect(this, &ResourceBase::abortRequested, d, &ResourceBasePrivate::slotAbortRequested);
    connect(this, &ResourceBase::synchronized, d->scheduler, &ResourceScheduler::taskDone);
    connect(this, &ResourceBase::collectionTreeSynchronized, d->scheduler, &ResourceScheduler::taskDone);
    connect(this, &AgentBase::agentNameChanged, this, &ResourceBase::nameChanged);
    connect(&d->mProgressEmissionCompressor, &QTimer::timeout, d, &ResourceBasePrivate::slotDelayedEmitProgress);

    d->scheduler->setOnline(d->mOnline);
    if (!d->mChangeRecorder->isEmpty()) {
        d->scheduler->scheduleChangeReplay();
    }

    new ResourceSelectJob(identifier());

    connect(d->mChangeRecorder->session(), &Session::reconnected, d, &ResourceBasePrivate::slotSessionReconnected);
}

ResourceBase::~ResourceBase() = default;

void ResourceBase::synchronize()
{
    d_func()->scheduler->scheduleFullSync();
}

void ResourceBase::setName(const QString &name)
{
    AgentBase::setAgentName(name);
}

QString ResourceBase::name() const
{
    return AgentBase::agentName();
}

QString ResourceBase::parseArguments(int argc, char **argv)
{
    Q_UNUSED(argc)

    QCommandLineOption identifierOption(QStringLiteral("identifier"), i18nc("@label command line option", "Resource identifier"), QStringLiteral("argument"));
    QCommandLineParser parser;
    parser.addOption(identifierOption);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*qApp);
    parser.setApplicationDescription(i18n("Akonadi Resource"));

    if (!parser.isSet(identifierOption)) {
        qCDebug(AKONADIAGENTBASE_LOG) << "Identifier argument missing";
        exit(1);
    }

    const QString identifier = parser.value(identifierOption);

    if (identifier.isEmpty()) {
        qCDebug(AKONADIAGENTBASE_LOG) << "Identifier is empty";
        exit(1);
    }

    QCoreApplication::setApplicationName(ServerManager::addNamespace(identifier));
    QCoreApplication::setApplicationVersion(QStringLiteral(AKONADI_FULL_VERSION));

    const QFileInfo fi(QString::fromLocal8Bit(argv[0]));
    // strip off full path and possible .exe suffix
    const QString catalog = fi.baseName();

    auto translator = new QTranslator(qApp);
    translator->load(catalog);
    QCoreApplication::installTranslator(translator);

    return identifier;
}

int ResourceBase::init(ResourceBase &r)
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("libakonadi5"));
    KAboutData::setApplicationData(r.aboutData());
    return qApp->exec();
}

void ResourceBasePrivate::slotAbortRequested()
{
    Q_Q(ResourceBase);

    scheduler->cancelQueues();
    q->abortActivity();
}

void ResourceBase::itemRetrieved(const Item &item)
{
    Q_D(ResourceBase);
    Q_ASSERT(d->scheduler->currentTask().type == ResourceScheduler::FetchItem);
    if (!item.isValid()) {
        d->scheduler->itemFetchDone(i18nc("@info", "Invalid item retrieved"));
        return;
    }

    const QSet<QByteArray> requestedParts = d->scheduler->currentTask().itemParts;
    for (const QByteArray &part : requestedParts) {
        if (!item.loadedPayloadParts().contains(part)) {
            qCWarning(AKONADIAGENTBASE_LOG) << "Item does not provide part" << part;
        }
    }

    auto job = new ItemModifyJob(item);
    job->d_func()->setSilent(true);
    // FIXME: remove once the item with which we call retrieveItem() has a revision number
    job->disableRevisionCheck();
    connect(job, &KJob::result, d, &ResourceBasePrivate::slotDeliveryDone);
}

void ResourceBasePrivate::slotDeliveryDone(KJob *job)
{
    Q_Q(ResourceBase);
    Q_ASSERT(scheduler->currentTask().type == ResourceScheduler::FetchItem);
    if (job->error()) {
        Q_EMIT q->error(i18nc("@info", "Error while creating item: %1", job->errorString()));
    }
    scheduler->itemFetchDone(QString());
}

void ResourceBase::collectionAttributesRetrieved(const Collection &collection)
{
    Q_D(ResourceBase);
    Q_ASSERT(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionAttributes);
    if (!collection.isValid()) {
        Q_EMIT attributesSynchronized(d->scheduler->currentTask().collection.id());
        d->scheduler->taskDone();
        return;
    }

    auto job = new CollectionModifyJob(collection);
    connect(job, &KJob::result, d, &ResourceBasePrivate::slotCollectionAttributesSyncDone);
}

void ResourceBasePrivate::slotCollectionAttributesSyncDone(KJob *job)
{
    Q_Q(ResourceBase);
    Q_ASSERT(scheduler->currentTask().type == ResourceScheduler::SyncCollectionAttributes);
    if (job->error()) {
        Q_EMIT q->error(i18nc("@info", "Error while updating collection: %1", job->errorString()));
    }
    Q_EMIT q->attributesSynchronized(scheduler->currentTask().collection.id());
    scheduler->taskDone();
}

void ResourceBasePrivate::slotDeleteResourceCollection()
{
    Q_Q(ResourceBase);

    auto job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel);
    job->fetchScope().setResource(q->identifier());
    connect(job, &KJob::result, this, &ResourceBasePrivate::slotDeleteResourceCollectionDone);
}

void ResourceBasePrivate::slotDeleteResourceCollectionDone(KJob *job)
{
    Q_Q(ResourceBase);
    if (job->error()) {
        Q_EMIT q->error(job->errorString());
        scheduler->taskDone();
    } else {
        const auto fetchJob = static_cast<const CollectionFetchJob *>(job);

        if (!fetchJob->collections().isEmpty()) {
            auto job = new CollectionDeleteJob(fetchJob->collections().at(0));
            connect(job, &KJob::result, this, &ResourceBasePrivate::slotCollectionDeletionDone);
        } else {
            // there is no resource collection, so just ignore the request
            scheduler->taskDone();
        }
    }
}

void ResourceBasePrivate::slotCollectionDeletionDone(KJob *job)
{
    Q_Q(ResourceBase);
    if (job->error()) {
        Q_EMIT q->error(job->errorString());
    }

    scheduler->taskDone();
}

void ResourceBasePrivate::slotInvalidateCache(const Akonadi::Collection &collection)
{
    Q_Q(ResourceBase);
    auto job = new InvalidateCacheJob(collection, q);
    connect(job, &KJob::result, scheduler, &ResourceScheduler::taskDone);
}

void ResourceBase::changeCommitted(const Item &item)
{
    changesCommitted(Item::List() << item);
}

void ResourceBase::changesCommitted(const Item::List &items)
{
    Q_D(ResourceBase);
    auto transaction = new TransactionSequence(this);
    connect(transaction, &KJob::finished, d, &ResourceBasePrivate::changeCommittedResult);

    // Modify the items one-by-one, because STORE does not support mass RID change
    for (const Item &item : items) {
        auto job = new ItemModifyJob(item, transaction);
        job->d_func()->setClean();
        job->disableRevisionCheck(); // TODO: remove, but where/how do we handle the error?
        job->setIgnorePayload(true); // we only want to reset the dirty flag and update the remote id
    }
}

void ResourceBase::changeCommitted(const Collection &collection)
{
    Q_D(ResourceBase);
    auto job = new CollectionModifyJob(collection);
    connect(job, &KJob::result, d, &ResourceBasePrivate::changeCommittedResult);
}

void ResourceBasePrivate::changeCommittedResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADIAGENTBASE_LOG) << job->errorText();
    }

    Q_Q(ResourceBase);
    if (qobject_cast<CollectionModifyJob *>(job)) {
        if (job->error()) {
            Q_EMIT q->error(i18nc("@info", "Updating local collection failed: %1.", job->errorText()));
        }
        mChangeRecorder->d_ptr->invalidateCache(static_cast<CollectionModifyJob *>(job)->collection());
    } else {
        if (job->error()) {
            Q_EMIT q->error(i18nc("@info", "Updating local items failed: %1.", job->errorText()));
        }
        // Item and tag cache is invalidated by modify job
    }

    changeProcessed();
}

void ResourceBase::changeCommitted(const Tag &tag)
{
    Q_D(ResourceBase);
    auto job = new TagModifyJob(tag);
    connect(job, &KJob::result, d, &ResourceBasePrivate::changeCommittedResult);
}

void ResourceBase::requestItemDelivery(const QList<qint64> &uids, const QByteArrayList &parts)
{
    Q_D(ResourceBase);
    if (!isOnline()) {
        const QString errorMsg = i18nc("@info", "Cannot fetch item in offline mode.");
        sendErrorReply(QDBusError::Failed, errorMsg);
        Q_EMIT error(errorMsg);
        return;
    }

    setDelayedReply(true);

    const auto items = uids | Views::transform([](const auto uid) {
                           return Item{uid};
                       })
        | Actions::toQVector;

    const QSet<QByteArray> partSet = QSet<QByteArray>(parts.begin(), parts.end());
    d->scheduler->scheduleItemsFetch(items, partSet, message());
}

void ResourceBase::collectionsRetrieved(const Collection::List &collections)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree || d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::collectionsRetrieved()",
               "Calling collectionsRetrieved() although no collection retrieval is in progress");
    if (!d->mCollectionSyncer) {
        d->mCollectionSyncer = new CollectionSync(identifier());
        d->mCollectionSyncer->setHierarchicalRemoteIds(d->mHierarchicalRid);
        d->mCollectionSyncer->setKeepLocalChanges(d->mKeepLocalCollectionChanges);
        connect(d->mCollectionSyncer, &KJob::percentChanged, d,
                &ResourceBasePrivate::slotPercent); // NOLINT(google-runtime-int): ulong comes from KJob
        connect(d->mCollectionSyncer, &KJob::result, d, &ResourceBasePrivate::slotCollectionSyncDone);
    }
    d->mCollectionSyncer->setRemoteCollections(collections);
}

void ResourceBase::collectionsRetrievedIncremental(const Collection::List &changedCollections, const Collection::List &removedCollections)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree || d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::collectionsRetrievedIncremental()",
               "Calling collectionsRetrievedIncremental() although no collection retrieval is in progress");
    if (!d->mCollectionSyncer) {
        d->mCollectionSyncer = new CollectionSync(identifier());
        d->mCollectionSyncer->setHierarchicalRemoteIds(d->mHierarchicalRid);
        d->mCollectionSyncer->setKeepLocalChanges(d->mKeepLocalCollectionChanges);
        connect(d->mCollectionSyncer, &KJob::percentChanged, d,
                &ResourceBasePrivate::slotPercent); // NOLINT(google-runtime-int): ulong comes from KJob
        connect(d->mCollectionSyncer, &KJob::result, d, &ResourceBasePrivate::slotCollectionSyncDone);
    }
    d->mCollectionSyncer->setRemoteCollections(changedCollections, removedCollections);
}

void ResourceBase::setCollectionStreamingEnabled(bool enable)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree || d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::setCollectionStreamingEnabled()",
               "Calling setCollectionStreamingEnabled() although no collection retrieval is in progress");
    if (!d->mCollectionSyncer) {
        d->mCollectionSyncer = new CollectionSync(identifier());
        d->mCollectionSyncer->setHierarchicalRemoteIds(d->mHierarchicalRid);
        connect(d->mCollectionSyncer, &KJob::percentChanged, d,
                &ResourceBasePrivate::slotPercent); // NOLINT(google-runtime-int): ulong comes from KJob
        connect(d->mCollectionSyncer, &KJob::result, d, &ResourceBasePrivate::slotCollectionSyncDone);
    }
    d->mCollectionSyncer->setStreamingEnabled(enable);
}

void ResourceBase::collectionsRetrievalDone()
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree || d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::collectionsRetrievalDone()",
               "Calling collectionsRetrievalDone() although no collection retrieval is in progress");
    // streaming enabled, so finalize the sync
    if (d->mCollectionSyncer) {
        d->mCollectionSyncer->retrievalDone();
    } else {
        // user did the sync himself, we are done now
        // FIXME: we need the same special case for SyncAll as in slotCollectionSyncDone here!
        d->scheduler->taskDone();
    }
}

void ResourceBase::setKeepLocalCollectionChanges(const QSet<QByteArray> &parts)
{
    Q_D(ResourceBase);
    d->mKeepLocalCollectionChanges = parts;
}

void ResourceBasePrivate::slotCollectionSyncDone(KJob *job)
{
    Q_Q(ResourceBase);
    mCollectionSyncer = nullptr;
    if (job->error()) {
        if (job->error() != Job::UserCanceled) {
            Q_EMIT q->error(job->errorString());
        }
    } else {
        if (scheduler->currentTask().type == ResourceScheduler::SyncAll) {
            auto list = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
            list->setFetchScope(q->changeRecorder()->collectionFetchScope());
            list->fetchScope().fetchAttribute<SpecialCollectionAttribute>();
            list->fetchScope().fetchAttribute<FavoriteCollectionAttribute>();
            list->fetchScope().setResource(mId);
            list->fetchScope().setListFilter(CollectionFetchScope::Sync);
            connect(list, &KJob::result, this, &ResourceBasePrivate::slotLocalListDone);
            return;
        } else if (scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree) {
            scheduler->scheduleCollectionTreeSyncCompletion();
        }
    }
    scheduler->taskDone();
}

namespace
{
bool sortCollectionsForSync(const Collection &l, const Collection &r)
{
    const auto lType = l.hasAttribute<SpecialCollectionAttribute>() ? l.attribute<SpecialCollectionAttribute>()->collectionType() : QByteArray();
    const bool lInbox = (lType == "inbox") || (QStringView(l.remoteId()).mid(1).compare(QLatin1String("inbox"), Qt::CaseInsensitive) == 0);
    const bool lFav = l.hasAttribute<FavoriteCollectionAttribute>();

    const auto rType = r.hasAttribute<SpecialCollectionAttribute>() ? r.attribute<SpecialCollectionAttribute>()->collectionType() : QByteArray();
    const bool rInbox = (rType == "inbox") || (QStringView(r.remoteId()).mid(1).compare(QLatin1String("inbox"), Qt::CaseInsensitive) == 0);
    const bool rFav = r.hasAttribute<FavoriteCollectionAttribute>();

    // inbox is always first
    if (lInbox) {
        return true;
    } else if (rInbox) {
        return false;
    }

    // favorites right after inbox
    if (lFav) {
        return !rInbox;
    } else if (rFav) {
        return lInbox;
    }

    // trash is always last (unless it's favorite)
    if (lType == "trash") {
        return false;
    } else if (rType == "trash") {
        return true;
    }

    // Fallback to sorting by id
    return l.id() < r.id();
}

} // namespace

void ResourceBasePrivate::slotLocalListDone(KJob *job)
{
    Q_Q(ResourceBase);
    if (job->error()) {
        Q_EMIT q->error(job->errorString());
    } else {
        Collection::List cols = static_cast<CollectionFetchJob *>(job)->collections();
        std::sort(cols.begin(), cols.end(), sortCollectionsForSync);
        for (const Collection &col : std::as_const(cols)) {
            scheduler->scheduleSync(col);
        }
        scheduler->scheduleFullSyncCompletion();
    }
    scheduler->taskDone();
}

void ResourceBasePrivate::slotSynchronizeCollection(const Collection &col)
{
    Q_Q(ResourceBase);
    currentCollection = col;
    // This can happen due to FetchHelper::triggerOnDemandFetch() in the akonadi server (not an error).
    if (!col.remoteId().isEmpty()) {
        // check if this collection actually can contain anything
        QStringList contentTypes = currentCollection.contentMimeTypes();
        contentTypes.removeAll(Collection::mimeType());
        contentTypes.removeAll(Collection::virtualMimeType());
        if (!contentTypes.isEmpty() || col.isVirtual()) {
            if (mAutomaticProgressReporting) {
                Q_EMIT q->status(AgentBase::Running, i18nc("@info:status", "Syncing folder '%1'", currentCollection.displayName()));
            }

            qCDebug(AKONADIAGENTBASE_LOG) << "Preparing collection sync of collection" << currentCollection.id() << currentCollection.displayName();
            auto fetchJob = new Akonadi::CollectionFetchJob(col, CollectionFetchJob::Base, this);
            fetchJob->setFetchScope(q->changeRecorder()->collectionFetchScope());
            connect(fetchJob, &KJob::result, this, &ResourceBasePrivate::slotItemRetrievalCollectionFetchDone);
            mCurrentCollectionFetchJob = fetchJob;
            return;
        }
    }
    scheduler->taskDone();
}

void ResourceBasePrivate::slotItemRetrievalCollectionFetchDone(KJob *job)
{
    Q_Q(ResourceBase);
    mCurrentCollectionFetchJob = nullptr;
    if (job->error()) {
        qCWarning(AKONADIAGENTBASE_LOG) << "Failed to retrieve collection for sync: " << job->errorString();
        q->cancelTask(i18n("Failed to retrieve collection for sync."));
        return;
    }
    auto fetchJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    const Collection::List collections = fetchJob->collections();
    if (collections.isEmpty()) {
        qCWarning(AKONADIAGENTBASE_LOG) << "The fetch job returned empty collection set. This is unexpected.";
        q->cancelTask(i18n("Failed to retrieve collection for sync."));
        return;
    }
    q->retrieveItems(collections.at(0));
}

int ResourceBase::itemSyncBatchSize() const
{
    Q_D(const ResourceBase);
    return d->mItemSyncBatchSize;
}

void ResourceBase::setItemSyncBatchSize(int batchSize)
{
    Q_D(ResourceBase);
    d->mItemSyncBatchSize = batchSize;
}

void ResourceBase::setScheduleAttributeSyncBeforeItemSync(bool enable)
{
    Q_D(ResourceBase);
    d->mScheduleAttributeSyncBeforeCollectionSync = enable;
}

void ResourceBasePrivate::slotSynchronizeCollectionAttributes(const Collection &col)
{
    Q_Q(ResourceBase);
    auto fetchJob = new Akonadi::CollectionFetchJob(col, CollectionFetchJob::Base, this);
    fetchJob->setFetchScope(q->changeRecorder()->collectionFetchScope());
    connect(fetchJob, &KJob::result, this, &ResourceBasePrivate::slotAttributeRetrievalCollectionFetchDone);
    Q_ASSERT(!mCurrentCollectionFetchJob);
    mCurrentCollectionFetchJob = fetchJob;
}

void ResourceBasePrivate::slotAttributeRetrievalCollectionFetchDone(KJob *job)
{
    mCurrentCollectionFetchJob = nullptr;
    Q_Q(ResourceBase);
    if (job->error()) {
        qCWarning(AKONADIAGENTBASE_LOG) << "Failed to retrieve collection for attribute sync: " << job->errorString();
        q->cancelTask(i18n("Failed to retrieve collection for attribute sync."));
        return;
    }
    auto fetchJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    // FIXME: Why not call q-> directly?
    QMetaObject::invokeMethod(q, "retrieveCollectionAttributes", Q_ARG(Akonadi::Collection, fetchJob->collections().at(0)));
}

void ResourceBasePrivate::slotSynchronizeTags()
{
    Q_Q(ResourceBase);
    QMetaObject::invokeMethod(this, [q] {
        q->retrieveTags();
    });
}

void ResourceBasePrivate::slotSynchronizeRelations()
{
    Q_Q(ResourceBase);
    QMetaObject::invokeMethod(this, [q] {
        q->retrieveRelations();
    });
}

void ResourceBasePrivate::slotPrepareItemRetrieval(const Item &item)
{
    Q_Q(ResourceBase);
    auto fetch = new ItemFetchJob(item, this);
    // we always need at least parent so we can use ItemCreateJob to merge
    fetch->fetchScope().setAncestorRetrieval(qMax(ItemFetchScope::Parent, q->changeRecorder()->itemFetchScope().ancestorRetrieval()));
    fetch->fetchScope().setCacheOnly(true);
    fetch->fetchScope().setFetchRemoteIdentification(true);

    // copy list of attributes to fetch
    const QSet<QByteArray> attributes = q->changeRecorder()->itemFetchScope().attributes();
    for (const auto &attribute : attributes) {
        fetch->fetchScope().fetchAttribute(attribute);
    }

    connect(fetch, &KJob::result, this, &ResourceBasePrivate::slotPrepareItemRetrievalResult);
}

void ResourceBasePrivate::slotPrepareItemRetrievalResult(KJob *job)
{
    Q_Q(ResourceBase);
    Q_ASSERT_X(scheduler->currentTask().type == ResourceScheduler::FetchItem,
               "ResourceBasePrivate::slotPrepareItemRetrievalResult()",
               "Preparing item retrieval although no item retrieval is in progress");
    if (job->error()) {
        q->cancelTask(job->errorText());
        return;
    }
    auto fetch = qobject_cast<ItemFetchJob *>(job);
    if (fetch->items().count() != 1) {
        q->cancelTask(i18n("The requested item no longer exists"));
        return;
    }
    const QSet<QByteArray> parts = scheduler->currentTask().itemParts;
    if (!q->retrieveItem(fetch->items().at(0), parts)) {
        q->cancelTask();
    }
}

void ResourceBasePrivate::slotPrepareItemsRetrieval(const QList<Item> &items)
{
    Q_Q(ResourceBase);
    auto fetch = new ItemFetchJob(items, this);
    // we always need at least parent so we can use ItemCreateJob to merge
    fetch->fetchScope().setAncestorRetrieval(qMax(ItemFetchScope::Parent, q->changeRecorder()->itemFetchScope().ancestorRetrieval()));
    fetch->fetchScope().setCacheOnly(true);
    fetch->fetchScope().setFetchRemoteIdentification(true);
    // It's possible that one or more items were removed before this task was
    // executed, so ignore it and just handle the rest.
    fetch->fetchScope().setIgnoreRetrievalErrors(true);

    // copy list of attributes to fetch
    const QSet<QByteArray> attributes = q->changeRecorder()->itemFetchScope().attributes();
    for (const auto &attribute : attributes) {
        fetch->fetchScope().fetchAttribute(attribute);
    }

    connect(fetch, &KJob::result, this, &ResourceBasePrivate::slotPrepareItemsRetrievalResult);
}

void ResourceBasePrivate::slotPrepareItemsRetrievalResult(KJob *job)
{
    Q_Q(ResourceBase);
    Q_ASSERT_X(scheduler->currentTask().type == ResourceScheduler::FetchItems,
               "ResourceBasePrivate::slotPrepareItemsRetrievalResult()",
               "Preparing items retrieval although no items retrieval is in progress");
    if (job->error()) {
        q->cancelTask(job->errorText());
        return;
    }
    auto fetch = qobject_cast<ItemFetchJob *>(job);
    const auto items = fetch->items();
    if (items.isEmpty()) {
        q->cancelTask();
        return;
    }

    const QSet<QByteArray> parts = scheduler->currentTask().itemParts;
    Q_ASSERT(items.first().parentCollection().isValid());
    if (!q->retrieveItems(items, parts)) {
        q->cancelTask();
    }
}

void ResourceBasePrivate::slotRecursiveMoveReplay(RecursiveMover *mover)
{
    Q_ASSERT(mover);
    Q_ASSERT(!m_recursiveMover);
    m_recursiveMover = mover;
    connect(mover, &KJob::result, this, &ResourceBasePrivate::slotRecursiveMoveReplayResult);
    mover->start();
}

void ResourceBasePrivate::slotRecursiveMoveReplayResult(KJob *job)
{
    Q_Q(ResourceBase);
    m_recursiveMover = nullptr;

    if (job->error()) {
        q->deferTask();
        return;
    }

    changeProcessed();
}

void ResourceBase::itemsRetrievalDone()
{
    Q_D(ResourceBase);
    // streaming enabled, so finalize the sync
    if (d->mItemSyncer) {
        d->mItemSyncer->deliveryDone();
    } else {
        if (d->scheduler->currentTask().type == ResourceScheduler::FetchItems) {
            d->scheduler->currentTask().sendDBusReplies(QString());
        }
        // user did the sync himself, we are done now
        d->scheduler->taskDone();
    }
}

void ResourceBase::clearCache()
{
    Q_D(ResourceBase);
    d->scheduler->scheduleResourceCollectionDeletion();
}

void ResourceBase::invalidateCache(const Collection &collection)
{
    Q_D(ResourceBase);
    d->scheduler->scheduleCacheInvalidation(collection);
}

Collection ResourceBase::currentCollection() const
{
    Q_D(const ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollection,
               "ResourceBase::currentCollection()",
               "Trying to access current collection although no item retrieval is in progress");
    return d->currentCollection;
}

Item ResourceBase::currentItem() const
{
    Q_D(const ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::FetchItem,
               "ResourceBase::currentItem()",
               "Trying to access current item although no item retrieval is in progress");
    return d->scheduler->currentTask().items[0];
}

Item::List ResourceBase::currentItems() const
{
    Q_D(const ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::FetchItems,
               "ResourceBase::currentItems()",
               "Trying to access current items although no items retrieval is in progress");
    return d->scheduler->currentTask().items;
}

void ResourceBase::synchronizeCollectionTree()
{
    d_func()->scheduler->scheduleCollectionTreeSync();
}

void ResourceBase::synchronizeTags()
{
    d_func()->scheduler->scheduleTagSync();
}

void ResourceBase::synchronizeRelations()
{
    d_func()->scheduler->scheduleRelationSync();
}

void ResourceBase::cancelTask()
{
    Q_D(ResourceBase);
    if (d->mCurrentCollectionFetchJob) {
        d->mCurrentCollectionFetchJob->kill();
        d->mCurrentCollectionFetchJob = nullptr;
    }
    switch (d->scheduler->currentTask().type) {
    case ResourceScheduler::FetchItem:
        itemRetrieved(Item()); // sends the error reply and
        break;
    case ResourceScheduler::FetchItems:
        itemsRetrieved(Item::List());
        break;
    case ResourceScheduler::ChangeReplay:
        d->changeProcessed();
        break;
    case ResourceScheduler::SyncCollectionTree:
    case ResourceScheduler::SyncAll:
        if (d->mCollectionSyncer) {
            d->mCollectionSyncer->rollback();
        } else {
            d->scheduler->taskDone();
        }
        break;
    case ResourceScheduler::SyncCollection:
        if (d->mItemSyncer) {
            d->mItemSyncer->rollback();
        } else {
            d->scheduler->taskDone();
        }
        break;
    default:
        d->scheduler->taskDone();
    }
}

void ResourceBase::cancelTask(const QString &msg)
{
    cancelTask();

    Q_EMIT error(msg);
}

void ResourceBase::deferTask()
{
    Q_D(ResourceBase);
    qCDebug(AKONADIAGENTBASE_LOG) << "Deferring task" << d->scheduler->currentTask();
    // Deferring a CollectionSync is just not implemented.
    // We'd need to d->mItemSyncer->rollback() but also to NOT call taskDone in slotItemSyncDone() here...
    Q_ASSERT(!d->mItemSyncer);
    d->scheduler->deferTask();
}

void ResourceBase::doSetOnline(bool state)
{
    d_func()->scheduler->setOnline(state);
}

void ResourceBase::synchronizeCollection(qint64 collectionId)
{
    synchronizeCollection(collectionId, false);
}

void ResourceBase::synchronizeCollection(qint64 collectionId, bool recursive)
{
    Q_D(ResourceBase);
    auto job = new CollectionFetchJob(Collection(collectionId), recursive ? CollectionFetchJob::Recursive : CollectionFetchJob::Base);
    job->setFetchScope(changeRecorder()->collectionFetchScope());
    job->fetchScope().setResource(identifier());
    job->fetchScope().setListFilter(CollectionFetchScope::Sync);
    connect(job, &KJob::result, d, &ResourceBasePrivate::slotCollectionListDone);
}

void ResourceBasePrivate::slotCollectionListDone(KJob *job)
{
    if (!job->error()) {
        const Collection::List list = static_cast<CollectionFetchJob *>(job)->collections();
        for (const Collection &collection : list) {
            // We also get collections that should not be synced but are part of the tree.
            if (collection.shouldList(Collection::ListSync)) {
                if (mScheduleAttributeSyncBeforeCollectionSync) {
                    scheduler->scheduleAttributesSync(collection);
                }
                scheduler->scheduleSync(collection);
            }
        }
    } else {
        qCWarning(AKONADIAGENTBASE_LOG) << "Failed to fetch collection for collection sync: " << job->errorString();
    }
}

void ResourceBase::synchronizeCollectionAttributes(const Akonadi::Collection &col)
{
    Q_D(ResourceBase);
    d->scheduler->scheduleAttributesSync(col);
}

void ResourceBase::synchronizeCollectionAttributes(qint64 collectionId)
{
    Q_D(ResourceBase);
    auto job = new CollectionFetchJob(Collection(collectionId), CollectionFetchJob::Base);
    job->setFetchScope(changeRecorder()->collectionFetchScope());
    job->fetchScope().setResource(identifier());
    connect(job, &KJob::result, d, &ResourceBasePrivate::slotCollectionListForAttributesDone);
}

void ResourceBasePrivate::slotCollectionListForAttributesDone(KJob *job)
{
    if (!job->error()) {
        const Collection::List list = static_cast<CollectionFetchJob *>(job)->collections();
        if (!list.isEmpty()) {
            const Collection &col = list.first();
            scheduler->scheduleAttributesSync(col);
        }
    }
    // TODO: error handling
}

void ResourceBase::setTotalItems(int amount)
{
    qCDebug(AKONADIAGENTBASE_LOG) << amount;
    Q_D(ResourceBase);
    setItemStreamingEnabled(true);
    if (d->mItemSyncer) {
        d->mItemSyncer->setTotalItems(amount);
    }
}

void ResourceBase::setDisableAutomaticItemDeliveryDone(bool disable)
{
    Q_D(ResourceBase);
    if (d->mItemSyncer) {
        d->mItemSyncer->setDisableAutomaticDeliveryDone(disable);
    }
    d->mDisableAutomaticItemDeliveryDone = disable;
}

void ResourceBase::setItemStreamingEnabled(bool enable)
{
    Q_D(ResourceBase);
    d->createItemSyncInstanceIfMissing();
    if (d->mItemSyncer) {
        d->mItemSyncer->setStreamingEnabled(enable);
    }
}

void ResourceBase::itemsRetrieved(const Item::List &items)
{
    Q_D(ResourceBase);
    if (d->scheduler->currentTask().type == ResourceScheduler::FetchItems) {
        auto trx = new TransactionSequence(this);
        connect(trx, &KJob::result, d, &ResourceBasePrivate::slotItemSyncDone);
        for (const Item &item : items) {
            Q_ASSERT(item.parentCollection().isValid());
            if (item.isValid()) { // NOLINT(bugprone-branch-clone)
                new ItemModifyJob(item, trx);
            } else if (!item.remoteId().isEmpty()) {
                auto job = new ItemCreateJob(item, item.parentCollection(), trx);
                job->setMerge(ItemCreateJob::RID);
            } else {
                // This should not happen, but just to be sure...
                new ItemModifyJob(item, trx);
            }
        }
        trx->commit();
    } else {
        d->createItemSyncInstanceIfMissing();
        if (d->mItemSyncer) {
            d->mItemSyncer->setFullSyncItems(items);
        }
    }
}

void ResourceBase::itemsRetrievedIncremental(const Item::List &changedItems, const Item::List &removedItems)
{
    Q_D(ResourceBase);
    d->createItemSyncInstanceIfMissing();
    if (d->mItemSyncer) {
        d->mItemSyncer->setIncrementalSyncItems(changedItems, removedItems);
    }
}

void ResourceBasePrivate::slotItemSyncDone(KJob *job)
{
    mItemSyncer = nullptr;
    Q_Q(ResourceBase);
    if (job->error() && job->error() != Job::UserCanceled) {
        Q_EMIT q->error(job->errorString());
    }
    if (scheduler->currentTask().type == ResourceScheduler::FetchItems) {
        scheduler->currentTask().sendDBusReplies((job->error() && job->error() != Job::UserCanceled) ? job->errorString() : QString());
    }
    scheduler->taskDone();
}

void ResourceBasePrivate::slotDelayedEmitProgress()
{
    Q_Q(ResourceBase);
    if (mAutomaticProgressReporting) {
        Q_EMIT q->percent(mUnemittedProgress);

        for (const QVariantMap &statusMap : std::as_const(mUnemittedAdvancedStatus)) {
            Q_EMIT q->advancedStatus(statusMap);
        }
    }
    mUnemittedProgress = 0;
    mUnemittedAdvancedStatus.clear();
}

void ResourceBasePrivate::slotPercent(KJob *job, quint64 percent)
{
    mUnemittedProgress = static_cast<int>(percent);

    const auto collection = job->property("collection").value<Collection>();
    if (collection.isValid()) {
        QVariantMap statusMap;
        statusMap.insert(QStringLiteral("key"), QStringLiteral("collectionSyncProgress"));
        statusMap.insert(QStringLiteral("collectionId"), collection.id());
        statusMap.insert(QStringLiteral("percent"), static_cast<unsigned int>(percent));

        mUnemittedAdvancedStatus[collection.id()] = statusMap;
    }
    // deliver completion right away, intermediate progress at 1s intervals
    if (percent == 100U) {
        mProgressEmissionCompressor.stop();
        slotDelayedEmitProgress();
    } else if (!mProgressEmissionCompressor.isActive()) {
        mProgressEmissionCompressor.start();
    }
}

void ResourceBase::setHierarchicalRemoteIdentifiersEnabled(bool enable)
{
    Q_D(ResourceBase);
    d->mHierarchicalRid = enable;
}

void ResourceBase::scheduleCustomTask(QObject *receiver, const char *method, const QVariant &argument, SchedulePriority priority)
{
    Q_D(ResourceBase);
    d->scheduler->scheduleCustomTask(receiver, method, argument, priority);
}

void ResourceBase::taskDone()
{
    Q_D(ResourceBase);
    d->scheduler->taskDone();
}

void ResourceBase::retrieveCollectionAttributes(const Collection &collection)
{
    collectionAttributesRetrieved(collection);
}

void ResourceBase::retrieveTags()
{
    Q_D(ResourceBase);
    d->scheduler->taskDone();
}

void ResourceBase::retrieveRelations()
{
    Q_D(ResourceBase);
    d->scheduler->taskDone();
}

bool ResourceBase::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(item)
    Q_UNUSED(parts)
    // retrieveItem() can no longer be pure virtual, because then we could not mark
    // it as deprecated (i.e. implementations would still be forced to implement it),
    // so instead we assert here.
    // NOTE: Don't change to Q_ASSERT_X here: while the macro can be disabled at
    // compile time, we want to hit this assert *ALWAYS*.
    qt_assert_x("Akonadi::ResourceBase::retrieveItem()",
                "The base implementation of retrieveItem() must never be reached. "
                "You must implement either retrieveItem() or retrieveItems(Akonadi::Item::List, QSet<QByteArray>) overload "
                "to handle item retrieval requests.",
                __FILE__,
                __LINE__);
    return false;
}

bool ResourceBase::retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts)
{
    Q_D(ResourceBase);

    // If we reach this implementation of retrieveItems() then it means that the
    // resource is still using the deprecated retrieveItem() method, so we explode
    // this to a myriad of tasks in scheduler and let them be processed one by one

    const qint64 id = d->scheduler->currentTask().serial;
    for (const auto &item : items) {
        d->scheduler->scheduleItemFetch(item, parts, d->scheduler->currentTask().dbusMsgs, id);
    }
    taskDone();
    return true;
}

void Akonadi::ResourceBase::abortActivity()
{
}

void ResourceBase::setItemTransactionMode(ItemSync::TransactionMode mode)
{
    Q_D(ResourceBase);
    d->mItemTransactionMode = mode;
}

void ResourceBase::setItemMergingMode(ItemSync::MergeMode mode)
{
    Q_D(ResourceBase);
    d->mItemMergeMode = mode;
}

void ResourceBase::setAutomaticProgressReporting(bool enabled)
{
    Q_D(ResourceBase);
    d->mAutomaticProgressReporting = enabled;
}

QString ResourceBase::dumpNotificationListToString() const
{
    Q_D(const ResourceBase);
    return d->dumpNotificationListToString();
}

QString ResourceBase::dumpSchedulerToString() const
{
    Q_D(const ResourceBase);
    return d->dumpToString();
}

void ResourceBase::dumpMemoryInfo() const
{
    Q_D(const ResourceBase);
    d->dumpMemoryInfo();
}

QString ResourceBase::dumpMemoryInfoToString() const
{
    Q_D(const ResourceBase);
    return d->dumpMemoryInfoToString();
}

void ResourceBase::tagsRetrieved(const Tag::List &tags, const QHash<QString, Item::List> &tagMembers)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncTags || d->scheduler->currentTask().type == ResourceScheduler::SyncAll
                   || d->scheduler->currentTask().type == ResourceScheduler::Custom,
               "ResourceBase::tagsRetrieved()",
               "Calling tagsRetrieved() although no tag retrieval is in progress");
    if (!d->mTagSyncer) {
        d->mTagSyncer = new TagSync(this);
        connect(d->mTagSyncer, &KJob::percentChanged, d,
                &ResourceBasePrivate::slotPercent); // NOLINT(google-runtime-int): ulong comes from KJob
        connect(d->mTagSyncer, &KJob::result, d, &ResourceBasePrivate::slotTagSyncDone);
    }
    d->mTagSyncer->setFullTagList(tags);
    d->mTagSyncer->setTagMembers(tagMembers);
}

void ResourceBasePrivate::slotTagSyncDone(KJob *job)
{
    Q_Q(ResourceBase);
    mTagSyncer = nullptr;
    if (job->error()) {
        if (job->error() != Job::UserCanceled) {
            qCWarning(AKONADIAGENTBASE_LOG) << "TagSync failed: " << job->errorString();
            Q_EMIT q->error(job->errorString());
        }
    }

    scheduler->taskDone();
}

void ResourceBase::relationsRetrieved(const Relation::List &relations)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncRelations || d->scheduler->currentTask().type == ResourceScheduler::SyncAll
                   || d->scheduler->currentTask().type == ResourceScheduler::Custom,
               "ResourceBase::relationsRetrieved()",
               "Calling relationsRetrieved() although no relation retrieval is in progress");
    if (!d->mRelationSyncer) {
        d->mRelationSyncer = new RelationSync(this);
        connect(d->mRelationSyncer, &KJob::percentChanged, d,
                &ResourceBasePrivate::slotPercent); // NOLINT(google-runtime-int): ulong comes from KJob
        connect(d->mRelationSyncer, &KJob::result, d, &ResourceBasePrivate::slotRelationSyncDone);
    }
    d->mRelationSyncer->setRemoteRelations(relations);
}

void ResourceBasePrivate::slotRelationSyncDone(KJob *job)
{
    Q_Q(ResourceBase);
    mRelationSyncer = nullptr;
    if (job->error()) {
        if (job->error() != Job::UserCanceled) {
            qCWarning(AKONADIAGENTBASE_LOG) << "RelationSync failed: " << job->errorString();
            Q_EMIT q->error(job->errorString());
        }
    }

    scheduler->taskDone();
}

#include "moc_resourcebase.cpp"
#include "resourcebase.moc"
