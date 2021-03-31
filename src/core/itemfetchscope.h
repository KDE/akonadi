/*
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QDateTime>
#include <QMetaType>
#include <QSet>
#include <QSharedDataPointer>

template<typename T> class QSet;

namespace Akonadi
{
class ItemFetchScopePrivate;
class TagFetchScope;

/**
 * @short Specifies which parts of an item should be fetched from the Akonadi storage.
 *
 * When items are fetched from server either by using ItemFetchJob explicitly or
 * when it is being used internally by other classes, e.g. ItemModel, the scope
 * of the fetch operation can be tailored to the application's current needs.
 *
 * There are two supported ways of changing the currently active ItemFetchScope
 * of classes:
 * - in-place: modify the ItemFetchScope object the other class holds as a member
 * - replace: replace the other class' member with a new scope object
 *
 * Example: modifying an ItemFetchJob's scope @c in-place
 * @code
 * Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( collection );
 * job->fetchScope().fetchFullPayload();
 * job->fetchScope().fetchAttribute<MyAttribute>();
 * @endcode
 *
 * Example: @c replacing an ItemFetchJob's scope
 * @code
 * Akonadi::ItemFetchScope scope;
 * scope.fetchFullPayload();
 * scope.fetchAttribute<MyAttribute>();
 *
 * Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( collection );
 * job->setFetchScope( scope );
 * @endcode
 *
 * This class is implicitly shared.
 *
 * @author Kevin Krammer <kevin.krammer@gmx.at>
 */
class AKONADICORE_EXPORT ItemFetchScope
{
public:
    /**
     * Describes the ancestor retrieval depth.
     * @since 4.4
     */
    enum AncestorRetrieval {
        None, ///< No ancestor retrieval at all (the default)
        Parent, ///< Only retrieve the immediate parent collection
        All ///< Retrieve all ancestors, up to Collection::root()
    };

    /**
     * Creates an empty item fetch scope.
     *
     * Using an empty scope will only fetch the very basic meta data of items,
     * e.g. local id, remote id and mime type
     */
    ItemFetchScope();

    /**
     * Creates a new item fetch scope from an @p other.
     */
    ItemFetchScope(const ItemFetchScope &other);

    /**
     * Destroys the item fetch scope.
     */
    ~ItemFetchScope();

    /**
     * Assigns the @p other to this scope and returns a reference to this scope.
     */
    ItemFetchScope &operator=(const ItemFetchScope &other);

    /**
     * Returns the payload parts that should be fetched.
     *
     * @see fetchPayloadPart()
     */
    Q_REQUIRED_RESULT QSet<QByteArray> payloadParts() const;

    /**
     * Sets which payload parts shall be fetched.
     *
     * @param part The payload part identifier.
     *             Valid values depend on the item type.
     * @param fetch @c true to fetch this part, @c false otherwise.
     */
    void fetchPayloadPart(const QByteArray &part, bool fetch = true);

    /**
     * Returns whether the full payload should be fetched.
     *
     * @see fetchFullPayload()
     */
    Q_REQUIRED_RESULT bool fullPayload() const;

    /**
     * Sets whether the full payload shall be fetched.
     * The default is @c false.
     *
     * @param fetch @c true if the full payload should be fetched, @c false otherwise.
     */
    void fetchFullPayload(bool fetch = true);

    /**
     * Returns all explicitly fetched attributes.
     *
     * Undefined if fetchAllAttributes() returns true.
     *
     * @see fetchAttribute()
     */
    Q_REQUIRED_RESULT QSet<QByteArray> attributes() const;

    /**
     * Sets whether the attribute of the given @p type should be fetched.
     *
     * @param type The attribute type to fetch.
     * @param fetch @c true if the attribute should be fetched, @c false otherwise.
     */
    void fetchAttribute(const QByteArray &type, bool fetch = true);

