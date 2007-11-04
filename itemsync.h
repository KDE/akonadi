/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEMSYNC_H
#define AKONADI_ITEMSYNC_H

#include <libakonadi/item.h>
#include <libakonadi/transactionjobs.h>

namespace Akonadi {

class Collection;

/**
  @internal

  Syncs remote and local items.
*/
class ItemSync : public TransactionSequence
{
  Q_OBJECT

  public:
    /**
      Creates a new item synchronizer.

      @param collection The collection we are syncing.
      @param parent The parent object.
    */
    explicit ItemSync( const Collection &collection, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~ItemSync();

    /**
      Sets the result of a full remote item listing.
      @param remoteItems A list of items.
      Important: All of these need a unique remote identifier.
    */
    void setRemoteItems( const Item::List &remoteItems );

    /**
      Sets the result of an incremental remote item listing.
      @param changedItems A list of remotely added or changed items.
      @param removedItems A list of remotely deleted items.
    */
    void setRemoteItems( const Item::List &changedItems,
                         const Item::List &removedItems );

  protected:
    void doStart();

  private:
    void createLocalItem( const Item &item );
    void checkDone();

  private Q_SLOTS:
    void slotLocalListDone( KJob *job );
    void slotLocalChangeDone( KJob *job );

  private:
    class Private;
    Private* const d;
};

}

#endif
