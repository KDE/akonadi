/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_PERSISTENTSEARCHATTRIBUTE_H
#define AKONADI_PERSISTENTSEARCHATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

namespace Akonadi {

class Collection;

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
    PersistentSearchAttribute();

    /**
     * Destroys the persistent search attribute.
     */
    ~PersistentSearchAttribute();

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
    QList<qint64> queryCollections() const;

    /**
     * Sets collections to be queried.
     * @param collections List of collections to be queries
     * @since 4.13
     */
    void setQueryCollections(const QList<Collection> &collections);

    /**
     * Sets IDs of collections to be queries
     * @param collectionsIDs IDs of collections to query
     * @since 4.13
     */
    void setQueryCollections(const QList<qint64> &collectionsIds);

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

    //@cond PRIVATE
    virtual QByteArray type() const Q_DECL_OVERRIDE;
    virtual Attribute *clone() const Q_DECL_OVERRIDE;
    virtual QByteArray serialized() const Q_DECL_OVERRIDE;
    virtual void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;
    //@endcond

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
