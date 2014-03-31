/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
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

#include "itemsync.h"

#include "job_p.h"
#include "collection.h"
#include "item.h"
#include "item_p.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "transactionsequence.h"
#include "itemfetchscope.h"

#include <kdebug.h>

#include <QtCore/QStringList>

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::ItemSyncPrivate : public JobPrivate
{
public:
    ItemSyncPrivate(ItemSync *parent)
        : JobPrivate(parent)
        , mTransactionMode(ItemSync::SingleTransaction)
        , mCurrentTransaction(0)
        , mTransactionJobs(0)
        , mPendingJobs(0)
        , mProgress(0)
        , mTotalItems(-1)
        , mTotalItemsProcessed(0)
        , mStreaming(false)
        , mIncremental(false)
        , mLocalListDone(false)
        , mDeliveryDone(false)
        , mFinished(false)
        , mLocalListStarted( false )
    {
        // we want to fetch all data by default
        mFetchScope.fetchFullPayload();
        mFetchScope.fetchAllAttributes();
    }

    void createLocalItem(const Item &item);
    void checkDone();
    void slotLocalListDone(KJob *job);
    void slotLocalDeleteDone(KJob *);
    void slotLocalChangeDone(KJob *job);
    void execute();
    void processItems();
    void deleteItems(const Item::List &items);
    void slotTransactionResult(KJob *job);
    Job *subjobParent() const;
    void fetchLocalItems();
    QString jobDebuggingString() const /*Q_DECL_OVERRIDE*/;

    Q_DECLARE_PUBLIC(ItemSync)
    Collection mSyncCollection;
    QHash<Item::Id, Akonadi::Item> mLocalItemsById;
    QHash<QString, Akonadi::Item> mLocalItemsByRemoteId;
    QSet<Akonadi::Item> mUnprocessedLocalItems;

    ItemSync::TransactionMode mTransactionMode;
    TransactionSequence *mCurrentTransaction;
    int mTransactionJobs;

    // fetch scope for initial item listing
    ItemFetchScope mFetchScope;

    // remote items
    Akonadi::Item::List mRemoteItems;

    // removed remote items
    Item::List mRemovedRemoteItems;

    // create counter
    int mPendingJobs;
    int mProgress;
    int mTotalItems;
    int mTotalItemsProcessed;

    bool mStreaming;
    bool mIncremental;
    bool mLocalListDone;
    bool mDeliveryDone;
    bool mFinished;
    bool mLocalListStarted;
};

void ItemSyncPrivate::createLocalItem(const Item &item)
{
    Q_Q(ItemSync);
    // don't try to do anything in error state
    if (q->error()) {
        return;
    }
    mPendingJobs++;
    ItemCreateJob *create = new ItemCreateJob(item, mSyncCollection, subjobParent());
    q->connect(create, SIGNAL(result(KJob*)), q, SLOT(slotLocalChangeDone(KJob*)));
}

void ItemSyncPrivate::checkDone()
{
    Q_Q(ItemSync);
    q->setProcessedAmount(KJob::Bytes, mProgress);
    if (mPendingJobs > 0 || !mDeliveryDone || mTransactionJobs > 0) {
        return;
    }

    if (!mFinished) {
        // prevent double result emission, can happen since checkDone() is called from all over the place
        mFinished = true;
        q->emitResult();
    }
}

ItemSync::ItemSync(const Collection &collection, QObject *parent)
    : Job(new ItemSyncPrivate(this), parent)
{
    Q_D(ItemSync);
    d->mSyncCollection = collection;
}

ItemSync::~ItemSync()
{
}

void ItemSync::setFullSyncItems(const Item::List &items)
{
    Q_D(ItemSync);
    Q_ASSERT(!d->mIncremental);
    if (!d->mStreaming) {
        d->mDeliveryDone = true;
    }
    d->mRemoteItems += items;
    d->mTotalItemsProcessed += items.count();
    kDebug() << "Received: " << items.count() << "In total: " << d->mTotalItemsProcessed << " Wanted: " << d->mTotalItems;
    setTotalAmount(KJob::Bytes, d->mTotalItemsProcessed);
    if (d->mTotalItemsProcessed == d->mTotalItems) {
        d->mDeliveryDone = true;
    }
    d->execute();
}

