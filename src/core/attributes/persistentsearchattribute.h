/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <memory>

namespace Akonadi
{
class Collection;
class PersistentSearchAttributePrivate;

/**
 * @short An attribute to store query properties of persistent search collections.
 *
 * This attribute is attached to persistent search collections automatically when
 * creating a new persistent search with SearchCreateJob.
 * Later on the search query can be changed by modifying this attribute of the
 * persistent search collection with an CollectionModifyJob.
 *
 * Example:
 *
 * @code
 *
 * const QString name = "My search folder";
 * const QString query = "...";
 *
 * Akonadi::SearchCreateJob *job = new Akonadi::SearchCreateJob( name, query );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   const Collection searchCollection = job->createdCollection();
 *   ...
 *
 *   // now let's change the query
 *   if ( searchCollection.hasAttribute<Akonadi::PersistentSearchAttribute>() ) {
 *     Akonadi::PersistentSearchAttribute *attribute = searchCollection.attribute<Akonadi::PersistentSearchAttribute>();
 *     attribute->setQueryString( "... another query string ..." );
 *
 *     Akonadi::CollectionModifyJob *modifyJob = new Akonadi::CollectionModifyJob( searchCollection );
 *     connect( modifyJob, SIGNAL(result(KJob*)), SLOT(modifyFinished(KJob*)) );
 *   }
 *   ...
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.5
 */
class AKONADICORE_EXPORT PersistentSearchAttribute : public Akonadi::Attribute
{
public:
    /**
     * Creates a new persistent search attribute.
     */
    explicit PersistentSearchAttribute();

    /**
     * Destroys the persistent search attribute.
     */
    ~PersistentSearchAttribute() override;

    /**
     * Returns the query string used for this search.
     */
    QString queryString() const;

    /**
     * Sets the query string to be used for this search.
     * @param query The query string.
     */
    void setQueryString(const QString &query);

    /**
     * Returns IDs of collections that will be queried
     * @since 4.13
     */
    QVector<qint64> queryCollections() const;

    /**
     * Sets collections to be queried.
     * @param collections List of collections to be queries
     * @since 4.13
     */
    void setQueryCollections(const QVector<Collection> &collections);

    /**
     * Sets IDs of collections to be queries
     * @param collectionsIds IDs of collections to query
     * @since 4.13
     */
    void setQueryCollections(const QVector<qint64> &collectionsIds);

    /**
     * Sets whether resources should be queried too.
     *
     * When set to true, Akonadi will search local indexed items and will also
     * query resources that support server-side search, to forward the query
     * to remote storage (for example using SEARCH feature on IMAP servers) and
     * merge their results with results from local index.
     *
     * This is useful especially when searching resources, that don't fetch full
     * payload by default, for example the IMAP resource, which only fetches headers
     * by default and the body is fetched on demand, which means that emails that
     * were not yet fully fetched cannot be indexed in local index, and thus cannot
     * be searched. With remote search, even those emails can be included in search
     * results.
     *
     * @param enabled Whether remote search is enabled
     * @since 4.13
     */
    void setRemoteSearchEnabled(bool enabled);

    /**
     * Returns whether remote search is enabled.
     *
     * @since 4.13
     */
    bool isRemoteSearchEnabled() const;

    /**
     * Sets whether the search should recurse into collections
     *
     * When set to true, all child collections of the specific collections will
     * be search recursively.
     *
     * @param recursive Whether to search recursively
     * @since 4.13
     */
    void setRecursive(bool recursive);

    /**
     * Returns whether the search is recursive
     *
     * @since 4.13
     */
    bool isRecursive() const;

    /// @cond PRIVATE
    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;
    /// @endcond

private:
    /// @cond PRIVATE
    const std::unique_ptr<PersistentSearchAttributePrivate> d;
    /// @endcond
};

} // namespace Akonadi

