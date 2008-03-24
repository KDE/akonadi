/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMMODIFYJOB_H
#define AKONADI_ITEMMODIFYJOB_H

#include "akonadi_export.h"

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class ItemModifyJobPrivate;

/**
  Modifies an existing Item.
*/
class AKONADI_EXPORT ItemModifyJob : public Job
{
  Q_OBJECT

  public:
    /**
      Stores data at the item.
      By default only the meta data is stored, you need to explicitly enable storing
      of the payload data.
      @param item The Item object to store.
      @param parent The parent object.
    */
    explicit ItemModifyJob( const Item &item, QObject *parent = 0 );

    /**
      Destroys this job.
     */
    virtual ~ItemModifyJob();

    /**
      Store the payload data.
    */
    void storePayload();
    //FIXME_API:(volker) remove this method

    /**
      Disable revision checking.
    */
    void disableRevisionCheck();

    /**
      Moves the item to the given collection.
      @param collection Path to the new collection this item is moved into.
    */
    //FIXME_API: remove this method in favour of an ItemMoveJob.
    KDE_DEPRECATED void setCollection( const Collection &collection );

    /**
      Resets the item dirty flag. Should only be used by resources after
      writing changes back to the corresponding server.
    */
    //FIXME_API: move to private class, make ResourceBase a friend class
    void setClean();

    /**
      Returns the stored item (including the changed revision number),
    */
    Item item() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemModifyJob )
};

}

#endif
