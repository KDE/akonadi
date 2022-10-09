/*
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QSet>
#include <QSharedDataPointer>

#include <QStringList>

namespace Akonadi
{
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
     * Describes the list filter
     *
     * @since 4.14
     */
    enum ListFilter {
        NoFilter, ///< No filtering, retrieve all collections
        Display, ///< Only retrieve collections for display, taking the local preference and enabled into account.
        Sync, ///< Only retrieve collections for synchronization, taking the local preference and enabled into account.
        Index, ///< Only retrieve collections for indexing, taking the local preference and enabled into account.
        Enabled ///< Only retrieve enabled collections, ignoring the local preference.
    };

    /**
     * Sets a filter for the collections to be listed.
     *
     * Note that collections that do not match the filter are included if required to complete the tree.
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
    Q_REQUIRED_RESULT ListFilter listFilter() const;

    /**
     * Returns whether collection statistics should be included in the retrieved results.
     *
     * @see setIncludeStatistics()
     */
    Q_REQUIRED_RESULT bool includeStatistics() const;

    /**
     * Sets whether collection statistics should be included in the retrieved results.
     *
     * @param include @c true to include collection statistics, @c false otherwise (the default).
     */
    void setIncludeStatistics(bool include);

    /**
     * Returns the resource identifier that is used as filter.
     *
     * @see setResource()
     */
    Q_REQUIRED_RESULT QString resource() const;

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
    Q_REQUIRED_RESULT QStringList contentMimeTypes() const;

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
    Q_REQUIRED_RESULT AncestorRetrieval ancestorRetrieval() const;

    /**
     * Sets the fetch scope for ancestor retrieval.
     *
     * @see setAncestorRetrieval()
     */
    void setAncestorFetchScope(const CollectionFetchScope &scope);

    /**
     * Returns the fetch scope for ancestor retrieval.
     */
    Q_REQUIRED_RESULT CollectionFetchScope ancestorFetchScope() const;

    /**
     * Returns the fetch scope for ancestor retrieval.
     */
    CollectionFetchScope &ancestorFetchScope();

    /**
     * Returns all explicitly fetched attributes.
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
    template<typename T>
    inline void fetchAttribute(bool fetch = true)
    {
        T dummy;
        fetchAttribute(dummy.type(), fetch);
    }

    /**
     * Sets whether only the id or the complete tag should be fetched.
     *
     * The default is @c false.
     *
     * @since 4.15
     */
    void setFetchIdOnly(bool fetchIdOnly);

    /**
     * Sets whether only the id of the tags should be retrieved or the complete tag.
     *
     * @see tagFetchScope()
     * @since 4.15
     */
    Q_REQUIRED_RESULT bool fetchIdOnly() const;

    /**
     * Ignore retrieval errors while fetching collections, and always deliver what is available.
     *
     * This flag is useful to fetch a list of collections, where some might no longer be available.
     *
     * @since KF5
     */
    void setIgnoreRetrievalErrors(bool enabled);

    /**
     * Returns whether retrieval errors should be ignored.
     *
     * @see setIgnoreRetrievalErrors()
     * @since KF5
     */
    Q_REQUIRED_RESULT bool ignoreRetrievalErrors() const;

    /**
     * Returns @c true if there is nothing to fetch.
     */
    Q_REQUIRED_RESULT bool isEmpty() const;

private:
    /// @cond PRIVATE
    QSharedDataPointer<CollectionFetchScopePrivate> d;
    /// @endcond
};

}
