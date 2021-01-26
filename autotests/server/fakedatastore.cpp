/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakedatastore.h"
#include "akonadischema.h"
#include "dbpopulator.h"
#include "fakeakonadiserver.h"
#include "inspectablenotificationcollector.h"
#include "storage/dbconfig.h"
#include "storage/dbinitializer.h"

#include <QThread>

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

FakeDataStoreFactory::FakeDataStoreFactory(FakeAkonadiServer &akonadi)
    : m_akonadi(akonadi)
{
}

DataStore *FakeDataStoreFactory::createStore()
{
    return new FakeDataStore(m_akonadi);
}

FakeDataStore::FakeDataStore(FakeAkonadiServer &akonadi)
    : DataStore(akonadi)
    , mPopulateDb(true)
{
    mNotificationCollector = std::make_unique<InspectableNotificationCollector>(m_akonadi, this);
}

FakeDataStore::~FakeDataStore() = default;

bool FakeDataStore::init()
{
    if (!DataStore::init()) {
        return false;
    }

    if (mPopulateDb) {
        DbPopulator dbPopulator;
        if (!dbPopulator.run()) {
            qWarning() << "Failed to populate database";
            return false;
        }
    }

    return true;
}

bool FakeDataStore::setItemsFlags(const PimItem::List &items,
                                  const QVector<Flag> *currentFlags,
                                  const QVector<Flag> &flags,
                                  bool *flagsChanged,
                                  const Collection &col,
                                  bool silent)
{
    mChanges.insert(QStringLiteral("setItemsFlags"),
                    QVariantList() << QVariant::fromValue(items) << QVariant::fromValue(flags) << QVariant::fromValue(col) << silent);
    return DataStore::setItemsFlags(items, currentFlags, flags, flagsChanged, col, silent);
}

bool FakeDataStore::appendItemsFlags(const PimItem::List &items,
                                     const QVector<Flag> &flags,
                                     bool *flagsChanged,
                                     bool checkIfExists,
                                     const Collection &col,
                                     bool silent)
{
    mChanges.insert(QStringLiteral("appendItemsFlags"),
                    QVariantList() << QVariant::fromValue(items) << QVariant::fromValue(flags) << checkIfExists << QVariant::fromValue(col) << silent);
    return DataStore::appendItemsFlags(items, flags, flagsChanged, checkIfExists, col, silent);
}

bool FakeDataStore::removeItemsFlags(const PimItem::List &items, const QVector<Flag> &flags, bool *flagsChanged, const Collection &col, bool silent)
{
    mChanges.insert(QStringLiteral("removeItemsFlags"),
                    QVariantList() << QVariant::fromValue(items) << QVariant::fromValue(flags) << QVariant::fromValue(col) << silent);
    return DataStore::removeItemsFlags(items, flags, flagsChanged, col, silent);
}

bool FakeDataStore::setItemsTags(const PimItem::List &items, const Tag::List &tags, bool *tagsChanged, bool silent)
{
    mChanges.insert(QStringLiteral("setItemsTags"), QVariantList() << QVariant::fromValue(items) << QVariant::fromValue(tags) << silent);
    return DataStore::setItemsTags(items, tags, tagsChanged, silent);
}

bool FakeDataStore::appendItemsTags(const PimItem::List &items,
                                    const Tag::List &tags,
                                    bool *tagsChanged,
                                    bool checkIfExists,
                                    const Collection &col,
                                    bool silent)
{
    mChanges.insert(QStringLiteral("appendItemsTags"),
                    QVariantList() << QVariant::fromValue(items) << QVariant::fromValue(tags) << checkIfExists << QVariant::fromValue(col) << silent);
    return DataStore::appendItemsTags(items, tags, tagsChanged, checkIfExists, col, silent);
}

bool FakeDataStore::removeItemsTags(const PimItem::List &items, const Tag::List &tags, bool *tagsChanged, bool silent)
{
    mChanges.insert(QStringLiteral("removeItemsTags"), QVariantList() << QVariant::fromValue(items) << QVariant::fromValue(tags) << silent);
    return DataStore::removeItemsTags(items, tags, tagsChanged, silent);
}

bool FakeDataStore::removeItemParts(const PimItem &item, const QSet<QByteArray> &parts)
{
    mChanges.insert(QStringLiteral("remoteItemParts"), QVariantList() << QVariant::fromValue(item) << QVariant::fromValue(parts));
    return DataStore::removeItemParts(item, parts);
}

bool FakeDataStore::invalidateItemCache(const PimItem &item)
{
    mChanges.insert(QStringLiteral("invalidateItemCache"), QVariantList() << QVariant::fromValue(item));
    return DataStore::invalidateItemCache(item);
}

