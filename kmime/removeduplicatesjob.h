/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>
    Copyright (c) 2012 Dan Vrátil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef REMOVEDUPLICATESJOB_H
#define REMOVEDUPLICATESJOB_H

#include <akonadi/job.h>
#include <akonadi/collection.h>

#include "akonadi-kmime_export.h"

class QAbstractItemModel;

namespace Akonadi {

/**
 * @short Job that finds and removes duplicate messages in given collection
 *
 * This jobs compares all messages in given collections by their Message-Id
 * headers and hashes of their bodies and removes duplicates.
 *
 * @since 4.10
 */
class AKONADI_KMIME_EXPORT RemoveDuplicatesJob : public Akonadi::Job
{
    Q_OBJECT

public:
    /**
     * Creates a new job that will remove duplicates in @p folder.
     *
     * @param folder The folder where to search for duplicates
     * @param parent The parent object
     */
    RemoveDuplicatesJob(const Akonadi::Collection &folder, QObject *parent = 0);

    /**
     * Creates a new job that will remove duplicates in all @p folders.
     *
     * @param folders Folders where to search for duplicates
     * @param parent The parent object
     */
    RemoveDuplicatesJob(const Akonadi::Collection::List &folders, QObject *parent);

    /**
     * Destroys the job.
     */
    virtual ~RemoveDuplicatesJob();

protected:
    virtual void doStart();
    virtual bool doKill();

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void slotFetchDone(KJob *job))
    Q_PRIVATE_SLOT(d, void slotDeleteDone(KJob *job))
};

} /* namespace Akonadi */

#endif // REMOVEDUPLICATESJOB_H
