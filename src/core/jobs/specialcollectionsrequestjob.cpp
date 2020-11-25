/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "specialcollectionsrequestjob.h"

#include "specialcollectionattribute.h"
#include "specialcollections_p.h"
#include "specialcollectionshelperjobs_p.h"

#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "entitydisplayattribute.h"


#include "akonadicore_debug.h"


using namespace Akonadi;

/**
  @internal
*/
class Akonadi::SpecialCollectionsRequestJobPrivate
{
public:
    SpecialCollectionsRequestJobPrivate(SpecialCollections *collections, SpecialCollectionsRequestJob *qq);

    bool isEverythingReady() const;
    void lockResult(KJob *job);   // slot
    void releaseLock(); // slot
    void nextResource();
    void resourceScanResult(KJob *job);   // slot
    void createRequestedFolders(ResourceScanJob *job, QHash<QByteArray, bool> &requestedFolders);
    void collectionCreateResult(KJob *job);   // slot

    SpecialCollectionsRequestJob *const q;
    SpecialCollections *mSpecialCollections = nullptr;
    int mPendingCreateJobs;

    QByteArray mRequestedType;
    AgentInstance mRequestedResource;

    // Input:
    QHash<QByteArray, bool> mDefaultFolders;
    bool mRequestingDefaultFolders;
    QHash< QString, QHash<QByteArray, bool> > mFoldersForResource;
    QString mDefaultResourceType;
    QVariantMap mDefaultResourceOptions;
    QList<QByteArray> mKnownTypes;
    QMap<QByteArray, QString> mNameForTypeMap;
    QMap<QByteArray, QString> mIconForTypeMap;

    // Output:
    QStringList mToForget;
    QVector< QPair<Collection, QByteArray> > mToRegister;
};

SpecialCollectionsRequestJobPrivate::SpecialCollectionsRequestJobPrivate(SpecialCollections *collections,
        SpecialCollectionsRequestJob *qq)
    : q(qq)
    , mSpecialCollections(collections)
    , mPendingCreateJobs(0)
    , mRequestingDefaultFolders(false)
{
}

bool SpecialCollectionsRequestJobPrivate::isEverythingReady() const
{
    // check if all requested folders are known already
    if (mRequestingDefaultFolders) {
        for (auto it = mDefaultFolders.cbegin(), end = mDefaultFolders.cend(); it != end; ++it) {
            if (it.value() && !mSpecialCollections->hasDefaultCollection(it.key())) {
                return false;
            }
        }
    }

    for (auto resourceIt = mFoldersForResource.cbegin(), end = mFoldersForResource.cend(); resourceIt != end; ++resourceIt) {
        const QHash<QByteArray, bool> &requested = resourceIt.value();
        for (auto it = requested.cbegin(), end = requested.cend(); it != end; ++it) {
            if (it.value() && !mSpecialCollections->hasCollection(it.key(), AgentManager::self()->instance(resourceIt.key()))) {
                return false;
            }
        }
    }

    return true;
}

void SpecialCollectionsRequestJobPrivate::lockResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Failed to get lock:" << job->errorString();
        q->setError(job->error());
        q->setErrorText(job->errorString());
        q->emitResult();
        return;
    }

    if (mRequestingDefaultFolders) {
        // If default folders are requested, deal with that first.
        auto *resjob = new DefaultResourceJob(mSpecialCollections->d->mSettings, q);
        resjob->setDefaultResourceType(mDefaultResourceType);
        resjob->setDefaultResourceOptions(mDefaultResourceOptions);
        resjob->setTypes(mKnownTypes);
        resjob->setNameForTypeMap(mNameForTypeMap);
        resjob->setIconForTypeMap(mIconForTypeMap);
        QObject::connect(resjob, &KJob::result, q, [this](KJob *job) { resourceScanResult(job); });
    } else {
        // If no default folders are requested, go straight to the next step.
        nextResource();
    }
}

