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

#ifndef AKONADI_INTERFACE_H_
#define AKONADI_INTERFACE_H_

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"
#include "relation.h"
#include "tag.h"
#include "task.h"

#include <QSharedPointer>

namespace Akonadi
{

class StorageInterface;

class ItemFetchScope;
class CollectionFetchScope;
class TagFetchScope;
class SearchQuery;

/**
 * Replaces the default storage implementation (Akonadi::Storage) with @p
 * storage.
 *
 * It is not necessary to use this function unless you want to replace the real
 * Akonadi Storage with a mock one.
 *
 * This function is thread-safe.
 *
 * @param storage An implementation of StorageInterface to replace the default
 *                implementation.
 * @return The previous implementation that has been replaced.
 *
 * @see storage()
 * @since 5.12
 */
AKONADICORE_EXPORT
QSharedPointer<StorageInterface> setStorage(const QSharedPointer<StorageInterface> &storage);

/**
 * Returns the current storage implementation.
 *
 * Returns Akonadi::Storage, unless it was replaced by a custom implementation
 * using setStorage().
 *
 * Use this to obtain reference to the current implementation.
 *
 * This function is thread-safe.
 *
 * @see setStorage()
 * @since 5.12
 */
AKONADICORE_EXPORT
QSharedPointer<StorageInterface> storage();

/**
 * Creates a new Session.
 *
 * The session can be passed as the @p parent argument to any other method in this
 * interface to enforce the returned job to run on this session.
 *
 * FIXME: Return a QSharedPointer!
 */
AKONADICORE_EXPORT
Session *createSession(const QByteArray &name);

/**
 * Returns the default session for the current thread.
 *
 * This method will not cause a new session to be created if none exists
 * yet, so it can return a null pointer.
 *
 * FIXME: Return a QSharedPointer!
 */
AKONADICORE_EXPORT
Session *defaultSession();

class ItemFetchOptionsPrivate;
/**
 * Additional options to customize item retrieval.
 *
 * @since 5.12
 */
class AKONADICORE_EXPORT ItemFetchOptions
{
public:
    explicit ItemFetchOptions();
    explicit ItemFetchOptions(const Collection &collection);
    ItemFetchOptions(const ItemFetchScope &fetchScope);
    ItemFetchOptions(const ItemFetchScope &fetchScope, const Collection &collection);
    ItemFetchOptions(const ItemFetchOptions &);
    ItemFetchOptions &operator=(const ItemFetchOptions &);
    ItemFetchOptions(ItemFetchOptions &&) noexcept;
    ItemFetchOptions &operator=(ItemFetchOptions &&) noexcept;
    ~ItemFetchOptions();

    /**
     * Returns the item fetch scope.
     *
     * Returns a reference so it can be used to conveniently modify the
     * current scope in-place.
     */
    ItemFetchScope &itemFetchScope();
    const ItemFetchScope &itemFetchScope() const;

    /**
     * Replaces the current fetch scope with @p fetchScope.
     */
    void setItemFetchScope(const ItemFetchScope &fetchScope);

    /**
     * Returns which Collection to retrieve the Item from.
     */
    Collection collection() const;

    /**
     * Specifies the Collection the Item is in.
     *
     * This is only required when retrieving an Item based on its Remote ID
     * which might not be unique globally.
     */
    void setCollection(const Collection &collection);

private:
    QSharedDataPointer<ItemFetchOptionsPrivate> d;
};

/**
 * Fetches all Items from the given @p collection.
 *
 * @param collection The parent collection to fetch all items from.
 * @param options Additional options to configure the retrieval.
 *
 * @since fetchItem(), fetchItems(), fetchItemsForTags()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item::List> fetchItemsFromCollection(const Collection &collection, const ItemFetchOptions &options, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item::List> fetchItems(const Item::List &items, const ItemFetchOptions &options, Session *session = nullptr);

/**
 * Fetches all Items tagged with the specified @p tag.
 *
 * @param tag The tag to fetch all items from.
 * @param options Additional options to configure the retrieval.
 *
 * @see fetchItemsFromCollection(), fetchItem(), fetchItems()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item::List> fetchItemsForTag(const Tag &tag, const ItemFetchOptions &options, Session *session = nullptr);

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
 * @since 5.12
 *
 * TODO: AkOptional<Item> in case the Item does not exist?!
 */
AKONADICORE_EXPORT
Task<Item> fetchItem(const Item &item, const ItemFetchOptions &options, Session *session = nullptr);

enum class ItemCreateFlag {
    NoMerge = 0, ///< Don't merge
    RID = 1, ///< Merge by Remote ID
    GID = 2, ///< Merge by GID
    Silent = 4, ///< Only return the ID of the merged/create Item.

