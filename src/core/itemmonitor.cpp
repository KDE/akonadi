/*
    SPDX-FileCopyrightText: 2007-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemmonitor.h"
#include "itemmonitor_p.h"
#include "interface.h"

#include "itemfetchscope.h"


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
    ItemFetchOptions options;
    options.setItemFetchScope(fetchScope());
    Akonadi::fetchItem(d->mItem, options)
        .then([this](const Item &item) {
            d->mItem = item;
            Q_EMIT itemChanged(item);
        });
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
