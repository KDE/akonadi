/*
 * SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "akonadicore_export.h"

#include <KJob>

#include <memory>

namespace Akonadi
{
class AgentInstance;
class ResourceSynchronizationJobPrivate;

/*!
 * \brief Job that synchronizes a resource.
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
 * \code
 * using namespace Akonadi;
 *
 * const AgentInstance resource = AgentManager::self()->instance( "myresourceidentifier" );
 *
 * ResourceSynchronizationJob *job = new ResourceSynchronizationJob( resource );
 * connect( job, SIGNAL(result(KJob*)), SLOT(synchronizationFinished(KJob*)) );
 * job->start();
 *
 * \endcode
 *
 * \
ote This is a KJob, not an Akonadi::Job, so it won't auto-start!
 *
 * \author Volker Krause <vkrause@kde.org>
 * \since 4.4
 */
class AKONADICORE_EXPORT ResourceSynchronizationJob : public KJob
{
    Q_OBJECT

public:
    /*!
     * Creates a new synchronization job for the given resource.
     *
     * \a instance The resource instance to synchronize.
     */
    explicit ResourceSynchronizationJob(const AgentInstance &instance, QObject *parent = nullptr);

    /*!
     * Destroys the synchronization job.
     */
    ~ResourceSynchronizationJob() override;

    /*!
     * Returns whether a full synchronization will be done, or just the collection tree (without items).
     * The default is \\ false, i.e. a full sync will be requested.
     *
     * \since 4.8
     */
    [[nodiscard]] bool collectionTreeOnly() const;

    /*!
     * Sets the collectionTreeOnly property.
     *
     * \a collectionTreeOnly If set, only the collection tree will be synchronized.
     * \since 4.8
     */
    void setCollectionTreeOnly(bool collectionTreeOnly);

    /*!
     * Returns the resource that has been synchronized.
     */
    [[nodiscard]] AgentInstance resource() const;

    /* reimpl */
    void start() override;

    /*
     * \since 5.1
     */
    void setTimeoutCountLimit(int count);
    /*!
     */
    [[nodiscard]] int timeoutCountLimit() const;

private:
    friend class ResourceSynchronizationJobPrivate;
    std::unique_ptr<ResourceSynchronizationJobPrivate> const d;
};

}