    Default = NoMerge
};
Q_DECLARE_FLAGS(ItemCreateFlags, ItemCreateFlag)

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
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item> createItem(const Item &item, const Collection &collection, ItemCreateFlags flags = ItemCreateFlag::Default, Session *session = nullptr);

enum class ItemModifyFlag {
    /**
     * When set the payload of the modified Item will be ignored by the
     * storage.
     */
    IgnorePayload = 1 << 0,
    /**
     * Sets whether GID shall be updated either from the GID parameter or by
     * extracting it from the payload.
     *
     * Normally this should not be neccessary as GID is automatically extracted
     * when the Item is first created and it should never be updated later.
     */
    UpdateGid = 1 << 1,
    /**
     * Check if the revision number of the modified Item is not smaller than
     * the revision number in the database.
     */
    RevisionCheck = 1 << 2,
    /**
     * When set the task will bring up a dialog to resolve a conflict that
     * might happen when modifying an Item.
     */
    AutomaticConflictHandling = 1 << 3,

    Default = RevisionCheck | AutomaticConflictHandling
};
Q_DECLARE_FLAGS(ItemModifyFlags, ItemModifyFlag)

/**
 * Modifies an existing Item in the Akonadi storage.
 *
 * @param item The Item to modify
 * @param flags Additional options.
 *
 * @see updateItems()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item> updateItem(const Item &item, ItemModifyFlags flags = ItemModifyFlag::Default, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item::List> updateItems(const Item::List &items, ItemModifyFlags flags = ItemModifyFlag::Default, Session *session = nullptr);

/**
 * Permanently delete an Item from the Akonadi storage.
 *
 * @param item The Item to delete.
 *
 * @see deleteItems(), deleteItemsFromCollection(), deleteItemsWithTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteItem(const Item &item, Session *session = nullptr);

/**
 * Permanently delete Items from the Akonadi storage.
 *
 * @param items Items to delete.
 *
 * @see deleteItems(), deleteItemsFromCollection(), deleteItemsWithTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteItems(const Item::List &items, Session *session = nullptr);

/**
 * Permanently deletes all Items from the given @p collection.
 *
 * @param collection The Collection to delete all Items from.
 *
 * @see deleteItem(), deleteItemsFromCollection(), deleteItemsWithTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteItemsFromCollection(const Collection &collection, Session *session = nullptr);

/**
 * Permanently deletes all Items tagged with the given @p tag.
 *
 * @param tag All Items tagged with this tag will be removed.
 * @param parent A parent Session to run the fetch job on, a parent job to
 *        add this job as a child to, or any QObject.
 *
 * @see deleteItem(), deleteItems(), deleteItemsFromCollection()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteItemsWithTag(const Tag &tag, Session *session = nullptr);

/**
 * Moves the given @p item to the given @p destination.
 *
 * @param item The Item to move.
 * @param destination The Collection to move the Item into.
 *
 * @see moveItems(), moveItemsFromCollection()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> moveItem(const Item &item, const Collection &destination, Session *session = nullptr);

/**
 * Moves given @p items to the given @p destination.
 *
 * @param items Items to move.
 * @param destination The Collection to move the Items into.
 *
 * @see moveItem(), moveItemsFromCollection()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> moveItems(const Item::List &items, const Collection &destination, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> moveItemsFromCollection(const Item::List &items, const Collection &source, const Collection &destination, Session *session = nullptr);

/**
 * Copies the given @p item to the @p destination.
 *
 * @param item The Item to copy.
 * @param destination The Collection to copy into.
 *
 * @see copyItems()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> copyItem(const Item &item, const Collection &destination, Session *session = nullptr);

/**
 * Copies given @p items to the @p destination.
 *
 * @param items Items to copy.
 * @param destination The Collection to copy into.
 *
 * @see copyItem()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> copyItems(const Item::List &items, const Collection &destination, Session *session = nullptr);

/**
 * Links given @p items to the virtual collection @p destination.
 *
 * @param items Items to link.
 * @param destination Collection to link items into. Must be a virtual
 *        collection.
 *
 * @see unlinkItems()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> linkItems(const Item::List &items, const Collection &destination, Session *session = nullptr);

/**
 * Unlinks given @p items from the virtual collection @p destination.
 *
 * @param items Items to unlink.
 * @param destination Collection to unlink items from.
 *
 * @see linkItems()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> unlinkItems(const Item::List &items, const Collection &collection, Session *session = nullptr);

class ItemSearchOptionsPrivate;

/**
 * Additional options to configure Item search.
 */
class AKONADICORE_EXPORT ItemSearchOptions
{
public:
    ItemSearchOptions();
    ItemSearchOptions(const ItemSearchOptions &);
    ItemSearchOptions &operator=(const ItemSearchOptions &);
    ItemSearchOptions(ItemSearchOptions &&) noexcept;
    ItemSearchOptions &operator=(ItemSearchOptions &&) noexcept;
    ~ItemSearchOptions();

