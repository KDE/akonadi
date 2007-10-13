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

#ifndef AKONADI_ITEMSTOREJOB_H
#define AKONADI_ITEMSTOREJOB_H

#include "libakonadi_export.h"
#include <libakonadi/collection.h>
#include <libakonadi/item.h>
#include <libakonadi/job.h>

namespace Akonadi {

/**
  Modifies an existing Item.
*/
class AKONADI_EXPORT ItemStoreJob : public Job
{
  Q_OBJECT

  public:
    /**
      Stores the given item.
      By default only the meta data is stored, you need to explicitly enable storing
      of the payload data.
      @param item A reference to the Item object to store.
      @param parent The parent object.
    */
    explicit ItemStoreJob( Item &item, QObject *parent = 0 );

    /**
      Stores data at the item.
      By default only the meta data is stored, you need to explicitly enable storing
      of the payload data.
      @param item The Item object to store.
      @param parent The parent object.
    */
    explicit ItemStoreJob( const Item &item, QObject *parent = 0 );

    /**
      Destroys this job.
     */
    virtual ~ItemStoreJob();

    /**
      Store the payload data.
    */
    void storePayload();

    /**
      Disable revision checking.
    */
    void noRevCheck();

    /**
      Sets the item flags to @p flags.
      @param flags The new item flags.
    */
    void setFlags( const Item::Flags &flags );

    /**
      Adds the given flag. Existing flags will not be changed.
      @param flag The flag to be added.
    */
    void addFlag( const Item::Flag &flag );

    /**
      Removes the given flag. Other already set flags will not be changed.
      @param flag The flag to be removed.
    */
    void removeFlag( const Item::Flag &flag );

    /**
      Removes the given part. Other existing parts will not be changed.
      @param part The part to be removed.
    */
    void removePart( const QByteArray &part );

    /**
      Moves the item to the given collection.
      @param collection Path to the new collection this item is moved into.
    */
    void setCollection( const Collection &collection );

    /**
      Resets the item dirty flag. Should only be used by resources after
      writing changes back to the corresponding server.
    */
    void setClean();

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    class Private;
    Private* const d;

};

}

#endif