    /**
     * Sets whether the attribute of the requested type should be fetched.
     *
     * @param fetch @c true if the attribute should be fetched, @c false otherwise.
     */
    template<typename T> inline void fetchAttribute(bool fetch = true)
    {
        T dummy;
        fetchAttribute(dummy.type(), fetch);
    }

    /**
     * Returns whether all available attributes should be fetched.
     *
     * @see fetchAllAttributes()
     */
    Q_REQUIRED_RESULT bool allAttributes() const;

    /**
     * Sets whether all available attributes should be fetched.
     * The default is @c false.
     *
     * @param fetch @c true if all available attributes should be fetched, @c false otherwise.
     */
    void fetchAllAttributes(bool fetch = true);

    /**
     * Returns whether payload data should be requested from remote sources or just
     * from the local cache.
     *
     * @see setCacheOnly()
     */
    Q_REQUIRED_RESULT bool cacheOnly() const;

    /**
     * Sets whether payload data should be requested from remote sources or just
     * from the local cache.
     *
     * @param cacheOnly @c true if no remote data should be requested,
     * @c false otherwise (the default).
     */
    void setCacheOnly(bool cacheOnly);

    /**
     * Sets whether payload will be fetched or there will be only a test performed if the
     * requested payload is in the cache. Calling it calls @see setCacheOnly with true automatically.
     * Default is fetching the data.
     *
     * @since 4.11
     */
    void setCheckForCachedPayloadPartsOnly(bool check = true);

    /**
     * Returns whether payload data should be fetched or only checked for presence in the cache.
     *
     * @see setCheckForCachedPayloadPartsOnly()
     *
     * @since 4.11
     */
    Q_REQUIRED_RESULT bool checkForCachedPayloadPartsOnly() const;

    /**
     * Sets how many levels of ancestor collections should be included in the retrieval.
     * The default is AncestorRetrieval::None.
     *
     * @param ancestorDepth The desired ancestor retrieval depth.
     * @since 4.4
     */
    void setAncestorRetrieval(AncestorRetrieval ancestorDepth);

    /**
     * Returns the ancestor retrieval depth.
     *
     * @see setAncestorRetrieval()
     * @since 4.4
     */
    Q_REQUIRED_RESULT AncestorRetrieval ancestorRetrieval() const;

    /**
     * Enables retrieval of the item modification time.
     * This is enabled by default for backward compatibility reasons.
     *
     * @param retrieveMtime @c true to retrieve the modification time, @c false otherwise
     * @since 4.6
     */
    void setFetchModificationTime(bool retrieveMtime);

    /**
     * Returns whether item modification time should be retrieved.
     *
     * @see setFetchModificationTime()
     * @since 4.6
     */
    Q_REQUIRED_RESULT bool fetchModificationTime() const;

    /**
     * Enables retrieval of the item GID.
     * This is disabled by default.
     *
     * @param retrieveGID @c true to retrieve the GID, @c false otherwise
     * @since 4.12
     */
    void setFetchGid(bool retrieveGID);

    /**
     * Returns whether item GID should be retrieved.
     *
     * @see setFetchGid()
     * @since 4.12
     */
    Q_REQUIRED_RESULT bool fetchGid() const;

    /**
     * Ignore retrieval errors while fetching items, and always deliver what is available.
     * If items have missing parts and the part can't be retrieved from the resource (i.e. because the system is offline),
     * the fetch job would normally just fail. By setting this flag, the errors are ignored,
     * and all items which could be fetched completely are returned.
     * Note that all items that are returned are completely fetched, and incomplete items are simply ignored.
     * This flag is useful for displaying everything that is available, where it is not crucial to have all items.
     * Never use this for things like data migration or alike.
     *
     * @since 4.10
     */
    void setIgnoreRetrievalErrors(bool enabled);

    /**
     * Returns whether retrieval errors should be ignored.
     *
     * @see setIgnoreRetrievalErrors()
     * @since 4.10
     */
    Q_REQUIRED_RESULT bool ignoreRetrievalErrors() const;

    /**
     * Returns @c true if there is nothing to fetch.
     */
    Q_REQUIRED_RESULT bool isEmpty() const;