    /**
     * Returns reference to current ItemFetchScope.
     *
     * The fetch scope affects what portions of Items that match the query
     * should be retrieved.
     */
    ItemFetchScope &itemFetchScope();
    const ItemFetchScope &itemFetchScope() const;
    /**
     * Replaces the current item fetch scope with @p fetchScope
     */
    void setItemFetchScope(const ItemFetchScope &fetchScope);

    /**
     * Returns reference to current TagFetchScope.
     *
     * The fech scope affects what portions of Tags for each Item that matches
     * the query should be retrieved.
     */
    TagFetchScope &tagFetchScope();
    const TagFetchScope &tagFetchScope() const;
    /**
     * Replaces the current tag fetch scope with @p fetchScope.
     */
    void setTagFetchScope(const TagFetchScope &fetchScope);

    /**
     * Restricts the search only to given @p collections.
     *
     * @see searchCollections(), setRecursive()
     */
    void setSearchCollections(const Collection::List &collections);
    /**
     * Returns Collections that the search is restricted to.
     *
     * @see setSearchCollections(), isRecursive()
     */
    Collection::List searchCollections() const;

    /**
     * Set which types of Items the search will match.
     *
     * @see mimeTypes()
     */
    void setMimeTypes(const QStringList &mimeTypes);
    /***
     * Returns which types of Items the search will match.
     *
     * @see setMimeTypes()
     */
    QStringList mimeTypes() const;

    /**
     * Returns whether the search should recurse into subcollections.
     *
     * This option is off by default.
     *
     * @see isRecursive(), setSearchCollections()
     */
    void setRecursive(bool recursive);
    /**
     * Returns whether the search should recursce into subcollections.
     *
     * @see setRecursive(), searchCollections()
     */
    bool isRecursive() const;

