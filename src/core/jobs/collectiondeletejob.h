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

#include "akonadicore_export.h"
#include "job.h"

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
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(deletionResult(KJob*)) );
 *
 * @endcode
 *
 * @note This job deletes the data from the backend storage. To delete the collection
 * from the Akonadi storage only, leaving the backend storage unchanged, delete
 * the Agent instead, as follows. (Note that if it's a sub-collection, deleting
 * the agent will also delete its parent collection; in this case the only
 * option is to delete the sub-collection data in both Akonadi and backend
 * storage.)
 *
 * @code
 *
 * const Akonadi::AgentInstance instance =
 *                   Akonadi::AgentManager::self()->instance( collection.resource() );
 * if ( instance.isValid() ) {
 *   Akonadi::AgentManager::self()->removeInstance( instance );
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionDeleteJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new collection delete job. The collection needs to either have a unique
     * identifier or a remote identifier set. Note that using a remote identifier only works
     * in a resource context (that is from within ResourceBase), as remote identifiers
     * are not guaranteed to be globally unique.
     *
     * @param collection The collection to delete.
     * @param parent The parent object.
     */
    explicit CollectionDeleteJob(const Collection &collection, QObject *parent = 0);

    /**
     * Destroys the collection delete job.
     */
    ~CollectionDeleteJob();

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(CollectionDeleteJob)
};

}

#endif
