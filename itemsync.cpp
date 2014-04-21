/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
        , mDeliveryDone(false)
        , mFinished(false)
        , mFullListingDone(false)
        , mProcessingBatch(false)
        , mDisableAutomaticDeliveryDone(false)
        , mBatchSize(10)
    {
        // we want to fetch all data by default
        mFetchScope.fetchFullPayload();
        mFetchScope.fetchAllAttributes();
    }

    void createLocalItem(const Item &item);
    void modifyLocalItem(const Item &remoteItem, Akonadi::Item::Id localId);
    void checkDone();
    void slotItemsReceived(const Item::List &items);
    void slotLocalListDone(KJob *job);
    void slotLocalFetchDone(KJob *job);
    void slotLocalDeleteDone(KJob *);
    void slotLocalChangeDone(KJob *job);
    void execute();
    void processItems();
    void processBatch();
    void deleteItems(const Item::List &items);
    void slotTransactionResult(KJob *job);
    void requestTransaction();
    Job *subjobParent() const;
    void fetchLocalItems();
    QString jobDebuggingString() const /*Q_DECL_OVERRIDE*/;
    bool allProcessed() const;

    Q_DECLARE_PUBLIC(ItemSync)
    Collection mSyncCollection;
    QSet<Akonadi::Item::Id> mUnprocessedLocalIds;
    QHash<QString, Akonadi::Item::Id> mLocalIdByRid;

    ItemSync::TransactionMode mTransactionMode;
    TransactionSequence *mCurrentTransaction;
    int mTransactionJobs;

    // fetch scope for initial item listing
    ItemFetchScope mFetchScope;

    Akonadi::Item::List mRemoteItemQueue;
    Akonadi::Item::List mRemovedRemoteItemQueue;
    Akonadi::Item::List mCurrentBatchRemoteItems;
    Akonadi::Item::List mCurrentBatchRemovedRemoteItems;

    // create counter
    int mPendingJobs;
    int mProgress;
    int mTotalItems;
    int mTotalItemsProcessed;

    bool mStreaming;
    bool mIncremental;
    bool mDeliveryDone;
    bool mFinished;
    bool mFullListingDone;
    bool mProcessingBatch;
    bool mDisableAutomaticDeliveryDone;

    int mBatchSize;
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

void ItemSyncPrivate::modifyLocalItem(const Item &remoteItem, Akonadi::Item::Id localId)
{
    Q_Q(ItemSync);
    // don't try to do anything in error state
    if (q->error()) {
        return;
    }

    //we fetch the local item to check if a modification is required and to make sure we have all parts
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(Akonadi::Item(localId), subjobParent());
    fetchJob->setFetchScope(mFetchScope);
    fetchJob->fetchScope().setCacheOnly(true);
    fetchJob->setDeliveryOption(ItemFetchJob::ItemGetter);
    q->connect(fetchJob, SIGNAL(result(KJob*)), q, SLOT(slotLocalFetchDone(KJob*)));
    fetchJob->setProperty("remoteItem", QVariant::fromValue(remoteItem));
    mPendingJobs++;
}

void ItemSyncPrivate::slotLocalFetchDone(KJob *job)
{
    Q_Q(ItemSync);
    mPendingJobs--;
    if (job->error()) {
        kWarning() << job->errorString();
        checkDone();
        return;
    }
    Akonadi::ItemFetchJob *fetchJob = static_cast<Akonadi::ItemFetchJob*>(job);
    Akonadi::Item remoteItem = fetchJob->property("remoteItem").value<Akonadi::Item>();
    if (fetchJob->items().isEmpty()) {
        kWarning() << "Failed to fetch local item: " << remoteItem.remoteId() << remoteItem.gid();
        checkDone();
        return;
    }
    const Akonadi::Item localItem = fetchJob->items().first();
    if (q->updateItem(localItem, remoteItem)) {
        remoteItem.setId(localItem.id());
        remoteItem.setRevision(localItem.revision());
        remoteItem.setSize(localItem.size());
        remoteItem.setRemoteId(localItem.remoteId());    // in case someone clears remoteId by accident
        ItemModifyJob *mod = new ItemModifyJob(remoteItem, subjobParent());
        mod->disableRevisionCheck();
        q->connect(mod, SIGNAL(result(KJob*)), q, SLOT(slotLocalChangeDone(KJob*)));
        mPendingJobs++;
    } else {
        mProgress++;
    }
    checkDone();
}

bool ItemSyncPrivate::allProcessed() const
{
    return mDeliveryDone && mCurrentBatchRemoteItems.isEmpty() && mRemoteItemQueue.isEmpty() && mRemovedRemoteItemQueue.isEmpty() && mCurrentBatchRemovedRemoteItems.isEmpty();
}

