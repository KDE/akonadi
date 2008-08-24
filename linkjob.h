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

#ifndef AKONADI_LINKJOB_H
#define AKONADI_LINKJOB_H

#include <akonadi/job.h>
#include <akonadi/item.h>

namespace Akonadi {

class Collection;
class LinkJobPrivate;

/**
 * This job allows you to create references to a set of items in a virtual
 * collection.
 *
 * @since 4.2
 * @see UnlinkJob
 */
class AKONADI_EXPORT LinkJob : public Job
{
  Q_OBJECT
  public:
    /**
     * Create references to the given items in the given collection.
     * @param collection The collection in which the references should be created.
     * @param items The items of which the references should be created.
     * @param parent The parent object.
     */
    LinkJob( const Collection &collection, const Item::List &items, QObject *parent = 0 );

    /**
     * Destructor.
     */
    ~LinkJob();

  protected:
    void doStart();

  private:
    Q_DECLARE_PRIVATE( LinkJob )
};

}

#endif
