/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemfetchjob.h"

#include "akonadicore_debug.h"
#include "attributefactory.h"
#include "collection.h"
#include "itemfetchscope.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"
#include "session_p.h"
#include "tagfetchscope.h"

#include <QTimer>

#include <span>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
    static constexpr std::size_t MaxBatchSize = 10'000UL;

public:
    explicit ItemFetchJobPrivate(ItemFetchJob *parent)
        : JobPrivate(parent)
        , mCollection(Collection::root())
    {
        mEmitTimer.setSingleShot(true);
        mEmitTimer.setInterval(std::chrono::milliseconds{100});
    }

    ~ItemFetchJobPrivate() override
    {
        delete mValuePool;
    }

    void init()
    {
        QObject::connect(&mEmitTimer, &QTimer::timeout, q_ptr, [this]() {
            timeout();
        });
    }

    void aboutToFinish() override
    {
        timeout();
    }

    void timeout()
    {
        Q_Q(ItemFetchJob);

        mEmitTimer.stop(); // in case we are called by result()
        if (!mPendingItems.isEmpty()) {
            if (!q->error()) {
                Q_EMIT q->itemsReceived(mPendingItems);
            }
            mPendingItems.clear();
        }
    }

    bool requestBatch()
    {
        if (!mRequestedItems.empty()) {
            // If there are more items to fetch, but we already received the LIMIT number of items,
            // we are technically done...
            if (mItemsLimit.limit() > -1 && mCount >= mItemsLimit.limit()) {
                return true;
            }

            const auto batchSize = qMin(MaxBatchSize, mRemainingItems.size());
            if (batchSize == 0) {
                // If we would fetch an empty batch, we are done.
                return true;
            }

            const auto batch = mRemainingItems.subspan(0, batchSize);
            mRemainingItems = mRemainingItems.subspan(batch.size());
            const auto batchItems = QList(batch.begin(), batch.end());
            sendCommand(Protocol::FetchItemsCommandPtr::create(ProtocolHelper::entitySetToScope(batchItems),
                                                               ProtocolHelper::commandContextToProtocol(mCollection, mCurrentTag, batchItems),
                                                               ProtocolHelper::itemFetchScopeToProtocol(mFetchScope),
                                                               ProtocolHelper::tagFetchScopeToProtocol(mFetchScope.tagFetchScope()),
                                                               mItemsLimit));

        } else { // collection- or tag-based fetch (unbatched)
            Q_ASSERT(mCount == 0); // can only be called once
            sendCommand(Protocol::FetchItemsCommandPtr::create(Scope(),
                                                               ProtocolHelper::commandContextToProtocol(mCollection, mCurrentTag, mRequestedItems),
                                                               ProtocolHelper::itemFetchScopeToProtocol(mFetchScope),
                                                               ProtocolHelper::tagFetchScopeToProtocol(mFetchScope.tagFetchScope()),
                                                               mItemsLimit));
        }

        return false;
    }

    QString jobDebuggingString() const override
    {
        if (mRequestedItems.isEmpty()) {
            QString str = QStringLiteral("All items from collection %1").arg(mCollection.id());
            if (mFetchScope.fetchChangedSince().isValid()) {
                str += QStringLiteral(" changed since %1").arg(mFetchScope.fetchChangedSince().toString());
            }
            return str;

        } else {
            try {
                QString itemStr = QStringLiteral("items id: ");
                bool firstItem = true;
                for (const Akonadi::Item &item : std::as_const(mRequestedItems)) {
                    if (firstItem) {
                        firstItem = false;
                    } else {
                        itemStr += QStringLiteral(", ");
                    }
                    itemStr += QString::number(item.id());
                    const Akonadi::Collection parentCollection = item.parentCollection();
                    if (parentCollection.isValid()) {
                        itemStr += QStringLiteral(" from collection %1").arg(parentCollection.id());
                    }
                }
                return itemStr;
                // return QString(); //QString::fromLatin1(ProtocolHelper::entitySetToScope(mRequestedItems));
            } catch (const Exception &e) {
                return QString::fromUtf8(e.what());
            }
        }
    }

    Q_DECLARE_PUBLIC(ItemFetchJob)

    Collection mCollection;
    Tag mCurrentTag;
    Item::List mRequestedItems;
    Item::List mResultItems;
    ItemFetchScope mFetchScope;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer mEmitTimer;
    ProtocolHelperValuePool *mValuePool = nullptr;
    ItemFetchJob::DeliveryOptions mDeliveryOptions = ItemFetchJob::Default;
    int mCount = 0;
    Protocol::FetchLimit mItemsLimit;
    std::span<Item> mRemainingItems;
};

