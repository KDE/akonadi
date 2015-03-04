/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemmodel.h"

#include "itemfetchjob.h"
#include "collectionfetchjob.h"
#include "itemfetchscope.h"
#include "monitor.h"
#include "pastehelper_p.h"
#include "session.h"

#include <klocalizedstring.h>
#include <QUrl>

#include <QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>

using namespace Akonadi;

/**
 * @internal
 *
 * This struct is used for optimization reasons.
 * because it embeds the row.
 *
 * Semantically, we could have used an item instead.
 */
struct ItemContainer
{
    ItemContainer(const Item &i, int r)
        : item(i)
        , row(r)
    {
    }
    Item item;
    int row;
};

/**
 * @internal
 */
class ItemModel::Private
{
public:
    Private(ItemModel *parent)
        : mParent(parent)
        , monitor(new Monitor())
    {
        session = new Session(QCoreApplication::instance()->applicationName().toUtf8()
                              + QByteArray("-ItemModel-") + QByteArray::number(qrand()), mParent);

        monitor->ignoreSession(session);

        mParent->connect(monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
                         mParent, SLOT(itemChanged(Akonadi::Item,QSet<QByteArray>)));
        mParent->connect(monitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
                         mParent, SLOT(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)));
        mParent->connect(monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
                         mParent, SLOT(itemAdded(Akonadi::Item)));
        mParent->connect(monitor, SIGNAL(itemRemoved(Akonadi::Item)),
                         mParent, SLOT(itemRemoved(Akonadi::Item)));
        mParent->connect(monitor, SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)),
                         mParent, SLOT(itemAdded(Akonadi::Item)));
        mParent->connect(monitor, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)),
                         mParent, SLOT(itemRemoved(Akonadi::Item)));
    }

    ~Private()
    {
        delete monitor;
    }

    void listingDone(KJob *job);
    void collectionFetchResult(KJob *job);
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &);
    void itemsAdded(const Akonadi::Item::List &list);
    void itemAdded(const Akonadi::Item &item);
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &src, const Akonadi::Collection &dst);
    void itemRemoved(const Akonadi::Item &item);
    int rowForItem(const Akonadi::Item &item);
    bool collectionIsCompatible() const;

    ItemModel *mParent;

    QList<ItemContainer *> items;
    QHash<Item, ItemContainer *> itemHash;

    Collection collection;
    Monitor *monitor;
    Session *session;
};

bool ItemModel::Private::collectionIsCompatible() const
{
    // in the generic case, we show any collection
    if (mParent->mimeTypes() == QStringList(QStringLiteral("text/uri-list"))) {
        return true;
    }
    // if the model's mime types are more specific, limit to those
    // collections that have matching types
    Q_FOREACH (const QString &type, mParent->mimeTypes()) {
        if (collection.contentMimeTypes().contains(type)) {
            return true;
        }
    }
    return false;
}

void ItemModel::Private::listingDone(KJob *job)
{
    ItemFetchJob *fetch = static_cast<ItemFetchJob *>(job);
    Q_UNUSED(fetch);
    if (job->error()) {
        // TODO
        qWarning() << "Item query failed:" << job->errorString();
    }
}

void ItemModel::Private::collectionFetchResult(KJob *job)
{
    CollectionFetchJob *fetch = static_cast<CollectionFetchJob *>(job);

    if (fetch->collections().isEmpty()) {
        return;
    }

    Q_ASSERT(fetch->collections().count() == 1);   // we only listed base
    Collection c = fetch->collections().first();
    // avoid recursion, if this fails for some reason
    if (!c.contentMimeTypes().isEmpty()) {
        mParent->setCollection(c);
    } else {
        qWarning() << "Failed to retrieve the contents mime type of the collection: " << c;
        mParent->setCollection(Collection());
    }
}

int ItemModel::Private::rowForItem(const Akonadi::Item &item)
{
    ItemContainer *container = itemHash.value(item);
    if (!container) {
        return -1;
    }

    /* Try to find the item directly;

       If items have been removed, this first try won't succeed because
       the ItemContainer rows have not been updated (costs too much).
    */
    if (container->row < items.count()
        && items.at(container->row) == container) {
        return container->row;
    } else {
        // Slow solution if the fist one has not succeeded
        int row = -1;
        const int numberOfItems(items.size());
        for (int i = 0; i < numberOfItems; ++i) {
            if (items.at(i)->item == item) {
                row = i;
                break;
            }
        }
        return row;
    }

}

void ItemModel::Private::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    int row = rowForItem(item);
    if (row < 0) {
        return;
    }

    items[row]->item = item;
    itemHash.remove(item);
    itemHash[item] = items[row];

    QModelIndex start = mParent->index(row, 0, QModelIndex());
    QModelIndex end = mParent->index(row, mParent->columnCount(QModelIndex()) - 1, QModelIndex());

    mParent->dataChanged(start, end);
}

