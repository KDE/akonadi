/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
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
    explicit CollectionDeleteJob(const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the collection delete job.
     */
    ~CollectionDeleteJob() override;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionDeleteJob)
};

}

