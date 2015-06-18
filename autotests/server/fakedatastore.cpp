/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "fakedatastore.h"
#include "dbpopulator.h"
#include "storage/dbconfig.h"
#include "storage/notificationcollector.h"
#include "akonadischema.h"
#include "storage/dbinitializer.h"

#include <QUuid>
#include <QThread>
#include <QFile>

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(PimItem)
Q_DECLARE_METATYPE(PimItem::List)
Q_DECLARE_METATYPE(Collection)
Q_DECLARE_METATYPE(Flag)
Q_DECLARE_METATYPE(QVector<Flag>)
Q_DECLARE_METATYPE(Tag)
Q_DECLARE_METATYPE(QVector<Tag>)
Q_DECLARE_METATYPE(MimeType)
Q_DECLARE_METATYPE(QList<QByteArray>)

Akonadi::Server::FakeDataStore::FakeDataStore()
    : DataStore()
    , mPopulateDb(true)
{
    notificationCollector();
}

FakeDataStore::~FakeDataStore()
{
}

NotificationCollector *FakeDataStore::notificationCollector()
{
    if (!mNotificationCollector) {
        mNotificationCollector = new NotificationCollector(this);
    }
    return mNotificationCollector;
}

DataStore *FakeDataStore::self()
{
    if (!sInstances.hasLocalData()) {
        sInstances.setLocalData(new FakeDataStore());
    }

    return sInstances.localData();
}

bool FakeDataStore::init()
{
    if (!DataStore::init()) {
        return false;
    }

    if (mPopulateDb) {
        DbPopulator dbPopulator;
        if (!dbPopulator.run()) {
            akError() << "Failed to populate database";
            return false;
        }
    }

    return true;
}

bool FakeDataStore::setItemsFlags(const PimItem::List &items,
                                  const QVector<Flag> &flags,
                                  bool *flagsChanged,
                                  const Collection &col,
                                  bool silent)
{
    mChanges.insert(QLatin1String("setItemsFlags"),
                    QVariantList() << QVariant::fromValue(items)
                    << QVariant::fromValue(flags)
                    << QVariant::fromValue(col)
                    << silent);
    return DataStore::setItemsFlags(items, flags, flagsChanged, col, silent);
}

bool FakeDataStore::appendItemsFlags(const PimItem::List &items,
                                     const QVector<Flag> &flags,
                                     bool *flagsChanged,
                                     bool checkIfExists,
                                     const Collection &col,
                                     bool silent)
{
    mChanges.insert(QLatin1String("appendItemsFlags"),
                    QVariantList() << QVariant::fromValue(items)
                    << QVariant::fromValue(flags)
                    << checkIfExists
                    << QVariant::fromValue(col)
                    << silent);
    return DataStore::appendItemsFlags(items, flags, flagsChanged, checkIfExists, col, silent);
}

bool FakeDataStore::removeItemsFlags(const PimItem::List &items,
                                     const QVector<Flag> &flags,
                                     bool *flagsChanged,
                                     const Collection &col,
                                     bool silent)
{
    mChanges.insert(QLatin1String("removeItemsFlags"),
                    QVariantList() << QVariant::fromValue(items)
                    << QVariant::fromValue(flags)
                    << QVariant::fromValue(col)
                    << silent);
    return DataStore::removeItemsFlags(items, flags, flagsChanged, col, silent);
}

bool FakeDataStore::setItemsTags(const PimItem::List &items,
                                 const Tag::List &tags,
                                 bool *tagsChanged,
                                 bool silent)
{
    mChanges.insert(QLatin1String("setItemsTags"),
                    QVariantList() << QVariant::fromValue(items)
                    << QVariant::fromValue(tags)
                    << silent);
    return DataStore::setItemsTags(items, tags, tagsChanged, silent);
}

bool FakeDataStore::appendItemsTags(const PimItem::List &items,
                                    const Tag::List &tags,
                                    bool *tagsChanged,
                                    bool checkIfExists,
                                    const Collection &col,
                                    bool silent)
{
    mChanges.insert(QLatin1String("appendItemsTags"),
                    QVariantList() << QVariant::fromValue(items)
                    << QVariant::fromValue(tags)
                    << checkIfExists
                    << QVariant::fromValue(col)
                    << silent);
    return DataStore::appendItemsTags(items, tags, tagsChanged,
                                      checkIfExists, col, silent);
}

bool FakeDataStore::removeItemsTags(const PimItem::List &items,
                                    const Tag::List &tags,
                                    bool *tagsChanged,
                                    bool silent)
{
    mChanges.insert(QLatin1String("removeItemsTags"),
                    QVariantList() << QVariant::fromValue(items)
                    << QVariant::fromValue(tags)
                    << silent);
    return DataStore::removeItemsTags(items, tags, tagsChanged, silent);
}

bool FakeDataStore::removeItemParts(const PimItem &item,
                                    const QSet<QByteArray> &parts)
{
    mChanges.insert(QLatin1String("remoteItemParts"),
                    QVariantList() << QVariant::fromValue(item)
                    << QVariant::fromValue(parts));
    return DataStore::removeItemParts(item, parts);
}