ItemFetchJob::ItemFetchJob(const Collection &collection, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);
    d->init();

    d->mCollection = collection;
    d->mValuePool = new ProtocolHelperValuePool; // only worth it for lots of results
}

ItemFetchJob::ItemFetchJob(const Item &item, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);
    d->init();

    d->mRequestedItems.append(item);
}

ItemFetchJob::ItemFetchJob(const Item::List &items, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);
    d->init();

    d->mRequestedItems = items;
}

ItemFetchJob::ItemFetchJob(const QList<Item::Id> &items, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);
    d->init();

    d->mRequestedItems.reserve(items.size());
    for (auto id : items) {
        d->mRequestedItems.append(Item(id));
    }
}
ItemFetchJob::ItemFetchJob(const Tag &tag, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);
    d->init();

    d->mCurrentTag = tag;
    d->mValuePool = new ProtocolHelperValuePool;
}

ItemFetchJob::~ItemFetchJob() = default;

void ItemFetchJob::doStart()
{
    Q_D(ItemFetchJob);

    try {
        d->mRemainingItems = std::span(d->mRequestedItems.begin(), d->mRequestedItems.size());
        d->requestBatch();
    } catch (const Akonadi::Exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }
}

bool ItemFetchJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(ItemFetchJob);

    if (!response->isResponse() || response->type() != Protocol::Command::FetchItems) {
        return Job::doHandleResponse(tag, response);
    }

    const auto &resp = Protocol::cmdCast<Protocol::FetchItemsResponse>(response);
    // Invalid ID marks the last part of the response
    if (resp.id() < 0) {
        if (d->mRemainingItems.empty()) {
            return true;
        }
        return d->requestBatch();
    }

    const Item item = ProtocolHelper::parseItemFetchResult(resp, nullptr, d->mValuePool);
    if (!item.isValid()) {
        return false;
    }

    d->mCount++;

    if (d->mDeliveryOptions & ItemGetter) {
        d->mResultItems.append(item);
    }

    if (d->mDeliveryOptions & EmitItemsInBatches) {
        d->mPendingItems.append(item);
        if (!d->mEmitTimer.isActive()) {
            d->mEmitTimer.start();
        }
    } else if (d->mDeliveryOptions & EmitItemsIndividually) {
        Q_EMIT itemsReceived(Item::List() << item);
    }

    return false;
}

Item::List ItemFetchJob::items() const
{
    Q_D(const ItemFetchJob);

    return d->mResultItems;
}

void ItemFetchJob::clearItems()
{
    Q_D(ItemFetchJob);

    d->mResultItems.clear();
}

void ItemFetchJob::setFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(ItemFetchJob);

    d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemFetchJob::fetchScope()
{
    Q_D(ItemFetchJob);

    return d->mFetchScope;
}

void ItemFetchJob::setCollection(const Akonadi::Collection &collection)
{
    Q_D(ItemFetchJob);

    d->mCollection = collection;
}

void ItemFetchJob::setDeliveryOption(DeliveryOptions options)
{
    Q_D(ItemFetchJob);

    d->mDeliveryOptions = options;
}

ItemFetchJob::DeliveryOptions ItemFetchJob::deliveryOptions() const
{
    Q_D(const ItemFetchJob);

    return d->mDeliveryOptions;
}

int ItemFetchJob::count() const
{
    Q_D(const ItemFetchJob);

    return d->mCount;
}

void ItemFetchJob::setLimit(int limit, int start, Qt::SortOrder order)
{
    Q_D(ItemFetchJob);
    d->mItemsLimit.setLimit(limit);
    d->mItemsLimit.setLimitOffset(start);
    d->mItemsLimit.setSortOrder(order);
}
#include "moc_itemfetchjob.cpp"
