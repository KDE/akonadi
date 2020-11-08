/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionfetchjob.h"

#include "job_p.h"
#include "protocolhelper_p.h"
#include "collection_p.h"
#include "collectionfetchscope.h"
#include "collectionutils.h"
#include "private/protocol_p.h"

#include "akonadicore_debug.h"


#include <KLocalizedString>

#include <QObject>
#include <QHash>
#include <QTimer>

using namespace Akonadi;

class Akonadi::CollectionFetchJobPrivate : public JobPrivate
{
public:
    explicit CollectionFetchJobPrivate(CollectionFetchJob *parent)
        : JobPrivate(parent)
    {
        mEmitTimer.setSingleShot(true);
        mEmitTimer.setInterval(std::chrono::milliseconds{100});
    }

    void init()
    {
        QObject::connect(&mEmitTimer, &QTimer::timeout, q_ptr, [this]() { timeout(); });
    }

    Q_DECLARE_PUBLIC(CollectionFetchJob)

    CollectionFetchJob::Type mType = CollectionFetchJob::Base;
    Collection mBase;
    Collection::List mBaseList;
    Collection::List mCollections;
    CollectionFetchScope mScope;
    Collection::List mPendingCollections;
    QTimer mEmitTimer;
    bool mBasePrefetch = false;
    Collection::List mPrefetchList;

    void aboutToFinish() override {
        timeout();
    }

    void timeout()
    {
        Q_Q(CollectionFetchJob);

        mEmitTimer.stop(); // in case we are called by result()
        if (!mPendingCollections.isEmpty()) {
            if (!q->error() || mScope.ignoreRetrievalErrors()) {
                Q_EMIT q->collectionsReceived(mPendingCollections);
            }
            mPendingCollections.clear();
        }
    }

    void subJobCollectionReceived(const Akonadi::Collection::List &collections)
    {
        mPendingCollections += collections;
        if (!mEmitTimer.isActive()) {
            mEmitTimer.start();
        }
    }

    QString jobDebuggingString() const override
    {
        if (mBase.isValid()) {
            return QStringLiteral("Collection Id %1").arg(mBase.id());
        } else if (CollectionUtils::hasValidHierarchicalRID(mBase)) {
            //return QLatin1String("(") + ProtocolHelper::hierarchicalRidToScope(mBase).hridChain().join(QLatin1String(", ")) + QLatin1Char(')');
            return QStringLiteral("HRID chain");
        } else {
            return QStringLiteral("Collection RemoteId %1").arg(mBase.remoteId());
        }
    }

    bool jobFailed(KJob *job)
    {
        Q_Q(CollectionFetchJob);
        if (mScope.ignoreRetrievalErrors()) {
            int error = job->error();
            if (error && !q->error()) {
                q->setError(error);
                q->setErrorText(job->errorText());
            }

            return error == Job::ConnectionFailed
                    || error == Job::ProtocolVersionMismatch
                    || error == Job::UserCanceled;
        } else {
            return job->error();
        }
    }
};

CollectionFetchJob::CollectionFetchJob(const Collection &collection, Type type, QObject *parent)
    : Job(new CollectionFetchJobPrivate(this), parent)
{
    Q_D(CollectionFetchJob);
    d->init();

    d->mBase = collection;
    d->mType = type;
}

CollectionFetchJob::CollectionFetchJob(const Collection::List &cols, QObject *parent)
    : Job(new CollectionFetchJobPrivate(this), parent)
{
    Q_D(CollectionFetchJob);
    d->init();

    Q_ASSERT(!cols.isEmpty());
    if (cols.size() == 1) {
        d->mBase = cols.first();
    } else {
        d->mBaseList = cols;
    }
    d->mType = CollectionFetchJob::Base;
}

CollectionFetchJob::CollectionFetchJob(const Collection::List &cols, Type type, QObject *parent)
    : Job(new CollectionFetchJobPrivate(this), parent)
{
    Q_D(CollectionFetchJob);
    d->init();

    Q_ASSERT(!cols.isEmpty());
    if (cols.size() == 1) {
        d->mBase = cols.first();
    } else {
        d->mBaseList = cols;
    }
    d->mType = type;
}

