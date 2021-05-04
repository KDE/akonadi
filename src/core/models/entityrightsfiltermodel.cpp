/*
    SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno.virlet@gmail.com>
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entityrightsfiltermodel.h"

using namespace Akonadi;

namespace Akonadi
{
/**
 * @internal
 */
class EntityRightsFilterModelPrivate
{
public:
    explicit EntityRightsFilterModelPrivate(EntityRightsFilterModel *parent)
        : q_ptr(parent)
        , mAccessRights(Collection::AllRights)
    {
    }

    bool rightsMatches(const QModelIndex &index) const
    {
        if (mAccessRights == Collection::AllRights || mAccessRights == Collection::ReadOnly) {
            return true;
        }

        const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        if (collection.isValid()) {
            return (mAccessRights & collection.rights());
        } else {
            const Item item = index.data(EntityTreeModel::ItemRole).value<Item>();
            if (item.isValid()) {
                const auto collection = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
                return (mAccessRights & collection.rights());
            } else {
                return false;
            }
        }
    }

    Q_DECLARE_PUBLIC(EntityRightsFilterModel)
    EntityRightsFilterModel *q_ptr;

    Collection::Rights mAccessRights;
};

} // namespace Akonadi

EntityRightsFilterModel::EntityRightsFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new EntityRightsFilterModelPrivate(this))
{
    setRecursiveFilteringEnabled(true);
}

EntityRightsFilterModel::~EntityRightsFilterModel()
{
    delete d_ptr;
}

bool EntityRightsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_D(const EntityRightsFilterModel);

    const QModelIndex modelIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    return d->rightsMatches(modelIndex);
}

void EntityRightsFilterModel::setAccessRights(Collection::Rights rights)
{
    Q_D(EntityRightsFilterModel);
    d->mAccessRights = rights;
    invalidateFilter();
}

Collection::Rights EntityRightsFilterModel::accessRights() const
{
    Q_D(const EntityRightsFilterModel);
    return d->mAccessRights;
}

Qt::ItemFlags EntityRightsFilterModel::flags(const QModelIndex &index) const
{
    Q_D(const EntityRightsFilterModel);

    if (d->rightsMatches(index)) {
        return QSortFilterProxyModel::flags(index);
    } else {
        return QSortFilterProxyModel::flags(index) & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }
}

QModelIndexList EntityRightsFilterModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (role < Qt::UserRole) {
        return QSortFilterProxyModel::match(start, role, value, hits, flags);
    }

    QModelIndexList list;
    QModelIndex proxyIndex;
    const auto matches = sourceModel()->match(mapToSource(start), role, value, hits, flags);
    for (const auto &idx : matches) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid()) {
            list.push_back(proxyIndex);
        }
    }

    return list;
}
