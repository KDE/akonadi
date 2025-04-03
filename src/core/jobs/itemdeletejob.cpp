/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemdeletejob.h"

#include "collection.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <span>

using namespace Akonadi;

class Akonadi::ItemDeleteJobPrivate : public JobPrivate
{
    static constexpr std::size_t MaxBatchSize = 10'000UL;

public:
    explicit ItemDeleteJobPrivate(ItemDeleteJob *parent)
        : JobPrivate(parent)
    {
    }

    bool requestBatch()
    {
        if (!mItems.empty()) { // item-based removal
            const auto batchSize = qMin(MaxBatchSize, mRemainingItems.size());
            if (batchSize == 0) {
                return true;
            }

            const auto batch = mRemainingItems.subspan(0, batchSize);
            mRemainingItems = mRemainingItems.subspan(batch.size());
            const auto batchItems = QList(batch.begin(), batch.end());

            sendCommand(Protocol::DeleteItemsCommandPtr::create(ProtocolHelper::entitySetToScope(batchItems),
                                                                ProtocolHelper::commandContextToProtocol(mCollection, mCurrentTag, batchItems)));
        } else { // collection- or tag-based removal
            sendCommand(Protocol::DeleteItemsCommandPtr::create(Scope(), ProtocolHelper::commandContextToProtocol(mCollection, mCurrentTag, {})));
        }

        return false;
    }

    Q_DECLARE_PUBLIC(ItemDeleteJob)
    QString jobDebuggingString() const override;

    Item::List mItems;
    Collection mCollection;
    Tag mCurrentTag;
    std::span<const Item> mRemainingItems;
};

QString Akonadi::ItemDeleteJobPrivate::jobDebuggingString() const
{
    QString itemStr = QStringLiteral("items id: ");
    bool firstItem = true;
    for (const Akonadi::Item &item : std::as_const(mItems)) {
        if (firstItem) {
            firstItem = false;
        } else {
            itemStr += QStringLiteral(", ");
        }
        itemStr += QString::number(item.id());
    }

    return QStringLiteral("Remove %1 from collection id %2").arg(itemStr).arg(mCollection.id());
}

ItemDeleteJob::ItemDeleteJob(const Item &item, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mItems << item;
    d->mRemainingItems = std::span(d->mItems);
}

ItemDeleteJob::ItemDeleteJob(const Item::List &items, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mItems = items;
    d->mRemainingItems = std::span(d->mItems);
}

ItemDeleteJob::ItemDeleteJob(const Collection &collection, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mCollection = collection;
}

ItemDeleteJob::ItemDeleteJob(const Tag &tag, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mCurrentTag = tag;
}

ItemDeleteJob::~ItemDeleteJob() = default;

Item::List ItemDeleteJob::deletedItems() const
{
    Q_D(const ItemDeleteJob);

    return d->mItems;
}

void ItemDeleteJob::doStart()
{
    Q_D(ItemDeleteJob);

    try {
        d->requestBatch();
    } catch (const Akonadi::Exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }
}

bool ItemDeleteJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(ItemDeleteJob);

    if (!response->isResponse() || response->type() != Protocol::Command::DeleteItems) {
        return Job::doHandleResponse(tag, response);
    }

    if (!d->mRemainingItems.empty()) {
        return d->requestBatch();
    }

    return true;
}

#include "moc_itemdeletejob.cpp"
