/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemsync.h"

#include "collection.h"
#include "item_p.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "private/protocol_p.h"
#include "job_p.h"
#include "protocol_p.h"

#include "akonadicore_debug.h"

#include <memory>

using namespace Akonadi;

namespace
{

class BeginItemSyncJob : public Job
{
    Q_OBJECT
public:
    explicit BeginItemSyncJob(const Collection &col, QObject *parent)
        : Job(new JobPrivate(this), parent)
        , mCollection(col)
    {}

    void setIncremental(bool incremental)
    {
        mIncremental = incremental;
    }

    void setMergeMode(ItemSync::MergeMode mergeMode)
    {
        switch (mergeMode) {
        case ItemSync::RIDMerge:
            mMergeMode = Protocol::BeginItemSyncCommand::RIDMerge;
            break;
        case ItemSync::GIDMerge:
            mMergeMode = Protocol::BeginItemSyncCommand::GIDMerge;
        }
    }

protected:
    void doStart() override
    {
        Q_D(Job);

        auto cmd = Protocol::BeginItemSyncCommandPtr::create();
        cmd->setCollectionId(mCollection.id());
        cmd->setIncremental(mIncremental);
        cmd->setMergeMode(mMergeMode);
        d->sendCommand(cmd);
    }

    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override
    {
        if (!response->isResponse() || response->type() != Protocol::Command::BeginItemSync) {
            return Job::doHandleResponse(tag, response);
        }

        return true;
    }

private:
    Q_DECLARE_PRIVATE(Job)

    Collection mCollection;
    Protocol::BeginItemSyncCommand::MergeMode mMergeMode = Protocol::BeginItemSyncCommand::RIDMerge;
    bool mIncremental = false;
};

class EndItemSyncJob : public Job
{
    Q_OBJECT
public:
    explicit EndItemSyncJob(QObject *parent)
        : Job(new JobPrivate(this), parent)
    {}

    void rollback()
    {
        mCommit = false;
    }

protected:
    void doStart() override
    {
        Q_D(Job);

        auto cmd = Protocol::EndItemSyncCommandPtr::create();
        cmd->setCommit(mCommit);
        d->sendCommand(cmd);
    }

    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override
    {
        if (!response->isResponse() || response->type() != Protocol::Command::EndItemSync) {
            return Job::doHandleResponse(tag, response);
        }

        return true;
    }

private:
    Q_DECLARE_PRIVATE(Job)

    bool mCommit = true;
};

} // namespace

/**
 * @internal
 */
class Akonadi::ItemSyncPrivate : public JobPrivate
{
public:
    explicit ItemSyncPrivate(ItemSync *parent)
        : JobPrivate(parent)
    {
    }

    void beginItemSyncIfNeeded();
    void createOrMerge(const Item &item);
    void checkDone();
    QString jobDebuggingString() const override;
    bool allProcessed() const;

    Q_DECLARE_PUBLIC(ItemSync)
    Collection mSyncCollection;

    QDateTime mItemSyncStart;

    // create counter
    int mProgress = 0;
    int mTotalItems = -1;
    int mTotalItemsProcessed = 0;

    bool mStarted = false;
    bool mStreaming = false;
    bool mIncremental = false;
    bool mDeliveryDone = false;
    bool mFinished = false;
    bool mDisableAutomaticDeliveryDone = false;

    Akonadi::ItemSync::MergeMode mMergeMode = ItemSync::RIDMerge;

    // Deprecated
    std::unique_ptr<ItemFetchScope> mFetchScope;
};

void ItemSyncPrivate::beginItemSyncIfNeeded()
{
    if (mStarted) {
        return;
    }

    Q_Q(ItemSync);
    auto *job = new BeginItemSyncJob(mSyncCollection, q);
    job->setIncremental(mIncremental);
    q->connect(job, &BeginItemSyncJob::result, q, [q]() {
        Q_EMIT q->readyForNextBatch(std::numeric_limits<int>::max());
    });
    mStarted = true;
}

void ItemSyncPrivate::createOrMerge(const Item &item)
{
    Q_Q(ItemSync);
    // don't try to do anything in error state
    if (q->error()) {
        return;
    }
    Item modifiedItem = item;
    if (mItemSyncStart.isValid()) {
        modifiedItem.setModificationTime(mItemSyncStart);
    }
    auto create = new ItemCreateJob(modifiedItem, mSyncCollection, q);
    ItemCreateJob::MergeOptions merge = ItemCreateJob::Silent;
    if (mMergeMode == ItemSync::GIDMerge && !item.gid().isEmpty()) {
        merge |= ItemCreateJob::GID;
    } else {
        merge |= ItemCreateJob::RID;
    }
    create->setMerge(merge);
}

bool ItemSyncPrivate::allProcessed() const
{
    Q_Q(const ItemSync);

    return mDeliveryDone && !q->hasSubjobs();
}

