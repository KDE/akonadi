/*
    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "resourcebase.h"
#include "agentbase_p.h"

#include "resourceadaptor.h"
#include "collectiondeletejob.h"
#include "collectionsync_p.h"
#include "dbusconnectionpool.h"
#include "itemsync.h"
#include "tagsync.h"
#include "kdepimlibs-version.h"
#include "resourcescheduler_p.h"
#include "tracerinterface.h"
#include "xdgbasedirs_p.h"

#include "changerecorder.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "collectionmodifyjob.h"
#include "invalidatecachejob_p.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "itemmodifyjob_p.h"
#include "session.h"
#include "resourceselectjob_p.h"
#include "monitor_p.h"
#include "servermanager_p.h"
#include "recursivemover_p.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kglobal.h>
#include <akonadi/tagmodifyjob.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QApplication>
#include <QtDBus/QtDBus>

using namespace Akonadi;

class Akonadi::ResourceBasePrivate : public AgentBasePrivate
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dfaure")

public:
    ResourceBasePrivate(ResourceBase *parent)
        : AgentBasePrivate(parent)
        , scheduler(0)
        , mItemSyncer(0)
        , mItemSyncFetchScope(0)
        , mItemTransactionMode(ItemSync::SingleTransaction)
        , mCollectionSyncer(0)
        , mTagSyncer(0)
        , mHierarchicalRid(false)
        , mUnemittedProgress(0)
        , mAutomaticProgressReporting(true)
        , mDisableAutomaticItemDeliveryDone(false)
        , mItemSyncBatchSize(10)
        , mCurrentCollectionFetchJob(0)
    {
        Internal::setClientType(Internal::Resource);
        mStatusMessage = defaultReadyMessage();
        mProgressEmissionCompressor.setInterval(1000);
        mProgressEmissionCompressor.setSingleShot(true);
        // HACK: skip local changes of the EntityDisplayAttribute by default. Remove this for KDE5 and adjust resource implementations accordingly.
        mKeepLocalCollectionChanges << "ENTITYDISPLAY";
    }

    ~ResourceBasePrivate()
    {
        delete mItemSyncFetchScope;
    }

    Q_DECLARE_PUBLIC(ResourceBase)

    void delayedInit()
    {
        const QString serviceId = ServerManager::agentServiceName(ServerManager::Resource, mId);
        if (!DBusConnectionPool::threadConnection().registerService(serviceId)) {
            QString reason = DBusConnectionPool::threadConnection().lastError().message();
            if (reason.isEmpty()) {
                reason = QString::fromLatin1("this service is probably running already.");
            }
            kError() << "Unable to register service" << serviceId << "at D-Bus:" << reason;

            if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
                QCoreApplication::instance()->exit(1);
            }

        } else {
            AgentBasePrivate::delayedInit();
        }
    }

    virtual void changeProcessed()
    {
        if (m_recursiveMover) {
            m_recursiveMover->changeProcessed();
            QTimer::singleShot(0, m_recursiveMover, SLOT(replayNext()));
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
    void slotAttributeRetrievalCollectionFetchDone(KJob *job);

    void slotItemSyncDone(KJob *job);

    void slotPercent(KJob *job, unsigned long percent);
    void slotDelayedEmitProgress();
    void slotDeleteResourceCollection();
    void slotDeleteResourceCollectionDone(KJob *job);
    void slotCollectionDeletionDone(KJob *job);

    void slotInvalidateCache(const Akonadi::Collection &collection);

    void slotPrepareItemRetrieval(const Akonadi::Item &item);
    void slotPrepareItemRetrievalResult(KJob *job);

    void changeCommittedResult(KJob *job);

    void slotRecursiveMoveReplay(RecursiveMover *mover);
    void slotRecursiveMoveReplayResult(KJob *job);

    void slotTagSyncDone(KJob *job);
    void slotSessionReconnected()
    {
        Q_Q(ResourceBase);

        new ResourceSelectJob(q->identifier());
    }

    void createItemSyncInstanceIfMissing()
    {
        Q_Q(ResourceBase);
        Q_ASSERT_X(scheduler->currentTask().type == ResourceScheduler::SyncCollection,
                   "createItemSyncInstance", "Calling items retrieval methods although no item retrieval is in progress");
        if (!mItemSyncer) {
            mItemSyncer = new ItemSync(q->currentCollection());
            mItemSyncer->setTransactionMode(mItemTransactionMode);
            mItemSyncer->setBatchSize(mItemSyncBatchSize);
            if (mItemSyncFetchScope) {
                mItemSyncer->setFetchScope(*mItemSyncFetchScope);
            }
            mItemSyncer->setDisableAutomaticDeliveryDone(mDisableAutomaticItemDeliveryDone);
            mItemSyncer->setProperty("collection", QVariant::fromValue(q->currentCollection()));
            connect(mItemSyncer, SIGNAL(percent(KJob*,ulong)), q, SLOT(slotPercent(KJob*,ulong)));
            connect(mItemSyncer, SIGNAL(result(KJob*)), q, SLOT(slotItemSyncDone(KJob*)));
            connect(mItemSyncer, SIGNAL(readyForNextBatch(int)), q, SIGNAL(retrieveNextItemSyncBatch(int)));
        }
        Q_ASSERT(mItemSyncer);
    }

public Q_SLOTS:
    // Dump the state of the scheduler
    Q_SCRIPTABLE QString dumpToString() const
    {
        Q_Q(const ResourceBase);
        QString retVal;
        QMetaObject::invokeMethod(const_cast<ResourceBase *>(q), "dumpResourceToString", Qt::DirectConnection, Q_RETURN_ARG(QString, retVal));
        return scheduler->dumpToString() + QLatin1Char('\n') + retVal;
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

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemAdded(item, collection);
    }

    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers)
    {
        if (item.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemChanged(item, partIdentifiers);
    }

    void itemsFlagsChanged(const Item::List &items, const QSet< QByteArray > &addedFlags,
                           const QSet< QByteArray > &removedFlags)
    {
        if (addedFlags.isEmpty() && removedFlags.isEmpty()) {
            changeProcessed();
            return;
        }

        Item::List validItems;
        foreach (const Akonadi::Item &item, items) {
            if (!item.remoteId().isEmpty()) {
                validItems << item;
            }
        }
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsFlagsChanged(validItems, addedFlags, removedFlags);
    }

    void itemsTagsChanged(const Item::List &items, const QSet<Tag> &addedTags, const QSet<Tag> &removedTags)
    {
        if (addedTags.isEmpty() && removedTags.isEmpty()) {
            changeProcessed();
            return;
        }

        Item::List validItems;
        foreach (const Akonadi::Item &item, items) {
            if (!item.remoteId().isEmpty()) {
                validItems << item;
            }
        }
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsTagsChanged(validItems, addedTags, removedTags);
    }

    // TODO move the move translation code from AgentBasePrivate here, it's wrong for agents
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &destination)
    {
        if (item.remoteId().isEmpty() || destination.remoteId().isEmpty() || destination == source) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemMoved(item, source, destination);
    }

    void itemsMoved(const Item::List &items, const Collection &source, const Collection &destination)
    {
        if (destination.remoteId().isEmpty() || destination == source) {
            changeProcessed();
            return;
        }

        Item::List validItems;
        foreach (const Akonadi::Item &item, items) {
            if (!item.remoteId().isEmpty()) {
                validItems << item;
            }
        }
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsMoved(validItems, source, destination);
    }

    void itemRemoved(const Akonadi::Item &item)
    {
        if (item.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::itemRemoved(item);
    }

    void itemsRemoved(const Item::List &items)
    {
        Item::List validItems;
        foreach (const Akonadi::Item &item, items) {
            if (!item.remoteId().isEmpty()) {
                validItems << item;
            }
        }
        if (validItems.isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::itemsRemoved(validItems);
    }

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
    {
        if (parent.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionAdded(collection, parent);
    }

    void collectionChanged(const Akonadi::Collection &collection)
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionChanged(collection);
    }

    void collectionChanged(const Akonadi::Collection &collection, const QSet< QByteArray > &partIdentifiers)
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionChanged(collection, partIdentifiers);
    }

    void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &destination)
    {
        // unknown destination or source == destination means we can't do/don't have to do anything
        if (destination.remoteId().isEmpty() || source == destination) {
            changeProcessed();
            return;
        }

        // inter-resource moves, requires we know which resources the source and destination are in though
        if (!source.resource().isEmpty() && !destination.resource().isEmpty() && source.resource() != destination.resource()) {
            if (source.resource() == q_ptr->identifier()) {   // moved away from us
                AgentBasePrivate::collectionRemoved(collection);
            } else if (destination.resource() == q_ptr->identifier()) {   // moved to us
                scheduler->taskDone(); // stop change replay for now
                RecursiveMover *mover = new RecursiveMover(this);
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

    void collectionRemoved(const Akonadi::Collection &collection)
    {
        if (collection.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }
        AgentBasePrivate::collectionRemoved(collection);
    }

    void tagAdded(const Akonadi::Tag &tag)
    {
        if (!tag.isValid()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::tagAdded(tag);
    }

    void tagChanged(const Akonadi::Tag &tag)
    {
        if (tag.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::tagChanged(tag);
    }

    void tagRemoved(const Akonadi::Tag &tag)
    {
        if (tag.remoteId().isEmpty()) {
            changeProcessed();
            return;
        }

        AgentBasePrivate::tagRemoved(tag);
    }

public:
    // synchronize states
    Collection currentCollection;

    ResourceScheduler *scheduler;
    ItemSync *mItemSyncer;
    ItemFetchScope *mItemSyncFetchScope;
    ItemSync::TransactionMode mItemTransactionMode;
    CollectionSync *mCollectionSyncer;
    TagSync *mTagSyncer;
    bool mHierarchicalRid;
    QTimer mProgressEmissionCompressor;
    int mUnemittedProgress;
    QMap<Akonadi::Collection::Id, QVariantMap> mUnemittedAdvancedStatus;
    bool mAutomaticProgressReporting;
    bool mDisableAutomaticItemDeliveryDone;
    QPointer<RecursiveMover> m_recursiveMover;
    int mItemSyncBatchSize;
    QSet<QByteArray> mKeepLocalCollectionChanges;
    KJob *mCurrentCollectionFetchJob;
};

ResourceBase::ResourceBase(const QString &id)
    : AgentBase(new ResourceBasePrivate(this), id)
{
    Q_D(ResourceBase);

    new Akonadi__ResourceAdaptor(this);

    d->scheduler = new ResourceScheduler(this);

    d->mChangeRecorder->setChangeRecordingEnabled(true);
    d->mChangeRecorder->setCollectionMoveTranslationEnabled(false);   // we deal with this ourselves
    connect(d->mChangeRecorder, SIGNAL(changesAdded()),
            d->scheduler, SLOT(scheduleChangeReplay()));

    d->mChangeRecorder->setResourceMonitored(d->mId.toLatin1());
    d->mChangeRecorder->fetchCollection(true);

    connect(d->scheduler, SIGNAL(executeFullSync()),
            SLOT(retrieveCollections()));
    connect(d->scheduler, SIGNAL(executeCollectionTreeSync()),
            SLOT(retrieveCollections()));
    connect(d->scheduler, SIGNAL(executeCollectionSync(Akonadi::Collection)),
            SLOT(slotSynchronizeCollection(Akonadi::Collection)));
    connect(d->scheduler, SIGNAL(executeCollectionAttributesSync(Akonadi::Collection)),
            SLOT(slotSynchronizeCollectionAttributes(Akonadi::Collection)));
    connect(d->scheduler, SIGNAL(executeTagSync()),
            SLOT(slotSynchronizeTags()));
    connect(d->scheduler, SIGNAL(executeItemFetch(Akonadi::Item,QSet<QByteArray>)),
            SLOT(slotPrepareItemRetrieval(Akonadi::Item)));
    connect(d->scheduler, SIGNAL(executeResourceCollectionDeletion()),
            SLOT(slotDeleteResourceCollection()));
    connect(d->scheduler, SIGNAL(executeCacheInvalidation(Akonadi::Collection)),
            SLOT(slotInvalidateCache(Akonadi::Collection)));
    connect(d->scheduler, SIGNAL(status(int,QString)),
            SIGNAL(status(int,QString)));
    connect(d->scheduler, SIGNAL(executeChangeReplay()),
            d->mChangeRecorder, SLOT(replayNext()));
    connect(d->scheduler, SIGNAL(executeRecursiveMoveReplay(RecursiveMover*)),
            SLOT(slotRecursiveMoveReplay(RecursiveMover*)));
    connect(d->scheduler, SIGNAL(fullSyncComplete()), SIGNAL(synchronized()));
    connect(d->scheduler, SIGNAL(collectionTreeSyncComplete()), SIGNAL(collectionTreeSynchronized()));
    connect(d->mChangeRecorder, SIGNAL(nothingToReplay()), d->scheduler, SLOT(taskDone()));
    connect(d->mChangeRecorder, SIGNAL(collectionRemoved(Akonadi::Collection)),
            d->scheduler, SLOT(collectionRemoved(Akonadi::Collection)));
    connect(this, SIGNAL(abortRequested()), this, SLOT(slotAbortRequested()));
    connect(this, SIGNAL(synchronized()), d->scheduler, SLOT(taskDone()));
    connect(this, SIGNAL(collectionTreeSynchronized()), d->scheduler, SLOT(taskDone()));
    connect(this, SIGNAL(agentNameChanged(QString)),
            this, SIGNAL(nameChanged(QString)));

    connect(&d->mProgressEmissionCompressor, SIGNAL(timeout()),
            this, SLOT(slotDelayedEmitProgress()));

    d->scheduler->setOnline(d->mOnline);
    if (!d->mChangeRecorder->isEmpty()) {
        d->scheduler->scheduleChangeReplay();
    }

    new ResourceSelectJob(identifier());

    connect(d->mChangeRecorder->session(), SIGNAL(reconnected()), SLOT(slotSessionReconnected()));
}

ResourceBase::~ResourceBase()
{
}

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
    QString identifier;
    if (argc < 3) {
        kDebug() << "Not enough arguments passed...";
        exit(1);
    }

    for (int i = 1; i < argc - 1; ++i) {
        if (QLatin1String(argv[i]) == QLatin1String("--identifier")) {
            identifier = QLatin1String(argv[i + 1]);
        }
    }

    if (identifier.isEmpty()) {
        kDebug() << "Identifier argument missing";
        exit(1);
    }

    const QFileInfo fi(QString::fromLocal8Bit(argv[0]));
    // strip off full path and possible .exe suffix
    const QByteArray catalog = fi.baseName().toLatin1();

    KCmdLineArgs::init(argc, argv, ServerManager::addNamespace(identifier).toLatin1(), catalog,
                       ki18nc("@title application name", "Akonadi Resource"), KDEPIMLIBS_VERSION,
                       ki18nc("@title application description", "Akonadi Resource"));

    KCmdLineOptions options;
    options.add("identifier <argument>",
                ki18nc("@label commandline option", "Resource identifier"));
    KCmdLineArgs::addCmdLineOptions(options);

    return identifier;
}

int ResourceBase::init(ResourceBase *r)
{
    QApplication::setQuitOnLastWindowClosed(false);
    KGlobal::locale()->insertCatalog(QLatin1String("libakonadi"));
    int rv = kapp->exec();
    delete r;
    return rv;
}

void ResourceBasePrivate::slotAbortRequested()
{
    Q_Q(ResourceBase);

    scheduler->cancelQueues();
    QMetaObject::invokeMethod(q, "abortActivity");
}

void ResourceBase::itemRetrieved(const Item &item)
{
    Q_D(ResourceBase);
    Q_ASSERT(d->scheduler->currentTask().type == ResourceScheduler::FetchItem);
    if (!item.isValid()) {
        d->scheduler->currentTask().sendDBusReplies(i18nc("@info", "Invalid item retrieved"));
        d->scheduler->taskDone();
        return;
    }

    Item i(item);
    QSet<QByteArray> requestedParts = d->scheduler->currentTask().itemParts;
    foreach (const QByteArray &part, requestedParts) {
        if (!item.loadedPayloadParts().contains(part)) {
            kWarning() << "Item does not provide part" << part;
        }
    }

    ItemModifyJob *job = new ItemModifyJob(i);
  job->d_func()->setSilent( true );
    // FIXME: remove once the item with which we call retrieveItem() has a revision number
    job->disableRevisionCheck();
    connect(job, SIGNAL(result(KJob*)), SLOT(slotDeliveryDone(KJob*)));
}

void ResourceBasePrivate::slotDeliveryDone(KJob *job)
{
    Q_Q(ResourceBase);
    Q_ASSERT(scheduler->currentTask().type == ResourceScheduler::FetchItem);
    if (job->error()) {
        emit q->error(i18nc("@info", "Error while creating item: %1", job->errorString()));
    }
    scheduler->currentTask().sendDBusReplies(job->error() ? job->errorString() : QString());
    scheduler->taskDone();
}

void ResourceBase::collectionAttributesRetrieved(const Collection &collection)
{
    Q_D(ResourceBase);
    Q_ASSERT(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionAttributes);
    if (!collection.isValid()) {
        emit attributesSynchronized(d->scheduler->currentTask().collection.id());
        d->scheduler->taskDone();
        return;
    }

    CollectionModifyJob *job = new CollectionModifyJob(collection);
    connect(job, SIGNAL(result(KJob*)), SLOT(slotCollectionAttributesSyncDone(KJob*)));
}

void ResourceBasePrivate::slotCollectionAttributesSyncDone(KJob *job)
{
    Q_Q(ResourceBase);
    Q_ASSERT(scheduler->currentTask().type == ResourceScheduler::SyncCollectionAttributes);
    if (job->error()) {
        emit q->error(i18nc("@info", "Error while updating collection: %1", job->errorString()));
    }
    emit q->attributesSynchronized(scheduler->currentTask().collection.id());
    scheduler->taskDone();
}

void ResourceBasePrivate::slotDeleteResourceCollection()
{
    Q_Q(ResourceBase);

    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel);
    job->fetchScope().setResource(q->identifier());
    connect(job, SIGNAL(result(KJob*)), q, SLOT(slotDeleteResourceCollectionDone(KJob*)));
}

void ResourceBasePrivate::slotDeleteResourceCollectionDone(KJob *job)
{
    Q_Q(ResourceBase);
    if (job->error()) {
        emit q->error(job->errorString());
        scheduler->taskDone();
    } else {
        const CollectionFetchJob *fetchJob = static_cast<const CollectionFetchJob *>(job);

        if (!fetchJob->collections().isEmpty()) {
            CollectionDeleteJob *job = new CollectionDeleteJob(fetchJob->collections().first());
            connect(job, SIGNAL(result(KJob*)), q, SLOT(slotCollectionDeletionDone(KJob*)));
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
        emit q->error(job->errorString());
    }

    scheduler->taskDone();
}

void ResourceBasePrivate::slotInvalidateCache(const Akonadi::Collection &collection)
{
    Q_Q(ResourceBase);
    InvalidateCacheJob *job = new InvalidateCacheJob(collection, q);
    connect(job, SIGNAL(result(KJob*)), scheduler, SLOT(taskDone()));
}

void ResourceBase::changeCommitted(const Item &item)
{
    changesCommitted(Item::List() << item);
}

void ResourceBase::changesCommitted(const Item::List &items)
{
    TransactionSequence *transaction = new TransactionSequence(this);
    connect(transaction, SIGNAL(finished(KJob*)),
            this, SLOT(changeCommittedResult(KJob*)));

    // Modify the items one-by-one, because STORE does not support mass RID change
    Q_FOREACH (const Item &item, items) {
        ItemModifyJob *job = new ItemModifyJob(item, transaction);
        job->d_func()->setClean();
        job->disableRevisionCheck(); // TODO: remove, but where/how do we handle the error?
        job->setIgnorePayload(true);   // we only want to reset the dirty flag and update the remote id
        job->setUpdateGid(true);   // allow resources to update GID too
    }
}

void ResourceBase::changeCommitted(const Collection &collection)
{
    CollectionModifyJob *job = new CollectionModifyJob(collection);
    connect(job, SIGNAL(result(KJob*)), SLOT(changeCommittedResult(KJob*)));
}

void ResourceBasePrivate::changeCommittedResult(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorText();
    }

    Q_Q(ResourceBase);
    if (qobject_cast<CollectionModifyJob *>(job)) {
        if (job->error()) {
            emit q->error(i18nc("@info", "Updating local collection failed: %1.", job->errorText()));
        }
        mChangeRecorder->d_ptr->invalidateCache(static_cast<CollectionModifyJob *>(job)->collection());
    } else {
        if (job->error()) {
            emit q->error(i18nc("@info", "Updating local items failed: %1.", job->errorText()));
        }
        // Item and tag cache is invalidated by modify job
    }

    changeProcessed();
}

void ResourceBase::changeCommitted(const Tag &tag)
{
    TagModifyJob *job = new TagModifyJob(tag);
    connect(job, SIGNAL(result(KJob*)), SLOT(changeCommittedResult(KJob*)));
}

bool ResourceBase::requestItemDelivery(qint64 uid, const QString &remoteId,
                                       const QString &mimeType, const QStringList &parts)
{
    return requestItemDeliveryV2(uid, remoteId, mimeType, parts).isEmpty();
}

QString ResourceBase::requestItemDeliveryV2(qint64 uid, const QString &remoteId, const QString &mimeType, const QStringList &_parts)
{
    Q_D(ResourceBase);
    if (!isOnline()) {
        const QString errorMsg = i18nc("@info", "Cannot fetch item in offline mode.");
        emit error(errorMsg);
        return errorMsg;
    }

    setDelayedReply(true);
    // FIXME: we need at least the revision number too
    Item item(uid);
    item.setMimeType(mimeType);
    item.setRemoteId(remoteId);

    QSet<QByteArray> parts;
    Q_FOREACH (const QString &str, _parts) {
        parts.insert(str.toLatin1());
    }

    d->scheduler->scheduleItemFetch(item, parts, message());

    return QString();

}

void ResourceBase::collectionsRetrieved(const Collection::List &collections)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree ||
               d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::collectionsRetrieved()",
               "Calling collectionsRetrieved() although no collection retrieval is in progress");
    if (!d->mCollectionSyncer) {
        d->mCollectionSyncer = new CollectionSync(identifier());
        d->mCollectionSyncer->setHierarchicalRemoteIds(d->mHierarchicalRid);
        d->mCollectionSyncer->setKeepLocalChanges(d->mKeepLocalCollectionChanges);
        connect(d->mCollectionSyncer, SIGNAL(percent(KJob*,ulong)), SLOT(slotPercent(KJob*,ulong)));
        connect(d->mCollectionSyncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)));
    }
    d->mCollectionSyncer->setRemoteCollections(collections);
}

void ResourceBase::collectionsRetrievedIncremental(const Collection::List &changedCollections,
                                                   const Collection::List &removedCollections)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree ||
               d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::collectionsRetrievedIncremental()",
               "Calling collectionsRetrievedIncremental() although no collection retrieval is in progress");
    if (!d->mCollectionSyncer) {
        d->mCollectionSyncer = new CollectionSync(identifier());
        d->mCollectionSyncer->setHierarchicalRemoteIds(d->mHierarchicalRid);
        d->mCollectionSyncer->setKeepLocalChanges(d->mKeepLocalCollectionChanges);
        connect(d->mCollectionSyncer, SIGNAL(percent(KJob*,ulong)), SLOT(slotPercent(KJob*,ulong)));
        connect(d->mCollectionSyncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)));
    }
    d->mCollectionSyncer->setRemoteCollections(changedCollections, removedCollections);
}

void ResourceBase::setCollectionStreamingEnabled(bool enable)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree ||
               d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
               "ResourceBase::setCollectionStreamingEnabled()",
               "Calling setCollectionStreamingEnabled() although no collection retrieval is in progress");
    if (!d->mCollectionSyncer) {
        d->mCollectionSyncer = new CollectionSync(identifier());
        d->mCollectionSyncer->setHierarchicalRemoteIds(d->mHierarchicalRid);
        connect(d->mCollectionSyncer, SIGNAL(percent(KJob*,ulong)), SLOT(slotPercent(KJob*,ulong)));
        connect(d->mCollectionSyncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)));
    }
    d->mCollectionSyncer->setStreamingEnabled(enable);
}

void ResourceBase::collectionsRetrievalDone()
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree ||
               d->scheduler->currentTask().type == ResourceScheduler::SyncAll,
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
    mCollectionSyncer = 0;
    if (job->error()) {
        if (job->error() != Job::UserCanceled) {
            emit q->error(job->errorString());
        }
    } else {
        if (scheduler->currentTask().type == ResourceScheduler::SyncAll) {
            CollectionFetchJob *list = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
            list->setFetchScope(q->changeRecorder()->collectionFetchScope());
            list->fetchScope().setResource(mId);
            list->fetchScope().setListFilter(CollectionFetchScope::Sync);
            q->connect(list, SIGNAL(result(KJob*)), q, SLOT(slotLocalListDone(KJob*)));
            return;
        } else if (scheduler->currentTask().type == ResourceScheduler::SyncCollectionTree) {
            scheduler->scheduleCollectionTreeSyncCompletion();
        }
    }
    scheduler->taskDone();
}

void ResourceBasePrivate::slotLocalListDone(KJob *job)
{
    Q_Q(ResourceBase);
    if (job->error()) {
        emit q->error(job->errorString());
    } else {
        Collection::List cols = static_cast<CollectionFetchJob *>(job)->collections();
        foreach (const Collection &col, cols) {
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
                emit q->status(AgentBase::Running, i18nc("@info:status", "Syncing folder '%1'", currentCollection.displayName()));
            }

            Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(col, CollectionFetchJob::Base, this);
            fetchJob->setFetchScope(q->changeRecorder()->collectionFetchScope());
            connect(fetchJob, SIGNAL(result(KJob*)), q, SLOT(slotItemRetrievalCollectionFetchDone(KJob*)));
            mCurrentCollectionFetchJob = fetchJob;
            return;
        }
    }
    scheduler->taskDone();
}

void ResourceBasePrivate::slotItemRetrievalCollectionFetchDone(KJob *job)
{
    Q_Q(ResourceBase);
    mCurrentCollectionFetchJob = 0;
    if (job->error()) {
        kWarning() << "Failed to retrieve collection for sync: " << job->errorString();
        q->cancelTask(i18n("Failed to retrieve collection for sync."));
        return;
    }
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    q->retrieveItems(fetchJob->collections().first());
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

void ResourceBasePrivate::slotSynchronizeCollectionAttributes(const Collection &col)
{
    Q_Q(ResourceBase);
    Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(col, CollectionFetchJob::Base, this);
    fetchJob->setFetchScope(q->changeRecorder()->collectionFetchScope());
    connect(fetchJob, SIGNAL(result(KJob*)), q, SLOT(slotAttributeRetrievalCollectionFetchDone(KJob*)));
    Q_ASSERT(!mCurrentCollectionFetchJob);
    mCurrentCollectionFetchJob = fetchJob;
}

void ResourceBasePrivate::slotAttributeRetrievalCollectionFetchDone(KJob *job)
{
    mCurrentCollectionFetchJob = 0;
    Q_Q(ResourceBase);
    if (job->error()) {
        kWarning() << "Failed to retrieve collection for attribute sync: " << job->errorString();
        q->cancelTask(i18n("Failed to retrieve collection for attribute sync."));
        return;
    }
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    QMetaObject::invokeMethod(q, "retrieveCollectionAttributes", Q_ARG(Akonadi::Collection, fetchJob->collections().first()));
}

void ResourceBasePrivate::slotSynchronizeTags()
{
    Q_Q(ResourceBase);
    QMetaObject::invokeMethod(q, "retrieveTags");
}

void ResourceBasePrivate::slotPrepareItemRetrieval(const Akonadi::Item &item)
{
    Q_Q(ResourceBase);
    ItemFetchJob *fetch = new ItemFetchJob(item, this);
    fetch->fetchScope().setAncestorRetrieval(q->changeRecorder()->itemFetchScope().ancestorRetrieval());
    fetch->fetchScope().setCacheOnly(true);

    // copy list of attributes to fetch
    const QSet<QByteArray> attributes = q->changeRecorder()->itemFetchScope().attributes();
    foreach (const QByteArray &attribute, attributes) {
        fetch->fetchScope().fetchAttribute(attribute);
    }

    q->connect(fetch, SIGNAL(result(KJob*)), SLOT(slotPrepareItemRetrievalResult(KJob*)));
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
    ItemFetchJob *fetch = qobject_cast<ItemFetchJob *>(job);
    if (fetch->items().count() != 1) {
        q->cancelTask(i18n("The requested item no longer exists"));
        return;
    }
    const Item item = fetch->items().first();
    const QSet<QByteArray> parts = scheduler->currentTask().itemParts;
    if (!q->retrieveItem(item, parts)) {
        q->cancelTask();
    }
}

void ResourceBasePrivate::slotRecursiveMoveReplay(RecursiveMover *mover)
{
    Q_Q(ResourceBase);
    Q_ASSERT(mover);
    Q_ASSERT(!m_recursiveMover);
    m_recursiveMover = mover;
    connect(mover, SIGNAL(result(KJob*)), q, SLOT(slotRecursiveMoveReplayResult(KJob*)));
    mover->start();
}

void ResourceBasePrivate::slotRecursiveMoveReplayResult(KJob *job)
{
    Q_Q(ResourceBase);
    m_recursiveMover = 0;

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
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncCollection ,
               "ResourceBase::currentCollection()",
               "Trying to access current collection although no item retrieval is in progress");
    return d->currentCollection;
}

Item ResourceBase::currentItem() const
{
    Q_D(const ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::FetchItem ,
               "ResourceBase::currentItem()",
               "Trying to access current item although no item retrieval is in progress");
    return d->scheduler->currentTask().item;
}

void ResourceBase::synchronizeCollectionTree()
{
    d_func()->scheduler->scheduleCollectionTreeSync();
}

void ResourceBase::synchronizeTags()
{
    d_func()->scheduler->scheduleTagSync();
}

void ResourceBase::cancelTask()
{
    Q_D(ResourceBase);
    if (d->mCurrentCollectionFetchJob) {
        d->mCurrentCollectionFetchJob->kill();
        d->mCurrentCollectionFetchJob = 0;
    }
    switch (d->scheduler->currentTask().type) {
    case ResourceScheduler::FetchItem:
        itemRetrieved(Item());   // sends the error reply and
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

    emit error(msg);
}

void ResourceBase::deferTask()
{
    Q_D(ResourceBase);
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
    CollectionFetchJob *job = new CollectionFetchJob(Collection(collectionId), recursive ? CollectionFetchJob::Recursive : CollectionFetchJob::Base);
    job->setFetchScope(changeRecorder()->collectionFetchScope());
    job->fetchScope().setResource(identifier());
    job->fetchScope().setListFilter(CollectionFetchScope::Sync);
    connect(job, SIGNAL(result(KJob*)), SLOT(slotCollectionListDone(KJob*)));
}

void ResourceBasePrivate::slotCollectionListDone(KJob *job)
{
    if (!job->error()) {
        const Collection::List list = static_cast<CollectionFetchJob *>(job)->collections();
        Q_FOREACH (const Collection &collection, list) {
            //We also get collections that should not be synced but are part of the tree.
            if (collection.shouldList(Collection::ListSync) || collection.referenced()) {
                scheduler->scheduleSync(collection);
            }
        }
    } else {
        kWarning() << "Failed to fetch collection for collection sync: " << job->errorString();
    }
}

void ResourceBase::synchronizeCollectionAttributes(qint64 collectionId)
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection(collectionId), CollectionFetchJob::Base);
    job->setFetchScope(changeRecorder()->collectionFetchScope());
    job->fetchScope().setResource(identifier());
    connect(job, SIGNAL(result(KJob*)), SLOT(slotCollectionListForAttributesDone(KJob*)));
}

void ResourceBasePrivate::slotCollectionListForAttributesDone(KJob *job)
{
    if (!job->error()) {
        Collection::List list = static_cast<CollectionFetchJob *>(job)->collections();
        if (!list.isEmpty()) {
            Collection col = list.first();
            scheduler->scheduleAttributesSync(col);
        }
    }
    // TODO: error handling
}

void ResourceBase::setTotalItems(int amount)
{
    kDebug() << amount;
    Q_D(ResourceBase);
    setItemStreamingEnabled(true);
    if (d->mItemSyncer) {
        d->mItemSyncer->setTotalItems(amount);
    }
}

void ResourceBase::setDisableAutomaticItemDeliveryDone(bool disable)
{
    Q_D(ResourceBase);
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
    d->createItemSyncInstanceIfMissing();
    if (d->mItemSyncer) {
        d->mItemSyncer->setFullSyncItems(items);
    }
}

void ResourceBase::itemsRetrievedIncremental(const Item::List &changedItems,
                                             const Item::List &removedItems)
{
    Q_D(ResourceBase);
    d->createItemSyncInstanceIfMissing();
    if (d->mItemSyncer) {
        d->mItemSyncer->setIncrementalSyncItems(changedItems, removedItems);
    }
}

void ResourceBasePrivate::slotItemSyncDone(KJob *job)
{
    mItemSyncer = 0;
    Q_Q(ResourceBase);
    if (job->error() && job->error() != Job::UserCanceled) {
        emit q->error(job->errorString());
    }
    scheduler->taskDone();
}

void ResourceBasePrivate::slotDelayedEmitProgress()
{
    Q_Q(ResourceBase);
    if (mAutomaticProgressReporting) {
        emit q->percent(mUnemittedProgress);

        Q_FOREACH (const QVariantMap &statusMap, mUnemittedAdvancedStatus) {
            emit q->advancedStatus(statusMap);
        }
    }
    mUnemittedProgress = 0;
    mUnemittedAdvancedStatus.clear();
}

void ResourceBasePrivate::slotPercent(KJob *job, unsigned long percent)
{
    mUnemittedProgress = percent;

    const Collection collection = job->property("collection").value<Collection>();
    if (collection.isValid()) {
        QVariantMap statusMap;
        statusMap.insert(QLatin1String("key"), QString::fromLatin1("collectionSyncProgress"));
        statusMap.insert(QLatin1String("collectionId"), collection.id());
        statusMap.insert(QLatin1String("percent"), static_cast<unsigned int>(percent));

        mUnemittedAdvancedStatus[collection.id()] = statusMap;
    }
    // deliver completion right away, intermediate progress at 1s intervals
    if (percent == 100) {
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

void Akonadi::ResourceBase::abortActivity()
{
}

void ResourceBase::setItemTransactionMode(ItemSync::TransactionMode mode)
{
    Q_D(ResourceBase);
    d->mItemTransactionMode = mode;
}

void ResourceBase::setItemSynchronizationFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(ResourceBase);
    if (!d->mItemSyncFetchScope) {
        d->mItemSyncFetchScope = new ItemFetchScope;
    }
    *(d->mItemSyncFetchScope) = fetchScope;
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
    return d->dumpMemoryInfo();
}

QString ResourceBase::dumpMemoryInfoToString() const
{
    Q_D(const ResourceBase);
    return d->dumpMemoryInfoToString();
}

void ResourceBase::tagsRetrieved(const Tag::List &tags, const QHash<QString, Item::List> &tagMembers)
{
    Q_D(ResourceBase);
    Q_ASSERT_X(d->scheduler->currentTask().type == ResourceScheduler::SyncTags ||
               d->scheduler->currentTask().type == ResourceScheduler::SyncAll ||
               d->scheduler->currentTask().type == ResourceScheduler::Custom,
               "ResourceBase::tagsRetrieved()",
               "Calling tagsRetrieved() although no tag retrieval is in progress");
    if (!d->mTagSyncer) {
        d->mTagSyncer = new TagSync(this);
        connect(d->mTagSyncer, SIGNAL(percent(KJob*,ulong)), SLOT(slotPercent(KJob*,ulong)));
        connect(d->mTagSyncer, SIGNAL(result(KJob*)), SLOT(slotTagSyncDone(KJob*)));
    }
    d->mTagSyncer->setFullTagList(tags);
    d->mTagSyncer->setTagMembers(tagMembers);
}

void ResourceBasePrivate::slotTagSyncDone(KJob *job)
{
    Q_Q(ResourceBase);
    mTagSyncer = 0;
    if (job->error()) {
        if (job->error() != Job::UserCanceled) {
            kWarning() << "TagSync failed: " << job->errorString();
            emit q->error(job->errorString());
        }
    }
    scheduler->taskDone();
}


#include "resourcebase.moc"
#include "moc_resourcebase.cpp"