void ItemSyncPrivate::checkDone()
{
    Q_Q(ItemSync);
    q->setProcessedAmount(KJob::Bytes, mProgress);
    if (mPendingJobs > 0) {
        return;
    }

    if (mTransactionJobs > 0) {
        //Commit the current transaction if we're in batch processing mode or done
        //and wait until the transaction is committed to process the next batch
        if (mTransactionMode == ItemSync::MultipleTransactions || (mDeliveryDone && mRemoteItemQueue.isEmpty())) {
            if (mCurrentTransaction) {
                q->emit transactionCommitted();
                mCurrentTransaction->commit();
                mCurrentTransaction = 0;
            }
            return;
        }
    }
    mProcessingBatch = false;
    if (!mRemoteItemQueue.isEmpty()) {
        execute();
        //We don't have enough items, request more
        if (!mProcessingBatch) {
            q->emit readyForNextBatch(mBatchSize - mRemoteItemQueue.size());
        }
        return;
    }
    q->emit readyForNextBatch(mBatchSize);

    if (allProcessed() && !mFinished) {
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
  /*
   * We received a list of items from the server:
   * * fetch all local id's + rid's only
   * * check each full sync item wether it's locally available
   * * if it is modify the item
   * * if it's not create it
   * * delete all superfluous items
   */
    Q_D(ItemSync);
    Q_ASSERT(!d->mIncremental);
    if (!d->mStreaming) {
        d->mDeliveryDone = true;
    }
    d->mRemoteItemQueue += items;
    d->mTotalItemsProcessed += items.count();
    kDebug() << "Received: " << items.count() << "In total: " << d->mTotalItemsProcessed << " Wanted: " << d->mTotalItems;
    if (!d->mDisableAutomaticDeliveryDone && (d->mTotalItemsProcessed == d->mTotalItems)) {
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
    if (!d->mDisableAutomaticDeliveryDone && (d->mTotalItems == 0)) {
        d->mDeliveryDone = true;
        d->execute();
    }
}

void ItemSync::setDisableAutomaticDeliveryDone(bool disable)
{
    Q_D(ItemSync);
    d->mDisableAutomaticDeliveryDone = disable;
}

void ItemSync::setIncrementalSyncItems(const Item::List &changedItems, const Item::List &removedItems)
{
  /*
   * We received an incremental listing of items:
   * * for each changed item:
   * ** If locally available => modify
   * ** else => create
   * * removed items can be removed right away
   */
    Q_D(ItemSync);
    d->mIncremental = true;
    if (!d->mStreaming) {
        d->mDeliveryDone = true;
    }
    d->mRemoteItemQueue += changedItems;
    d->mRemovedRemoteItemQueue += removedItems;
    d->mTotalItemsProcessed += changedItems.count() + removedItems.count();
    kDebug() << "Received: " << changedItems.count() << "Removed: " << removedItems.count() << "In total: " << d->mTotalItemsProcessed << " Wanted: " << d->mTotalItems;
    if (!d->mDisableAutomaticDeliveryDone && (d->mTotalItemsProcessed == d->mTotalItems)) {
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
    ItemFetchJob* job;
    if (mIncremental) {
        //Try fetching the items so we have their id and know if they're available
        const Akonadi::Item::List itemsToFetch = mCurrentBatchRemoteItems + mCurrentBatchRemovedRemoteItems;
        if (itemsToFetch.isEmpty()) {
            // The fetch job produces an error with an empty set
            processBatch();
            return;
        }
        // We need to fetch the items only to detect if they are new or modified
        job = new ItemFetchJob(itemsToFetch, subjobParent());
        job->fetchScope().setFetchRemoteIdentification(true);
        job->fetchScope().setFetchModificationTime(false);
        job->setCollection(mSyncCollection);
        job->setDeliveryOption(ItemFetchJob::EmitItemsIndividually);
        // We use this to check if items are available locally, so errors are inevitable
        job->fetchScope().setIgnoreRetrievalErrors(true);
        QObject::connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)), q, SLOT(slotItemsReceived(Akonadi::Item::List)));
    } else {
        if (mFullListingDone) {
            processBatch();
            return;
        }
        //Otherwise we'll remove the created items again during the second run
        mFullListingDone = true;
        job = new ItemFetchJob(mSyncCollection, subjobParent());
        job->fetchScope().setFetchRemoteIdentification(true);
        job->fetchScope().setFetchModificationTime(false);
        job->setDeliveryOption(ItemFetchJob::EmitItemsIndividually);
        QObject::connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)), q, SLOT(slotItemsReceived(Akonadi::Item::List)));
    }

    // we only can fetch parts already in the cache, otherwise this will deadlock
    job->fetchScope().setCacheOnly(true);

    QObject::connect(job, SIGNAL(result(KJob*)), q, SLOT(slotLocalListDone(KJob*)));
    mPendingJobs++;
}

