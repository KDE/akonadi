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

#ifndef AKONADI_COLLECTIONCOPYJOB_H
#define AKONADI_COLLECTIONCOPYJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

class Collection;
class CollectionCopyJobPrivate;

/**
 * @short Job that copies a collection into another collection in the Akonadi storage.
 *
 * This job copies a single collection into a specified target collection.
 *
 * @code
 *
 * Akonadi::Collection source = ...
 * Akonadi::Collection target = ...
 *
 * Akonadi::CollectionCopyJob *job = new Akonadi::CollectionCopyJob( source, target );
 * connect( job, SIGNAL(result(KJob*)), SLOT(copyFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::copyFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Copied successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionCopyJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new collection copy job to copy the given @p source collection into @p target.
     *
     * @param source The collection to copy.
     * @param target The target collection.
     * @param parent The parent object.
     */
    CollectionCopyJob(const Collection &source, const Collection &target, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the collection copy job.
     */
    ~CollectionCopyJob();

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(CollectionCopyJob)
};

}

#endif
