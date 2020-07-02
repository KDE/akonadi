/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "etmviewstatesaver.h"

#include <QModelIndex>

#include "entitytreemodel.h"

using namespace Akonadi;

ETMViewStateSaver::ETMViewStateSaver(QObject *parent)
    : KConfigViewStateSaver(parent)
{
}

QModelIndex ETMViewStateSaver::indexFromConfigString(const QAbstractItemModel *model, const QString &key) const
{
    if (key.startsWith(QLatin1Char('x'))) {
        return QModelIndex();
    }

    Item::Id id = key.mid(1).toLongLong();
    if (id < 0) {
        return QModelIndex();
    }

    if (key.startsWith(QLatin1Char('c'))) {
        const QModelIndex idx = EntityTreeModel::modelIndexForCollection(model, Collection(id));
        if (!idx.isValid()) {
            return QModelIndex();
        }
        return idx;
    } else if (key.startsWith(QLatin1Char('i'))) {
        const QModelIndexList list = EntityTreeModel::modelIndexesForItem(model, Item(id));
        if (list.isEmpty()) {
            return QModelIndex();
        }
        return list.first();
    }
    return QModelIndex();
}

QString ETMViewStateSaver::indexToConfigString(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QStringLiteral("x-1");
    }
    const Collection c = index.data(EntityTreeModel::CollectionRole).value<Collection>();
    if (c.isValid()) {
        return QStringLiteral("c%1").arg(c.id());
    }
    Item::Id id = index.data(EntityTreeModel::ItemIdRole).value<Item::Id>();
    if (id >= 0) {
        return QStringLiteral("i%1").arg(id);
    }
    return QString();
}

void ETMViewStateSaver::selectCollections(const Akonadi::Collection::List &list)
{
    QStringList colStrings;
    colStrings.reserve(list.count());
    for (const Collection &col : list) {
        colStrings << QStringLiteral("c%1").arg(col.id());
    }
    restoreSelection(colStrings);
}

void ETMViewStateSaver::selectCollections(const QList< Collection::Id > &list)
{
    QStringList colStrings;
    colStrings.reserve(list.count());
    for (const Collection::Id &colId : list) {
        colStrings << QStringLiteral("c%1").arg(colId);
    }
    restoreSelection(colStrings);
}

void ETMViewStateSaver::selectItems(const Akonadi::Item::List &list)
{
    QStringList itemStrings;
    itemStrings.reserve(list.count());
    for (const Item &item : list) {
        itemStrings << QStringLiteral("i%1").arg(item.id());
    }
    restoreSelection(itemStrings);
}

void ETMViewStateSaver::selectItems(const QList< Item::Id > &list)
{
    QStringList itemStrings;
    itemStrings.reserve(list.count());
    for (const Item::Id &itemId : list) {
        itemStrings << QStringLiteral("i%1").arg(itemId);
    }
    restoreSelection(itemStrings);
}

void ETMViewStateSaver::setCurrentItem(const Akonadi::Item &item)
{
    restoreCurrentItem(QStringLiteral("i%1").arg(item.id()));
}

void ETMViewStateSaver::setCurrentCollection(const Akonadi::Collection &col)
{
    restoreCurrentItem(QStringLiteral("c%1").arg(col.id()));
}
