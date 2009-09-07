/*
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

#ifndef AKONADI_COLLECTIONMOVEJOB_H
#define AKONADI_COLLECTIONMOVEJOB_H

#include "akonadi_export.h"

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class CollectionMoveJobPrivate;

/**
 * @short Job that moves a collection in the Akonadi storage to a new parent collection.
 *
 * This job moves an existing collection to a new parent collection.
 *
 * @code
 *
 * const Akonadi::Collection collection = ...
 * const Akonadi::Collection newParent = ...
 *
 * Akonadi::CollectionMoveJob *job = new Akonadi::CollectionMoveJob( collection, newParent );
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( moveResult( KJob* ) ) );
 *
 * @endcode
 *
 * @since 4.4
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CollectionMoveJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection move job for the given collection and destination
     *
     * @param collection The collection to move.
     * @param destination The destination collection where @p collection should be moved to.
     * @param parent The parent object.
     */
    CollectionMoveJob( const Collection &collection, const Collection &destination, QObject *parent = 0 );

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( CollectionMoveJob )
    template <typename T, typename MoveJob> friend class MoveJobImpl;
};

}

#endif