CollectionFetchJob::CollectionFetchJob(const QList<Collection::Id> &cols, Type type, QObject *parent)
    : Job(new CollectionFetchJobPrivate(this), parent)
{
    Q_D(CollectionFetchJob);
    d->init();

    Q_ASSERT(!cols.isEmpty());
    if (cols.size() == 1) {
        d->mBase = Collection(cols.first());
    } else {
        for (Collection::Id id : cols) {
            d->mBaseList.append(Collection(id));
        }
    }
    d->mType = type;
}

CollectionFetchJob::~CollectionFetchJob() = default;

Akonadi::Collection::List CollectionFetchJob::collections() const
{
    Q_D(const CollectionFetchJob);

    return d->mCollections;
}

void CollectionFetchJob::doStart()
{
    Q_D(CollectionFetchJob);

    if (!d->mBaseList.isEmpty()) {
        if (d->mType == Recursive) {
            // Because doStart starts several subjobs and @p cols could contain descendants of
            // other elements in the list, if type is Recursive, we could end up with duplicates in the result.
            // To fix this we require an initial fetch of @p cols with Base and RetrieveAncestors,
            // Iterate over that result removing intersections and then perform the Recursive fetch on
            // the remainder.
            d->mBasePrefetch = true;
            // No need to connect to the collectionsReceived signal here. This job is internal. The
            // result needs to be filtered through filterDescendants before it is useful.
            new CollectionFetchJob(d->mBaseList, NonOverlappingRoots, this);
        } else if (d->mType == NonOverlappingRoots) {
            for (const Collection &col : qAsConst(d->mBaseList)) {
                // No need to connect to the collectionsReceived signal here. This job is internal. The (aggregated)
                // result needs to be filtered through filterDescendants before it is useful.
                auto *subJob = new CollectionFetchJob(col, Base, this);
                subJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
            }
        } else {
            for (const Collection &col : qAsConst(d->mBaseList)) {
                auto *subJob = new CollectionFetchJob(col, d->mType, this);
                connect(subJob, &CollectionFetchJob::collectionsReceived, this, [d](const auto &cols) { d->subJobCollectionReceived(cols); });
                subJob->setFetchScope(fetchScope());
            }
        }
        return;
    }

    if (!d->mBase.isValid() && d->mBase.remoteId().isEmpty()) {
        setError(Unknown);
        setErrorText(i18n("Invalid collection given."));
        emitResult();
        return;
    }

    const auto cmd = Protocol::FetchCollectionsCommandPtr::create(ProtocolHelper::entityToScope(d->mBase));
    switch (d->mType) {
    case Base:
        cmd->setDepth(Protocol::FetchCollectionsCommand::BaseCollection);
        break;
    case Akonadi::CollectionFetchJob::FirstLevel:
        cmd->setDepth(Protocol::FetchCollectionsCommand::ParentCollection);
        break;
    case Akonadi::CollectionFetchJob::Recursive:
        cmd->setDepth(Protocol::FetchCollectionsCommand::AllCollections);
        break;
    default:
        Q_ASSERT(false);
    }
    cmd->setResource(d->mScope.resource());
    cmd->setMimeTypes(d->mScope.contentMimeTypes());

    switch (d->mScope.listFilter()) {
    case CollectionFetchScope::Display:
        cmd->setDisplayPref(true);
        break;
    case CollectionFetchScope::Sync:
        cmd->setSyncPref(true);
        break;
    case CollectionFetchScope::Index:
        cmd->setIndexPref(true);
        break;
    case CollectionFetchScope::Enabled:
        cmd->setEnabled(true);
        break;
    case CollectionFetchScope::NoFilter:
        break;
    default:
        Q_ASSERT(false);
    }

    cmd->setFetchStats(d->mScope.includeStatistics());
    switch (d->mScope.ancestorRetrieval()) {
    case CollectionFetchScope::None:
        cmd->setAncestorsDepth(Protocol::Ancestor::NoAncestor);
        break;
    case CollectionFetchScope::Parent:
        cmd->setAncestorsDepth(Protocol::Ancestor::ParentAncestor);
        break;
    case CollectionFetchScope::All:
        cmd->setAncestorsDepth(Protocol::Ancestor::AllAncestors);
        break;
    }
    if (d->mScope.ancestorRetrieval() != CollectionFetchScope::None) {
        cmd->setAncestorsAttributes(d->mScope.ancestorFetchScope().attributes());
    }

    d->sendCommand(cmd);
}