void ItemModel::Private::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &colSrc, const Akonadi::Collection &colDst)
{
    if (colSrc == collection && colDst != collection) {
        // item leaving this model
        itemRemoved(item);
        return;
    }

    if (colDst == collection && colSrc != collection) {
        itemAdded(item);
        return;
    }
}

void ItemModel::Private::itemsAdded(const Akonadi::Item::List &list)
{
    if (list.isEmpty()) {
        return;
    }
    mParent->beginInsertRows(QModelIndex(), items.count(), items.count() + list.count() - 1);
    foreach (const Item &item, list) {
        ItemContainer *c = new ItemContainer(item, items.count());
        items.append(c);
        itemHash[item] = c;
    }
    mParent->endInsertRows();
}

void ItemModel::Private::itemAdded(const Akonadi::Item &item)
{
    Item::List l;
    l << item;
    itemsAdded(l);
}

void ItemModel::Private::itemRemoved(const Akonadi::Item &_item)
{
    int row = rowForItem(_item);
    if (row < 0) {
        return;
    }

    mParent->beginRemoveRows(QModelIndex(), row, row);
    const Item item = items.at(row)->item;
    Q_ASSERT(item.isValid());
    itemHash.remove(item);
    delete items.takeAt(row);
    mParent->endRemoveRows();
}

ItemModel::ItemModel(QObject *parent)
    : QAbstractTableModel(parent)
    , d(new Private(this))
{
}

ItemModel::~ItemModel()
{
    delete d;
}

QVariant ItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.row() >= d->items.count()) {
        return QVariant();
    }
    const Item item = d->items.at(index.row())->item;
    if (!item.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Id:
            return QString::number(item.id());
        case RemoteId:
            return item.remoteId();
        case MimeType:
            return item.mimeType();
        default:
            return QVariant();
        }
    }

    if (role == IdRole) {
        return item.id();
    }

    if (role == ItemRole) {
        QVariant var;
        var.setValue(item);
        return var;
    }

    if (role == MimeTypeRole) {
        return item.mimeType();
    }

    return QVariant();
}

int ItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return d->items.count();
    }
    return 0;
}

int ItemModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 3; // keep in sync with Column enum
    }
    return 0;
}

QVariant ItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Id:
            return i18n("Id");
        case RemoteId:
            return i18n("Remote Id");
        case MimeType:
            return i18n("MimeType");
        default:
            return QString();
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void ItemModel::setCollection(const Collection &collection)
{
    qDebug();
    if (d->collection == collection) {
        return;
    }

    // if we don't know anything about this collection yet, fetch it
    if (collection.isValid() && collection.contentMimeTypes().isEmpty()) {
        CollectionFetchJob *job = new CollectionFetchJob(collection, CollectionFetchJob::Base, this);
        connect(job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchResult(KJob*)));
        return;
    }

    d->monitor->setCollectionMonitored(d->collection, false);

    d->collection = collection;

    d->monitor->setCollectionMonitored(d->collection, true);

    // the query changed, thus everything we have already is invalid
    qDeleteAll(d->items);
    d->items.clear();
    reset();

    // stop all running jobs
    d->session->clear();

    // start listing job
    if (d->collectionIsCompatible()) {
        ItemFetchJob *job = new ItemFetchJob(collection, session());
        job->setFetchScope(d->monitor->itemFetchScope());
        connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)),
                SLOT(itemsAdded(Akonadi::Item::List)));
        connect(job, SIGNAL(result(KJob*)), SLOT(listingDone(KJob*)));
    }

    emit collectionChanged(collection);
}

void ItemModel::setFetchScope(const ItemFetchScope &fetchScope)
{
    d->monitor->setItemFetchScope(fetchScope);
}

ItemFetchScope &ItemModel::fetchScope()
{
    return d->monitor->itemFetchScope();
}

Item ItemModel::itemForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Akonadi::Item();
    }

    if (index.row() >= d->items.count()) {
        return Akonadi::Item();
    }

    Item item = d->items.at(index.row())->item;
    if (item.isValid()) {
        return item;
    } else {
        return Akonadi::Item();
    }
}

Qt::ItemFlags ItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    if (index.isValid()) {
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    } else {
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
}

QStringList ItemModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("text/uri-list");
}

Session *ItemModel::session() const
{
    return d->session;
}

QMimeData *ItemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = new QMimeData();
    // Add item uri to the mimedata for dropping in external applications
    QList<QUrl> urls;
    foreach (const QModelIndex &index, indexes) {
        if (index.column() != 0) {
            continue;
        }

        urls << itemForIndex(index).url(Item::UrlWithMimeType);
    }
    data->setUrls(urls);

    return data;
}

QModelIndex ItemModel::indexForItem(const Akonadi::Item &item, const int column) const
{
    return index(d->rowForItem(item), column);
}

bool ItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    KJob *job = PasteHelper::paste(data, d->collection, action != Qt::MoveAction);
    // TODO: error handling
    return job;
}

Collection ItemModel::collection() const
{
    return d->collection;
}

Qt::DropActions ItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

#include "moc_itemmodel.cpp"
