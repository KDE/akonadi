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

#include <libakonadi/collection.h>
#include <libakonadi/item.h>
#include <libakonadi/job.h>
#include <kdepim_export.h>

namespace Akonadi {

/**
  Modifies an existing Item.
*/
class AKONADI_EXPORT ItemStoreJob : public Job
{
  Q_OBJECT

  public:
    /**
      Modifies the item with the identifier @p ref.
      @param ref The reference of the item to change.
      @param parent The parent object.
    */
    explicit ItemStoreJob( const DataReference &ref, QObject *parent = 0 );

    /**
      Destoys this job.
     */
    virtual ~ItemStoreJob();

    /**
      Set the item data to @p data.
      @param data The new item data.
    */
    void setData( const QByteArray &data );

    /**
      Sets the item flags to @p flags.
      @param flags the new item flags.
    */
    void setFlags( const Item::Flags &flags );

    /**
      Adds the given flag. Existing flags will not be changed.
      @param flag the flag to be added
    */
    void addFlag( const Item::Flag &flag );

    /**
      Removes the given flag. Other already set flags will not be changed.
      @param flag the flag to be removed
    */
    void removeFlag( const Item::Flag &flag );

    /**
      Moves the item to the given collection.
      @param collection Path to the new collection this item is moved into.
    */
    void setCollection( const Collection &collection );

    /**
      Sets the remote id based on the DataReference object given in the
      constructor.
      Use for resources to update the remote id after adding an item to the server.
    */
    void setRemoteId();

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
