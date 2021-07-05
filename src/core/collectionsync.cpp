/*
    SPDX-FileCopyrightText: 2007, 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadicore_debug.h"
#include "collection.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "collectionsync_p.h"

#include "cachepolicy.h"

#include <KLocalizedString>
#include <QHash>
#include <QList>

#include <functional>

using namespace Akonadi;

static const char CONTENTMIMETYPES[] = "CONTENTMIMETYPES";

static const char ROOTPARENTRID[] = "AKONADI_ROOT_COLLECTION";

class RemoteId
{
public:
    explicit RemoteId()
    {
    }

    explicit inline RemoteId(const QStringList &ridChain)
        : ridChain(ridChain)
    {
    }

    explicit inline RemoteId(const QString &rid)
    {
        ridChain.append(rid);
    }

    inline bool isAbsolute() const
    {
        return ridChain.last() == QString::fromLatin1(ROOTPARENTRID);
    }

    inline bool isEmpty() const
    {
        return ridChain.isEmpty();
    }

    inline bool operator==(const RemoteId &other) const
    {
        return ridChain == other.ridChain;
    }

    QStringList ridChain;

    static RemoteId rootRid;
};

RemoteId RemoteId::rootRid = RemoteId(QStringList() << QString::fromLatin1(ROOTPARENTRID));

Q_DECLARE_METATYPE(RemoteId)

uint qHash(const RemoteId &rid)
{
    uint hash = 0;
    for (QStringList::ConstIterator iter = rid.ridChain.constBegin(), end = rid.ridChain.constEnd(); iter != end; ++iter) {
        hash += qHash(*iter);
    }
    return hash;
}

inline bool operator<(const RemoteId &r1, const RemoteId &r2)
{
    if (r1.ridChain.length() == r2.ridChain.length()) {
        auto it1 = r1.ridChain.constBegin();
        auto end1 = r1.ridChain.constEnd();
        auto it2 = r2.ridChain.constBegin();
        while (it1 != end1) {
            if ((*it1) == (*it2)) {
                ++it1;
                ++it2;
                continue;
            }
            return (*it1) < (*it2);
        }
    } else {
        return r1.ridChain.length() < r2.ridChain.length();
    }
    return false;
}

QDebug operator<<(QDebug s, const RemoteId &rid)
{
    s.nospace() << "RemoteId(" << rid.ridChain << ")";
    return s;
}

/**
 * @internal
 */
class CollectionSync::Private
{
public:
    explicit Private(CollectionSync *parent)
        : q(parent)
        , pendingJobs(0)
        , progress(0)
        , currentTransaction(nullptr)
        , incremental(false)
        , streaming(false)
        , hierarchicalRIDs(false)
        , localListDone(false)
        , deliveryDone(false)
        , akonadiRootCollection(Collection::root())
        , resultEmitted(false)
    {
    }

    ~Private()
    {
    }

    RemoteId remoteIdForCollection(const Collection &collection) const
    {
        if (collection == Collection::root()) {
            return RemoteId::rootRid;
        }

        if (!hierarchicalRIDs) {
            return RemoteId(collection.remoteId());
        }

        RemoteId rid;
        Collection parent = collection;
        while (parent.isValid() || !parent.remoteId().isEmpty()) {
            QString prid = parent.remoteId();
            if (prid.isEmpty() && parent.isValid()) {
                prid = uidRidMap.value(parent.id());
            }
            if (prid.isEmpty()) {
                break;
            }
            rid.ridChain.append(prid);
            parent = parent.parentCollection();
            if (parent == akonadiRootCollection) {
                rid.ridChain.append(QString::fromLatin1(ROOTPARENTRID));
                break;
            }
        }
        return rid;
    }

    void addRemoteColection(const Collection &collection, bool removed = false)
    {
        QHash<RemoteId, Collection::List> &map = (removed ? removedRemoteCollections : remoteCollections);
        const Collection parentCollection = collection.parentCollection();
        if (parentCollection.remoteId() == akonadiRootCollection.remoteId() || parentCollection.id() == akonadiRootCollection.id()) {
            Collection c2(collection);
            c2.setParentCollection(akonadiRootCollection);
            map[RemoteId::rootRid].append(c2);
        } else {
            Q_ASSERT(!parentCollection.remoteId().isEmpty());
            map[remoteIdForCollection(parentCollection)].append(collection);
        }
    }

