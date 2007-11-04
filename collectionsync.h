/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONSYNC_H
#define AKONADI_COLLECTIONSYNC_H

#include <libakonadi/collection.h>
#include <libakonadi/transactionjobs.h>

namespace Akonadi {

/**
  @internal

  Syncs remote and local collections.
*/
class CollectionSync : public TransactionSequence
{
  Q_OBJECT

  public:
    /**
      Creates a new collection synchronzier.
      @param resourceId The identifier of the resource we are syncing.
      @param parent The parent object.
    */
    explicit CollectionSync( const QString &resourceId, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~CollectionSync();

    /**
      Sets the result of a full remote collection listing.
      @param remoteCollections A list of collections.
      Important: All of these need a unique remote identifier and parent remote
      identifier.
    */
    void setRemoteCollections( const Collection::List &remoteCollections );

    /**
      Sets the result of an incremental remote collection listing.
      @param changedCollections A list of remotely added or changed collections.
      @param removedCollections A lost of remotely deleted collections.
    */
    void setRemoteCollections( const Collection::List &changedCollections,
                               const Collection::List &removedCollections );

  protected:
    void doStart();

  private:
    void createLocalCollection( const Collection &c, const Collection &parent );
    void checkDone();

  private Q_SLOTS:
    void slotLocalListDone( KJob *job );
    void slotLocalCreateDone( KJob *job );
    void slotLocalChangeDone( KJob *job );

  private:
    class Private;
    Private* const d;
};

}

#endif