bool CollectionFetchJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(CollectionFetchJob);

    if (d->mBasePrefetch || d->mType == NonOverlappingRoots) {
        return false;
    }

    if (!response->isResponse() || response->type() != Protocol::Command::FetchCollections) {
        return Job::doHandleResponse(tag, response);
    }

    const auto &resp = Protocol::cmdCast<Protocol::FetchCollectionsResponse>(response);
    // Invalid response (no ID) means this was the last response
    if (resp.id() == -1) {
        return true;
    }

    Collection collection = ProtocolHelper::parseCollection(resp, true);
    if (!collection.isValid()) {
        return false;
    }

    collection.d_ptr->resetChangeLog();
    d->mCollections.append(collection);
    d->mPendingCollections.append(collection);
    if (!d->mEmitTimer.isActive()) {
        d->mEmitTimer.start();
    }

    return false;
}

static Collection::List filterDescendants(const Collection::List &list)
{
    Collection::List result;

    QVector<QList<Collection::Id> > ids;
    ids.reserve(list.count());
    for (const Collection &collection : list) {
        QList<Collection::Id> ancestors;
        Collection parent = collection.parentCollection();
        ancestors << parent.id();
        if (parent != Collection::root()) {
            while (parent.parentCollection() != Collection::root()) {
                parent = parent.parentCollection();
                QList<Collection::Id>::iterator i = std::lower_bound(ancestors.begin(), ancestors.end(), parent.id());
                ancestors.insert(i, parent.id());
            }
        }
        ids << ancestors;
    }

    QSet<Collection::Id> excludeList;
    for (const Collection &collection : list) {
        int i = 0;
        for (const QList<Collection::Id> &ancestors : qAsConst(ids)) {
            if (std::binary_search(ancestors.cbegin(), ancestors.cend(), collection.id())) {
                excludeList.insert(list.at(i).id());
            }
            ++i;
        }
    }

    for (const Collection &collection : list) {
        if (!excludeList.contains(collection.id())) {
            result.append(collection);
        }
    }

    return result;
}

void CollectionFetchJob::slotResult(KJob *job)
{
    Q_D(CollectionFetchJob);

    auto *list = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(job);

    if (d->mType == NonOverlappingRoots) {
        d->mPrefetchList += list->collections();
    } else if (!d->mBasePrefetch) {
        d->mCollections += list->collections();
    }

    if (d_ptr->mCurrentSubJob == job && !d->jobFailed(job)) {
        if (job->error()) {
            qCWarning(AKONADICORE_LOG) << "Error during CollectionFetchJob: " << job->errorString();
        }
        d_ptr->mCurrentSubJob = nullptr;
        removeSubjob(job);
        QTimer::singleShot(0, this, [d]() { d->startNext(); });
    } else {
        Job::slotResult(job);
    }

    if (d->mBasePrefetch) {
        d->mBasePrefetch = false;
        const Collection::List roots = list->collections();
        Q_ASSERT(!hasSubjobs());
        if (!job->error()) {
            for (const Collection &col : roots) {
                auto *subJob = new CollectionFetchJob(col, d->mType, this);
                connect(subJob, &CollectionFetchJob::collectionsReceived, this, [d](const auto &cols) { d->subJobCollectionReceived(cols); });
                subJob->setFetchScope(fetchScope());
            }
        }
        // No result yet.
    } else if (d->mType == NonOverlappingRoots) {
        if (!d->jobFailed(job) && !hasSubjobs()) {
            const Collection::List result = filterDescendants(d->mPrefetchList);
            d->mPendingCollections += result;
            d->mCollections = result;
            d->delayedEmitResult();
        }
    } else {
        if (!d->jobFailed(job) && !hasSubjobs()) {
            d->delayedEmitResult();
        }
    }
}

void CollectionFetchJob::setFetchScope(const CollectionFetchScope &scope)
{
    Q_D(CollectionFetchJob);
    d->mScope = scope;
}

CollectionFetchScope &CollectionFetchJob::fetchScope()
{
    Q_D(CollectionFetchJob);
    return d->mScope;
}

#include "moc_collectionfetchjob.cpp"
