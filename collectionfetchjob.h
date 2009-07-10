/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONFETCHJOB_H
#define AKONADI_COLLECTIONFETCHJOB_H

#include "akonadi_export.h"
#include <akonadi/collection.h>
#include <akonadi/job.h>

namespace Akonadi {

class CollectionFetchJobPrivate;

/**
 * @short Job that fetches collections from the Akonadi storage.
 *
 * This class can be used to retrieve the complete or partial collection tree
 * from the Akonadi storage.
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * // fetching all collections recursive, starting at the root collection
 * CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
 * if ( job->exec() ) {
 *   Collection::List collections = job->collections();
 *   foreach( const Collection &collection, collections ) {
 *     qDebug() << "Name:" << collection.name();
 *   }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CollectionFetchJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Describes the type of fetch depth.
     */
    enum Type
    {
      Base,       ///< Only fetch the base collection.
      FirstLevel, ///< Only list direct sub-collections of the base collection.
      Recursive   ///< List all sub-collections.
    };

    /**
     * Creates a new collection fetch job. If the given base collection
     * has a unique identifier, this is used to identify the collection in the
     * Akonadi server. If only a remote identifier is avaiable the collection
     * is identified using that, given a resource search context has been
     * specified. There two ways of doing that: by calling setResource()
     * or globally using Akonadi::ResourceSelectJob.
     *
     * @param collection The base collection for the listing.
     * @param type The type of fetch depth.
     * @param parent The parent object.
     */
    explicit CollectionFetchJob( const Collection &collection, Type type = FirstLevel, QObject *parent = 0 );

    /**
     * Creates a new collection fetch job to retrieve a list of collections.
     * The same rules for identifiers apply as noted in the constructor above.
     *
     * @param collections A list of collections to fetch. Must not be empty.
     * @param parent The parent object.
     */
    explicit CollectionFetchJob( const Collection::List &collections, QObject *parent = 0 );

    /**
     * Destroys the collection fetch job.
     */
    virtual ~CollectionFetchJob();

    /**
     * Returns the list of fetched collection.
     */
    Collection::List collections() const;

    /**
     * Sets a resource identifier to limit collection listing to one resource.
     *
     * @param resource The resource identifier.
     */
    void setResource( const QString &resource );

    /**
     * Sets a content mimetype filter, that is include only collections in the result
     * that match the given filter.
     * @param contentMimeTypes A list of content mimetypes.
     *
     * @since 4.4
     */
    void setContentMimeTypes( const QStringList &contentMimeTypes );

    /**
     * Include also unsubscribed collections.
     */
    void includeUnsubscribed( bool include = true );

    /**
     * Include also statistics about the collections.
     *
     * @since 4.3
     */
    void includeStatistics( bool include = true );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the job has received collections.
     *
     * @param collections The received collections.
     */
    void collectionsReceived( const Akonadi::Collection::List &collections );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  protected Q_SLOTS:
    //@cond PRIVATE
    void slotResult( KJob* job );
    //@endcond

  private:
    Q_DECLARE_PRIVATE( CollectionFetchJob )

    //@cond PRIVATE
    Q_PRIVATE_SLOT( d_func(), void timeout() )
    //@endcond
};

}

#endif
