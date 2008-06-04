/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMDELETEJOB_H
#define AKONADI_ITEMDELETEJOB_H

#include "akonadi_export.h"

#include <akonadi/job.h>

namespace Akonadi {

class Item;
class ItemDeleteJobPrivate;

/**
 * @short Job that deletes an item from the Akonadi storage.
 *
 * This job removes the given item from the Akonadi storage.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::Item item = ...
 *
 * ItemDeleteJob *job = new ItemDeleteJob( item );
 *
 * if ( job->exec() )
 *   qDebug() << "Item deleted successfully";
 * else
 *   qDebug() << "Error occurred";
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT ItemDeleteJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new item delete job.
     *
     * @param item The item to delete.
     * @param parent The parent object.
     */
    explicit ItemDeleteJob( const Item &item, QObject *parent = 0 );

    /**
     * Destroys the item delete job.
     */
    ~ItemDeleteJob();

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( ItemDeleteJob )

    //@cond PRIVATE
    Q_PRIVATE_SLOT( d_func(), void jobDone( KJob* ) )
    //@endcond
};

}

#endif