void ItemSyncPrivate::slotItemsReceived(const Item::List &items)
{
    foreach (const Akonadi::Item &item, items) {
        //Don't delete items that have not yet been synchronized
        if (item.remoteId().isEmpty()) {
            continue;
        }
        if (mLocalIdByRid.contains(item.remoteId())) {
            kWarning() << "Found multiple items with the same rid : " << item.remoteId() << item.id();
        } else {
            mLocalIdByRid.insert(item.remoteId(), item.id());
        }
        if (!mIncremental) {
            mUnprocessedLocalIds << item.id();
        }
    }
}

void ItemSyncPrivate::slotLocalListDone(KJob *job)
{
    mPendingJobs--;
    if (job->error()) {
        kWarning() << job->errorString();
    }
    processBatch();
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
    //shouldn't happen
    if (mFinished) {
        kWarning() << "Call to execute() on finished job.";
        Q_ASSERT(false);
        return;
    }
    //not doing anything, start processing
    if (!mProcessingBatch) {
        if (mRemoteItemQueue.size() >= mBatchSize || mDeliveryDone) {
            //we have a new batch to process
            const int num = qMin(mBatchSize, mRemoteItemQueue.size());
            for (int i = 0; i < num; i++) {
                mCurrentBatchRemoteItems << mRemoteItemQueue.takeFirst();
            }
            mCurrentBatchRemovedRemoteItems += mRemovedRemoteItemQueue;
            mRemovedRemoteItemQueue.clear();
        } else {
            //nothing to do, let's wait for more data
            return;
        }
        mProcessingBatch = true;
        fetchLocalItems();
        return;
    }
    checkDone();
}

//process the current batch of items
void ItemSyncPrivate::processBatch()
{
    Q_Q(ItemSync);
    if (mCurrentBatchRemoteItems.isEmpty() && !mDeliveryDone) {
        return;
    }

    //request a transaction, there are items that require processing
    requestTransaction();

    processItems();

    // removed
    Akonadi::Item::List itemsToDelete;
    if (!mIncremental && allProcessed()) {
        //the full listing is done and we know which items to remove
        foreach (Akonadi::Item::Id id, mUnprocessedLocalIds) {
            itemsToDelete << Akonadi::Item(id);
        }
        mUnprocessedLocalIds.clear();
    } else {
        foreach (const Akonadi::Item &removedItem, mCurrentBatchRemovedRemoteItems) {
            if (!mLocalIdByRid.contains(removedItem.remoteId())) {
                kWarning() << "cannot remove item because it's not available locally. RID: " << removedItem.remoteId();
                continue;
            }
            itemsToDelete << Akonadi::Item(mLocalIdByRid.value(removedItem.remoteId()));
        }
        mCurrentBatchRemovedRemoteItems.clear();
    }
    deleteItems(itemsToDelete);

    if (mIncremental) {
        //no longer required, we processed all items of the current batch
        mLocalIdByRid.clear();
    }
    checkDone();
}

void ItemSyncPrivate::processItems()
{
    Q_Q(ItemSync);
    // added / updated
    foreach (const Item &remoteItem, mCurrentBatchRemoteItems) {
        if (remoteItem.remoteId().isEmpty()) {
            kWarning() << "Item " << remoteItem.id() << " does not have a remote identifier";
            continue;
        }

        //TODO also check by id and gid
        //Locally available
        if (mLocalIdByRid.contains(remoteItem.remoteId())) {
            const Akonadi::Item::Id localId = mLocalIdByRid.value(remoteItem.remoteId());
            if (!mIncremental) {
                mUnprocessedLocalIds.remove(localId);
            }
            modifyLocalItem(remoteItem, localId);
        } else {
            createLocalItem(remoteItem);
        }
    }
    mCurrentBatchRemoteItems.clear();
}

void ItemSyncPrivate::deleteItems(const Item::List &itemsToDelete)
{
    Q_Q(ItemSync);
    // if in error state, better not change anything anymore
    if (q->error()) {
        return;
    }

    if (itemsToDelete.isEmpty()) {
        return;
    }

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

void ItemSyncPrivate::requestTransaction()
{
    Q_Q(ItemSync);
    //we never want parallel transactions, single transaction just makes one big transaction, and multi transaction uses multiple transaction sequentially
    if (!mCurrentTransaction) {
        ++mTransactionJobs;
        mCurrentTransaction = new TransactionSequence(q);
        mCurrentTransaction->setAutomaticCommittingEnabled(false);
        QObject::connect(mCurrentTransaction, SIGNAL(result(KJob*)), q, SLOT(slotTransactionResult(KJob*)));
    }
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

int ItemSync::batchSize() const
{
    Q_D(const ItemSync);
    return d->mBatchSize;
}

void ItemSync::setBatchSize(int size)
{
    Q_D(ItemSync);
    d->mBatchSize = size;
}


#include "moc_itemsync.cpp"