void ItemSync::setTotalItems(int amount)
{
    Q_D(ItemSync);
    Q_ASSERT(!d->mIncremental);
    Q_ASSERT(amount >= 0);
    setStreamingEnabled(true);
    kDebug() << amount;
    d->mTotalItems = amount;
    setTotalAmount(KJob::Bytes, amount);
    if (d->mTotalItems == 0) {
        d->mDeliveryDone = true;
        d->execute();
    }
}

void ItemSync::setIncrementalSyncItems(const Item::List &changedItems, const Item::List &removedItems)
{
    Q_D(ItemSync);
    d->mIncremental = true;
    if (!d->mStreaming) {
        d->mDeliveryDone = true;
    }
    d->mRemoteItems += changedItems;
    d->mRemovedRemoteItems += removedItems;
    d->mTotalItemsProcessed += changedItems.count() + removedItems.count();
    setTotalAmount(KJob::Bytes, d->mTotalItemsProcessed);
    if (d->mTotalItemsProcessed == d->mTotalItems) {
        d->mDeliveryDone = true;
    }
    d->execute();
}

void ItemSync::setFetchScope(ItemFetchScope &fetchScope)
{
    Q_D(ItemSync);
    d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemSync::fetchScope()
{
    Q_D(ItemSync);
    return d->mFetchScope;
}

void ItemSync::doStart()
{
}

bool ItemSync::updateItem(const Item &storedItem, Item &newItem)
{
    Q_D(ItemSync);
    // we are in error state, better not change anything at all anymore
    if (error()) {
        return false;
    }

    /*
     * We know that this item has changed (as it is part of the
     * incremental changed list), so we just put it into the
     * storage.
     */
    if (d->mIncremental) {
        return true;
    }

    if (newItem.d_func()->mClearPayload) {
        return true;
    }

    // Check whether the remote revisions differ
    if (storedItem.remoteRevision() != newItem.remoteRevision()) {
        return true;
    }

    // Check whether the flags differ
    if (storedItem.flags() != newItem.flags()) {
        kDebug() << "Stored flags "  << storedItem.flags()
                 << "new flags " << newItem.flags();
        return true;
    }

    // Check whether the new item contains unknown parts
    QSet<QByteArray> missingParts = newItem.loadedPayloadParts();
    missingParts.subtract(storedItem.loadedPayloadParts());
    if (!missingParts.isEmpty()) {
        return true;
    }

    // ### FIXME SLOW!!!
    // If the available part identifiers don't differ, check
    // whether the content of the payload differs
    if (newItem.hasPayload()
        && storedItem.payloadData() != newItem.payloadData()) {
        return true;
     }

    // check if remote attributes have been changed
    foreach (Attribute *attr, newItem.attributes()) {
        if (!storedItem.hasAttribute(attr->type())) {
            return true;
        }
        if (attr->serialized() != storedItem.attribute(attr->type())->serialized()) {
            return true;
        }
    }

    return false;
}

void ItemSyncPrivate::fetchLocalItems()
{
    Q_Q( ItemSync );
    if (mLocalListStarted) {
        return;
    }
    mLocalListStarted = true;
    ItemFetchJob* job;
    if ( mIncremental ) {
        if ( mRemoteItems.isEmpty() ) {
            // The fetch job produces an error with an empty set
            mLocalListDone = true;
            execute();
            return;
        }
        // We need to fetch the items only to detect if they are new or modified
        job = new ItemFetchJob( mRemoteItems, q );
        job->setFetchScope( mFetchScope );
        job->setCollection( mSyncCollection );
        // We use this to check if items are available locally, so errors are inevitable
        job->fetchScope().setIgnoreRetrievalErrors( true );
    } else {
        job = new ItemFetchJob( mSyncCollection, q );
        job->setFetchScope( mFetchScope );
    }

    // we only can fetch parts already in the cache, otherwise this will deadlock
    job->fetchScope().setCacheOnly( true );

    QObject::connect( job, SIGNAL(result(KJob*)), q, SLOT(slotLocalListDone(KJob*)) );
}

void ItemSyncPrivate::slotLocalListDone(KJob *job)
{
    if (!job->error()) {
        const Item::List list = static_cast<ItemFetchJob *>(job)->items();
        foreach (const Item &item, list) {
            if (item.remoteId().isEmpty()) {
                continue;
            }
            mLocalItemsById.insert(item.id(), item);
            mLocalItemsByRemoteId.insert(item.remoteId(), item);
            mUnprocessedLocalItems.insert(item);
        }
    }

    mLocalListDone = true;
    execute();
}

QString ItemSyncPrivate::jobDebuggingString() const /*Q_DECL_OVERRIDE*/
{
  // TODO: also print out mIncremental and mTotalItemsProcessed, but they are set after the job
  // started, so this requires passing jobDebuggingString to jobEnded().
  return QString::fromLatin1("Collection %1 (%2)").arg(mSyncCollection.id()).arg(mSyncCollection.name());
}

void ItemSyncPrivate::execute()
{
    Q_Q(ItemSync);
    if (!mLocalListDone) {
        // Start fetching local items only once the delivery is done for incremental fetch,
        // so we can fetch only the required items
        if (mDeliveryDone || !mIncremental) {
            fetchLocalItems();
        }
        return;
    }

    // early exit to avoid unnecessary TransactionSequence creation in MultipleTransactions mode
    // TODO: do the transaction handling in a nicer way instead, only creating TransactionSequences when really needed
    if (!mDeliveryDone && mRemoteItems.isEmpty()) {
        return;
    }

    if ((mTransactionMode == ItemSync::SingleTransaction && !mCurrentTransaction) || mTransactionMode == ItemSync::MultipleTransactions) {
        ++mTransactionJobs;
        mCurrentTransaction = new TransactionSequence(q);
        mCurrentTransaction->setAutomaticCommittingEnabled(false);
        QObject::connect(mCurrentTransaction, SIGNAL(result(KJob*)), q, SLOT(slotTransactionResult(KJob*)));
    }

    processItems();
    if (!mDeliveryDone) {
        if (mTransactionMode == ItemSync::MultipleTransactions && mCurrentTransaction) {
            mCurrentTransaction->commit();
            mCurrentTransaction = 0;
        }
        return;
    }

    // removed
    if (!mIncremental) {
        mRemovedRemoteItems = mUnprocessedLocalItems.toList();
        mUnprocessedLocalItems.clear();
    }

    deleteItems(mRemovedRemoteItems);
    mLocalItemsById.clear();
    mLocalItemsByRemoteId.clear();
    mRemovedRemoteItems.clear();

    if (mCurrentTransaction) {
        mCurrentTransaction->commit();
        mCurrentTransaction = 0;
    }

    checkDone();
}

void ItemSyncPrivate::processItems()
{
    Q_Q(ItemSync);
    // added / updated
    foreach (Item remoteItem, mRemoteItems) {    //krazy:exclude=foreach non-const is needed here
#ifndef NDEBUG
        if (remoteItem.remoteId().isEmpty()) {
            kWarning() << "Item " << remoteItem.id() << " does not have a remote identifier";
        }
#endif

        Item localItem = mLocalItemsById.value(remoteItem.id());
        if (!localItem.isValid()) {
            localItem = mLocalItemsByRemoteId.value(remoteItem.remoteId());
        }
        mUnprocessedLocalItems.remove(localItem);
        // missing locally
        if (!localItem.isValid()) {
            createLocalItem(remoteItem);
            continue;
        }

        if (q->updateItem(localItem, remoteItem)) {
            mPendingJobs++;

            remoteItem.setId(localItem.id());
            remoteItem.setRevision(localItem.revision());
            remoteItem.setSize(localItem.size());
            remoteItem.setRemoteId(localItem.remoteId());    // in case someone clears remoteId by accident
            ItemModifyJob *mod = new ItemModifyJob(remoteItem, subjobParent());
            mod->disableRevisionCheck();
            q->connect(mod, SIGNAL(result(KJob*)), q, SLOT(slotLocalChangeDone(KJob*)));
        } else {
            mProgress++;
        }
    }
    mRemoteItems.clear();
}

void ItemSyncPrivate::deleteItems(const Item::List &items)
{
    Q_Q(ItemSync);
    // if in error state, better not change anything anymore
    if (q->error()) {
        return;
    }

    Item::List itemsToDelete;
    foreach (const Item &item, items) {
        Item delItem(item);
        if (!mIncremental) {
            if (!item.isValid()) {
                delItem = mLocalItemsByRemoteId.value(item.remoteId());
            }

            if (!delItem.isValid()) {
#ifndef NDEBUG
                kWarning() << "Delete item (remoteeId=" << item.remoteId()
                        << "mimeType=" << item.mimeType()
                        << ") does not have a valid UID and no item with that remote ID exists either";
#endif
                continue;
            }
        }

        if (delItem.remoteId().isEmpty()) {
            // don't attempt to remove items that never were written to the backend
            continue;
        }

        itemsToDelete.append(delItem);
    }

    if (!itemsToDelete.isEmpty()) {
        mPendingJobs++;
        ItemDeleteJob *job = new ItemDeleteJob(itemsToDelete, subjobParent());
        q->connect(job, SIGNAL(result(KJob*)), q, SLOT(slotLocalDeleteDone(KJob*)));

        // It can happen that the groupware servers report us deleted items
        // twice, in this case this item delete job will fail on the second try.
        // To avoid a rollback of the complete transaction we gracefully allow the job
        // to fail :)
        TransactionSequence *transaction = qobject_cast<TransactionSequence *>(subjobParent());
        if (transaction) {
            transaction->setIgnoreJobFailure(job);
        }
    }
}

void ItemSyncPrivate::slotLocalDeleteDone(KJob *)
{
    mPendingJobs--;
    mProgress++;

    checkDone();
}

void ItemSyncPrivate::slotLocalChangeDone(KJob *job)
{
    Q_UNUSED(job);
    mPendingJobs--;
    mProgress++;

    checkDone();
}

void ItemSyncPrivate::slotTransactionResult(KJob *job)
{
    --mTransactionJobs;
    if (mCurrentTransaction == job) {
        mCurrentTransaction = 0;
    }

    checkDone();
}

Job *ItemSyncPrivate::subjobParent() const
{
    Q_Q(const ItemSync);
    if (mCurrentTransaction && mTransactionMode != ItemSync::NoTransaction) {
        return mCurrentTransaction;
    }
    return const_cast<ItemSync *>(q);
}

void ItemSync::setStreamingEnabled(bool enable)
{
    Q_D(ItemSync);
    d->mStreaming = enable;
}

void ItemSync::deliveryDone()
{
    Q_D(ItemSync);
    Q_ASSERT(d->mStreaming);
    d->mDeliveryDone = true;
    d->execute();
}

void ItemSync::slotResult(KJob *job)
{
    if (job->error()) {
        // pretent there were no errors
        Akonadi::Job::removeSubjob(job);
        // propagate the first error we got but continue, we might still be fed with stuff from a resource
        if (!error()) {
            setError(job->error());
            setErrorText(job->errorText());
        }
    } else {
        Akonadi::Job::slotResult(job);
    }
}

void ItemSync::rollback()
{
    Q_D(ItemSync);
    setError(UserCanceled);
    if (d->mCurrentTransaction) {
        d->mCurrentTransaction->rollback();
    }
    d->mDeliveryDone = true; // user wont deliver more data
    d->execute(); // end this in an ordered way, since we have an error set no real change will be done
}

void ItemSync::setTransactionMode(ItemSync::TransactionMode mode)
{
    Q_D(ItemSync);
    d->mTransactionMode = mode;
}

#include "moc_itemsync.cpp"
