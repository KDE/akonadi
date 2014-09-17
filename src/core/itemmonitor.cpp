/*
    Copyright (c) 2007-2008 Tobias Koenig <tokoe@kde.org>

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

#include "itemmonitor.h"
#include "itemmonitor_p.h"

#include "itemfetchscope.h"

#include <QtCore/QStringList>

using namespace Akonadi;

ItemMonitor::ItemMonitor()
    : d(new Private(this))
{
}

ItemMonitor::~ItemMonitor()
{
    delete d;
}

void ItemMonitor::setItem(const Item &item)
{
    if (item == d->mItem) {
        return;
    }

    d->mMonitor->setItemMonitored(d->mItem, false);

    d->mItem = item;

    d->mMonitor->setItemMonitored(d->mItem, true);

    if (!d->mItem.isValid()) {
        itemRemoved();
        return;
    }

    // start initial fetch of the new item
    ItemFetchJob *job = new ItemFetchJob(d->mItem);
    job->setFetchScope(fetchScope());

    d->connect(job, SIGNAL(result(KJob*)), d, SLOT(initialFetchDone(KJob*)));
}

Item ItemMonitor::item() const
{
    return d->mItem;
}

void ItemMonitor::itemChanged(const Item &item)
{
    Q_UNUSED(item)
}

void ItemMonitor::itemRemoved()
{
}

void ItemMonitor::setFetchScope(const ItemFetchScope &fetchScope)
{
    d->mMonitor->setItemFetchScope(fetchScope);
}

ItemFetchScope &ItemMonitor::fetchScope()
{
    return d->mMonitor->itemFetchScope();
}

#include "moc_itemmonitor_p.cpp"
