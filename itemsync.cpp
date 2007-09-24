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

#include "itemsync.h"

#include <libakonadi/item.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>

#include <kdebug.h>

using namespace Akonadi;

class ItemSync::Private
{
  public:
    Private() :
      pendingJobs( 0 ),
      incremental( false )
    {
    }

    Collection syncCollection;

    // local: mapped remote id -> item, id -> item
    QHash<QString, Akonadi::Item> localItems;
    QSet<Akonadi::Item> unprocessedLocalItems;

    // remote: mapped id -> collection
    QHash<int, Akonadi::Item> remoteItems;

    // removed remote items
    Item::List removedRemoteItems;

    // create counter
    int pendingJobs;

    bool incremental;
};

ItemSync::ItemSync( const Collection &collection, QObject *parent ) :
    Job( parent ),
    d( new Private )
{
  d->syncCollection = collection;
}

ItemSync::~ItemSync()
{
  delete d;
}

void ItemSync::setRemoteItems( const Item::List & remoteItems )
{
  foreach ( const Item item, remoteItems ) {
    d->remoteItems.insert( item.reference().id(), item );
  }
}

void ItemSync::setRemoteItems( const Item::List & changedItems, const Item::List & removedItems )
{
  d->incremental = true;
  foreach ( const Item item, changedItems ) {
    d->remoteItems.insert( item.reference().id(), item );
  }

  d->removedRemoteItems = removedItems;
}

void ItemSync::doStart()
{
  ItemFetchJob* job = new ItemFetchJob( d->syncCollection, this );
  if ( !d->incremental )
    job->fetchAllParts();

  connect( job, SIGNAL( result( KJob* ) ), SLOT( slotLocalListDone( KJob* ) ) );
}

void ItemSync::slotLocalListDone( KJob * job )
{
  if ( job->error() )
    return;

  const Item::List list = static_cast<ItemFetchJob*>( job )->items();
  foreach ( const Item item, list ) {
    d->localItems.insert( item.reference().remoteId(), item );
    d->unprocessedLocalItems.insert( item );
  }

  // added / updated
  foreach ( const Item remoteItem, d->remoteItems ) {
    if ( remoteItem.reference().remoteId().isEmpty() ) {
      kWarning() << k_funcinfo << "Item " << remoteItem.reference().id() << " does not have a remote identifier - skipping";
      continue;
    }

    const Item localItem = d->localItems.value( remoteItem.reference().remoteId() );
    d->unprocessedLocalItems.remove( localItem );
    // missing locally
    if ( !localItem.isValid() ) {
      createLocalItem( remoteItem );
      continue;
    }

    bool needsUpdate = false;

    if ( d->incremental ) {
      /**
       * We know that this item has changed (as it is part of the
       * incremental changed list), so we just put it into the
       * storage.
       */
      needsUpdate = true;
    } else {
      if ( localItem.flags() != remoteItem.flags() ) // Check whether the flags differ
        needsUpdate = true;
      else if ( localItem.mimeType() != remoteItem.mimeType() ) // Check whether the mime-type differs
        needsUpdate = true;
      else {
        /**
         * Check whether the available part identifiers differ.
         */
        QStringList localParts = localItem.availableParts();
        localParts.sort();

        QStringList remoteParts = remoteItem.availableParts();
        remoteParts.sort();
        if ( localParts != remoteParts )
          needsUpdate = true;
        else {
          /**
           * If the available part identifiers don't differ, check
           * whether the content of the parts differs.
           */
          foreach ( const QString partId, localParts ) {
            if ( localItem.part( partId ) != remoteItem.part( partId ) ) {
              needsUpdate = true;
              break;
            }
          }
        }
      }
    }

    if ( needsUpdate ) {
      d->pendingJobs++;

      ItemStoreJob *mod = new ItemStoreJob( remoteItem, this );
      mod->storePayload();
      mod->setCollection( d->syncCollection );
      connect( mod, SIGNAL( result( KJob* ) ), SLOT( slotLocalChangeDone( KJob* ) ) );
    }
  }

  // removed
  if ( !d->incremental )
    d->removedRemoteItems = d->unprocessedLocalItems.toList();

  foreach ( const Item item, d->removedRemoteItems ) {
    d->pendingJobs++;
    ItemDeleteJob *job = new ItemDeleteJob( item.reference(), this );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( slotLocalChangeDone( KJob* ) ) );
  }
  d->localItems.clear();

  checkDone();
}

void ItemSync::slotLocalCreateDone( KJob * job )
{
  d->pendingJobs--;
  if ( job->error() )
    return;

  checkDone();
}

void ItemSync::createLocalItem( const Item & item )
{
  d->pendingJobs++;
  ItemAppendJob *create = new ItemAppendJob( item, d->syncCollection, this );
  connect( create, SIGNAL( result( KJob* ) ), SLOT( slotLocalCreateDone( KJob* ) ) );
}

void ItemSync::checkDone()
{
  // still running jobs
  if ( d->pendingJobs > 0 )
    return;

  emitResult();
}

void ItemSync::slotLocalChangeDone( KJob * job )
{
  if ( job->error() )
    return;

  d->pendingJobs--;
  checkDone();
}

#include "itemsync.moc"