void SpecialCollectionsRequestJobPrivate::releaseLock()
{
    const bool ok = Akonadi::releaseLock();
    if (!ok) {
        qCWarning(AKONADICORE_LOG) << "WTF, can't release lock.";
    }
}

void SpecialCollectionsRequestJobPrivate::nextResource()
{
    if (mFoldersForResource.isEmpty()) {
        qCDebug(AKONADICORE_LOG) << "All done! Committing.";

        mSpecialCollections->d->beginBatchRegister();

        // Forget everything we knew before about these resources.
        for (const QString &resourceId : qAsConst(mToForget)) {
            mSpecialCollections->d->forgetFoldersForResource(resourceId);
        }

        // Register all the collections that we fetched / created.
        typedef QPair<Collection, QByteArray> RegisterPair;
        for (const RegisterPair &pair : qAsConst(mToRegister)) {
            const bool ok = mSpecialCollections->registerCollection(pair.second, pair.first);
            Q_ASSERT(ok);
            Q_UNUSED(ok)
        }

        mSpecialCollections->d->endBatchRegister();

        // Release the lock once the transaction has been committed.
        QObject::connect(q, &KJob::result, q, [this]() { releaseLock(); });

        // We are done!
        q->commit();

    } else {
        const QString resourceId = mFoldersForResource.cbegin().key();
        qCDebug(AKONADICORE_LOG) << "A resource is done," << mFoldersForResource.count()
                                 << "more to do. Now doing resource" << resourceId;
        auto *resjob = new ResourceScanJob(resourceId, mSpecialCollections->d->mSettings, q);
        QObject::connect(resjob, &KJob::result, q, [this](KJob *job) { resourceScanResult(job); });
    }
}

void SpecialCollectionsRequestJobPrivate::resourceScanResult(KJob *job)
{
    auto *resjob = qobject_cast<ResourceScanJob *>(job);
    Q_ASSERT(resjob);

    const QString resourceId = resjob->resourceId();
    qCDebug(AKONADICORE_LOG) << "resourceId" << resourceId;

    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Failed to request resource" << resourceId << ":" << job->errorString();
        return;
    }

    if (qobject_cast<DefaultResourceJob *>(job)) {
        // This is the default resource.
        if (resourceId != mSpecialCollections->d->defaultResourceId()) {
            qCWarning(AKONADICORE_LOG) << "Resource id's don't match: " << resourceId
                                       << mSpecialCollections->d->defaultResourceId();
            Q_ASSERT(false);
        }
        //mToForget.append( mSpecialCollections->defaultResourceId() );
        createRequestedFolders(resjob, mDefaultFolders);
    } else {
        // This is not the default resource.
        QHash<QByteArray, bool> requestedFolders = mFoldersForResource[resourceId];
        mFoldersForResource.remove(resourceId);
        createRequestedFolders(resjob, requestedFolders);
    }
}

void SpecialCollectionsRequestJobPrivate::createRequestedFolders(ResourceScanJob *scanJob,
        QHash<QByteArray, bool> &requestedFolders)
{
    // Remove from the request list the folders which already exist.
    const Akonadi::Collection::List lstSpecialCols = scanJob->specialCollections();
    for (const Collection &collection : lstSpecialCols) {
        Q_ASSERT(collection.hasAttribute<SpecialCollectionAttribute>());
        const auto *attr = collection.attribute<SpecialCollectionAttribute>();
        const QByteArray type = attr->collectionType();

        if (!type.isEmpty()) {
            mToRegister.append(qMakePair(collection, type));
            requestedFolders.insert(type, false);
        }
    }
    mToForget.append(scanJob->resourceId());

    // Folders left in the request list must be created.
    Q_ASSERT(mPendingCreateJobs == 0);
    Q_ASSERT(scanJob->rootResourceCollection().isValid());

    QHashIterator<QByteArray, bool> it(requestedFolders);
    while (it.hasNext()) {
        it.next();

        if (it.value()) {
            Collection collection;
            collection.setParentCollection(scanJob->rootResourceCollection());
            collection.setName(mNameForTypeMap.value(it.key()));

            setCollectionAttributes(collection, it.key(), mNameForTypeMap, mIconForTypeMap);

            auto *createJob = new CollectionCreateJob(collection, q);
            createJob->setProperty("type", it.key());
            QObject::connect(createJob, &KJob::result, q, [this](KJob *job) { collectionCreateResult(job); });

            mPendingCreateJobs++;
        }
    }

    if (mPendingCreateJobs == 0) {
        nextResource();
    }
}

