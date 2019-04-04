/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_STORAGEINTERFACE_H
#define AKONADI_STORAGEINTERFACE_H

#include "interface.h"
#include "item.h"
#include "searchquery.h"
#include "tag.h"
#include "task.h"

class QObject;
class QByteArray;

namespace Akonadi
{

/**
 * StorageInterface is an interface for access to the Akonadi storage.
 *
 * The default implementation of the StorageInterface (Akonadi::Storage)
 * interacts with the real Akonadi Server. However one can provide a completely
 * custom implementation of the interface for testing without having to set up
 * and run a real Akonadi instance.
 *
 * @since 5.11
 */
class StorageInterface
{
public:
    virtual ~StorageInterface() = default;

    /**
     * Creates a new Session.
     *
     * The session can be passed as the @p parent argument to any other method in this
     * interface to enforce the returned job to run on this session.
     *
     * FIXME: This should be returning a QSharedPointer<Session>!!
     */
    virtual Session *createSession(const QByteArray &name) = 0;

    /**
     * Returns the default session for the current thread.
     *
     * This method will not cause a new session to be created if none exists
     * yet, so it can return a null pointer.
     *
     * FIXME: This should be returning a QSharedPointer<Session>!!!
     */
    virtual Session *defaultSession() = 0;

#if 0
    /**
     * Creates a new Monitor that monitors for changes in the storage.
     */
    virtual MonitorInterface *createMonitor(QObject *parent = {}) = 0;

