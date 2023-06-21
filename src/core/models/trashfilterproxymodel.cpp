/*
 SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "trashfilterproxymodel.h"
#include "entitydeletedattribute.h"
#include "entitytreemodel.h"
#include "item.h"

using namespace Akonadi;

class Akonadi::TrashFilterProxyModelPrivate
{
public:
    TrashFilterProxyModelPrivate()
    {
    }
    bool mTrashIsShown = false;
};

TrashFilterProxyModel::TrashFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new TrashFilterProxyModelPrivate())
{
    setRecursiveFilteringEnabled(true);
}

TrashFilterProxyModel::~TrashFilterProxyModel() = default;

void TrashFilterProxyModel::showTrash(bool enable)
{
    Q_D(TrashFilterProxyModel);
    d->mTrashIsShown = enable;
    invalidateFilter();
}

bool TrashFilterProxyModel::trashIsShown() const
{
    Q_D(const TrashFilterProxyModel);
    return d->mTrashIsShown;
}

bool TrashFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_D(const TrashFilterProxyModel);
    const QModelIndex &index = sourceModel()->index(sourceRow, 0, sourceParent);
    const Item &item = index.data(EntityTreeModel::ItemRole).value<Item>();
    if (item.isValid()) {
        if (item.hasAttribute<EntityDeletedAttribute>()) {
            return d->mTrashIsShown;
        }
    }
    const Collection &collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        if (collection.hasAttribute<EntityDeletedAttribute>()) {
            return d->mTrashIsShown;
        }
    }
    return !d->mTrashIsShown;
}

#include "moc_trashfilterproxymodel.cpp"
