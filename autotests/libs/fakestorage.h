/*
    Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_FAKESTORAGE_H_
#define AKONADI_FAKESTORAGE_H_

#include "akonaditests_export.h"
#include "storageinterface.h"

#include <memory>

class FakeStoragePrivate;

namespace Akonadi
{

/**
 * Fake implementation of storage interface for use in tests
 */
class AKONADICORE_EXPORT FakeStorage : public StorageInterface
{
public:
    explicit FakeStorage();
    ~FakeStorage() override;

    /**
     * Resets the storage to default state
     */
    void reset();

    Session *createSession(const QByteArray &name) override;
    Session *defaultSession() override;

    void setFetchResults(const Item::List &items);
    Task<Item::List> fetchItemsFromCollection(const Collection &collection, const ItemFetchOptions &options,
                                              Session *session) override;
    Task<Item::List> fetchItems(const Item::List &items, const ItemFetchOptions &options, Session *session) override;
    Task<Item::List> fetchItemsForTag(const Tag &tag, const ItemFetchOptions &options, Session *session) override;
    Task<Item> fetchItem(const Item &item, const ItemFetchOptions &options, Session *session) override;

    Item createdItem();
    Task<Item> createItem(const Item &item, const Collection &collection, ItemCreateFlags flags, Session *session) override;

    Item::List updatedItems();
    Task<Item> updateItem(const Item &item, ItemModifyFlags flags, Session *session) override;
    Task<Item::List> updateItems(const Item::List &items, ItemModifyFlags flags, Session *session) override;

    QVector<qint64> deletedItems();
    Task<void> deleteItem(const Item &item, Session *session) override;
    Task<void> deleteItems(const Item::List &items, Session *session) override;
    Task<void> deleteItemsFromCollection(const Collection &collection, Session *session) override;
    Task<void> deleteItemsWithTag(const Tag &tag, Session *session) override;

    Item::List movedItems();
    Task<void> moveItem(const Item &item, const Collection &destination, Session *session) override;
    Task<void> moveItems(const Item::List &items, const Collection &destination, Session *session) override;
    Task<void> moveItemsFromCollection(const Item::List &items, const Collection &source,
                                       const Collection &destination, Session *session) override;

    Task<void> copyItem(const Item &item, const Collection &destination, Session *session) override;
    Task<void> copyItems(const Item::List &items, const Collection &destination, Session *session) override;

    Item::List linkedItems();
    Task<void> linkItems(const Item::List &items, const Collection &destination, Session *session) override;

    Item::List unlinkedItems();
    Task<void> unlinkItems(const Item::List &items, const Collection &collection, Session *session) override;

    void setSearchResults(const Item::List &items);
    Task<Item::List> searchItems(const SearchQuery &searchQuery, const ItemSearchOptions &options,
                                 Session *session) override;


    void setFetchResults(const Collection::List &collections);
    Task<Collection> fetchCollection(const Collection &collection, const CollectionFetchOptions &options,
                                     Session *session) override;
    Task<Collection::List> fetchCollectionsRecursive(const Collection &root, const CollectionFetchOptions &options,
                                                     Session *session) override;
    Task<Collection::List> fetchCollectionsRecursive(const Collection::List &root, const CollectionFetchOptions &options,
                                                     Session *session) override;
    Task<Collection::List> fetchCollections(const Collection::List &collections, const CollectionFetchOptions &options,
                                            Session *session) override;
    Task<Collection::List> fetchSubcollections(const Collection &base, const CollectionFetchOptions &options,
                                               Session *session) override;

    Collection createdCollection();
    Task<Collection> createCollection(const Collection &collection, Session *session) override;

    Collection updatedCollection();
    Task<Collection> updateCollection(const Collection &collection, Session *session) override;

    qint64 deletedCollection();
    Task<void> deleteCollection(const Collection &collection, Session *session) override;

    Collection movedCollection();
    Task<void> moveCollection(const Collection &collection, const Collection &destination, Session *session) override;
    Task<void> copyCollection(const Collection &collection, const Collection &destination, Session *session) override;

    void setCollectionStatistics(const CollectionStatistics &stats);
    Task<CollectionStatistics> fetchCollectionStatistics(const Collection &collection, Session *session) override;


    void setFetchResults(const Tag::List &tags);
    Task<Tag> fetchTag(const Tag &tag, const TagFetchOptions &options, Session *session) override;
    Task<Tag::List> fetchTags(const Tag::List &tags, const TagFetchOptions &options,
                                     Session *session) override;
    Task<Tag::List> fetchAllTags(const TagFetchOptions &options, Session *session) override;

    Tag createdTag();
    Task<Tag> createTag(const Tag &tag, Session *session) override;
    Tag updatedTag();
    Task<Tag> updateTag(const Tag &tag, Session *session) override;
    QVector<qint64> deletedTags();
    Task<void> deleteTag(const Tag &tag, Session *session) override;
    Task<void> deleteTags(const Tag::List &tags, Session *session) override;

    void setFetchResult(const Relation::List &relations);
    Task<Relation::List> fetchRelation(const Relation &relation, Session *session) override;
    Task<Relation::List> fetchRelationsOfTypes(const QVector<QByteArray> &types, Session *session) override;
    Relation createdRelation();
    Task<Relation> createRelation(const Relation &relation, Session *session) override;

    Relation deletedRelation();
    Task<void> deleteRelation(const Relation &relation, Session *session) override;


    Task<Collection> createSearchCollection(const QString &name, const SearchQuery &searchQuery,
                                            const ItemSearchOptions &options, Session *session) override;


    Task<void> beginTransaction(Session *session) override;
    Task<void> commitTransaction(Session *session) override;
    Task<void> rollbackTransaction(Session *session) override;

    Task<void> selectResource(const QString &identifier, Session *session) override;

    ChangeNotificationDependenciesFactory &changeNotificationDependenciesFactory() override;

private:
    std::unique_ptr<FakeStoragePrivate> mStorage;
};

} // namespace Akonadi

#endif


