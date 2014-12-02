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

#ifndef AKONADI_RESOURCESYNCHRONIZATIONJOB_H
#define AKONADI_RESOURCESYNCHRONIZATIONJOB_H

#include "akonadicore_export.h"

#include <kjob.h>

namespace Akonadi {

class AgentInstance;
class ResourceSynchronizationJobPrivate;

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
 * ResourceSynchronizationJob *job = new ResourceSynchronizationJob( resource );
 * connect( job, SIGNAL(result(KJob*)), SLOT(synchronizationFinished(KJob*)) );
 * job->start();
 *
 * @endcode
 *
 * @note This is a KJob, not an Akonadi::Job, so it won't auto-start!
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.4
 */
class AKONADICORE_EXPORT ResourceSynchronizationJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new synchronization job for the given resource.
     *
     * @param instance The resource instance to synchronize.
     */
    explicit ResourceSynchronizationJob(const AgentInstance &instance, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the synchronization job.
     */
    ~ResourceSynchronizationJob();

    /**
     * Returns whether a full synchronization will be done, or just the collection tree (without items).
     * The default is @c false, i.e. a full sync will be requested.
     *
     * @since 4.8
     */
    bool collectionTreeOnly() const;

    /**
     * Sets the collectionTreeOnly property.
     *
     * @param collectionTreeOnly If set, only the collection tree will be synchronized.
     * @since 4.8
     */
    void setCollectionTreeOnly(bool collectionTreeOnly);

    /**
     * Returns the resource that has been synchronized.
     */
    AgentInstance resource() const;

    /* reimpl */
    void start() Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    ResourceSynchronizationJobPrivate *const d;
    friend class ResourceSynchronizationJobPrivate;

    Q_PRIVATE_SLOT(d, void slotSynchronized())
    Q_PRIVATE_SLOT(d, void slotTimeout())
    //@endcond
};

}

#endif
