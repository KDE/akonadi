/*
    SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno.virlet@gmail.com>
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitymimetypefiltermodel.h"
#include "akonadicore_debug.h"

#include <QString>
#include <QStringList>

using namespace Akonadi;

namespace Akonadi
{
/**
 * @internal
 */
class EntityMimeTypeFilterModelPrivate
{
public:
    explicit EntityMimeTypeFilterModelPrivate(EntityMimeTypeFilterModel *parent)
        : q_ptr(parent)
        , m_headerGroup(EntityTreeModel::EntityTreeHeaders)
    {
    }

    Q_DECLARE_PUBLIC(EntityMimeTypeFilterModel)
    EntityMimeTypeFilterModel *q_ptr;

    QStringList includedMimeTypes;
    QStringList excludedMimeTypes;

    QPersistentModelIndex m_rootIndex;

    EntityTreeModel::HeaderGroup m_headerGroup;
};

} // namespace Akonadi

EntityMimeTypeFilterModel::EntityMimeTypeFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new EntityMimeTypeFilterModelPrivate(this))
{
}

EntityMimeTypeFilterModel::~EntityMimeTypeFilterModel() = default;

void EntityMimeTypeFilterModel::addMimeTypeInclusionFilters(const QStringList &typeList)
{
    Q_D(EntityMimeTypeFilterModel);
    d->includedMimeTypes << typeList;
    invalidateFilter();
}

void EntityMimeTypeFilterModel::addMimeTypeExclusionFilters(const QStringList &typeList)
{
    Q_D(EntityMimeTypeFilterModel);
    d->excludedMimeTypes << typeList;
    invalidateFilter();
}

void EntityMimeTypeFilterModel::addMimeTypeInclusionFilter(const QString &type)
{
    Q_D(EntityMimeTypeFilterModel);
    d->includedMimeTypes << type;
    invalidateFilter();
}

void EntityMimeTypeFilterModel::addMimeTypeExclusionFilter(const QString &type)
{
    Q_D(EntityMimeTypeFilterModel);
    d->excludedMimeTypes << type;
    invalidateFilter();
}

bool EntityMimeTypeFilterModel::filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const
{
    if (sourceColumn >= columnCount(mapFromSource(sourceParent))) {
        return false;
    }
    return QSortFilterProxyModel::filterAcceptsColumn(sourceColumn, sourceParent);
}

bool EntityMimeTypeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_D(const EntityMimeTypeFilterModel);
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    const QString rowMimetype = idx.data(EntityTreeModel::MimeTypeRole).toString();

    if (d->excludedMimeTypes.contains(rowMimetype)) {
        return false;
    }

    if (d->includedMimeTypes.isEmpty() || d->includedMimeTypes.contains(rowMimetype)) {
        const auto item = idx.data(EntityTreeModel::ItemRole).value<Akonadi::Item>();

        if (item.isValid() && !item.hasPayload()) {
            qCDebug(AKONADICORE_LOG) << "Item " << item.id() << " doesn't have payload";
            return false;
        }

        return true;
    }

    return false;
}

QStringList EntityMimeTypeFilterModel::mimeTypeInclusionFilters() const
{
    Q_D(const EntityMimeTypeFilterModel);
    return d->includedMimeTypes;
}

QStringList EntityMimeTypeFilterModel::mimeTypeExclusionFilters() const
{
    Q_D(const EntityMimeTypeFilterModel);
    return d->excludedMimeTypes;
}

void EntityMimeTypeFilterModel::clearFilters()
{
    Q_D(EntityMimeTypeFilterModel);
    d->includedMimeTypes.clear();
    d->excludedMimeTypes.clear();
    invalidateFilter();
}

void EntityMimeTypeFilterModel::setHeaderGroup(EntityTreeModel::HeaderGroup headerGroup)
{
    Q_D(EntityMimeTypeFilterModel);
    if (d->m_headerGroup != headerGroup) {
        d->m_headerGroup = headerGroup; // this changes the column count
        invalidateFilter(); // and filterAcceptsColumn depends on the column count
    }
}

QVariant EntityMimeTypeFilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!sourceModel()) {
        return QVariant();
    }

    Q_D(const EntityMimeTypeFilterModel);
    role += (EntityTreeModel::TerminalUserRole * d->m_headerGroup);
    return sourceModel()->headerData(section, orientation, role);
}

QModelIndexList EntityMimeTypeFilterModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (!sourceModel()) {
        return QModelIndexList();
    }

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

int EntityMimeTypeFilterModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const EntityMimeTypeFilterModel);

    if (!sourceModel()) {
        return 0;
    }

    const QVariant value = sourceModel()->data(mapToSource(parent), EntityTreeModel::ColumnCountRole + (EntityTreeModel::TerminalUserRole * d->m_headerGroup));
    if (!value.isValid()) {
        return 0;
    }

    return value.toInt();
}

bool EntityMimeTypeFilterModel::hasChildren(const QModelIndex &parent) const
{
    if (!sourceModel()) {
        return false;
    }

    // QSortFilterProxyModel implementation is buggy in that it emits rowsAboutToBeInserted etc
    // only after the source model has emitted rowsInserted, instead of emitting it when the
    // source model emits rowsAboutToBeInserted. That means that the source and the proxy are out
    // of sync around the time of insertions, so we can't use the optimization below.
    return rowCount(parent) > 0;
#if 0

    if (!parent.isValid()) {
        return sourceModel()->hasChildren(parent);
    }

    Q_D(const EntityMimeTypeFilterModel);
    if (EntityTreeModel::ItemListHeaders == d->m_headerGroup) {
        return false;
    }

    if (EntityTreeModel::CollectionTreeHeaders == d->m_headerGroup) {
        QModelIndex childIndex = parent.child(0, 0);
        while (childIndex.isValid()) {
            Collection col = childIndex.data(EntityTreeModel::CollectionRole).value<Collection>();
            if (col.isValid()) {
                return true;
            }
            childIndex = childIndex.sibling(childIndex.row() + 1, childIndex.column());
        }
    }
    return false;
#endif
}

bool EntityMimeTypeFilterModel::canFetchMore(const QModelIndex &parent) const
{
    Q_D(const EntityMimeTypeFilterModel);
    if (EntityTreeModel::CollectionTreeHeaders == d->m_headerGroup) {
        return false;
    }
    return QSortFilterProxyModel::canFetchMore(parent);
}

#include "moc_entitymimetypefiltermodel.cpp"
