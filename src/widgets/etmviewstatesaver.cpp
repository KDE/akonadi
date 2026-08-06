/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "etmviewstatesaver.h"

#include <KConfigGroup>

#include <QItemSelectionModel>
#include <QModelIndex>

#include "entitytreemodel.h"

using namespace Akonadi;

ETMViewStateSaver::ETMViewStateSaver(QObject *parent)
    : KConfigViewStateSaver(parent)
{
}

void ETMViewStateSaver::setKeyFormat(KeyFormat format)
{
    mKeyFormat = format;
}

ETMViewStateSaver::KeyFormat ETMViewStateSaver::keyFormat() const
{
    return mKeyFormat;
}

QModelIndex ETMViewStateSaver::indexFromConfigString(const QAbstractItemModel *model, const QString &key) const
{
    if (key.startsWith(u'r')) {
        return EntityTreeModel::modelIndexForStableKey(model, key);
    }

    if (key.startsWith(u'x')) {
        return QModelIndex();
    }

    Item::Id id = key.mid(1).toLongLong();
    if (id < 0) {
        return QModelIndex();
    }

    if (key.startsWith(u'c')) {
        const QModelIndex idx = EntityTreeModel::modelIndexForCollection(model, Collection(id));
        if (!idx.isValid()) {
            return QModelIndex();
        }
        return idx;
    } else if (key.startsWith(u'i')) {
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
    const auto c = index.data(EntityTreeModel::CollectionRole).value<Collection>();
    if (c.isValid()) {
        if (mKeyFormat == RemotePathKeys) {
            const QString key = EntityTreeModel::stableKeyForCollectionIndex(index);
            if (!key.isEmpty()) {
                return key;
            }
            // No usable remote path (e.g. a search/virtual collection): fall back to the id key.
        }
        return QStringLiteral("c%1").arg(c.id());
    }
    auto id = index.data(EntityTreeModel::ItemIdRole).value<Item::Id>();
    if (id >= 0) {
        return QStringLiteral("i%1").arg(id);
    }
    return QString();
}

void ETMViewStateSaver::saveState(KConfigGroup &configGroup)
{
    // Same key KConfigViewStateSaver stores the selection under.
    static const char selectionKey[] = "Selection";

    // Only path keys benefit from (and are safe for) keeping missing collections, so id-based
    // users get the base behavior unchanged.
    if (mKeyFormat != RemotePathKeys || !selectionModel()) {
        KConfigViewStateSaver::saveState(configGroup);
        return;
    }

    const QStringList previous = configGroup.readEntry(selectionKey, QStringList());
    KConfigViewStateSaver::saveState(configGroup);
    if (previous.isEmpty()) {
        return;
    }

    // Keep stable path keys whose collections are currently missing from the model: restoring is
    // asynchronous, so saving on shutdown before the model is populated would otherwise drop them.
    const QAbstractItemModel *model = selectionModel()->model();
    QStringList keys = configGroup.readEntry(selectionKey, QStringList());
    for (const QString &key : previous) {
        if (!key.startsWith(u'r')) {
            continue;
        }
        if (keys.contains(key)) {
            continue;
        }
        if (indexFromConfigString(model, key).isValid()) {
            continue; // the collection is there, so the user really unchecked it
        }
        keys.append(key);
    }
    configGroup.writeEntry(selectionKey, keys);
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

void ETMViewStateSaver::selectCollections(const QList<Collection::Id> &list)
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

void ETMViewStateSaver::selectItems(const QList<Item::Id> &list)
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

#include "moc_etmviewstatesaver.cpp"