bool FakeDataStore::invalidateItemCache(const PimItem &item)
{
    mChanges.insert(QLatin1String("invalidateItemCache"),
                    QVariantList() << QVariant::fromValue(item));
    return DataStore::invalidateItemCache(item);
}

bool FakeDataStore::appendCollection(Collection &collection)
{
    mChanges.insert(QLatin1String("appendCollection"),
                    QVariantList() << QVariant::fromValue(collection));
    return DataStore::appendCollection(collection);
}

bool FakeDataStore::cleanupCollection(Collection &collection)
{
    mChanges.insert(QLatin1String("cleanupCollection"),
                    QVariantList() << QVariant::fromValue(collection));
    return DataStore::cleanupCollection(collection);
}

bool FakeDataStore::cleanupCollection_slow(Collection &collection)
{
    mChanges.insert(QLatin1String("cleanupCollection_slow"),
                    QVariantList() << QVariant::fromValue(collection));
    return DataStore::cleanupCollection_slow(collection);
}

bool FakeDataStore::moveCollection(Collection &collection,
                                   const Collection &newParent)
{
    mChanges.insert(QLatin1String("moveCollection"),
                    QVariantList() << QVariant::fromValue(collection)
                    << QVariant::fromValue(newParent));
    return DataStore::moveCollection(collection, newParent);
}

bool FakeDataStore::appendMimeTypeForCollection(qint64 collectionId,
                                                const QStringList &mimeTypes)
{
    mChanges.insert(QLatin1String("appendMimeTypeForCollection"),
                    QVariantList() << collectionId
                    << QVariant::fromValue(mimeTypes));
    return DataStore::appendMimeTypeForCollection(collectionId, mimeTypes);
}

void FakeDataStore::activeCachePolicy(Collection &col)
{
    mChanges.insert(QLatin1String("activeCachePolicy"),
                    QVariantList() << QVariant::fromValue(col));
    return DataStore::activeCachePolicy(col);
}

bool FakeDataStore::appendMimeType(const QString &mimetype,
                                   qint64 *insertId)
{
    mChanges.insert(QLatin1String("appendMimeType"),
                    QVariantList() << mimetype);
    return DataStore::appendMimeType(mimetype, insertId);
}

bool FakeDataStore::appendPimItem(QVector<Part> &parts,
                                  const MimeType &mimetype,
                                  const Collection &collection,
                                  const QDateTime &dateTime,
                                  const QString &remote_id,
                                  const QString &remoteRevision,
                                  const QString &gid,
                                  PimItem &pimItem)
{
    mChanges.insert(QLatin1String("appendPimItem"),
                    QVariantList() << QVariant::fromValue(mimetype)
                    << QVariant::fromValue(collection)
                    << dateTime
                    << remote_id
                    << remoteRevision
                    << gid);
    return DataStore::appendPimItem(parts, mimetype, collection, dateTime,
                                    remote_id, remoteRevision, gid, pimItem);
}

bool FakeDataStore::cleanupPimItems(const PimItem::List &items)
{
    mChanges.insert(QLatin1String("cleanupPimItems"),
                    QVariantList() << QVariant::fromValue(items));
    return DataStore::cleanupPimItems(items);
}

bool FakeDataStore::unhidePimItem(PimItem &pimItem)
{
    mChanges.insert(QLatin1String("unhidePimItem"),
                    QVariantList() << QVariant::fromValue(pimItem));
    return DataStore::unhidePimItem(pimItem);
}

bool FakeDataStore::unhideAllPimItems()
{
    mChanges.insert(QLatin1String("unhideAllPimItems"), QVariantList());
    return DataStore::unhideAllPimItems();
}

bool FakeDataStore::addCollectionAttribute(const Collection &col,
                                           const QByteArray &key,
                                           const QByteArray &value)
{
    mChanges.insert(QLatin1String("addCollectionAttribute"),
                    QVariantList() << QVariant::fromValue(col)
                    << key << value);
    return DataStore::addCollectionAttribute(col, key, value);
}

bool FakeDataStore::removeCollectionAttribute(const Collection &col,
                                              const QByteArray &key)
{
    mChanges.insert(QLatin1String("removeCollectionAttribute"),
                    QVariantList() << QVariant::fromValue(col) << key);
    return DataStore::removeCollectionAttribute(col, key);
}

bool FakeDataStore::beginTransaction()
{
    mChanges.insert(QLatin1String("beginTransaction"), QVariantList());
    return DataStore::beginTransaction();
}

bool FakeDataStore::commitTransaction()
{
    mChanges.insert(QLatin1String("commitTransaction"), QVariantList());
    return DataStore::commitTransaction();
}

bool FakeDataStore::rollbackTransaction()
{
    mChanges.insert(QLatin1String("rollbackTransaction"), QVariantList());
    return DataStore::rollbackTransaction();
}

void FakeDataStore::setPopulateDb(bool populate)
{
    mPopulateDb = populate;
}