    /**
     * Only fetch items that were added or modified after given timestamp
     *
     * When this property is set, all results are filtered, i.e. even when you
     * request an item with a specific ID, it will not be fetched unless it was
     * modified after @p changedSince timestamp.
     *
     * @param changedSince The timestamp of oldest modified item to fetch
     * @since 4.11
     */
    void setFetchChangedSince(const QDateTime &changedSince);

    /**
     * Returns timestamp of the oldest item to fetch.
     */
    Q_REQUIRED_RESULT QDateTime fetchChangedSince() const;

    /**
     * Fetch remote identification for items.
     *
     * These include Akonadi::Item::remoteId() and Akonadi::Item::remoteRevision(). This should
     * be off for normal clients usually, to save memory (not to mention normal clients should
     * not be concerned with these information anyway). It is however crucial for resource agents.
     * For backward compatibility the default is @c true.
     *
     * @param retrieveRid whether or not to load remote identification.
     * @since 4.12
     */
    void setFetchRemoteIdentification(bool retrieveRid);

    /**
     * Returns whether item remote identification should be retrieved.
     *
     * @see setFetchRemoteIdentification()
     * @since 4.12
     */
    Q_REQUIRED_RESULT bool fetchRemoteIdentification() const;

    /**
     * Fetch tags for items.
     *
     * The fetched tags have only the Tag::id() set and need to be fetched first to access further attributes.
     *
     * The default is @c false.
     *
     * @param fetchTags whether or not to load tags.
     * @since 4.13
     */
    void setFetchTags(bool fetchTags);

    /**
     * Returns whether tags should be retrieved.
     *
     * @see setFetchTags()
     * @since 4.13
     */
    Q_REQUIRED_RESULT bool fetchTags() const;

    /**
     * Sets the tag fetch scope.
     *
     * The TagFetchScope controls how much of an tags's data is fetched
     * from the server.
     *
     * By default setFetchIdOnly is set to true on the tag fetch scope.
     *
     * @param fetchScope The new fetch scope for tag fetch operations.
     * @see fetchScope()
     * @since 4.15
     */
    void setTagFetchScope(const TagFetchScope &fetchScope);

    /**
     * Returns the tag fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the TagFetchScope documentation
     * for an example.
     *
     * By default setFetchIdOnly is set to true on the tag fetch scope.
     *
     * @return a reference to the current tag fetch scope
     *
     * @see setFetchScope() for replacing the current tag fetch scope
     * @since 4.15
     */
    TagFetchScope &tagFetchScope();

    /**
     * Returns the tag fetch scope.
     *
     * By default setFetchIdOnly is set to true on the tag fetch scope.
     *
     * @return a reference to the current tag fetch scope
     *
     * @see setFetchScope() for replacing the current tag fetch scope
     * @since 4.15
     */
    Q_REQUIRED_RESULT TagFetchScope tagFetchScope() const;

    /**
     * Returns whether to fetch list of virtual collections the item is linked to
     *
     * @param fetchVRefs whether or not to fetch virtualc references
     * @since 4.14
     */
    void setFetchVirtualReferences(bool fetchVRefs);

    /**
     * Returns whether virtual references should be retrieved.
     *
     * @see setFetchVirtualReferences()
     * @since 4.14
     */
    Q_REQUIRED_RESULT bool fetchVirtualReferences() const;

    /**
     * Fetch relations for items.
     *
     * The default is @c false.
     *
     * @param fetchRelations whether or not to load relations.
     * @since 4.15
     */
    void setFetchRelations(bool fetchRelations);

    /**
     * Returns whether relations should be retrieved.
     *
     * @see setFetchRelations()
     * @since 4.15
     */
    Q_REQUIRED_RESULT bool fetchRelations() const;

private:
    /// @cond PRIVATE
    QSharedDataPointer<ItemFetchScopePrivate> d;
    /// @endcond
};

}

Q_DECLARE_METATYPE(Akonadi::ItemFetchScope)

