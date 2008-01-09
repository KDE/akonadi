/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
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
      progress( 0 ),
      incremental( false )
    {
    }

    Collection syncCollection;

    // local: mapped remote id -> item, id -> item
    QHash<QString, Akonadi::Item> localItems;
    QSet<Akonadi::Item> unprocessedLocalItems;

    // remote items
    Akonadi::Item::List remoteItems;

    // removed remote items
    Item::List removedRemoteItems;

    // create counter
    int pendingJobs;
    int progress;

    bool incremental;
};

ItemSync::ItemSync( const Collection &collection, QObject *parent ) :
    /*TransactionSequence*/ Job( parent ),
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
  d->remoteItems = remoteItems;
  setTotalAmount( KJob::Bytes, d->remoteItems.count() );
}

void ItemSync::setRemoteItems( const Item::List & changedItems, const Item::List & removedItems )
{
  d->incremental = true;
  d->remoteItems = changedItems;
  d->removedRemoteItems = removedItems;
  setTotalAmount( KJob::Bytes, d->remoteItems.count() + d->removedRemoteItems.count() );
}

void ItemSync::doStart()
{
  ItemFetchJob* job = new ItemFetchJob( d->syncCollection, this );
  // FIXME this will deadlock, we only can fetch parts already in the cache
//   if ( !d->incremental )
//     job->fetchAllParts();

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
      kWarning( 5250 ) << "Item " << remoteItem.reference().id() << " does not have a remote identifier - skipping";
      d->progress++;
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
      if ( localItem.flags() != remoteItem.flags() ) { // Check whether the flags differ
        kDebug( 5250 ) << "Local flags "  << localItem.flags()
                       << "remote flags " << remoteItem.flags();
        needsUpdate = true;
      } else {
        /*
         * Check whether the new item contains unknown parts
         */
        const QStringList localParts = localItem.availableParts();
        QStringList missingParts = localParts;
        foreach( const QString remotePart, remoteItem.availableParts() )
          missingParts.removeAll( remotePart );
        if ( !missingParts.isEmpty() )
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

      Item i( remoteItem );
      i.setReference( DataReference( localItem.reference().id(), remoteItem.reference().remoteId() ) );
      i.setRev( localItem.rev() );
      ItemStoreJob *mod = new ItemStoreJob( (const Item)i, this );
      mod->storePayload();
      mod->setCollection( d->syncCollection );
      connect( mod, SIGNAL( result( KJob* ) ), SLOT( slotLocalChangeDone( KJob* ) ) );
    } else {
      d->progress++;
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

void ItemSync::createLocalItem( const Item & item )
{
  d->pendingJobs++;
  ItemAppendJob *create = new ItemAppendJob( item, d->syncCollection, this );
  connect( create, SIGNAL( result( KJob* ) ), SLOT( slotLocalChangeDone( KJob* ) ) );
}

void ItemSync::checkDone()
{
  setProcessedAmount( KJob::Bytes, d->progress );
  if ( d->pendingJobs > 0 )
    return;
//   commit();
  emitResult();
}

void ItemSync::slotLocalChangeDone( KJob * job )
{
  if ( job->error() )
    return;

  d->pendingJobs--;
  d->progress++;
  checkDone();
}

#include "itemsync.moc"
