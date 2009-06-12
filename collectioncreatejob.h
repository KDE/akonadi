/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONCREATEJOB_H
#define AKONADI_COLLECTIONCREATEJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class CollectionCreateJobPrivate;

/**
 * @short Job that creates a new collection in the Akonadi storage.
 *
 * This job creates a new collection with all the set properties.
 * You have to use setParentCollection() to define the collection, the
 * new collection shall be located in.
 *
 * @code
 *
 * // create a new top-level collection
 * Akonadi::Collection collection;
 * collection.setParent( Collection::root() );
 * collection.setName( "Events" );
 * collection.setContentMimeTypes( QStringList( "text/calendar" ) );
 *
 * Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( collection );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(createResult(KJob*)) );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CollectionCreateJob : public Job
{
  Q_OBJECT
  public:
    /**
     * Creates a new collection create job.
     *
     * @param collection The new collection. @p collection must have a parent collection
     * set with a unique identifier. If a resource context is specified in the current session
     * (that is you are using it within Akonadi::ResourceBase), the parent collection can be
     * identified by its remote identifier as well.
     * @param parent The parent object.
     */
    explicit CollectionCreateJob( const Collection &collection, QObject *parent = 0 );

    /**
     * Destroys the collection create job.
     */
    virtual ~CollectionCreateJob();

    /**
     * Returns the created collection if the job was executed succesfull.
     */
    Collection collection() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( CollectionCreateJob )
};

}

#endif