    /* Compares collections by remoteId and falls back to name comparison in case
     * local collection does not have remoteId (which can happen in some cases)
     */
    bool matchLocalAndRemoteCollection(const Collection &local, const Collection &remote)
    {
        if (!local.remoteId().isEmpty()) {
            return local.remoteId() == remote.remoteId();
        } else {
            return local.name() == remote.name();
        }
    }

    void localCollectionsReceived(const Akonadi::Collection::List &localCols)
    {
        for (const Akonadi::Collection &collection : localCols) {
            const RemoteId parentRid = remoteIdForCollection(collection.parentCollection());
            localCollections[parentRid] += collection;
        }
    }

    void processCollections(const RemoteId &parentRid)
    {
        Collection::List remoteChildren = remoteCollections.value(parentRid);
        Collection::List removedChildren = removedRemoteCollections.value(parentRid);
        Collection::List localChildren = localCollections.value(parentRid);

        // Iterate over the list of local children of localParent
        for (auto localIter = localChildren.begin(), localEnd = localChildren.end(); localIter != localEnd;) {
            const Collection localCollection = *localIter;
            bool matched = false;
            uidRidMap.insert(localIter->id(), localIter->remoteId());

            // Try to map removed remote collections (from incremental sync) to local collections
            for (auto removedIter = removedChildren.begin(), removedEnd = removedChildren.end(); removedIter != removedEnd;) {
                Collection removedCollection = *removedIter;

                if (matchLocalAndRemoteCollection(localCollection, removedCollection)) {
                    matched = true;
                    if (!localCollection.remoteId().isEmpty()) {
                        localCollectionsToRemove.append(localCollection);
                    }
                    // Remove the matched removed collection from the list so that
                    // we don't have to iterate over it again next time.
                    removedIter = removedChildren.erase(removedIter);
                    removedEnd = removedChildren.end();
                    break;
                } else {
                    // Keep looking
                    ++removedIter;
                }
            }

            if (matched) {
                // Remove the matched local collection from the list, because we
                // have already put it into localCollectionsToRemove
                localIter = localChildren.erase(localIter);
                localEnd = localChildren.end();
                continue;
            }

            // Try to find a matching collection in the list of remote children
            for (auto remoteIter = remoteChildren.begin(), remoteEnd = remoteChildren.end(); !matched && remoteIter != remoteEnd;) {
                Collection remoteCollection = *remoteIter;

                // Yay, we found a match!
                if (matchLocalAndRemoteCollection(localCollection, remoteCollection)) {
                    matched = true;

                    // "Virtual" flag cannot be updated: we need to recreate
                    // the collection from scratch.
                    if (localCollection.isVirtual() != remoteCollection.isVirtual()) {
                        // Mark the local collection and all its children for deletion and re-creation
                        QList<QPair<Collection /*local*/, Collection /*remote*/>> parents = {{localCollection, remoteCollection}};
                        while (!parents.empty()) {
                            auto parent = parents.takeFirst();
                            qCDebug(AKONADICORE_LOG) << "Local collection " << parent.first.name() << " will be recreated";
                            localCollectionsToRemove.push_back(parent.first);
                            remoteCollectionsToCreate.push_back(parent.second);
                            for (auto it = localChildren.begin(), end = localChildren.end(); it != end;) {
                                if (it->parentCollection() == parent.first) {
                                    Collection remoteParent;
                                    auto remoteIt = std::find_if(
                                        remoteChildren.begin(),
                                        remoteChildren.end(),
                                        std::bind(&CollectionSync::Private::matchLocalAndRemoteCollection, this, parent.first, std::placeholders::_1));
                                    if (remoteIt != remoteChildren.end()) {
                                        remoteParent = *remoteIt;
                                        remoteEnd = remoteChildren.erase(remoteIt);
                                    }
                                    parents.push_back({*it, remoteParent});
                                    it = localChildren.erase(it);
                                    localEnd = end = localChildren.end();
                                } else {
                                    ++it;
                                }
                            }
                        }
                    } else if (collectionNeedsUpdate(localCollection, remoteCollection)) {
                        // We need to store both local and remote collections, so that
                        // we can copy over attributes to be preserved
                        remoteCollectionsToUpdate.append(qMakePair(localCollection, remoteCollection));
                    } else {
                        // Collections are the same, no need to update anything
                    }

                    // Remove the matched remote collection from the list so that
                    // in the end we are left with list of collections that don't
                    // exist locally (i.e. new collections)
                    remoteIter = remoteChildren.erase(remoteIter);
                    remoteEnd = remoteChildren.end();
                    break;
                } else {
                    // Keep looking
                    ++remoteIter;
                }
            }

            if (matched) {
                // Remove the matched local collection from the list so that
                // in the end we are left with list of collections that don't
                // exist remotely (i.e. removed collections)
                localIter = localChildren.erase(localIter);
                localEnd = localChildren.end();
            } else {
                ++localIter;
            }
        }

        if (!removedChildren.isEmpty()) {
            removedRemoteCollections[parentRid] = removedChildren;
        } else {
            removedRemoteCollections.remove(parentRid);
        }

        if (!remoteChildren.isEmpty()) {
            remoteCollections[parentRid] = remoteChildren;
        } else {
            remoteCollections.remove(parentRid);
        }

        if (!localChildren.isEmpty()) {
            localCollections[parentRid] = localChildren;
        } else {
            localCollections.remove(parentRid);
        }
    }

