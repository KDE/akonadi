/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef ITEMFETCHSCOPE_H
#define ITEMFETCHSCOPE_H

#include "akonadi_export.h"

#include <QtCore/QSharedDataPointer>

class QStringList;
template <typename T> class QSet;

namespace Akonadi {

class ItemFetchScopePrivate;

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
class AKONADI_EXPORT ItemFetchScope
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
    ItemFetchScope( const ItemFetchScope &other );

    /**
     * Destroys the item fetch scope.
     */
    ~ItemFetchScope();

    /**
     * Assigns the @p other to this scope and returns a reference to this scope.
     */
    ItemFetchScope &operator=( const ItemFetchScope &other );

    /**
     * Returns the payload parts that should be fetched.
     *
     * @see fetchPayloadPart()
     */
    QSet<QByteArray> payloadParts() const;

    /**
     * Sets which payload parts shall be fetched.
     *
     * @param part The payload part identifier.
     *             Valid values depend on the item type.
     * @param fetch @c true to fetch this part, @c false otherwise.
     */
    void fetchPayloadPart( const QByteArray &part, bool fetch = true );

    /**
     * Returns whether the full payload should be fetched.
     *
     * @see fetchFullPayload()
     */
    bool fullPayload() const;

    /**
     * Sets whether the full payload shall be fetched.
     * The default is @c false.
     *
     * @param fetch @c true if the full payload should be fetched, @c false otherwise.
     */
    void fetchFullPayload( bool fetch = true );

    /**
     * Returns all explicitly fetched attributes.
     *
     * Undefined if fetchAllAttributes() returns true.
     *
     * @see fetchAttribute()
     */
    QSet<QByteArray> attributes() const;

    /**
     * Sets whether the attribute of the given @p type should be fetched.
     *
     * @param type The attribute type to fetch.
     * @param fetch @c true if the attribute should be fetched, @c false otherwise.
     */
    void fetchAttribute( const QByteArray &type, bool fetch = true );

    /**
     * Sets whether the attribute of the requested type should be fetched.
     *
     * @param fetch @c true if the attribute should be fetched, @c false otherwise.
     */
    template <typename T> inline void fetchAttribute( bool fetch = true )
    {
      T dummy;
      fetchAttribute( dummy.type(), fetch );
    }

    /**
     * Returns whether all available attributes should be fetched.
     *
     * @see fetchAllAttributes()
     */
    bool allAttributes() const;

    /**
     * Sets whether all available attributes should be fetched.
     * The default is @c false.
     *
     * @param fetch @c true if all available attributes should be fetched, @c false otherwise.
     */
    void fetchAllAttributes( bool fetch = true );

    /**
     * Returns whether payload data should be requested from remote sources or just
     * from the local cache.
     *
     * @see setCacheOnly()
     */
    bool cacheOnly() const;

    /**
     * Sets whether payload data should be requested from remote sources or just
     * from the local cache.
     *
     * @param cacheOnly @c true if no remote data should be requested,
     * @c false otherwise (the default).
     */
    void setCacheOnly( bool cacheOnly );

    /**
     * Sets how many levels of ancestor collections should be included in the retrieval.
     * The default is AncestorRetrieval::None.
     *
     * @param ancestorDepth The desired ancestor retrieval depth.
     * @since 4.4
     */
    void setAncestorRetrieval( AncestorRetrieval ancestorDepth );

    /**
     * Returns the ancestor retrieval depth.
     *
     * @see setAncestorRetrieval()
     * @since 4.4
     */
    AncestorRetrieval ancestorRetrieval() const;

    /**
     * Returns @c true if there is nothing to fetch.
     */
    bool isEmpty() const;

  private:
    //@cond PRIVATE
    QSharedDataPointer<ItemFetchScopePrivate> d;
    //@endcond
};

}

#endif