void SpecialCollectionsRequestJobPrivate::collectionCreateResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Failed CollectionCreateJob." << job->errorString();
        return;
    }

    auto *createJob = qobject_cast<CollectionCreateJob *>(job);
    Q_ASSERT(createJob);

    const Collection collection = createJob->collection();
    mToRegister.append(qMakePair(collection, createJob->property("type").toByteArray()));

    Q_ASSERT(mPendingCreateJobs > 0);
    mPendingCreateJobs--;
    qCDebug(AKONADICORE_LOG) << "mPendingCreateJobs now" << mPendingCreateJobs;

    if (mPendingCreateJobs == 0) {
        nextResource();
    }
}

// TODO KDE5: do not inherit from TransactionSequence
SpecialCollectionsRequestJob::SpecialCollectionsRequestJob(SpecialCollections *collections, QObject *parent)
    : TransactionSequence(parent)
    , d(new SpecialCollectionsRequestJobPrivate(collections, this))
{
    setProperty("transactionsDisabled", true);
}

SpecialCollectionsRequestJob::~SpecialCollectionsRequestJob()
{
    delete d;
}

void SpecialCollectionsRequestJob::requestDefaultCollection(const QByteArray &type)
{
    d->mDefaultFolders[type] = true;
    d->mRequestingDefaultFolders = true;
    d->mRequestedType = type;
}

void SpecialCollectionsRequestJob::requestCollection(const QByteArray &type, const AgentInstance &instance)
{
    d->mFoldersForResource[instance.identifier()][type] = true;
    d->mRequestedType = type;
    d->mRequestedResource = instance;
}

Akonadi::Collection SpecialCollectionsRequestJob::collection() const
{
    if (d->mRequestedResource.isValid()) {
        return d->mSpecialCollections->collection(d->mRequestedType, d->mRequestedResource);
    } else {
        return d->mSpecialCollections->defaultCollection(d->mRequestedType);
    }
}

void SpecialCollectionsRequestJob::setDefaultResourceType(const QString &type)
{
    d->mDefaultResourceType = type;
}

void SpecialCollectionsRequestJob::setDefaultResourceOptions(const QVariantMap &options)
{
    d->mDefaultResourceOptions = options;
}

void SpecialCollectionsRequestJob::setTypes(const QList<QByteArray> &types)
{
    d->mKnownTypes = types;
}

void SpecialCollectionsRequestJob::setNameForTypeMap(const QMap<QByteArray, QString> &map)
{
    d->mNameForTypeMap = map;
}

void SpecialCollectionsRequestJob::setIconForTypeMap(const QMap<QByteArray, QString> &map)
{
    d->mIconForTypeMap = map;
}

void SpecialCollectionsRequestJob::doStart()
{
    if (d->isEverythingReady()) {
        emitResult();
    } else {
        auto *lockJob = new GetLockJob(this);
        connect(lockJob, &GetLockJob::result, this, [this](KJob*job) { d->lockResult(job);});
        lockJob->start();
    }
}

void SpecialCollectionsRequestJob::slotResult(KJob *job)
{
    if (job->error()) {
        // If we failed, let others try.
        qCWarning(AKONADICORE_LOG) << "Failed SpecialCollectionsRequestJob::slotResult" << job->errorString();

        d->releaseLock();
    }
    TransactionSequence::slotResult(job);
}

#include "moc_specialcollectionsrequestjob.cpp"
