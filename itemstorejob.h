/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_ITEMSTOREJOB_H
#define PIM_ITEMSTOREJOB_H

#include <libakonadi/item.h>
#include <libakonadi/job.h>

namespace PIM {

class ItemStoreJobPrivate;

/**
  Stores the item flags.
*/
class ItemStoreJob : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new ItemStoreJob.
      @param item The item which flags should be stored.
      @param parent The parent object.
    */
    ItemStoreJob( Item* item, QObject *parent = 0 );

    /**
      Creates a new ItemStoreJob.
      @param ref The reference of the item to change.
      @param flags The new item flags.
      @param parent The parent object.
    */
    ItemStoreJob( const DataReference &ref, const Item::Flags &flags, QObject *parent = 0 );

    /**
      Destoys this job.
    */
    virtual ~ItemStoreJob();

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    ItemStoreJobPrivate *d;

};

}

#endif
