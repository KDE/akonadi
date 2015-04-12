/*
    Copyright (c) 2007, 2009 Volker Krause <vkrause@kde.org>

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

#include "collectionsync_p.h"
#include "collection.h"

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionfetchscope.h"
#include "collectionmovejob.h"

#include "cachepolicy.h"

#include <qdebug.h>
#include <KLocalizedString>
#include <QtCore/QVariant>
#include <QTime>
#include <QHash>

using namespace Akonadi;

static const char CONTENTMIMETYPES[] = "CONTENTMIMETYPES";

static const char ROOTPARENTRID[] = "AKONADI_ROOT_COLLECTION";

static const char PARENTCOLLECTIONRID[] = "ParentCollectionRid";
static const char PARENTCOLLECTION[] = "ParentCollection";

class RemoteId
{
public:
    explicit RemoteId()
    {
    }

    explicit inline RemoteId(const QStringList &ridChain):
        ridChain(ridChain)
    {
    }

    explicit inline RemoteId(const QString &rid)
    {
        ridChain.append(rid);
    }

    inline ~RemoteId()
    {
    }

    inline bool isAbsolute() const
    {
        return ridChain.last() == QString::fromAscii(ROOTPARENTRID);
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

RemoteId RemoteId::rootRid = RemoteId(QStringList() << QString::fromAscii(ROOTPARENTRID));

Q_DECLARE_METATYPE(RemoteId);


uint qHash(const RemoteId &rid)
{
    uint hash = 0;
    for (QStringList::ConstIterator iter = rid.ridChain.constBegin(),
                                    end = rid.ridChain.constEnd();
         iter != end;
         ++iter)
    {
        hash += qHash(*iter);
    }
    return hash;
}

inline bool operator<(const RemoteId &r1, const RemoteId &r2)
{
    if (r1.ridChain.length() == r2.ridChain.length()) {
        QStringList::ConstIterator it1 = r1.ridChain.constBegin(),
                                   end1 = r1.ridChain.constEnd(),
                                   it2 = r2.ridChain.constBegin();
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

QDebug operator<<(QDebug s, const RemoteId &rid) {
    s.nospace() << "RemoteId(" << rid.ridChain << ")";
    return s;
}

/**
 * @internal
 */
