/*
    Copyright (c) 2018 - 2019 Daniel Vrátil <dvratil@kde.org>

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

#include "interface.h"
#include "storage.h"

#include "collectionfetchscope.h"
#include "itemfetchscope.h"
#include "tagfetchscope.h"

#include <QMutex>
#include <QMutexLocker>
#include <QSharedData>

namespace Akonadi
{

class ItemFetchOptionsPrivate : public QSharedData
{
public:
    ItemFetchOptionsPrivate() = default;
    ItemFetchOptionsPrivate(const ItemFetchOptionsPrivate &other) = default;

    ItemFetchScope itemFetchScope;
    Collection collection;
};

class ItemSearchOptionsPrivate : public QSharedData
{
public:
    ItemSearchOptionsPrivate() = default;
    ItemSearchOptionsPrivate(const ItemSearchOptionsPrivate &) = default;

    ItemFetchScope itemFetchScope;
    TagFetchScope tagFetchScope;
    Collection::List collections;
    QStringList mimeTypes;
    bool recursive = false;
    bool remoteSearch = false;
};

class CollectionFetchOptionsPrivate : public QSharedData
{
public:
    CollectionFetchOptionsPrivate() = default;
    CollectionFetchOptionsPrivate(const CollectionFetchOptionsPrivate &) = default;

    CollectionFetchScope fetchScope;
};

class TagFetchOptionsPrivate : public QSharedData
{
public:
    TagFetchOptionsPrivate() = default;
    TagFetchOptionsPrivate(const TagFetchOptionsPrivate &) = default;

    TagFetchScope fetchScope;
};

ItemFetchOptions::ItemFetchOptions()
    : d(new ItemFetchOptionsPrivate)
{
}

ItemFetchOptions::ItemFetchOptions(const Collection &collection)
    : ItemFetchOptions()
{
    d->collection = collection;
}

ItemFetchOptions::ItemFetchOptions(const ItemFetchScope &fetchScope)
    : ItemFetchOptions()
{
    d->itemFetchScope = fetchScope;
}

ItemFetchOptions::ItemFetchOptions(const ItemFetchScope &fetchScope, const Collection &collection)
    : ItemFetchOptions()
{
    d->itemFetchScope = fetchScope;
    d->collection = collection;
}

ItemFetchOptions::ItemFetchOptions(const ItemFetchOptions &) = default;
ItemFetchOptions &ItemFetchOptions::operator=(const ItemFetchOptions &) = default;
ItemFetchOptions::ItemFetchOptions(ItemFetchOptions &&) noexcept = default;
ItemFetchOptions &ItemFetchOptions::operator=(ItemFetchOptions &&) noexcept = default;
ItemFetchOptions::~ItemFetchOptions() = default;

ItemFetchScope &ItemFetchOptions::itemFetchScope()
{
    return d->itemFetchScope;
}

const ItemFetchScope &ItemFetchOptions::itemFetchScope() const
{
    return d->itemFetchScope;
}

void ItemFetchOptions::setItemFetchScope(const ItemFetchScope &itemFetchScope)
{
    d->itemFetchScope = itemFetchScope;
}

Collection ItemFetchOptions::collection() const
{
    return d->collection;
}

void ItemFetchOptions::setCollection(const Collection &collection)
{
    d->collection = collection;
}

ItemSearchOptions::ItemSearchOptions()
    : d(new ItemSearchOptionsPrivate)
{
}

ItemSearchOptions::ItemSearchOptions(const ItemSearchOptions &) = default;
ItemSearchOptions &ItemSearchOptions::operator=(const ItemSearchOptions &) = default;
ItemSearchOptions::ItemSearchOptions(ItemSearchOptions &&) noexcept = default;
ItemSearchOptions &ItemSearchOptions::operator=(ItemSearchOptions &&) noexcept = default;
ItemSearchOptions::~ItemSearchOptions() = default;

ItemFetchScope &ItemSearchOptions::itemFetchScope()
{
    return d->itemFetchScope;
}

const ItemFetchScope &ItemSearchOptions::itemFetchScope() const
{
    return d->itemFetchScope;
}

void ItemSearchOptions::setItemFetchScope(const ItemFetchScope &fetchScope)
{
    d->itemFetchScope = fetchScope;
}

TagFetchScope &ItemSearchOptions::tagFetchScope()
{
    return d->tagFetchScope;
}

const TagFetchScope &ItemSearchOptions::tagFetchScope() const
{
    return d->tagFetchScope;
}

void ItemSearchOptions::setTagFetchScope(const TagFetchScope &fetchScope)
{
    d->tagFetchScope = fetchScope;
}

Collection::List ItemSearchOptions::searchCollections() const
{
    return d->collections;
}

void ItemSearchOptions::setSearchCollections(const Collection::List &collections)
{
    d->collections = collections;
}

QStringList ItemSearchOptions::mimeTypes() const
{
    return d->mimeTypes;
}

void ItemSearchOptions::setMimeTypes(const QStringList &mimeTypes)
{
    d->mimeTypes = mimeTypes;
}

bool ItemSearchOptions::isRecursive() const
{
    return d->recursive;
}

void ItemSearchOptions::setRecursive(bool recursive)
{
    d->recursive = recursive;
}

bool ItemSearchOptions::isRemoteSearchEnabled() const
{
    return d->remoteSearch;
}

void ItemSearchOptions::setRemoteSearchEnabled(bool enabled)
{
    d->remoteSearch = enabled;
}

CollectionFetchOptions::CollectionFetchOptions()
    : d(new CollectionFetchOptionsPrivate)
{
}

CollectionFetchOptions::CollectionFetchOptions(const CollectionFetchScope &scope)
    : CollectionFetchOptions()
{
    d->fetchScope = scope;
}

CollectionFetchOptions::CollectionFetchOptions(const CollectionFetchOptions &) = default;
CollectionFetchOptions &CollectionFetchOptions::operator=(const CollectionFetchOptions &) = default;
CollectionFetchOptions::CollectionFetchOptions(CollectionFetchOptions &&) noexcept = default;
CollectionFetchOptions &CollectionFetchOptions::operator=(CollectionFetchOptions &&) noexcept = default;
CollectionFetchOptions::~CollectionFetchOptions() = default;

CollectionFetchScope &CollectionFetchOptions::fetchScope()
{
    return d->fetchScope;
}

const CollectionFetchScope &CollectionFetchOptions::fetchScope() const
{
    return d->fetchScope;
}

void CollectionFetchOptions::setFetchScope(const CollectionFetchScope &fetchScope)
{
    d->fetchScope = fetchScope;
}

TagFetchOptions::TagFetchOptions()
    : d(new TagFetchOptionsPrivate)
{
}

TagFetchOptions::TagFetchOptions(const TagFetchScope &scope)
    : TagFetchOptions()
{
    d->fetchScope = scope;
}

TagFetchOptions::TagFetchOptions(const TagFetchOptions &) = default;
TagFetchOptions &TagFetchOptions::operator=(const TagFetchOptions &) = default;
TagFetchOptions::TagFetchOptions(TagFetchOptions &&) noexcept = default;
TagFetchOptions &TagFetchOptions::operator=(TagFetchOptions &&) noexcept = default;
TagFetchOptions::~TagFetchOptions() = default;

TagFetchScope &TagFetchOptions::fetchScope()
{
    return d->fetchScope;
}

const TagFetchScope &TagFetchOptions::fetchScope() const
{
    return d->fetchScope;
}

void TagFetchOptions::setFetchScope(const TagFetchScope &fetchScope)
{
    d->fetchScope = fetchScope;
}

namespace
{

QSharedPointer<Akonadi::StorageInterface> sInterface;
QMutex sInterfaceLock;

}

QSharedPointer<StorageInterface> setStorage(const QSharedPointer<StorageInterface> &storage)
{
    QMutexLocker locker(&sInterfaceLock);
    return std::exchange(sInterface, storage);
}

QSharedPointer<StorageInterface> storage()
{
    QMutexLocker locker(&sInterfaceLock);
    if (!sInterface) {
        sInterface = QSharedPointer<Storage>::create();
    }
    return sInterface;
}

Session *createSession(const QByteArray &name)
{
    return storage()->createSession(name);
}

Session *defaultSession()
{
    return storage()->defaultSession();
}

Task<Item::List> fetchItemsFromCollection(const Collection &collection, const ItemFetchOptions &options, Session *session)
{
    return storage()->fetchItemsFromCollection(collection, options, session);
}

Task<Item::List> fetchItems(const Item::List &items, const ItemFetchOptions &options, Session *session)
{
    return storage()->fetchItems(items, options, session);
}

Task<Item::List> fetchItemsForTag(const Tag &tag, const ItemFetchOptions &options, Session *session)
{
    return storage()->fetchItemsForTag(tag, options, session);
}

Task<Item> fetchItem(const Item &item, const ItemFetchOptions &options, Session *session)
{
    return storage()->fetchItem(item, options, session);
}

Task<Item> createItem(const Item &item, const Collection &collection, ItemCreateFlags flags, Session *session)
{
    return storage()->createItem(item, collection, flags, session);
}

Task<Item> updateItem(const Item &item, ItemModifyFlags flags, Session *session)
{
    return storage()->updateItem(item, flags, session);
}

Task<Item::List> updateItems(const Item::List &items, ItemModifyFlags flags, Session *session)
{
    return storage()->updateItems(items, flags, session);
}

Task<void> deleteItem(const Item &item, Session *session)
{
    return storage()->deleteItem(item, session);
}

Task<void> deleteItems(const Item::List &items, Session *session)
{
    return storage()->deleteItems(items, session);
}

Task<void> deleteItemsFromCollection(const Collection &collection, Session *session)
{
    return storage()->deleteItemsFromCollection(collection, session);
}

Task<void> deleteItemsWithTag(const Tag &tag, Session *session)
{
    return storage()->deleteItemsWithTag(tag, session);
}

Task<void> moveItem(const Item &item, const Collection &destination, Session *session)
{
    return storage()->moveItem(item, destination, session);
}

Task<void> moveItems(const Item::List &items, const Collection &destination, Session *session)
{
    return storage()->moveItems(items, destination, session);
}

Task<void> moveItemsFromCollection(const Item::List &items, const Collection &source, const Collection &destination, Session *session)
{
    return storage()->moveItemsFromCollection(items, source, destination, session);
}

Task<void> copyItem(const Item &item, const Collection &destination, Session *session)
{
    return storage()->copyItem(item, destination, session);
}

Task<void> copyItems(const Item::List &items, const Collection &destination, Session *session)
{
    return storage()->copyItems(items, destination, session);
}

Task<void> linkItems(const Item::List &items, const Collection &destination, Session *session)
{
    return storage()->linkItems(items, destination, session);
}

Task<void> unlinkItems(const Item::List &items, const Collection &collection, Session *session)
{
    return storage()->unlinkItems(items, collection, session);
}

Task<Item::List> searchItems(const SearchQuery &searchQuery, const ItemSearchOptions &options, Session *session)
{
    return storage()->searchItems(searchQuery, options, session);
}

Task<Collection> fetchCollection(const Collection &collection, const CollectionFetchOptions &options, Session *session)
{
    return storage()->fetchCollection(collection, options, session);
}

Task<Collection::List> fetchCollectionsRecursive(const Collection &root, const CollectionFetchOptions &options, Session *session)
{
    return storage()->fetchCollectionsRecursive(root, options, session);
}

Task<Collection::List> fetchCollectionsRecursive(const Collection::List &baseList, const CollectionFetchOptions &options, Session *session)
{
    return storage()->fetchCollectionsRecursive(baseList, options, session);
}

Task<Collection::List> fetchCollections(const Collection::List &collections, const CollectionFetchOptions &options, Session *session)
{
    return storage()->fetchCollections(collections, options, session);
}

Task<Collection::List> fetchSubcollections(const Collection &base, const CollectionFetchOptions &options, Session *session)
{
    return storage()->fetchSubcollections(base, options, session);
}

Task<Collection> createCollection(const Collection &collection, Session *session)
{
    return storage()->createCollection(collection, session);
}

Task<Collection> updateCollection(const Collection &collection, Session *session)
{
    return storage()->updateCollection(collection, session);
}

Task<void> deleteCollection(const Collection &collection, Session *session)
{
    return storage()->deleteCollection(collection, session);
}

Task<void> moveCollection(const Collection &collection, const Collection &destination, Session *session)
{
    return storage()->moveCollection(collection, destination, session);
}

Task<void> copyCollection(const Collection &collection, const Collection &destination, Session *session)
{
    return storage()->copyCollection(collection, destination, session);
}

Task<CollectionStatistics> fetchCollectionStatistics(const Collection &collection, Session *session)
{
    return storage()->fetchCollectionStatistics(collection, session);
}

Task<Tag> fetchTag(const Tag &tag, const TagFetchOptions &options, Session *session)
{
    return storage()->fetchTag(tag, options, session);
}

Task<Tag::List> fetchTags(const Tag::List &tags, const TagFetchOptions &options, Session *session)
{
    return storage()->fetchTags(tags, options, session);
}

Task<Tag::List> fetchAllTags(const TagFetchOptions &options, Session *session)
{
    return storage()->fetchAllTags(options, session);
}

Task<Tag> createTag(const Tag &tag, Session *session)
{
    return storage()->createTag(tag, session);
}

Task<Tag> updateTag(const Tag &tag, Session *session)
{
    return storage()->updateTag(tag, session);
}

Task<void> deleteTag(const Tag &tag, Session *session)
{
    return storage()->deleteTag(tag, session);
}

Task<void> deleteTags(const Tag::List &tags, Session *session)
{
    return storage()->deleteTags(tags, session);
}

Task<Relation::List> fetchRelation(const Relation &relation, Session *session)
{
    return storage()->fetchRelation(relation, session);
}

Task<Relation::List> fetchRelationsOfTypes(const QVector<QByteArray> &types, Session *session)
{
    return storage()->fetchRelationsOfTypes(types, session);
}

Task<Relation> createRelation(const Relation &relation, Session *session)
{
    return storage()->createRelation(relation, session);
}

Task<void> deleteRelation(const Relation &relation, Session *session)
{
    return storage()->deleteRelation(relation, session);
}

Task<Collection> createSearchCollection(const QString &name, const SearchQuery &searchQuery, const ItemSearchOptions &options, Session *session)
{
    return storage()->createSearchCollection(name, searchQuery, options, session);
}

Task<void> beginTransaction(Session *session)
{
    return storage()->beginTransaction(session);
}

Task<void> commitTransaction(Session *session)
{
    return storage()->commitTransaction(session);
}

Task<void> rollbackTransaction(Session *session)
{
    return storage()->rollbackTransaction(session);
}

Task<void> selectResource(const QString &identifier, Session *session)
{
    return storage()->selectResource(identifier, session);
}

} // namespace Akonadi