    /**
     * Returns whether remote search should be used as well.
     *
     * Remote search is performed on the backend service (like IMAP server),
     * making the search much slower but possibly more accurate.
     *
     * This option is off by default.
     *
     * @see isRemoteSearchEnabled()
     */
    void setRemoteSearchEnabled(bool enabled);
    /**
     * Returns whether remote search should be used as well.
     *
     * This option is off by default.
     *
     * @see setRemoteSearchEnabled()
     */
    bool isRemoteSearchEnabled() const;

private:
    QSharedDataPointer<ItemSearchOptionsPrivate> d;
};

/**
 * Retrieves all Items matching given search query.
 *
 * Additional search constraints can be set via the @p options.
 *
 * @param searchQuery The query to execute. It can be empty, then only the
 *        additional search constraints apply.
 * @param options Additional options to configure the search.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Item::List> searchItems(const SearchQuery &searchQuery, const ItemSearchOptions &options, Session *session = nullptr);

class CollectionFetchOptionsPrivate;
/**
 * Additional configuration for collection retrieval.
 */
class AKONADICORE_EXPORT CollectionFetchOptions
{
public:
    CollectionFetchOptions();
    CollectionFetchOptions(const CollectionFetchScope &scope);
    CollectionFetchOptions(const CollectionFetchOptions &);
    CollectionFetchOptions &operator=(const CollectionFetchOptions &);
    CollectionFetchOptions(CollectionFetchOptions &&) noexcept;
    CollectionFetchOptions &operator=(CollectionFetchOptions &&) noexcept;
    ~CollectionFetchOptions();

