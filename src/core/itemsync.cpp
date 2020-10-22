/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemsync.h"

#include "job_p.h"
#include "collection.h"
#include "item_p.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "transactionsequence.h"
#include "itemfetchscope.h"


#include "akonadicore_debug.h"


using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::ItemSyncPrivate : public JobPrivate
{
public:
    explicit ItemSyncPrivate(ItemSync *parent)
        : JobPrivate(parent)
        , mTransactionMode(ItemSync::SingleTransaction)
        , mCurrentTransaction(nullptr)
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
        , mMergeMode(Akonadi::ItemSync::RIDMerge)
    {
        // we want to fetch all data by default
        mFetchScope.fetchFullPayload();
        mFetchScope.fetchAllAttributes();
    }

    void createOrMerge(const Item &item);
    void checkDone();
    void slotItemsReceived(const Item::List &items);
    void slotLocalListDone(KJob *job);
    void slotLocalDeleteDone(KJob *job);
    void slotLocalChangeDone(KJob *job);
    void execute();
    void processItems();
    void processBatch();
    void deleteItems(const Item::List &items);
    void slotTransactionResult(KJob *job);
    void requestTransaction();
    Job *subjobParent() const;
    void fetchLocalItemsToDelete();
    QString jobDebuggingString() const override;
    bool allProcessed() const;

    Q_DECLARE_PUBLIC(ItemSync)
    Collection mSyncCollection;
    QSet<QString> mListedItems;

    ItemSync::TransactionMode mTransactionMode;
    TransactionSequence *mCurrentTransaction = nullptr;
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
    Akonadi::ItemSync::MergeMode mMergeMode;
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
    ItemCreateJob::MergeOptions merge = ItemCreateJob::Silent;
    if (mMergeMode == ItemSync::GIDMerge && !item.gid().isEmpty()) {
        merge |= ItemCreateJob::GID;
    } else {
        merge |= ItemCreateJob::RID;
    }
    create->setMerge(merge);
    q->connect(create, &ItemCreateJob::result, q, [this](KJob *job) {slotLocalChangeDone(job);});
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
                // Note that mCurrentTransaction->commit() is a no-op if we're already rolling back
                // so this signal is a bit misleading (but it's only used by unittests it seems)
                Q_EMIT q->transactionCommitted();
                mCurrentTransaction->commit();
                mCurrentTransaction = nullptr;
            }
            return;
        }
    }
    mProcessingBatch = false;

    if (q->error() == Job::UserCanceled && mTransactionJobs == 0 && !mFinished) {
        qCDebug(AKONADICORE_LOG) << "ItemSync of collection" << mSyncCollection.id() << "finished due to user cancelling";
        mFinished = true;
        q->emitResult();
        return;
    }

    if (!mRemoteItemQueue.isEmpty()) {
        execute();
        //We don't have enough items, request more
        if (!mProcessingBatch) {
            Q_EMIT q->readyForNextBatch(mBatchSize - mRemoteItemQueue.size());
        }
        return;
    }
    Q_EMIT q->readyForNextBatch(mBatchSize);

    if (allProcessed() && !mFinished) {
        // prevent double result emission, can happen since checkDone() is called from all over the place
        qCDebug(AKONADICORE_LOG) << "ItemSync of collection" << mSyncCollection.id() << "finished";
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
     * * check each full sync item whether it's locally available
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
    qCDebug(AKONADICORE_LOG) << "Received batch: " << items.count()
                             << "Already processed: " << d->mTotalItemsProcessed
                             << "Expected total amount: " << d->mTotalItems;
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
    qCDebug(AKONADICORE_LOG) << "Expected total amount:" << amount;
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
    qCDebug(AKONADICORE_LOG) << "Received: " << changedItems.count() << "Removed: " << removedItems.count() << "In total: " << d->mTotalItemsProcessed << " Wanted: " << d->mTotalItems;
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

void ItemSyncPrivate::fetchLocalItemsToDelete()
{
    Q_Q(ItemSync);
    if (mIncremental) {
        qFatal("This must not be called while in incremental mode");
        return;
    }
    ItemFetchJob *job = new ItemFetchJob(mSyncCollection, subjobParent());
    job->fetchScope().setFetchRemoteIdentification(true);
    job->fetchScope().setFetchModificationTime(false);
    job->setDeliveryOption(ItemFetchJob::EmitItemsIndividually);
    // we only can fetch parts already in the cache, otherwise this will deadlock
    job->fetchScope().setCacheOnly(true);

    QObject::connect(job, &ItemFetchJob::itemsReceived, q, [this](const Akonadi::Item::List &lst)  { slotItemsReceived(lst); });
    QObject::connect(job, &ItemFetchJob::result, q, [this](KJob *job) { slotLocalListDone(job); });
    mPendingJobs++;
}

void ItemSyncPrivate::slotItemsReceived(const Item::List &items)
{
    for (const Akonadi::Item &item : items) {
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
        qCWarning(AKONADICORE_LOG) << job->errorString();
    }
    deleteItems(mItemsToDelete);
    checkDone();
}

QString ItemSyncPrivate::jobDebuggingString() const
{
    // TODO: also print out mIncremental and mTotalItemsProcessed, but they are set after the job
    // started, so this requires passing jobDebuggingString to jobEnded().
    return QStringLiteral("Collection %1 (%2)").arg(mSyncCollection.id()).arg(mSyncCollection.name());
}

void ItemSyncPrivate::execute()
{
    //shouldn't happen
    if (mFinished) {
        qCWarning(AKONADICORE_LOG) << "Call to execute() on finished job.";
        Q_ASSERT(false);
        return;
    }
    //not doing anything, start processing
    if (!mProcessingBatch) {
        if (mRemoteItemQueue.size() >= mBatchSize || mDeliveryDone) {
            //we have a new batch to process
            const int num = qMin(mBatchSize, mRemoteItemQueue.size());
            mCurrentBatchRemoteItems.reserve(mBatchSize);
            std::move(mRemoteItemQueue.begin(), mRemoteItemQueue.begin() + num, std::back_inserter(mCurrentBatchRemoteItems));
            mRemoteItemQueue.erase(mRemoteItemQueue.begin(), mRemoteItemQueue.begin() + num);

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
    if (q->error() == Job::UserCanceled) {
        checkDone();
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
    // added / updated
    for (const Item &remoteItem : qAsConst(mCurrentBatchRemoteItems)) {
        if (remoteItem.remoteId().isEmpty()) {
            qCWarning(AKONADICORE_LOG) << "Item " << remoteItem.id() << " does not have a remote identifier";
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
    q->connect(job, &ItemDeleteJob::result, q, [this](KJob *job) { slotLocalDeleteDone(job); });

    // It can happen that the groupware servers report us deleted items
    // twice, in this case this item delete job will fail on the second try.
    // To avoid a rollback of the complete transaction we gracefully allow the job
    // to fail :)
    TransactionSequence *transaction = qobject_cast<TransactionSequence *>(subjobParent());
    if (transaction) {
        transaction->setIgnoreJobFailure(job);
    }
}

void ItemSyncPrivate::slotLocalDeleteDone(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Deleting items from the akonadi database failed:" << job->errorString();
    }
    mPendingJobs--;
    mProgress++;

    checkDone();
}

void ItemSyncPrivate::slotLocalChangeDone(KJob *job)
{
    if (job->error() && job->error() != Job::KilledJobError) {
        qCWarning(AKONADICORE_LOG) << "Creating/updating items from the akonadi database failed:" << job->errorString();
        mRemoteItemQueue.clear(); // don't try to process any more items after a rollback
    }
    mPendingJobs--;
    mProgress++;

    checkDone();
}

void ItemSyncPrivate::slotTransactionResult(KJob *job)
{
    --mTransactionJobs;
    if (mCurrentTransaction == job) {
        mCurrentTransaction = nullptr;
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
        QObject::connect(mCurrentTransaction, &TransactionSequence::result, q, [this](KJob *job) { slotTransactionResult(job); });
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
        qCWarning(AKONADICORE_LOG) << "Error during ItemSync: " << job->errorString();
        // pretend there were no errors
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
    qCDebug(AKONADICORE_LOG) << "The item sync is being rolled-back.";
    setError(UserCanceled);
    if (d->mCurrentTransaction) {
        d->mCurrentTransaction->rollback();
    }
    d->mDeliveryDone = true; // user won't deliver more data
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

ItemSync::MergeMode ItemSync::mergeMode() const
{
    Q_D(const ItemSync);
    return d->mMergeMode;
}

void ItemSync::setMergeMode(MergeMode mergeMode)
{
    Q_D(ItemSync);
    d->mMergeMode = mergeMode;
}

#include "moc_itemsync.cpp"
