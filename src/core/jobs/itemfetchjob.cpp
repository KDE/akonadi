/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemfetchjob.h"

#include "attributefactory.h"
#include "collection.h"
#include "itemfetchscope.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"
#include "session_p.h"
#include "tagfetchscope.h"

#include <QTimer>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
public:
    explicit ItemFetchJobPrivate(ItemFetchJob *parent)
        : JobPrivate(parent)
    {
        mCollection = Collection::root();
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

ItemFetchJob::ItemFetchJob(const QVector<Item::Id> &items, QObject *parent)
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
        d->sendCommand(Protocol::FetchItemsCommandPtr::create(d->mRequestedItems.isEmpty() ? Scope() : ProtocolHelper::entitySetToScope(d->mRequestedItems),
                                                              ProtocolHelper::commandContextToProtocol(d->mCollection, d->mCurrentTag, d->mRequestedItems),
                                                              ProtocolHelper::itemFetchScopeToProtocol(d->mFetchScope),
                                                              ProtocolHelper::tagFetchScopeToProtocol(d->mFetchScope.tagFetchScope())));
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
        return true;
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
#include "moc_itemfetchjob.cpp"
