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

#include <qdebug.h>
#include <KDebug>
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

    void createOrMerge(const Item &item);
    void checkDone();
    void slotItemsReceived(const Item::List &items);
    void slotLocalListDone(KJob *job);
    void slotLocalDeleteDone(KJob *);
    void slotLocalChangeDone(KJob *job);
    void execute();
    void processItems();
    void processBatch();
    void deleteItems(const Item::List &items);
    void slotTransactionResult(KJob *job);
    void requestTransaction();
    Job *subjobParent() const;
    void fetchLocalItemsToDelete();
    QString jobDebuggingString() const /*Q_DECL_OVERRIDE*/;
    bool allProcessed() const;

    Q_DECLARE_PUBLIC(ItemSync)
    Collection mSyncCollection;
    QSet<QString> mListedItems;

    ItemSync::TransactionMode mTransactionMode;
    TransactionSequence *mCurrentTransaction;
    int mTransactionJobs;

    // fetch scope for initial item listing
    ItemFetchScope mFetchScope;

    Akonadi::Item::List mRemoteItemQueue;
    Akonadi::Item::List mRemovedRemoteItemQueue;
    Akonadi::Item::List mCurrentBatchRemoteItems;
    Akonadi::Item::List mCurrentBatchRemovedRemoteItems;
    Akonadi::Item::List mItemsToDelete;

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

void ItemSyncPrivate::createOrMerge(const Item &item)
{
    Q_Q(ItemSync);
    // don't try to do anything in error state
    if (q->error()) {
        return;
    }
    mPendingJobs++;
    ItemCreateJob *create = new ItemCreateJob(item, mSyncCollection, subjobParent());
    if (!item.gid().isEmpty()) {
        create->setMerge(ItemCreateJob::GID|ItemCreateJob::Silent);
    } else {
        create->setMerge(ItemCreateJob::RID|ItemCreateJob::Silent);
    }
    q->connect(create, SIGNAL(result(KJob*)), q, SLOT(slotLocalChangeDone(KJob*)));
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
    qDebug() << "Received: " << items.count() << "In total: " << d->mTotalItemsProcessed << " Wanted: " << d->mTotalItems;
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
    qDebug() << amount;
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
<<<<<<< HEAD:akonadi/src/core/itemsync.cpp
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
        qDebug() << "Stored flags "  << storedItem.flags()
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

=======
    Q_UNUSED(storedItem);
    Q_UNUSED(newItem);
>>>>>>> origin/master:akonadi/itemsync.cpp
    return false;
}

void ItemSyncPrivate::fetchLocalItemsToDelete()
{
    Q_Q(ItemSync);
    if (mIncremental) {
        kFatal() << "This must not be called while in incremental mode";
        return;
    }
    ItemFetchJob *job = new ItemFetchJob(mSyncCollection, subjobParent());
    job->fetchScope().setFetchRemoteIdentification(true);
    job->fetchScope().setFetchModificationTime(false);
    job->setDeliveryOption(ItemFetchJob::EmitItemsIndividually);
    // we only can fetch parts already in the cache, otherwise this will deadlock
    job->fetchScope().setCacheOnly(true);

    QObject::connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)), q, SLOT(slotItemsReceived(Akonadi::Item::List)));
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
        if (!mListedItems.contains(item.remoteId())) {
            mItemsToDelete << Item(item.id());
        }
    }
}

void ItemSyncPrivate::slotLocalListDone(KJob *job)
{
    mPendingJobs--;
    if (job->error()) {
        kWarning() << job->errorString();
    }
    deleteItems(mItemsToDelete);
    checkDone();
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
        processBatch();
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
    if (!mIncremental && allProcessed()) {
        //the full listing is done and we know which items to remove
        fetchLocalItemsToDelete();
    } else {
        deleteItems(mCurrentBatchRemovedRemoteItems);
        mCurrentBatchRemovedRemoteItems.clear();
    }

    checkDone();
}

void ItemSyncPrivate::processItems()
{
    Q_Q(ItemSync);
    // added / updated
    foreach (const Item &remoteItem, mCurrentBatchRemoteItems) {
        if (remoteItem.remoteId().isEmpty()) {
            qWarning() << "Item " << remoteItem.id() << " does not have a remote identifier";
            continue;
        }
        if (!mIncremental) {
            mListedItems << remoteItem.remoteId();
        }
        createOrMerge(remoteItem);
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
