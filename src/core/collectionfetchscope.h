/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONFETCHSCOPE_H
#define AKONADI_COLLECTIONFETCHSCOPE_H

#include "akonadicore_export.h"

#include <QtCore/QSharedDataPointer>

class QStringList;

namespace Akonadi {

class CollectionFetchScopePrivate;

/**
 * @short Specifies which parts of a collection should be fetched from the Akonadi storage.
 *
 * When collections are fetched from the server either by using CollectionFetchJob
 * explicitly or when it is being used internally by other classes, e.g. Akonadi::Monitor,
 * the scope of the fetch operation can be tailored to the application's current needs.
 *
 * Note that CollectionFetchScope always includes fetching collection attributes.
 *
 * There are two supported ways of changing the currently active CollectionFetchScope
 * of classes:
 * - in-place: modify the CollectionFetchScope object the other class holds as a member
 * - replace: replace the other class' member with a new scope object
 *
 * Example: modifying a CollectionFetchJob's scope @c in-place
 * @code
 * Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( collection );
 * job->fetchScope().setIncludeUnsubscribed( true );
 * @endcode
 *
 * Example: @c replacing a CollectionFetchJob's scope
 * @code
 * Akonadi::CollectionFetchScope scope;
 * scope.setIncludeUnsubscribed( true );
 *
 * Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( collection );
 * job->setFetchScope( scope );
 * @endcode
 *
 * This class is implicitly shared.
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.4
 */
class AKONADICORE_EXPORT CollectionFetchScope
{
public:
    /**
     * Describes the ancestor retrieval depth.
     */
    enum AncestorRetrieval {
        None, ///< No ancestor retrieval at all (the default)
        Parent, ///< Only retrieve the immediate parent collection
        All ///< Retrieve all ancestors, up to Collection::root()
    };

    /**
     * Creates an empty collection fetch scope.
     *
     * Using an empty scope will only fetch the very basic meta data of collections,
     * e.g. local id, remote id and content mimetypes.
     */
    CollectionFetchScope();

    /**
     * Creates a new collection fetch scope from an @p other.
     */
    CollectionFetchScope(const CollectionFetchScope &other);

    /**
     * Destroys the collection fetch scope.
     */
    ~CollectionFetchScope();

    /**
     * Assigns the @p other to this scope and returns a reference to this scope.
     */
    CollectionFetchScope &operator=(const CollectionFetchScope &other);

    /**
     * Returns whether unsubscribed collection should be included.
     *
     * @see setIncludeUnsubscribed()
     * @since 4.5
     * @deprecated use listFilter() instead
     */
    AKONADI_DEPRECATED bool includeUnsubscribed() const;

    /**
     * Sets whether unsubscribed collections should be included in the collection listing.
     *
     * @param include @c true to include unsubscribed collections, @c false otherwise (the default).
     * @deprecated use setListFilter() instead
     */
    AKONADI_DEPRECATED void setIncludeUnsubscribed(bool include);

    /**
     * Describes the list filter
     *
     * @since 4.14
     */
    enum ListFilter {
        NoFilter, ///< No filtering, retrieve all collections
        Display,  ///< Only retrieve collections for display, taking the local preference and enabled into account.
        Sync,     ///< Only retrieve collections for synchronization, taking the local preference and enabled into account.
        Index,    ///< Only retrieve collections for indxing, taking the local preference and enabled into account.
        Enabled   ///< Only retrieve enabled collections, ignoring the local preference. This is the same as setIncludeUnsubscribed(false).
    };

    /**
     * Sets a filter for the collections to be listed.
     *
     * Note that collections that are required to complete the tree, but are not part of the collection are still included in the listing.
     *
     * @since 4.14
     */
    void setListFilter(ListFilter);

    /**
     * Returns the list filter.
     *
     * @see setListFilter()
     * @since 4.14
     */
    ListFilter listFilter() const;

    /**
     * Returns whether collection statistics should be included in the retrieved results.
     *
     * @see setIncludeStatistics()
     */
    bool includeStatistics() const;

    /**
     * Sets whether collection statistics should be included in the retrieved results.
     *
     * @param include @c true to include collction statistics, @c false otherwise (the default).
     */
    void setIncludeStatistics(bool include);

    /**
     * Returns the resource identifier that is used as filter.
     *
     * @see setResource()
     */
    QString resource() const;

    /**
     * Sets a resource filter, that is only collections owned by the specified resource are
     * retrieved.
     *
     * @param resource The resource identifier.
     */
    void setResource(const QString &resource);

    /**
     * Sets a content mimetypes filter, that is only collections that contain at least one of the
     * given mimetypes (or their parents) are retrieved.
     *
     * @param mimeTypes A list of mime types
     */
    void setContentMimeTypes(const QStringList &mimeTypes);

    /**
     * Returns the content mimetypes filter.
     *
     * @see setContentMimeTypes()
     */
    QStringList contentMimeTypes() const;

    /**
     * Sets how many levels of ancestor collections should be included in the retrieval.
     *
     * Only the ID and the remote ID of the ancestor collections are fetched. If
     * you want more information about the ancestor collections, like their name,
     * you will need to do an additional CollectionFetchJob for them.
     *
     * @param ancestorDepth The desired ancestor retrieval depth.
     */
    void setAncestorRetrieval(AncestorRetrieval ancestorDepth);

    /**
     * Returns the ancestor retrieval depth.
     *
     * @see setAncestorRetrieval()
     */
    AncestorRetrieval ancestorRetrieval() const;

    /**
     * Returns @c true if there is nothing to fetch.
     */
    bool isEmpty() const;

private:
    //@cond PRIVATE
    QSharedDataPointer<CollectionFetchScopePrivate> d;
    //@endcond
};

}

#endif