    /**
     * Returns reference to current collection fetch scope.
     */
    CollectionFetchScope &fetchScope();
    const CollectionFetchScope &fetchScope() const;
    /**
     * Replaces the current collection fetch scope with @p fetchScope.
     */
    void setFetchScope(const CollectionFetchScope &fetchScope);

private:
    QSharedDataPointer<CollectionFetchOptionsPrivate> d;
};

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection> fetchCollection(const Collection &collection, const CollectionFetchOptions &options = {}, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection::List> fetchCollectionsRecursive(const Collection &root, const CollectionFetchOptions &options = {}, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection::List> fetchCollectionsRecursive(const Collection::List &collections, const CollectionFetchOptions &options = {}, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection::List> fetchCollections(const Collection::List &collections, const CollectionFetchOptions &options = {}, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection::List> fetchSubcollections(const Collection &base, const CollectionFetchOptions &options = {}, Session *session = nullptr);

/**
 * Creates a new collection in the Akonadi storage.
 *
 * @param collection The new collection. @p collection must have a parent collection
 *        set with a unique identifier. If a resource context is specified in the
 *        current session (that is you are using it within Akonadi::ResourceBase),
 *        the parent collection can be identified by its remote identifier as well.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection> createCollection(const Collection &collection, Session *session = nullptr);

/**
 * Updates the given @p collection.
 *
 * @param collection The collection to update.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection> updateCollection(const Collection &collection, Session *session = nullptr);

/**
 * Permanently deletes the given @p collection, its subcollections and all
 * associated content from Akonadi storage.
 *
 * @param collection The base collection to remove.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteCollection(const Collection &collection, Session *session = nullptr);

/**
 * Moves the given @p collection from its current parent collection to the
 * specified @p destination.
 *
 * @param collection The collection to move.
 * @param destination The parent collection to move the @p collection into.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> moveCollection(const Collection &collection, const Collection &destination, Session *session = nullptr);

/**
 * Copies the given @p collection into another collection in the Akonadi
 * storage.
 *
 * @param collection The collection to copy.
 * @param destination The parent collection to copy the @p collection into.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> copyCollection(const Collection &collection, const Collection &destination, Session *session = nullptr);

/**
 * Retrieves collection statistics from the Akonadi storage.
 *
 * @param collection The collection of which to retrieve statistics.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<CollectionStatistics> fetchCollectionStatistics(const Collection &collection, Session *session = nullptr);

class TagFetchOptionsPrivate;
/**
 * Additional configuration for Tag retrieval.
 */
class AKONADICORE_EXPORT TagFetchOptions
{
public:
    TagFetchOptions();
    TagFetchOptions(const TagFetchScope &fetchScope);
    TagFetchOptions(const TagFetchOptions &);
    TagFetchOptions &operator=(const TagFetchOptions &);
    TagFetchOptions(TagFetchOptions &&) noexcept;
    TagFetchOptions &operator=(TagFetchOptions &&) noexcept;
    ~TagFetchOptions();

    /**
     * Returns reference to current tag fetch scope.
     */
    TagFetchScope &fetchScope();
    const TagFetchScope &fetchScope() const;
    /**
     * Replaces the current tag fetch scope with @p fetchScope.
     */
    void setFetchScope(const TagFetchScope &fetchScope);

private:
    QSharedDataPointer<TagFetchOptionsPrivate> d;
};

/**
 * Retrieves the specified tag from the Akonadi storage.
 *
 * @param tag The tag to retrieve.
 * @param options Additional options to configure the retrieval.
 *
 * @see fetchTags()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Tag> fetchTag(const Tag &tag, const TagFetchOptions &options, Session *session = nullptr);

/**
 * Retrieves the specified tags from the Akonadi storage.
 *
 * @param tags Tags to retrieve.
 * @param options Additional options to configure the retrieval.
 *
 * @see fetchTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Tag::List> fetchTags(const Tag::List &tags, const TagFetchOptions &options, Session *session = nullptr);

/**
 * Retrieves all tags from the Akonadi storage.
 *
 * @see fetchTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Tag::List> fetchAllTags(const TagFetchOptions &options, Session *session = nullptr);

/**
 * Creates a new tag in the Akonadi storage.
 *
 * @param tag The tag to create
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Tag> createTag(const Tag &tag, Session *session = nullptr);

/**
 * Updates the given @p tag in the Akonadi storage.
 *
 * @param tag The tag to update.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Tag> updateTag(const Tag &tag, Session *session = nullptr);

/**
 * Permanently deletes the given @p tag from Akonadi storage.
 *
 * This will untag all Items tagged with the tag and a corresponding update
 * notification will be sent out for all affected Items.
 *
 * @param tag The tag to delete.
 *
 * @see deleteTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteTag(const Tag &tag, Session *session = nullptr);

/**
 * Permanently deletes the given @p tags from Akonadi storage.
 *
 * This will untag all Items tagged with the tags and a corresponding update
 * notification will be sent out for all affected Items.
 *
 * @param tags Tags to delete.
 *
 * @see deleteTag()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteTags(const Tag::List &tags, Session *session = nullptr);

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
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Relation::List> fetchRelation(const Relation &relation, Session *session = nullptr);

/**
 * Retrieves all Relations with the specified types.
 *
 * @param types The types of which Relations should be retrieved.
 *
 * @see fetchRelation()
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Relation::List> fetchRelationsOfTypes(const QVector<QByteArray> &types, Session *session = nullptr);

/**
 * Stores the @p relation in the Akonadi storage.
 *
 * @param relation The new relation to store.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Relation> createRelation(const Relation &relation, Session *session = nullptr);

/**
 * Permanently deletes the given @p relation from Akonadi storage.
 *
 * A corresponding change notifiction will be emitted for all affected Items.
 *
 * @param relation The relation to delete.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> deleteRelation(const Relation &relation, Session *session = nullptr);

/**
 * Creates a collection with persistent search query.
 *
 * @param name Name of the created collection.
 * @param searchQuery Search query whose result will be linked to the
 *        collection.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<Collection> createSearchCollection(const QString &name, const SearchQuery &searchQuery, const ItemSearchOptions &options, Session *session = nullptr);

/**
 * Begins a new transaction in the storage backend.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> beginTransaction(Session *session = nullptr);

/**
 * Commits current transaction in the storage backend.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> commitTransaction(Session *session = nullptr);

/**
 * Rolls back current transaction in the storage backend.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> rollbackTransaction(Session *session = nullptr);

/**
 * @internal
 *
 * Marks the current Session as belonging to a Resource.
 *
 * This is an internal method that should never be used by clients. It is
 * present purely to make the interface complete.
 *
 * @param identifier Resource instance identifier.
 * @param parent A parent Session to run the fetch job on, a parent job to
 *        add this job as a child to, or any QObject.
 *
 * @since 5.12
 */
AKONADICORE_EXPORT
Task<void> selectResource(const QString &identifier, Session *session = nullptr);

} // namespace Akonadi

Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::ItemCreateFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::ItemModifyFlags)

#endif