    void processLocalCollections(const RemoteId &parentRid, const Collection &parentCollection)
    {
        const Collection::List originalChildren = localCollections.value(parentRid);
        processCollections(parentRid);

        const Collection::List remoteChildren = remoteCollections.take(parentRid);
        const Collection::List localChildren = localCollections.take(parentRid);

        // At this point remoteChildren contains collections that don't exist locally yet
        if (!remoteChildren.isEmpty()) {
            for (Collection c : remoteChildren) {
                c.setParentCollection(parentCollection);
                remoteCollectionsToCreate.append(c);
            }
        }
        // At this point localChildren contains collections that don't exist remotely anymore
        if (!localChildren.isEmpty() && !incremental) {
            for (const auto &c : localChildren) {
                if (!c.remoteId().isEmpty()) {
                    localCollectionsToRemove.push_back(c);
                }
            }
        }

        // Recurse into children
        for (const Collection &c : originalChildren) {
            processLocalCollections(remoteIdForCollection(c), c);
        }
    }

    void localCollectionFetchResult(KJob *job)
    {
        if (job->error()) {
            return; // handled by the base class
        }

        processLocalCollections(RemoteId::rootRid, akonadiRootCollection);
        localListDone = true;
        execute();
    }

    bool ignoreAttributeChanges(const Akonadi::Collection &col, const QByteArray &attribute) const
    {
        return (keepLocalChanges.contains(attribute) || col.keepLocalChanges().contains(attribute));
    }

    /**
      Checks if the given localCollection and remoteCollection are different
    */
    bool collectionNeedsUpdate(const Collection &localCollection, const Collection &remoteCollection) const
    {
        if (!ignoreAttributeChanges(remoteCollection, CONTENTMIMETYPES)) {
            if (localCollection.contentMimeTypes().size() != remoteCollection.contentMimeTypes().size()) {
                return true;
            } else {
                for (int i = 0, total = remoteCollection.contentMimeTypes().size(); i < total; ++i) {
                    const QString &m = remoteCollection.contentMimeTypes().at(i);
                    if (!localCollection.contentMimeTypes().contains(m)) {
                        return true;
                    }
                }
            }
        }

        if (localCollection.parentCollection().remoteId() != remoteCollection.parentCollection().remoteId()) {
            return true;
        }
        if (localCollection.name() != remoteCollection.name()) {
            return true;
        }
        if (localCollection.remoteId() != remoteCollection.remoteId()) {
            return true;
        }
        if (localCollection.remoteRevision() != remoteCollection.remoteRevision()) {
            return true;
        }
        if (!(localCollection.cachePolicy() == remoteCollection.cachePolicy())) {
            return true;
        }
        if (localCollection.enabled() != remoteCollection.enabled()) {
            return true;
        }

        // CollectionModifyJob adds the remote attributes to the local collection
        const Akonadi::Attribute::List lstAttr = remoteCollection.attributes();
        for (const Attribute *attr : lstAttr) {
            const Attribute *localAttr = localCollection.attribute(attr->type());
            if (localAttr && ignoreAttributeChanges(remoteCollection, attr->type())) {
                continue;
            }
            // The attribute must both exist and have equal contents
            if (!localAttr || localAttr->serialized() != attr->serialized()) {
                return true;
            }
        }

        return false;
    }

