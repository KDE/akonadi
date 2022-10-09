/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class UnlinkJobPrivate;

/**
 * @short Job that unlinks items inside the Akonadi storage.
 *
 * This job allows you to remove references to a set of items in a virtual
 * collection.
 *
 * Example:
 *
 * @code
 *
 * // Unlink the given items from the given collection
 * const Akonadi::Collection virtualCollection = ...
 * const Akonadi::Item::List items = ...
 *
 * Akonadi::UnlinkJob *job = new Akonadi::UnlinkJob( virtualCollection, items );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Unlinked items successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.2
 * @see LinkJob
 */
class AKONADICORE_EXPORT UnlinkJob : public Job
{
    Q_OBJECT
public:
    /**
     * Creates a new unlink job.
     *
     * The job will remove references to the given items from the given collection.
     *
     * @param collection The collection from which the references should be removed.
     * @param items The items of which the references should be removed.
     * @param parent The parent object.
     */
    UnlinkJob(const Collection &collection, const Item::List &items, QObject *parent = nullptr);

    /**
     * Destroys the unlink job.
     */
    ~UnlinkJob() override;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(UnlinkJob)
    template<typename T>
    friend class LinkJobImpl;
};

}
