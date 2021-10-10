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
class Collection;
class CollectionAttributesSynchronizationJobPrivate;

/**
 * @short Job that synchronizes the attributes of a collection.
 *
 * This job will trigger a resource to synchronize the attributes of
 * a collection based on what the backend is reporting to store them in the
 * Akonadi storage.
 *
 * Example:
 *
 * @code
 * using namespace Akonadi;
 *
 * const Collection collection = ...;
 *
 * CollectionAttributesSynchronizationJob *job = new CollectionAttributesSynchronizationJob( collection );
 * connect( job, SIGNAL(result(KJob*)), SLOT(synchronizationFinished(KJob*)) );
 *
 * @endcode
 *
 * @note This is a KJob not an Akonadi::Job, so it won't auto-start!
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.6
 */
class AKONADICORE_EXPORT CollectionAttributesSynchronizationJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new synchronization job for the given collection.
     *
     * @param collection The collection to synchronize.
     */
    explicit CollectionAttributesSynchronizationJob(const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the synchronization job.
     */
    ~CollectionAttributesSynchronizationJob() override;

    /* reimpl */
    void start() override;

private:
    /// @cond PRIVATE
    friend class CollectionAttributesSynchronizationJobPrivate;
    std::unique_ptr<CollectionAttributesSynchronizationJobPrivate> const d;
    /// @endcond
};

}

