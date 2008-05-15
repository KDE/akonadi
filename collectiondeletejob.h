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

#ifndef AKONADI_COLLECTIONDELETEJOB_H
#define AKONADI_COLLECTIONDELETEJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class CollectionDeleteJobPrivate;

/**
 * @short Job that deletes a collection in the Akonadi storage.
 *
 * This job deletes a collection and all its sub-collections as well as all associated content.
 *
 * @code
 *
 * Akonadi::Collection collection = ...
 *
 * Akonadi::CollectionDeleteJob *job = new Akonadi::CollectionDeleteJob( collection );
 *
 * if ( job->exec() )
 *   qDebug() << "Deleted successfully";
 * else
 *   qDebug() << "Error occured";
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CollectionDeleteJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection delete job.
     *
     * @param collection The collection to delete.
     * @param parent The parent object.
     */
    explicit CollectionDeleteJob( const Collection &collection, QObject *parent = 0 );

    /**
     * Destroys the collection delete job.
     */
    ~CollectionDeleteJob();

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( CollectionDeleteJob )
};

}

#endif
