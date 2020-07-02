/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entityorderproxymodel.h"

#include <QMimeData>

#include <KConfigGroup>
#include <QUrl>

#include "collection.h"
#include "item.h"
#include "entitytreemodel.h"

namespace Akonadi
{

class EntityOrderProxyModelPrivate
{
public:
    explicit EntityOrderProxyModelPrivate(EntityOrderProxyModel *qq)
        : q_ptr(qq)
    {
    }

    void saveOrder(const QModelIndex &index);

    KConfigGroup m_orderConfig;

    Q_DECLARE_PUBLIC(EntityOrderProxyModel)
    EntityOrderProxyModel *const q_ptr;

};

} // namespace Akonadi

using namespace Akonadi;

EntityOrderProxyModel::EntityOrderProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new EntityOrderProxyModelPrivate(this))
{
    setRecursiveFilteringEnabled(true);
    setDynamicSortFilter(true);
    //setSortCaseSensitivity( Qt::CaseInsensitive );
}

EntityOrderProxyModel::~EntityOrderProxyModel()
{
    delete d_ptr;
}

void EntityOrderProxyModel::setOrderConfig(const KConfigGroup &configGroup)
{
    Q_D(EntityOrderProxyModel);
    Q_EMIT layoutAboutToBeChanged();
    d->m_orderConfig = configGroup;
    Q_EMIT layoutChanged();
}

// reimplemented in FavoriteCollectionOrderProxyModel
Collection EntityOrderProxyModel::parentCollection(const QModelIndex &index) const
{
    return index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
}

static QString configKey(const Collection &col)
{
    return !col.isValid() ? QStringLiteral("0") : QString::number(col.id());
}

bool EntityOrderProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    Q_D(const EntityOrderProxyModel);

    if (!d->m_orderConfig.isValid()) {
        return QSortFilterProxyModel::lessThan(left, right);
    }
    const Collection col = parentCollection(left);

    const QStringList list = d->m_orderConfig.readEntry(configKey(col), QStringList());

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

QStringList EntityOrderProxyModel::configStringsForDroppedUrls(const QList<QUrl> &urls, const Akonadi::Collection &parentCol, bool *containsMove) const
{
    QStringList droppedList;
    droppedList.reserve(urls.count());
    for (const QUrl &url : urls) {
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

            if (!*containsMove && parentCollection(list.first()).id() != parentCol.id()) {
                *containsMove = true;
            }

            droppedList << configString(list.first());
        } else {
            const QModelIndex idx = EntityTreeModel::modelIndexForCollection(this, col);
            if (!idx.isValid()) {
                continue;
            }

            if (!*containsMove && parentCollection(idx).id() != parentCol.id()) {
                *containsMove = true;
            }

            droppedList << configString(idx);
        }
    }
    return droppedList;
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


    const QList<QUrl> urls = data->urls();
    if (urls.isEmpty()) {
        return false;
    }

    Collection parentCol;

    if (parent.isValid()) {
        parentCol = parent.data(EntityTreeModel::CollectionRole).value<Collection>();
    } else {
        if (!hasChildren(parent)) {
            return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
        }

        const QModelIndex targetIndex = index(0, column, parent);
        parentCol = parentCollection(targetIndex);
    }


    bool containsMove = false;
    QStringList droppedList = configStringsForDroppedUrls(urls, parentCol, &containsMove);

    // Dropping new favorite folders
    if (droppedList.isEmpty()) {
        const bool ok = QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
        if (ok) {
            droppedList = configStringsForDroppedUrls(urls, parentCol, &containsMove);
        }
    }

    QStringList existingList;
    if (d->m_orderConfig.hasKey(QString::number(parentCol.id()))) {
        existingList = d->m_orderConfig.readEntry(configKey(parentCol), QStringList());
    } else {
        const int rowCount = this->rowCount(parent);
        existingList.reserve(rowCount);
        for (int row = 0; row < rowCount; ++row) {
            static const int column = 0;
            const QModelIndex idx = this->index(row, column, parent);
            existingList.append(configString(idx));
        }
    }
    const int numberOfDroppedElement(droppedList.size());
    for (int i = 0; i < numberOfDroppedElement; ++i) {
        const QString &droppedItem = droppedList.at(i);
        const int existingIndex = existingList.indexOf(droppedItem);
        existingList.removeAt(existingIndex);
        existingList.insert(row + i - (existingIndex > row ? 0 : 1), droppedList.at(i));
    }

    d->m_orderConfig.writeEntry(configKey(parentCol), existingList);

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
    const auto matches = sourceModel()->match(mapToSource(start), role, value, hits, flags);
    for (const auto &idx : matches) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid()) {
            list.push_back(proxyIndex);
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

    const QString parentKey = q->parentConfigString(childIndex);

    if (parentKey.isEmpty()) {
        return;
    }

    QStringList list;

    list << q->configString(childIndex);
    saveOrder(childIndex);
    list.reserve(list.count() + rowCount);
    for (int row = 1; row < rowCount; ++row) {
        childIndex = q->index(row, column, parent);
        list << q->configString(childIndex);
        saveOrder(childIndex);
    }

    m_orderConfig.writeEntry(parentKey, list);
}

QString EntityOrderProxyModel::parentConfigString(const QModelIndex &index) const
{
    const Collection col = parentCollection(index);

    Q_ASSERT(col.isValid());
    if (!col.isValid()) {
        return QString();
    }

    return QString::number(col.id());
}

QString EntityOrderProxyModel::configString(const QModelIndex &index) const
{
    Item::Id iId = index.data(EntityTreeModel::ItemIdRole).toLongLong();
    if (iId != -1) {
        return QLatin1Char('i') + QString::number(iId);
    }
    Collection::Id cId = index.data(EntityTreeModel::CollectionIdRole).toLongLong();
    if (cId != -1) {
        return QLatin1Char('c') + QString::number(cId);
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
