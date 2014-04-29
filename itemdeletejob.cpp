/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemdeletejob.h"

#include "collection.h"
#include "collectionselectjob_p.h"
#include "item.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/imapset_p.h>
#include <akonadi/private/protocol_p.h>

using namespace Akonadi;

class Akonadi::ItemDeleteJobPrivate : public JobPrivate
{
public:
    ItemDeleteJobPrivate(ItemDeleteJob *parent)
        : JobPrivate(parent)
    {
    }

    Q_DECLARE_PUBLIC(ItemDeleteJob)

    Item::List mItems;
    Collection mCollection;
    Tag mTag;
};

ItemDeleteJob::ItemDeleteJob(const Item &item, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mItems << item;
}

ItemDeleteJob::ItemDeleteJob(const Item::List &items, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mItems = items;
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

    d->mTag = tag;
}

ItemDeleteJob::~ItemDeleteJob()
{
}

Item::List ItemDeleteJob::deletedItems() const
{
    Q_D(const ItemDeleteJob);

    return d->mItems;
}

void ItemDeleteJob::doStart()
{
    Q_D(ItemDeleteJob);

    QByteArray command = d->newTag();
    command += ProtocolHelper::commandContextToByteArray(d->mCollection, d->mTag, d->mItems, AKONADI_CMD_ITEMDELETE);
    command += '\n';

    d->writeData(command);
}

#include "moc_itemdeletejob.cpp"