    /**
     * Creates a new ChangeRecorder that monitors for changes in the storage
     * and stores them in a journal file for later change replay.
     */
    virtual ChangeRecorderInterface *createChangeRecorder(
                             const QString &name,
                             QObject *parent = {}) = 0;

#endif
    /**
     * Fetches all Items from the given @p collection.
     *
     * @param collection The parent collection to fetch all items from.
     * @param options Additional options to configure the retrieval.
     *
     * @since fetchItem(), fetchItems(), fetchItemsForTags()
     */
    virtual Task<Item::List> fetchItemsFromCollection(const Collection &collection, const ItemFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Fetches the specified items.
     *
     * If the items have an ID set, this is used to identify the item on the Akonadi
     * server. If only a remote identifier is available, that is used.
     * However, as remote identifiers are not necessarily globally unique, you
     * need to specify the collection to search in in that case, using
     * ItemFetchOptions::setCollection().
     *
     * @internal
     * For internal use only when using remote identifiers, the resource search
     * context can be set globally by selectResource().
     * @endinternal
     *
     * @param items The items to fetch.
     * @options Additional options to configure the retrieval.
     *
     * @see fetchItemsFromCollection(), fetchItem(), fetchItemsForTags()
     */
    virtual Task<Item::List> fetchItems(const Item::List &items, const ItemFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Fetches all Items tagged with the specified @p tag.
     *
     * @param tag The tag to fetch all items from.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchItemsFromCollection(), fetchItem(), fetchItems()
     */
    virtual Task<Item::List> fetchItemsForTag(const Tag &tag, const ItemFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Fetches the specified Item.
     *
     * If the item has a uid set, this is used to identify the item on the Akonadi
     * server. If only a remote identifier is available, that is used.
     * However, as remote identifiers are not necessarily globally unique, you
     * need to specify the collection to search in in that case, using
     * ItemFetchOptions::setCollection().
     *
     * @internal
     * For internal use only when using remote identifiers, the resource search
     * context can be set globally by selectResource().
     * @endinternal
     *
     * @param item The item to fetch.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchItemsFromCollection(), fetchItemsForTag(), fetchItems()
     */
    virtual Task<Item> fetchItem(const Item &item, const ItemFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Appends or merges given @p item in given @p collection.
     *
     * When merging is disabled the job will simply append the given @p item
     * to the given @p collection. When merging is enabled the server will
     * look in the given @p collection for an existing Item with the same
     * RID or GID (or both, depending on the ItemCreateFlags) as the provided
     * @p item.
     *
     * @param item The Item to append or merge.
     * @param collection Item's parent Collection.
     * @param flags Additional flags to enable and configure merging.
     */
    virtual Task<Item>
    createItem(const Item &item, const Collection &collection, ItemCreateFlags flags = ItemCreateFlag::Default, Session *session = nullptr) = 0;

    /**
     * Modifies an existing Item in the Akonadi storage.
     *
     * @param item The Item to modify
     * @param flags Additional options.
     *
     * @see updateItems()
     */
    virtual Task<Item> updateItem(const Item &item, ItemModifyFlags flags = ItemModifyFlag::Default, Session *session = nullptr) = 0;

    /**
     * Update multiple Items at once.
     *
     * Use this when applying the same change to a set of items, such as a mass-change of
     * item flags, not if you just want to store a bunch of randomly modified
     * items.
     *
     * Note that currently this will only store flag and tag changes.
     *
     * @param items The Items to modify.
     * @param flags Additional options.
     *
     * @see updateItem()
     */
    virtual Task<Item::List> updateItems(const Item::List &items, ItemModifyFlags flags = ItemModifyFlag::Default, Session *session = nullptr) = 0;

    /**
     * Permanently delete an Item from the Akonadi storage.
     *
     * @param item The Item to delete.
     *
     * @see deleteItems(), deleteItemsFromCollection(), deleteItemsWithTag()
     */
    virtual Task<void> deleteItem(const Item &item, Session *session = nullptr) = 0;

    /**
     * Permanently delete Items from the Akonadi storage.
     *
     * @param items Items to delete.
     *
     * @see deleteItems(), deleteItemsFromCollection(), deleteItemsWithTag()
     */
    virtual Task<void> deleteItems(const Item::List &items, Session *session = nullptr) = 0;

    /**
     * Permanently deletes all Items from the given @p collection.
     *
     * @param collection The Collection to delete all Items from.
     *
     * @see deleteItem(), deleteItemsFromCollection(), deleteItemsWithTag()
     */
    virtual Task<void> deleteItemsFromCollection(const Collection &collection, Session *session = nullptr) = 0;

    /**
     * Permanently deletes all Items tagged with the given @p tag.
     *
     * @param tag All Items tagged with this tag will be removed.
     * @param parent A parent Session to run the fetch job on, a parent job to
     *        add this job as a child to, or any QObject.
     *
     * @see deleteItem(), deleteItems(), deleteItemsFromCollection()
     */
    virtual Task<void> deleteItemsWithTag(const Tag &tag, Session *session = nullptr) = 0;

    /**
     * Moves the given @p item to the given @p destination.
     *
     * @param item The Item to move.
     * @param destination The Collection to move the Item into.
     *
     * @see moveItems(), moveItemsFromCollection()
     */
    virtual Task<void> moveItem(const Item &item, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Moves given @p items to the given @p destination.
     *
     * @param items Items to move.
     * @param destination The Collection to move the Items into.
     *
     * @see moveItem(), moveItemsFromCollection()
     */
    virtual Task<void> moveItems(const Item::List &items, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Moves given @p items from the @p source to the @destination.
     *
     * This method can be used when @p items are only identified by their
     * RemoteIDs and thus a source collection must be provided to resolve the
     * RIDs against, as RemoteIDs are not globally unique.
     *
     * @param items Items to move.
     * @param source The source collection resolve by Remote ID again.
     * @param destination The Collection to move the Items into.
     *
     * @see moveItem(), moveItems()
     */
    virtual Task<void>
    moveItemsFromCollection(const Item::List &items, const Collection &source, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Copies the given @p item to the @p destination.
     *
     * @param item The Item to copy.
     * @param destination The Collection to copy into.
     *
     * @see copyItems()
     */
    virtual Task<void> copyItem(const Item &item, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Copies given @p items to the @p destination.
     *
     * @param items Items to copy.
     * @param destination The Collection to copy into.
     *
     * @see copyItem()
     */
    virtual Task<void> copyItems(const Item::List &items, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Links given @p items to the virtual collection @p destination.
     *
     * @param items Items to link.
     * @param destination Collection to link items into. Must be a virtual
     *        collection.
     *
     * @see unlinkItems()
     */
    virtual Task<void> linkItems(const Item::List &items, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Unlinks given @p items from the virtual collection @p destination.
     *
     * @param items Items to unlink.
     * @param destination Collection to unlink items from.
     *
     * @see linkItems()
     */
    virtual Task<void> unlinkItems(const Item::List &items, const Collection &collection, Session *session = nullptr) = 0;

    /**
     * Retrieves all Items matching given search query.
     *
     * Additional search constraints can be set via the @p options.
     *
     * @param searchQuery The query to execute. It can be empty, then only the
     *        additional search constraints apply.
     * @param options Additional options to configure the search.
     */
    virtual Task<Item::List> searchItems(const SearchQuery &searchQuery, const ItemSearchOptions &options, Session *session = nullptr) = 0;

    /**
     * Fetches the given Collection from the Akonadi storage.
     *
     * If the given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is available
     * the collection is identified using that, provided that a resource search context
     * has been specified by calling CollectionFetchOptions::setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using selectResource().
     * @endinternal
     *
     * @param collection The collection to retrieve.
     * @param options Additional options to configure the retrieval
     *
     * @see fetchCollectionsRecursive(), fetchCollections(), fetchSubcollections()
     */
    virtual Task<Collection> fetchCollection(const Collection &collection, const CollectionFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Fetches the given collection and all its sub-collections.
     *
     * If the given base collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is available
     * the collection is identified using that, provided that a resource search context
     * has been specified by calling CollectionFetchOptions::setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collection The base collection for the listing.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchCollection(), fetchCollections(), fetchSubcollections()
     */
    virtual Task<Collection::List> fetchCollectionsRecursive(const Collection &root, const CollectionFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Fetches the base collections and all their sub-collections.
     *
     * The results are guaranteed to not contain duplicated results. If the given
     * base collection has a unique identifier, this is used to identify the
     * collection in the Akonadi server. If only a remote identifier is available
     * the collection is identifier using that, provided that a resource search context
     * has been specified by calling CollectionFetchOptions::setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collections Base collections for the listing.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchCollection(), fetchCollections(), fetchSubcollections(), fetchCollectionsRecursive()
     */
    AKONADICORE_EXPORT
    virtual Task<Collection::List>
    fetchCollectionsRecursive(const Collection::List &collections, const CollectionFetchOptions &options = {}, Session *session = nullptr) = 0;

    /**
     * Retrieves the given collections from the Akonadi storage.
     *
     * If the given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is available
     * the collection is identified using that, provided that a resource search context
     * has been specified by calling CollectionFetchOptions::setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collections A list of collections to fetch. Must not be empty.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchCollection(), fetchCollectionsRecursive(), fetchSubcollections()
     */
    virtual Task<Collection::List> fetchCollections(const Collection::List &collections, const CollectionFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Retrieves direct subcollections of the given @p base collection.
     *
     * Use Akonadi::fetchCollectionsRecursive() if you need all subcollections.
     *
     * If the given collection has a unique identifier, this is used to identify
     * the collection in the Akonadi server. If only a remote identifier is available
     * the collection is identified using that, provided that a resource search context
     * has been specified by calling CollectionFetchOptions::setResource().
     *
     * @internal
     * For internal use only, if a remote identifier is set, the resource
     * search context can be set globally using ResourceSelectJob.
     * @endinternal
     *
     * @param collections A list of collections to fetch. Must not be empty.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchCollection(), fetchCollections(), fetchCollectionsRecursive()
     */
    virtual Task<Collection::List> fetchSubcollections(const Collection &base, const CollectionFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Creates a new collection in the Akonadi storage.
     *
     * @param collection The new collection. @p collection must have a parent collection
     *        set with a unique identifier. If a resource context is specified in the
     *        current session (that is you are using it within Akonadi::ResourceBase),
     *        the parent collection can be identified by its remote identifier as well.
     */
    virtual Task<Collection> createCollection(const Collection &collection, Session *session = nullptr) = 0;

    /**
     * Updates the given @p collection.
     *
     * @param collection The collection to update.
     */
    virtual Task<Collection> updateCollection(const Collection &collection, Session *session = nullptr) = 0;

    /**
     * Permanently deletes the given @p collection, its subcollections and all
     * associated content from Akonadi storage.
     *
     * @param collection The base collection to remove.
     */
    virtual Task<void> deleteCollection(const Collection &collection, Session *session = nullptr) = 0;

    /**
     * Moves the given @p collection from its current parent collection to the
     * specified @p destination.
     *
     * @param collection The collection to move.
     * @param destination The parent collection to move the @p collection into.
     */
    virtual Task<void> moveCollection(const Collection &collection, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Copies the given @p collection into another collection in the Akonadi
     * storage.
     *
     * @param collection The collection to copy.
     * @param destination The parent collection to copy the @p collection into.
     */
    virtual Task<void> copyCollection(const Collection &collection, const Collection &destination, Session *session = nullptr) = 0;

    /**
     * Retrieves collection statistics from the Akonadi storage.
     *
     * @param collection The collection of which to retrieve statistics.
     */
    virtual Task<CollectionStatistics> fetchCollectionStatistics(const Collection &collection, Session *session = nullptr) = 0;

    /**
     * Retrieves the specified tag from the Akonadi storage.
     *
     * @param tag The tag to retrieve.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchTags()
     */
    virtual Task<Tag> fetchTag(const Tag &tag, const TagFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Retrieves the specified tags from the Akonadi storage.
     *
     * @param tags Tags to retrieve.
     * @param options Additional options to configure the retrieval.
     *
     * @see fetchTag()
     */
    virtual Task<Tag::List> fetchTags(const Tag::List &tags, const TagFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Retrieves all tags from the Akonadi storage.
     *
     * @see fetchTag()
     */
    virtual Task<Tag::List> fetchAllTags(const TagFetchOptions &options, Session *session = nullptr) = 0;

    /**
     * Creates a new tag in the Akonadi storage.
     *
     * @param tag The tag to create
     */
    virtual Task<Tag> createTag(const Tag &tag, Session *session = nullptr) = 0;

    /**
     * Updates the given @p tag in the Akonadi storage.
     *
     * @param tag The tag to update.
     */
    virtual Task<Tag> updateTag(const Tag &tag, Session *session = nullptr) = 0;

    /**
     * Permanently deletes the given @p tag from Akonadi storage.
     *
     * This will untag all Items tagged with the tag and a corresponding update
     * notification will be sent out for all affected Items.
     *
     * @param tag The tag to delete.
     *
     * @see deleteTag()
     */
    virtual Task<void> deleteTag(const Tag &tag, Session *session = nullptr) = 0;

    /**
     * Permanently deletes the given @p tags from Akonadi storage.
     *
     * This will untag all Items tagged with the tags and a corresponding update
     * notification will be sent out for all affected Items.
     *
     * @param tags Tags to delete.
     *
     * @see deleteTag()
     */
    virtual Task<void> deleteTags(const Tag::List &tags, Session *session = nullptr) = 0;

    /**
     * Retrieves the specified Relation from the Akonadi storage.
     *
     * The relation can be be specified only partially, e.g. only one side of
     * the relation may be specified. In that case all relations matching the
     * criteria will be returned.
     *
     * @param relation The relation to retrieve.
     *
     * @see fetchRelationsOfTypes()
     */
    virtual Task<Relation::List> fetchRelation(const Relation &relation, Session *session = nullptr) = 0;

    /**
     * Retrieves all Relations with the specified types.
     *
     * @param types The types of which Relations should be retrieved.
     *
     * @see fetchRelation()
     */
    virtual Task<Relation::List> fetchRelationsOfTypes(const QVector<QByteArray> &types, Session *session = nullptr) = 0;

    /**
     * Stores the @p relation in the Akonadi storage.
     *
     * @param relation The new relation to store.
     */
    virtual Task<Relation> createRelation(const Relation &relation, Session *session = nullptr) = 0;

    /**
     * Permanently deletes the given @p relation from Akonadi storage.
     *
     * A corresponding change notifiction will be emitted for all affected Items.
     *
     * @param relation The relation to delete.
     */
    virtual Task<void> deleteRelation(const Relation &relation, Session *session = nullptr) = 0;

    /**
     * Creates a collection with persistent search query.
     *
     * @param name Name of the created collection.
     * @param searchQuery Search query whose result will be linked to the
     *        collection.
     */
    virtual Task<Collection>
    createSearchCollection(const QString &name, const SearchQuery &searchQuery, const ItemSearchOptions &options, Session *session = nullptr) = 0;

    /**
     * Begins a new transaction in the storage backend.
     */
    virtual Task<void> beginTransaction(Session *session = nullptr) = 0;

    /**
     * Commits current transaction in the storage backend.
     */
    virtual Task<void> commitTransaction(Session *session = nullptr) = 0;

    /**
     * Rolls back current transaction in the storage backend.
     */
    virtual Task<void> rollbackTransaction(Session *session = nullptr) = 0;

    /**
     * @internal
     *
     * Marks the current Session as belonging to a Resource.
     *
     * This is an internal method that should never be used by clients. It is
     * present purely to make the interface complete.
     *
     * @param identifier Resource instance identifier.
     * @param session A parent Session to apply this task to.
     */
    virtual Task<void> selectResource(const QString &identifier, Session *session = nullptr) = 0;
};

} // namespace

#endif
