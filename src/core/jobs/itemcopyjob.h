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

#ifndef AKONADI_ITEMCOPYJOB_H
#define AKONADI_ITEMCOPYJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi {

class Collection;
class ItemCopyJobPrivate;

/**
 * @short Job that copies a set of items to a target collection in the Akonadi storage.
 *
 * The job can be used to copy one or several Item objects to another collection.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::Item::List items = ...
 * Akonadi::Collection collection = ...
 *
 * Akonadi::ItemCopyJob *job = new Akonadi::ItemCopyJob( items, collection );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Items copied successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemCopyJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new item copy job.
     *
     * @param item The item to copy.
     * @param target The target collection.
     * @param parent The parent object.
     */
    ItemCopyJob(const Item &item, const Collection &target, QObject *parent = Q_NULLPTR);

    /**
     * Creates a new item copy job.
     *
     * @param items A list of items to copy.
     * @param target The target collection.
     * @param parent The parent object.
     */
    ItemCopyJob(const Item::List &items, const Collection &target, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the item copy job.
     */
    ~ItemCopyJob();

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(ItemCopyJob)
};

}

#endif