void ItemSyncPrivate::checkDone()
{
    Q_Q(ItemSync);
    q->setProcessedAmount(KJob::Bytes, mProgress);
    if (!allProcessed()) {
        return;
    }

    if (q->error() == Job::UserCanceled && !mFinished) {
        qCDebug(AKONADICORE_LOG) << "ItemSync of collection" << mSyncCollection.id() << "finished due to user cancelling";
        mFinished = true;

        auto *job = new EndItemSyncJob(q);
        job->rollback();
        q->connect(job, &Job::result, q, [q]() { q->emitResult(); });
        return;
    }

    if (allProcessed() && !mFinished) {
        // prevent double result emission, can happen since checkDone() is called from all over the place
        qCDebug(AKONADICORE_LOG) << "ItemSync of collection" << mSyncCollection.id() << "finished";
        mFinished = true;

        auto *job = new EndItemSyncJob(q);
        q->connect(job, &Job::result, q, [q]() { q->emitResult(); });
    }
}

ItemSync::ItemSync(const Collection &collection, const QDateTime &timestamp, QObject *parent)
    : Job(new ItemSyncPrivate(this), parent)
{
    qCDebug(AKONADICORE_LOG) << "Created ItemSync(colId=" << collection.id() << ", timestamp=" << timestamp << ")";
    Q_D(ItemSync);
    d->mSyncCollection = collection;
    if (timestamp.isValid()) {
        d->mItemSyncStart = timestamp;
    }
}

ItemSync::~ItemSync() = default;

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
    d->mTotalItemsProcessed += items.count();
    qCDebug(AKONADICORE_LOG) << "Received batch: " << items.count() << "Already processed: " << d->mTotalItemsProcessed
                             << "Expected total amount: " << d->mTotalItems;
    if (!d->mDisableAutomaticDeliveryDone && (d->mTotalItemsProcessed == d->mTotalItems)) {
        d->mDeliveryDone = true;
    }

    d->beginItemSyncIfNeeded();

    for (const auto &item : items) {
        d->createOrMerge(item);
    }

    d->checkDone();
}

void ItemSync::setTotalItems(int amount)
{
    Q_D(ItemSync);
    Q_ASSERT(!d->mIncremental);
    Q_ASSERT(amount >= 0);
    setStreamingEnabled(true);
    d->mTotalItems = amount;
    setTotalAmount(KJob::Bytes, amount);
    if (!d->mDisableAutomaticDeliveryDone && (d->mTotalItems == 0)) {
        d->mDeliveryDone = true;
    }

    d->checkDone();
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

    d->beginItemSyncIfNeeded();

    for (const auto &item : changedItems) {
        d->createOrMerge(item);
    }

    if (!removedItems.empty()) {
        new ItemDeleteJob(removedItems, this);
    }

    d->mTotalItemsProcessed += changedItems.count() + removedItems.count();
    qCDebug(AKONADICORE_LOG) << "Received: " << changedItems.count() << "Removed: " << removedItems.count() << "In total: " << d->mTotalItemsProcessed
                             << " Wanted: " << d->mTotalItems;
    if (!d->mDisableAutomaticDeliveryDone && (d->mTotalItemsProcessed == d->mTotalItems)) {
        d->mDeliveryDone = true;
    }

    d->checkDone();
}

void ItemSync::doStart()
{
    Q_D(const ItemSync);
    qCDebug(AKONADICORE_LOG) << "ItemSync of collection" << d->mSyncCollection.id() << "starting.";
}

QString ItemSyncPrivate::jobDebuggingString() const
{
    // TODO: also print out mIncremental and mTotalItemsProcessed, but they are set after the job
    // started, so this requires passing jobDebuggingString to jobEnded().
    return QStringLiteral("Collection %1 (%2)").arg(mSyncCollection.id()).arg(mSyncCollection.name());
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

    d->checkDone();
}

void ItemSync::slotResult(KJob *job)
{
    Q_D(ItemSync);
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
        //qCDebug(AKONADICORE_LOG) << "ItemSync of collection" << d->mSyncCollection.id() << ", job" << job << "finished," << subjobs().size() << "subjobs pending";
        Akonadi::Job::slotResult(job);
    }

    d->checkDone();
}

void ItemSync::rollback()
{
    Q_D(ItemSync);
    if (!d->mStarted || error()) {
        d->mDeliveryDone = true;
        return;
    }

    qCDebug(AKONADICORE_LOG) << "The item sync is being rolled-back.";
    setError(UserCanceled);

    d->mDeliveryDone = true; // user won't deliver more data

    d->checkDone();
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

void ItemSync::setFetchScope(ItemFetchScope &/*scope*/)
{
}

ItemFetchScope &ItemSync::fetchScope()
{
    Q_D(ItemSync);
    if (!d->mFetchScope) {
        d->mFetchScope = std::make_unique<ItemFetchScope>();
    }

    return *d->mFetchScope;
}

void ItemSync::setTransactionMode(TransactionMode /*mode*/)
{
}

int ItemSync::batchSize() const
{
    return std::numeric_limits<int>::max();
}

void ItemSync::setBatchSize(int /*size*/)
{
}

#include "itemsync.moc"