    void createLocalCollections()
    {
        if (remoteCollectionsToCreate.isEmpty()) {
            updateLocalCollections();
            return;
        }

        for (auto iter = remoteCollectionsToCreate.begin(), end = remoteCollectionsToCreate.end(); iter != end;) {
            const Collection col = *iter;
            const Collection parentCollection = col.parentCollection();
            // The parent already exists locally
            if (parentCollection == akonadiRootCollection || parentCollection.id() > 0) {
                ++pendingJobs;
                auto create = new CollectionCreateJob(col, currentTransaction);
                QObject::connect(create, &KJob::result, q, [this](KJob *job) {
                    createLocalCollectionResult(job);
                });

                // Commit transaction after every 100 collections are created,
                // otherwise it overloads database journal and things get veeery slow
                if (pendingJobs % 100 == 0) {
                    currentTransaction->commit();
                    createTransaction();
                }

                iter = remoteCollectionsToCreate.erase(iter);
                end = remoteCollectionsToCreate.end();
            } else {
                // Skip the collection, we'll try again once we create all the other
                // collection we already have a parent for
                ++iter;
            }
        }
    }

    void createLocalCollectionResult(KJob *job)
    {
        --pendingJobs;
        if (job->error()) {
            return; // handled by the base class
        }

        q->setProcessedAmount(KJob::Bytes, ++progress);

        const Collection newLocal = static_cast<CollectionCreateJob *>(job)->collection();
        uidRidMap.insert(newLocal.id(), newLocal.remoteId());
        const RemoteId newLocalRID = remoteIdForCollection(newLocal);

        // See if there are any pending collections that this collection is parent of and
        // update them if so
        for (auto iter = remoteCollectionsToCreate.begin(), end = remoteCollectionsToCreate.end(); iter != end; ++iter) {
            const Collection parentCollection = iter->parentCollection();
            if (parentCollection != akonadiRootCollection && parentCollection.id() <= 0) {
                const RemoteId remoteRID = remoteIdForCollection(*iter);
                if (remoteRID.isAbsolute()) {
                    if (newLocalRID == remoteIdForCollection(*iter)) {
                        iter->setParentCollection(newLocal);
                    }
                } else if (!hierarchicalRIDs) {
                    if (remoteRID.ridChain.startsWith(parentCollection.remoteId())) {
                        iter->setParentCollection(newLocal);
                    }
                }
            }
        }

        // Enqueue all pending remote collections that are children of the just-created
        // collection
        Collection::List collectionsToCreate = remoteCollections.take(newLocalRID);
        if (collectionsToCreate.isEmpty() && !hierarchicalRIDs) {
            collectionsToCreate = remoteCollections.take(RemoteId(newLocal.remoteId()));
        }
        for (Collection col : std::as_const(collectionsToCreate)) {
            col.setParentCollection(newLocal);
            remoteCollectionsToCreate.append(col);
        }

        // If there are still any collections to create left, try if we just created
        // a parent for any of them
        if (!remoteCollectionsToCreate.isEmpty()) {
            createLocalCollections();
        } else if (pendingJobs == 0) {
            Q_ASSERT(remoteCollectionsToCreate.isEmpty());
            if (!remoteCollections.isEmpty()) {
                currentTransaction->rollback();
                q->setError(Unknown);
                q->setErrorText(i18n("Found unresolved orphan collections"));
                qCWarning(AKONADICORE_LOG) << "found unresolved orphan collection";
                emitResult();
                return;
            }

            currentTransaction->commit();
            createTransaction();

            // Otherwise move to next task: updating existing collections
            updateLocalCollections();
        }
        /*
         * else if (!remoteCollections.isEmpty()) {
            currentTransaction->rollback();
            q->setError(Unknown);
            q->setErrorText(i18n("Incomplete collection tree"));
            emitResult();
            return;
        }
        */
    }

