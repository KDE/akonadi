/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
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
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(moveResult(KJob*)) );
 *
 * @endcode
 *
 * @since 4.4
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionMoveJob : public Job
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
    CollectionMoveJob(const Collection &collection, const Collection &destination, QObject *parent = nullptr);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionMoveJob)
};

}
