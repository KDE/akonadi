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

#ifndef AKONADI_ITEMMOVEJOB_H
#define AKONADI_ITEMMOVEJOB_H

#include <akonadi/job.h>


namespace Akonadi {

class Collection;
class Item;
class ItemMoveJobPrivate;

/**
  Moves items into a different collection.
*/
class AKONADI_EXPORT ItemMoveJob : public Job
{
  Q_OBJECT
  public:
    /**
      Move the given item into collection @p target.
      @param item The item to move.
      @param target The target collection.
      @param parent The parent object.
    */
    ItemMoveJob( const Item &item, const Collection &target, QObject *parent = 0 );

    /**
      Destructor.
    */
    ~ItemMoveJob();

  protected:
    void doStart();

  private:
    Q_DECLARE_PRIVATE( ItemMoveJob )
};

}

#endif
