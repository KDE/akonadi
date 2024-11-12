/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemmovejob.h"

#include "collection.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <KLocalizedString>

#include <span>

using namespace Akonadi;

class Akonadi::ItemMoveJobPrivate : public Akonadi::JobPrivate
{
    static constexpr size_t MaxBatchSize = 10'000;

public:
    explicit ItemMoveJobPrivate(ItemMoveJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const override
    {
        QString str = QStringLiteral("Move item");
        if (source.isValid()) {
            str += QStringLiteral("from collection %1").arg(source.id());
        }
        str += QStringLiteral(" to collection %1. ").arg(destination.id());
        if (items.isEmpty()) {
            str += QStringLiteral("No Items defined.");
        } else {
            str += QStringLiteral("Items: ");
            const int nbItems = items.count();
            for (int i = 0; i < nbItems; ++i) {
                if (i != 0) {
                    str += QStringLiteral(", ");
                }
                str += QString::number(items.at(i).id());
            }
        }
        return str;
    }

    bool nextBatch()
    {
        Q_Q(ItemMoveJob);

        if (remainingItems.empty()) {
            return true;
        }

        try {
            const auto batchSize = qMin(MaxBatchSize, remainingItems.size());
            const auto batch = remainingItems.subspan(0, batchSize);
            remainingItems = remainingItems.subspan(batchSize);
            const auto batchItems = QList(batch.begin(), batch.end());
            sendCommand(Protocol::MoveItemsCommandPtr::create(ProtocolHelper::entitySetToScope(batchItems),
                                                              ProtocolHelper::commandContextToProtocol(source, Tag(), batchItems),
                                                              ProtocolHelper::entityToScope(destination)));
            return false;
        } catch (const Akonadi::Exception &e) {
            q->setError(Job::Unknown);
            q->setErrorText(QString::fromUtf8(e.what()));
            q->emitResult();
            return true;
        }
    }

    Item::List items;
    Collection destination;
    Collection source;
    std::span<Item> remainingItems;

    Q_DECLARE_PUBLIC(ItemMoveJob)
};

ItemMoveJob::ItemMoveJob(const Item &item, const Collection &destination, QObject *parent)
    : Job(new ItemMoveJobPrivate(this), parent)
{
    Q_D(ItemMoveJob);
    d->destination = destination;
    d->items.append(item);
}

ItemMoveJob::ItemMoveJob(const Item::List &items, const Collection &destination, QObject *parent)
    : Job(new ItemMoveJobPrivate(this), parent)
{
    Q_D(ItemMoveJob);
    d->destination = destination;
    d->items = items;
}

ItemMoveJob::ItemMoveJob(const Item::List &items, const Collection &source, const Collection &destination, QObject *parent)
    : Job(new ItemMoveJobPrivate(this), parent)
{
    Q_D(ItemMoveJob);
    d->source = source;
    d->destination = destination;
    d->items = items;
}

ItemMoveJob::~ItemMoveJob() = default;

void ItemMoveJob::doStart()
{
    Q_D(ItemMoveJob);

    if (d->items.isEmpty()) {
        setError(Job::Unknown);
        setErrorText(i18n("No objects specified for moving"));
        emitResult();
        return;
    }

    if (!d->destination.isValid() && d->destination.remoteId().isEmpty()) {
        setError(Job::Unknown);
        setErrorText(i18n("No valid destination specified"));
        emitResult();
        return;
    }

    d->remainingItems = std::span<Item>(d->items);
    d->nextBatch();
}

bool ItemMoveJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(ItemMoveJob);

    if (!response->isResponse() || response->type() != Protocol::Command::MoveItems) {
        return Job::doHandleResponse(tag, response);
    }

    return d->nextBatch();
}

Collection ItemMoveJob::destinationCollection() const
{
    Q_D(const ItemMoveJob);
    return d->destination;
}

Item::List ItemMoveJob::items() const
{
    Q_D(const ItemMoveJob);
    return d->items;
}

#include "moc_itemmovejob.cpp"
