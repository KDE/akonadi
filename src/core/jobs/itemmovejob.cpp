/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "itemmovejob.h"

#include "collection.h"
#include "item.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <akonadi/private/protocol_p.h>

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::ItemMoveJobPrivate : public Akonadi::JobPrivate
{
public:
    ItemMoveJobPrivate(ItemMoveJob *parent)
        : JobPrivate(parent)
    {
    }

    Item::List items;
    Collection destination;
    Collection source;

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

ItemMoveJob::~ItemMoveJob()
{
}

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

    d->sendCommand(Protocol::MoveItemsCommand(
                       ProtocolHelper::entitySetToScope(d->items),
                       ProtocolHelper::commandContextToProtocol(d->source, Tag(), d->items),
                       ProtocolHelper::entityToScope(d->destination)));
}

bool ItemMoveJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    if (!response.isResponse() || response.type() != Protocol::Command::MoveItems) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
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

