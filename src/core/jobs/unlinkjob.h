/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_UNLINKJOB_H
#define AKONADI_UNLINKJOB_H

#include "akonadicore_export.h"
#include "job.h"
#include "item.h"

namespace Akonadi {

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
    UnlinkJob(const Collection &collection, const Item::List &items, QObject *parent = 0);

    /**
     * Destroys the unlink job.
     */
    ~UnlinkJob();

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(UnlinkJob)
    template <typename T> friend class LinkJobImpl;
};

}

#endif