    /**
      Performs a local update for the given node pair.
    */
    void updateLocalCollections()
    {
        if (remoteCollectionsToUpdate.isEmpty()) {
            deleteLocalCollections();
            return;
        }

        using CollectionPair = QPair<Collection, Collection>;
        for (const CollectionPair &pair : std::as_const(remoteCollectionsToUpdate)) {
            const Collection local = pair.first;
            const Collection remote = pair.second;
            Collection upd(remote);

            Q_ASSERT(!upd.remoteId().isEmpty());
            Q_ASSERT(currentTransaction);
            upd.setId(local.id());
            if (ignoreAttributeChanges(remote, CONTENTMIMETYPES)) {
                upd.setContentMimeTypes(local.contentMimeTypes());
            }
            Q_FOREACH (Attribute *remoteAttr, upd.attributes()) {
                if (ignoreAttributeChanges(remote, remoteAttr->type()) && local.hasAttribute(remoteAttr->type())) {
                    // We don't want to overwrite the attribute changes with the defaults provided by the resource.
                    const Attribute *localAttr = local.attribute(remoteAttr->type());
                    upd.removeAttribute(localAttr->type());
                    upd.addAttribute(localAttr->clone());
                }
            }

            // ### HACK to work around the implicit move attempts of CollectionModifyJob
            // which we do explicitly below
            Collection c(upd);
            c.setParentCollection(local.parentCollection());
            ++pendingJobs;
            auto mod = new CollectionModifyJob(c, currentTransaction);
            QObject::connect(mod, &KJob::result, q, [this](KJob *job) {
                updateLocalCollectionResult(job);
            });

            // detecting moves is only possible with global RIDs
            if (!hierarchicalRIDs) {
                if (remote.parentCollection().isValid() && remote.parentCollection().id() != local.parentCollection().id()) {
                    ++pendingJobs;
                    auto move = new CollectionMoveJob(upd, remote.parentCollection(), currentTransaction);
                    QObject::connect(move, &KJob::result, q, [this](KJob *job) {
                        updateLocalCollectionResult(job);
                    });
                }
            }
        }
    }

    void updateLocalCollectionResult(KJob *job)
    {
        --pendingJobs;
        if (job->error()) {
            return; // handled by the base class
        }
        if (qobject_cast<CollectionModifyJob *>(job)) {
            q->setProcessedAmount(KJob::Bytes, ++progress);
        }

        // All updates are done, time to move on to next task: deletion
        if (pendingJobs == 0) {
            currentTransaction->commit();
            createTransaction();

            deleteLocalCollections();
        }
    }

    void deleteLocalCollections()
    {
        if (localCollectionsToRemove.isEmpty()) {
            done();
            return;
        }

        for (const Collection &col : std::as_const(localCollectionsToRemove)) {
            Q_ASSERT(!col.remoteId().isEmpty()); // empty RID -> stuff we haven't even written to the remote side yet

            ++pendingJobs;
            Q_ASSERT(currentTransaction);
            auto job = new CollectionDeleteJob(col, currentTransaction);
            connect(job, &KJob::result, q, [this](KJob *job) {
                deleteLocalCollectionsResult(job);
            });

            // It can happen that the groupware servers report us deleted collections
            // twice, in this case this collection delete job will fail on the second try.
            // To avoid a rollback of the complete transaction we gracefully allow the job
            // to fail :)
            currentTransaction->setIgnoreJobFailure(job);
        }
    }

    void deleteLocalCollectionsResult(KJob * /*unused*/)
    {
        --pendingJobs;
        q->setProcessedAmount(KJob::Bytes, ++progress);

        if (pendingJobs == 0) {
            currentTransaction->commit();
            currentTransaction = nullptr;

            done();
        }
    }

    void done()
    {
        if (currentTransaction) {
            // This can trigger a direct call of transactionSequenceResult
            currentTransaction->commit();
            currentTransaction = nullptr;
        }

        if (!remoteCollections.isEmpty()) {
            q->setError(Unknown);
            q->setErrorText(i18n("Found unresolved orphan collections"));
        }
        emitResult();
    }

    void emitResult()
    {
        // Prevent double result emission
        Q_ASSERT(!resultEmitted);
        if (!resultEmitted) {
            if (q->hasSubjobs()) {
                // If there are subjobs, pick one, wait for it to finish, then
                // try again. This way we make sure we don't emit result() signal
                // while there is still a Transaction job running
                KJob *subjob = q->subjobs().at(0);
                connect(
                    subjob,
                    &KJob::result,
                    q,
                    [this](KJob * /*unused*/) {
                        emitResult();
                    },
                    Qt::QueuedConnection);
            } else {
                resultEmitted = true;
                q->emitResult();
            }
        }
    }

    void createTransaction()
    {
        currentTransaction = new TransactionSequence(q);
        currentTransaction->setAutomaticCommittingEnabled(false);
        q->connect(currentTransaction, &TransactionSequence::finished, q, [this](KJob *job) {
            transactionSequenceResult(job);
        });
    }

    /** After the transaction has finished report we're done as well. */
    void transactionSequenceResult(KJob *job)
    {
        if (job->error()) {
            return; // handled by the base class
        }

        // If this was the last transaction, then finish, otherwise there's
        // a new transaction in the queue already
        if (job == currentTransaction) {
            currentTransaction = nullptr;
        }
    }

