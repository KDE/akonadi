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

#include "etmviewstatesaver.h"

#include <QtCore/QModelIndex>

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

    Entity::Id id = key.mid(1).toLongLong();
    if (id < 0) {
        return QModelIndex();
    }

    if (key.startsWith(QLatin1Char('c'))) {
        QModelIndex idx = EntityTreeModel::modelIndexForCollection(model, Collection(id));
        if (!idx.isValid()) {
            return QModelIndex();
        }
        return idx;
    } else if (key.startsWith(QLatin1Char('i'))) {
        QModelIndexList list = EntityTreeModel::modelIndexesForItem(model, Item(id));
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
    Entity::Id id = index.data(EntityTreeModel::ItemIdRole).value<Entity::Id>();
    if (id >= 0) {
        return QStringLiteral("i%1").arg(id);
    }
    return QString();
}

void ETMViewStateSaver::selectCollections(const Akonadi::Collection::List &list)
{
    QStringList colStrings;
    foreach (const Collection &col, list) {
        colStrings << QStringLiteral("c%1").arg(col.id());
    }
    restoreSelection(colStrings);
}

void ETMViewStateSaver::selectCollections(const QList< Collection::Id > &list)
{
    QStringList colStrings;
    foreach (const Collection::Id &colId, list) {
        colStrings << QStringLiteral("c%1").arg(colId);
    }
    restoreSelection(colStrings);
}

void ETMViewStateSaver::selectItems(const Akonadi::Item::List &list)
{
    QStringList itemStrings;
    foreach (const Item &item, list) {
        itemStrings << QStringLiteral("i%1").arg(item.id());
    }
    restoreSelection(itemStrings);
}

void ETMViewStateSaver::selectItems(const QList< Item::Id > &list)
{
    QStringList itemStrings;
    foreach (const Item::Id &itemId, list) {
        itemStrings << QStringLiteral("i%1").arg(itemId);
    }
    restoreSelection(itemStrings);
}