bool FakeDataStore::appendCollection(Collection &collection, const QStringList &mimeTypes, const QMap<QByteArray, QByteArray> &attributes)
{
    mChanges.insert(QStringLiteral("appendCollection"), QVariantList() << QVariant::fromValue(collection) << mimeTypes << QVariant::fromValue(attributes));
    return DataStore::appendCollection(collection, mimeTypes, attributes);
}

bool FakeDataStore::cleanupCollection(Collection &collection)
{
    mChanges.insert(QStringLiteral("cleanupCollection"), QVariantList() << QVariant::fromValue(collection));
    return DataStore::cleanupCollection(collection);
}

bool FakeDataStore::cleanupCollection_slow(Collection &collection)
{
    mChanges.insert(QStringLiteral("cleanupCollection_slow"), QVariantList() << QVariant::fromValue(collection));
    return DataStore::cleanupCollection_slow(collection);
}

bool FakeDataStore::moveCollection(Collection &collection, const Collection &newParent)
{
    mChanges.insert(QStringLiteral("moveCollection"), QVariantList() << QVariant::fromValue(collection) << QVariant::fromValue(newParent));
    return DataStore::moveCollection(collection, newParent);
}

bool FakeDataStore::appendMimeTypeForCollection(qint64 collectionId, const QStringList &mimeTypes)
{
    mChanges.insert(QStringLiteral("appendMimeTypeForCollection"), QVariantList() << collectionId << QVariant::fromValue(mimeTypes));
    return DataStore::appendMimeTypeForCollection(collectionId, mimeTypes);
}

void FakeDataStore::activeCachePolicy(Collection &col)
{
    mChanges.insert(QStringLiteral("activeCachePolicy"), QVariantList() << QVariant::fromValue(col));
    return DataStore::activeCachePolicy(col);
}

bool FakeDataStore::appendPimItem(QVector<Part> &parts,
                                  const QVector<Flag> &flags,
                                  const MimeType &mimetype,
                                  const Collection &collection,
                                  const QDateTime &dateTime,
                                  const QString &remote_id,
                                  const QString &remoteRevision,
                                  const QString &gid,
                                  PimItem &pimItem)
{
    mChanges.insert(QStringLiteral("appendPimItem"),
                    QVariantList() << QVariant::fromValue(mimetype) << QVariant::fromValue(collection) << dateTime << remote_id << remoteRevision << gid);
    return DataStore::appendPimItem(parts, flags, mimetype, collection, dateTime, remote_id, remoteRevision, gid, pimItem);
}

bool FakeDataStore::cleanupPimItems(const PimItem::List &items, bool silent)
{
    mChanges.insert(QStringLiteral("cleanupPimItems"), QVariantList() << QVariant::fromValue(items) << silent);
    return DataStore::cleanupPimItems(items, silent);
}

bool FakeDataStore::unhidePimItem(PimItem &pimItem)
{
    mChanges.insert(QStringLiteral("unhidePimItem"), QVariantList() << QVariant::fromValue(pimItem));
    return DataStore::unhidePimItem(pimItem);
}

bool FakeDataStore::unhideAllPimItems()
{
    mChanges.insert(QStringLiteral("unhideAllPimItems"), QVariantList());
    return DataStore::unhideAllPimItems();
}

bool FakeDataStore::addCollectionAttribute(const Collection &col, const QByteArray &key, const QByteArray &value, bool silent)
{
    mChanges.insert(QStringLiteral("addCollectionAttribute"), QVariantList() << QVariant::fromValue(col) << key << value << silent);
    return DataStore::addCollectionAttribute(col, key, value, silent);
}

bool FakeDataStore::removeCollectionAttribute(const Collection &col, const QByteArray &key)
{
    mChanges.insert(QStringLiteral("removeCollectionAttribute"), QVariantList() << QVariant::fromValue(col) << key);
    return DataStore::removeCollectionAttribute(col, key);
}

bool FakeDataStore::beginTransaction(const QString &name)
{
    mChanges.insert(QStringLiteral("beginTransaction"), QVariantList() << name);
    return DataStore::beginTransaction(name);
}

bool FakeDataStore::commitTransaction()
{
    mChanges.insert(QStringLiteral("commitTransaction"), QVariantList());
    return DataStore::commitTransaction();
}

bool FakeDataStore::rollbackTransaction()
{
    mChanges.insert(QStringLiteral("rollbackTransaction"), QVariantList());
    return DataStore::rollbackTransaction();
}

void FakeDataStore::setPopulateDb(bool populate)
{
    mPopulateDb = populate;
}
