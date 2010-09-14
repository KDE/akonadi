/*
 * Copyright (c) 2009 Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AKONADI_COLLECTIONATTRIBUTESSYNCHRONIZATIONJOB_H
#define AKONADI_COLLECTIONATTRIBUTESSYNCHRONIZATIONJOB_H

#include "akonadi_export.h"

#include <kjob.h>

namespace Akonadi {

class Collection;
class CollectionAttributesSynchronizationJobPrivate;

/**
 * @short Job that synchronizes a resource.
 *
 * This job will trigger a resource to synchronize the backend it is
 * responsible for (e.g. a local file or a groupware server) with the
 * Akonadi storage.
 *
 * If you only want to trigger the synchronization without being
 * interested in the result, using Akonadi::AgentInstance::synchronize() is enough.
 * If you want to wait until it's finished, use this class.
 *
 * Example:
 *
 * @code
 * using namespace Akonadi;
 *
 * const AgentInstance resource = AgentManager::self()->instance( "myresourceidentifier" );
 *
 * CollectionAttributesSynchronizationJob *job = new CollectionAttributesSynchronizationJob( resource );
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( synchronizationFinished( KJob* ) ) );
 *
 * @endcode
 *
 * @note This is a KJob not an Akonadi::Job, so it wont auto-start!
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.4
 */
class AKONADI_EXPORT CollectionAttributesSynchronizationJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new synchronization job for the given resource.
     *
     * @param instance The resource instance to synchronize.
     */
    explicit CollectionAttributesSynchronizationJob( const Collection &collection, QObject *parent = 0 );

    /**
     * Destroys the synchronization job.
     */
    ~CollectionAttributesSynchronizationJob();

    /**
     * Returns the resource that has been synchronized.
     */
    Collection collection() const;

    /* reimpl */
    void start();

  private:
    //@cond PRIVATE
    CollectionAttributesSynchronizationJobPrivate* const d;
    friend class CollectionAttributesSynchronizationJobPrivate;

    Q_PRIVATE_SLOT( d, void slotSynchronized(qlonglong) )
    Q_PRIVATE_SLOT( d, void slotTimeout() )
    //@endcond
};

}

#endif
