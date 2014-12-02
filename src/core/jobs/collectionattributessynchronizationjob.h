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

#include "akonadicore_export.h"

#include <kjob.h>

namespace Akonadi {

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
 * @note This is a KJob not an Akonadi::Job, so it wont auto-start!
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
    explicit CollectionAttributesSynchronizationJob(const Collection &collection, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the synchronization job.
     */
    ~CollectionAttributesSynchronizationJob();

    /* reimpl */
    void start() Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    CollectionAttributesSynchronizationJobPrivate *const d;
    friend class CollectionAttributesSynchronizationJobPrivate;

    Q_PRIVATE_SLOT(d, void slotSynchronized(qlonglong))
    Q_PRIVATE_SLOT(d, void slotTimeout())
    //@endcond
};

}

#endif