class CollectionSync::Private
{
public:
    Private(CollectionSync *parent)
        : q(parent)
        , pendingJobs(0)
        , progress(0)
        , currentTransaction(0)
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
                rid.ridChain.append(QString::fromAscii(ROOTPARENTRID));
                break;
            }
        }
        return rid;
    }

    void addRemoteColection(const Collection &collection, bool removed = false)
    {
        QHash<RemoteId, QList<Collection> > &map = (removed ? removedRemoteCollections : remoteCollections);
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

    /* Compares collections by remoteId and falls back to name comparision in case
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
        Q_FOREACH (const Akonadi::Collection &collection, localCols) {
            const RemoteId parentRid = remoteIdForCollection(collection.parentCollection());
            localCollections[parentRid] += collection;
        }
    }

    void processCollections(const RemoteId &parentRid)
    {
        QList<Collection> remoteChildren = remoteCollections.value(parentRid);
        QList<Collection> removedChildren = removedRemoteCollections.value(parentRid);
        QList<Collection> localChildren = localCollections.value(parentRid);

        // Iterate over the list of local children of localParent
        QList<Collection>::Iterator localIter, localEnd,
                                    removedIter, removedEnd,
                                    remoteIter, remoteEnd;

        for (localIter = localChildren.begin(), localEnd = localChildren.end(); localIter != localEnd;)
        {
            const Collection localCollection = *localIter;
            bool matched = false;
            uidRidMap.insert(localIter->id(), localIter->remoteId());

            // Try to map removed remote collections (from incremental sync) to local collections
            for (removedIter = removedChildren.begin(), removedEnd = removedChildren.end(); removedIter != removedEnd;)
            {
                Collection removedCollection = *removedIter;

                if (matchLocalAndRemoteCollection(localCollection, removedCollection)) {
                    matched = true;
                    localCollectionsToRemove.append(localCollection);
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
            for (remoteIter = remoteChildren.begin(), remoteEnd = remoteChildren.end(); !matched && remoteIter != remoteEnd;)
            {
                Collection remoteCollection = *remoteIter;

                // Yay, we found a match!
                if (matchLocalAndRemoteCollection(localCollection, remoteCollection)) {
                    matched = true;

                    // Check if the local and remote collections differ and thus if
                    // we need to update it
                    if (collectionNeedsUpdate(localCollection, remoteCollection)) {
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
        const QList<Collection> originalChildren = localCollections.value(parentRid);
        processCollections(parentRid);

        const QList<Collection> remoteChildren = remoteCollections.take(parentRid);
        const QList<Collection> localChildren = localCollections.take(parentRid);

        // At this point remoteChildren contains collections that don't exist locally yet
        if (!remoteChildren.isEmpty()) {
            Q_FOREACH (Collection c, remoteChildren) {
                c.setParentCollection(parentCollection);
                remoteCollectionsToCreate.append(c);
            }
        }
        // At this point localChildren contains collections that don't exist remotely anymore
        if (!localChildren.isEmpty() && !incremental) {
            localCollectionsToRemove += localChildren;
        }

        // Recurse into children
        Q_FOREACH (const Collection &c, originalChildren) {
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
                for (int i = 0; i < remoteCollection.contentMimeTypes().size(); i++) {
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
        Q_FOREACH (const Attribute *attr, remoteCollection.attributes()) {
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

        Collection::List::Iterator iter, end;
        for (iter = remoteCollectionsToCreate.begin(), end = remoteCollectionsToCreate.end(); iter != end;) {
            const Collection col = *iter;
            const Collection parentCollection = col.parentCollection();
            // The parent already exists locally
            if (parentCollection == akonadiRootCollection || parentCollection.id() > 0) {
                ++pendingJobs;
                CollectionCreateJob *create = new CollectionCreateJob(col, currentTransaction);
                connect(create, SIGNAL(result(KJob*)),
                        q, SLOT(createLocalCollectionResult(KJob*)));

                // Commit transaction after every 100 collections are created,
                // otherwise it overlads database journal and things get veeery slow
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
        Collection::List::Iterator iter, end;
        for (iter = remoteCollectionsToCreate.begin(), end = remoteCollectionsToCreate.end(); iter != end; ++iter) {
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
        QList<Collection> collectionsToCreate = remoteCollections.take(newLocalRID);
        if (collectionsToCreate.isEmpty() && !hierarchicalRIDs) {
            collectionsToCreate = remoteCollections.take(RemoteId(newLocal.remoteId()));
        }
        Q_FOREACH (Collection col, collectionsToCreate) {
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
                qWarning() << "found unresolved orphan collection";
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

        typedef QPair<Collection, Collection> CollectionPair;
        Q_FOREACH (const CollectionPair &pair, remoteCollectionsToUpdate) {
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
                    //We don't want to overwrite the attribute changes with the defaults provided by the resource.
                    Attribute *localAttr = local.attribute(remoteAttr->type());
                    upd.removeAttribute(localAttr->type());
                    upd.addAttribute(localAttr->clone());
                }
            }

            // ### HACK to work around the implicit move attempts of CollectionModifyJob
            // which we do explicitly below
            Collection c(upd);
            c.setParentCollection(local.parentCollection());
            ++pendingJobs;
            CollectionModifyJob *mod = new CollectionModifyJob(c, currentTransaction);
            connect(mod, SIGNAL(result(KJob*)), q, SLOT(updateLocalCollectionResult(KJob*)));

            // detecting moves is only possible with global RIDs
            if (!hierarchicalRIDs) {
                if (remote.parentCollection().isValid() && remote.parentCollection().id() != local.parentCollection().id()) {
                    ++pendingJobs;
                    CollectionMoveJob *move = new CollectionMoveJob(upd, remote.parentCollection(), currentTransaction);
                    connect(move, SIGNAL(result(KJob*)), q, SLOT(updateLocalCollectionResult(KJob*)));
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

       Q_FOREACH (const Collection &col, localCollectionsToRemove) {
            Q_ASSERT(!col.remoteId().isEmpty());   // empty RID -> stuff we haven't even written to the remote side yet

            ++pendingJobs;
            Q_ASSERT(currentTransaction);
            CollectionDeleteJob *job = new CollectionDeleteJob(col, currentTransaction);
            connect(job, SIGNAL(result(KJob*)), q, SLOT(deleteLocalCollectionsResult(KJob*)));

            // It can happen that the groupware servers report us deleted collections
            // twice, in this case this collection delete job will fail on the second try.
            // To avoid a rollback of the complete transaction we gracefully allow the job
            // to fail :)
            currentTransaction->setIgnoreJobFailure(job);
        }
    }

    void deleteLocalCollectionsResult(KJob *)
    {
        --pendingJobs;
        q->setProcessedAmount(KJob::Bytes, ++progress);

        if (pendingJobs == 0) {
            currentTransaction->commit();
            currentTransaction = 0;

            done();
        }
    }

    void done()
    {
        if (currentTransaction) {
            //This can trigger a direct call of transactionSequenceResult
            currentTransaction->commit();
            currentTransaction = 0;
        }

        if (!remoteCollections.isEmpty()) {
            q->setError(Unknown);
            q->setErrorText(i18n("Found unresolved orphan collections"));
        }
        emitResult();
    }

    void emitResult()
    {
        //Prevent double result emission
        Q_ASSERT(!resultEmitted);
        if (!resultEmitted) {
            resultEmitted = true;
            q->emitResult();
        }
    }

    void createTransaction()
    {
        currentTransaction = new TransactionSequence(q);
        currentTransaction->setAutomaticCommittingEnabled(false);
        q->connect(currentTransaction, SIGNAL(finished(KJob*)),
                   q, SLOT(transactionSequenceResult(KJob*)));
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
            currentTransaction = 0;
        }
    }

    /**
      Process what's currently available.
    */
    void execute()
    {
        qDebug() << "localListDone: " << localListDone << " deliveryDone: " << deliveryDone;
        if (!localListDone && !deliveryDone) {
            return;
        }

        if (!localListDone && deliveryDone) {
            Job *parent = (currentTransaction ? static_cast<Job *>(currentTransaction) : static_cast<Job *>(q));
            CollectionFetchJob *job = new CollectionFetchJob(akonadiRootCollection, CollectionFetchJob::Recursive, parent);
            job->fetchScope().setResource(resourceId);
            job->fetchScope().setIncludeUnsubscribed(true);
            job->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
            q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                       q, SLOT(localCollectionsReceived(Akonadi::Collection::List)));
            q->connect(job, SIGNAL(result(KJob*)),
                       q, SLOT(localCollectionFetchResult(KJob*)));
            return;
        }

        // If a transaction is not started yet, it means we just finished local listing
        if (!currentTransaction) {
            // There's nothing to do after local listing -> we are done!
            if (remoteCollectionsToCreate.isEmpty() && remoteCollectionsToUpdate.isEmpty() && localCollectionsToRemove.isEmpty()) {
                qDebug() << "Nothing to do";
                emitResult();
                return;
            }
            // Ok, there's some work to do, so create a transaction we can use
            createTransaction();
        }

        createLocalCollections();
    }


    CollectionSync *q;

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

    QHash<RemoteId /* parent */, QList<Collection> /* children */ > removedRemoteCollections;
    QHash<RemoteId /* parent */, QList<Collection> /* children */ > remoteCollections;
    QHash<RemoteId /* parent */, QList<Collection> /* children */ > localCollections;

    Collection::List localCollectionsToRemove;
    Collection::List remoteCollectionsToCreate;
    QList<QPair<Collection /* local */, Collection /* remote */> > remoteCollectionsToUpdate;
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
    Q_FOREACH (const Collection &c, remoteCollections) {
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
    Q_FOREACH (const Collection &c, changedCollections) {
        d->addRemoteColection(c);
    }
    Q_FOREACH (const Collection &c, removedCollections) {
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
    }
}

void CollectionSync::setKeepLocalChanges(const QSet<QByteArray> &parts)
{
    d->keepLocalChanges = parts;
}

#include "moc_collectionsync_p.cpp"