    /**
      Process what's currently available.
    */
    void execute()
    {
        qCDebug(AKONADICORE_LOG) << "localListDone: " << localListDone << " deliveryDone: " << deliveryDone;
        if (!localListDone && !deliveryDone) {
            return;
        }

        if (!localListDone && deliveryDone) {
            Job *parent = (currentTransaction ? static_cast<Job *>(currentTransaction) : static_cast<Job *>(q));
            auto job = new CollectionFetchJob(akonadiRootCollection, CollectionFetchJob::Recursive, parent);
            job->fetchScope().setResource(resourceId);
            job->fetchScope().setListFilter(CollectionFetchScope::NoFilter);
            job->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
            q->connect(job, &CollectionFetchJob::collectionsReceived, q, [this](const auto &cols) {
                localCollectionsReceived(cols);
            });
            q->connect(job, &KJob::result, q, [this](KJob *job) {
                localCollectionFetchResult(job);
            });
            return;
        }

        // If a transaction is not started yet, it means we just finished local listing
        if (!currentTransaction) {
            // There's nothing to do after local listing -> we are done!
            if (remoteCollectionsToCreate.isEmpty() && remoteCollectionsToUpdate.isEmpty() && localCollectionsToRemove.isEmpty()) {
                qCDebug(AKONADICORE_LOG) << "Nothing to do";
                emitResult();
                return;
            }
            // Ok, there's some work to do, so create a transaction we can use
            createTransaction();
        }

        createLocalCollections();
    }

    CollectionSync *const q;

    QString resourceId;

    int pendingJobs;
    int progress;

    TransactionSequence *currentTransaction;

    bool incremental;
    bool streaming;
    bool hierarchicalRIDs;

    bool localListDone;
    bool deliveryDone;

    // List of parts where local changes should not be overwritten
    QSet<QByteArray> keepLocalChanges;

    QHash<RemoteId /* parent */, Collection::List /* children */> removedRemoteCollections;
    QHash<RemoteId /* parent */, Collection::List /* children */> remoteCollections;
    QHash<RemoteId /* parent */, Collection::List /* children */> localCollections;

    Collection::List localCollectionsToRemove;
    Collection::List remoteCollectionsToCreate;
    QList<QPair<Collection /* local */, Collection /* remote */>> remoteCollectionsToUpdate;
    QHash<Collection::Id, QString> uidRidMap;

    // HACK: To workaround Collection copy constructor being very expensive, we
    // store the Collection::root() collection in a variable here for faster
    // access
    Collection akonadiRootCollection;

    bool resultEmitted;
};

CollectionSync::CollectionSync(const QString &resourceId, QObject *parent)
    : Job(parent)
    , d(new Private(this))
{
    d->resourceId = resourceId;
    setTotalAmount(KJob::Bytes, 0);
}

CollectionSync::~CollectionSync()
{
    delete d;
}

void CollectionSync::setRemoteCollections(const Collection::List &remoteCollections)
{
    setTotalAmount(KJob::Bytes, totalAmount(KJob::Bytes) + remoteCollections.count());
    for (const Collection &c : remoteCollections) {
        d->addRemoteColection(c);
    }

    if (!d->streaming) {
        d->deliveryDone = true;
    }
    d->execute();
}

void CollectionSync::setRemoteCollections(const Collection::List &changedCollections, const Collection::List &removedCollections)
{
    setTotalAmount(KJob::Bytes, totalAmount(KJob::Bytes) + changedCollections.count());
    d->incremental = true;
    for (const Collection &c : changedCollections) {
        d->addRemoteColection(c);
    }
    for (const Collection &c : removedCollections) {
        d->addRemoteColection(c, true);
    }

    if (!d->streaming) {
        d->deliveryDone = true;
    }
    d->execute();
}

void CollectionSync::doStart()
{
}

void CollectionSync::setStreamingEnabled(bool streaming)
{
    d->streaming = streaming;
}

void CollectionSync::retrievalDone()
{
    d->deliveryDone = true;
    d->execute();
}

void CollectionSync::setHierarchicalRemoteIds(bool hierarchical)
{
    d->hierarchicalRIDs = hierarchical;
}

void CollectionSync::rollback()
{
    if (d->currentTransaction) {
        d->currentTransaction->rollback();
    } else {
        setError(UserCanceled);
        emitResult();
    }
}

void CollectionSync::setKeepLocalChanges(const QSet<QByteArray> &parts)
{
    d->keepLocalChanges = parts;
}

#include "moc_collectionsync_p.cpp"
