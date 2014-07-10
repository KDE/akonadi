/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "entityorderproxymodel.h"

#include <QMimeData>

#include <KConfigGroup>
#include <KUrl>

#include "collection.h"
#include "item.h"
#include "entitytreemodel.h"

namespace Akonadi
{

class EntityOrderProxyModelPrivate
{
public:
    EntityOrderProxyModelPrivate(EntityOrderProxyModel *qq)
        : q_ptr(qq)
    {

    }

    void saveOrder(const QModelIndex &index);

    KConfigGroup m_orderConfig;

    Q_DECLARE_PUBLIC(EntityOrderProxyModel)
    EntityOrderProxyModel *const q_ptr;

};

}

using namespace Akonadi;

EntityOrderProxyModel::EntityOrderProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new EntityOrderProxyModelPrivate(this))
{
    setDynamicSortFilter(true);
    //setSortCaseSensitivity( Qt::CaseInsensitive );
}

EntityOrderProxyModel::~EntityOrderProxyModel()
{
    delete d_ptr;
}

void EntityOrderProxyModel::setOrderConfig(KConfigGroup &configGroup)
{
    Q_D(EntityOrderProxyModel);
    layoutAboutToBeChanged();
    d->m_orderConfig = configGroup;
    layoutChanged();
}

bool EntityOrderProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    Q_D(const EntityOrderProxyModel);

    if (!d->m_orderConfig.isValid()) {
        return QSortFilterProxyModel::lessThan(left, right);
    }
    Collection col = left.data(EntityTreeModel::ParentCollectionRole).value<Collection>();

    if (!d->m_orderConfig.hasKey(QString::number(col.id()))) {
        return QSortFilterProxyModel::lessThan(left, right);
    }

    const QStringList list = d->m_orderConfig.readEntry(QString::number(col.id()), QStringList());

    if (list.isEmpty()) {
        return QSortFilterProxyModel::lessThan(left, right);
    }

    const QString leftValue = configString(left);
    const QString rightValue = configString(right);

    const int leftPosition = list.indexOf(leftValue);
    const int rightPosition = list.indexOf(rightValue);

    if (leftPosition < 0 || rightPosition < 0) {
        return QSortFilterProxyModel::lessThan(left, right);
    }

    return leftPosition < rightPosition;
}

bool EntityOrderProxyModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_D(EntityOrderProxyModel);

    if (!d->m_orderConfig.isValid()) {
        return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
    }

    if (!data->hasFormat(QStringLiteral("text/uri-list"))) {
        return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
    }

    if (row == -1) {
        return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
    }

    bool containsMove = false;

    const KUrl::List urls = KUrl::List::fromMimeData(data);

    Collection parentCol;

    if (parent.isValid()) {
        parentCol = parent.data(EntityTreeModel::CollectionRole).value<Collection>();
    } else {
        if (!hasChildren(parent)) {
            return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
        }

        const QModelIndex targetIndex = index(0, column, parent);

        parentCol = targetIndex.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
    }

    QStringList droppedList;
    foreach (const KUrl &url, urls) {
        Collection col = Collection::fromUrl(url);

        if (!col.isValid()) {
            Item item = Item::fromUrl(url);
            if (!item.isValid()) {
                continue;
            }

            const QModelIndexList list = EntityTreeModel::modelIndexesForItem(this, item);
            if (list.isEmpty()) {
                continue;
            }

            if (!containsMove && list.first().data(EntityTreeModel::ParentCollectionRole).value<Collection>().id() != parentCol.id()) {
                containsMove = true;
            }

            droppedList << configString(list.first());
        } else {
            const QModelIndex idx = EntityTreeModel::modelIndexForCollection(this, col);
            if (!idx.isValid()) {
                continue;
            }

            if (!containsMove && idx.data(EntityTreeModel::ParentCollectionRole).value<Collection>().id() != parentCol.id()) {
                containsMove = true;
            }

            droppedList << configString(idx);
        }
    }

    QStringList existingList;
    if (d->m_orderConfig.hasKey(QString::number(parentCol.id()))) {
        existingList = d->m_orderConfig.readEntry(QString::number(parentCol.id()), QStringList());
    } else {
        const int rowCount = this->rowCount(parent);
        for (int row = 0; row < rowCount; ++row) {
            static const int column = 0;
            const QModelIndex idx = this->index(row, column, parent);
            existingList.append(configString(idx));
        }
    }
    const int numberOfDroppedElement(droppedList.size());
    for (int i = 0; i < numberOfDroppedElement; ++i) {
        const QString droppedItem = droppedList.at(i);
        const int existingIndex = existingList.indexOf(droppedItem);
        existingList.removeAt(existingIndex);
        existingList.insert(row + i - (existingIndex > row ? 0 : 1), droppedList.at(i));
    }

    d->m_orderConfig.writeEntry(QString::number(parentCol.id()), existingList);

    if (containsMove) {
        bool result = QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
        invalidate();
        return result;
    }
    invalidate();
    return true;
}

QModelIndexList EntityOrderProxyModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (role < Qt::UserRole) {
        return QSortFilterProxyModel::match(start, role, value, hits, flags);
    }

    QModelIndexList list;
    QModelIndex proxyIndex;
    foreach (const QModelIndex &idx, sourceModel()->match(mapToSource(start), role, value, hits, flags)) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid()) {
            list << proxyIndex;
        }
    }

    return list;
}

void EntityOrderProxyModelPrivate::saveOrder(const QModelIndex &parent)
{
    Q_Q(const EntityOrderProxyModel);
    int rowCount = q->rowCount(parent);

    if (rowCount == 0) {
        return;
    }

    static const int column = 0;
    QModelIndex childIndex = q->index(0, column, parent);

    QString parentKey = q->parentConfigString(childIndex);

    if (parentKey.isEmpty()) {
        return;
    }

    QStringList list;

    list << q->configString(childIndex);
    saveOrder(childIndex);

    for (int row = 1; row < rowCount; ++row) {
        childIndex = q->index(row, column, parent);
        list << q->configString(childIndex);
        saveOrder(childIndex);
    }

    m_orderConfig.writeEntry(parentKey, list);
}

QString EntityOrderProxyModel::parentConfigString(const QModelIndex &index) const
{
    const Collection col = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();

    Q_ASSERT(col.isValid());
    if (!col.isValid()) {
        return QString();
    }

    return QString::number(col.id());
}

QString EntityOrderProxyModel::configString(const QModelIndex &index) const
{
    Entity::Id eId = index.data(EntityTreeModel::ItemIdRole).toLongLong();
    if (eId != -1) {
        return QString::fromLatin1("i") + QString::number(eId);
    }
    eId = index.data(EntityTreeModel::CollectionIdRole).toLongLong();
    if (eId != -1) {
        return QString::fromLatin1("c") + QString::number(eId);
    }
    Q_ASSERT(!"Invalid entity");
    return QString();
}

void EntityOrderProxyModel::saveOrder()
{
    Q_D(EntityOrderProxyModel);
    d->saveOrder(QModelIndex());
    d->m_orderConfig.sync();
}

void EntityOrderProxyModel::clearOrder(const QModelIndex &parent)
{
    Q_D(EntityOrderProxyModel);

    const QString parentKey = parentConfigString(index(0, 0, parent));

    if (parentKey.isEmpty()) {
        return;
    }

    d->m_orderConfig.deleteEntry(parentKey);
    invalidate();
}

void EntityOrderProxyModel::clearTreeOrder()
{
    Q_D(EntityOrderProxyModel);
    d->m_orderConfig.deleteGroup();
    invalidate();
}
