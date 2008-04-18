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

#include "collection.h"
#include "item.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"

#include <kdebug.h>

#include <QtCore/QStringList>

using namespace Akonadi;

class ItemSync::Private
{
  public:
    Private( ItemSync *parent ) :
      q( parent ),
      mPendingJobs( 0 ),
      mProgress( 0 ),
      mTotalItems( 0 ),
      mTotalItemsProcessed( 0 ),
      mIncremental( false ),
      mParts( false )
    {
    }

    void createLocalItem( const Item &item );
    void checkDone();
    void slotLocalListDone( KJob* );
    void slotLocalChangeDone( KJob* );
    bool itemNeedsUpdate( const Item &localItem, const Item &remoteItem );
    void execute();

    ItemSync *q;
    Collection mSyncCollection;
    QHash<Item::Id, Akonadi::Item> mLocalItemsById;
    QHash<QString, Akonadi::Item> mLocalItemsByRemoteId;
    QSet<Akonadi::Item> mUnprocessedLocalItems;

    // remote items
    Akonadi::Item::List mRemoteItems;

    // removed remote items
    Item::List mRemovedRemoteItems;

    // create counter
    int mPendingJobs;
    int mProgress;
    int mTotalItems;
    int mTotalItemsProcessed;

    bool mIncremental;
    bool mParts;
};

void ItemSync::Private::createLocalItem( const Item & item )
{
  mPendingJobs++;
  ItemCreateJob *create = new ItemCreateJob( item, mSyncCollection, q );
  q->connect( create, SIGNAL( result( KJob* ) ), q, SLOT( slotLocalChangeDone( KJob* ) ) );
}

void ItemSync::Private::checkDone()
{
  q->setProcessedAmount( KJob::Bytes, mProgress );
  if ( mPendingJobs > 0 )
    return;

  q->commit();
}

ItemSync::ItemSync( const Collection &collection, QObject *parent ) :
    TransactionSequence( parent ),
    d( new Private( this ) )
{
  d->mSyncCollection = collection;
  ItemFetchJob* job = new ItemFetchJob( d->mSyncCollection, this );
  // FIXME this will deadlock, we only can fetch parts already in the cache
//   if ( !d->incremental )
//     job->fetchAllParts();

  connect( job, SIGNAL( result( KJob* ) ), SLOT( slotLocalListDone( KJob* ) ) );
}

ItemSync::~ItemSync()
{
  delete d;
}

void ItemSync::setFullSyncItems( const Item::List &items )
{
  d->mRemoteItems = items;
  setTotalAmount( KJob::Bytes, d->mRemoteItems.count() );
  d->execute();
}

void ItemSync::setTotalItems( int amount )
{
  kDebug() << amount;
  d->mParts = true;
  d->mTotalItems = amount;
  setTotalAmount( KJob::Bytes, amount );
}

void ItemSync::setPartSyncItems( const Item::List &items )
{
  d->mRemoteItems += items;
  d->mTotalItemsProcessed += items.count();
  kDebug() << "Received: " << items.count() << "In total: " << d->mTotalItemsProcessed << " Wanted: " << d->mTotalItems;
  if ( d->mTotalItemsProcessed == d->mTotalItems )
    d->execute();
}

void ItemSync::setIncrementalSyncItems( const Item::List &changedItems, const Item::List &removedItems )
{
  d->mIncremental = true;
  d->mRemoteItems = changedItems;
  d->mRemovedRemoteItems = removedItems;
  setTotalAmount( KJob::Bytes, d->mRemoteItems.count() + d->mRemovedRemoteItems.count() );
  d->execute();
}

void ItemSync::doStart()
{
}

bool ItemSync::Private::itemNeedsUpdate( const Item &localItem, const Item &remoteItem )
{
  // Check whether the flags differ
  if ( localItem.flags() != remoteItem.flags() ) {
    kDebug( 5250 ) << "Local flags "  << localItem.flags()
                    << "remote flags " << remoteItem.flags();
    return true;
  }

  // Check whether the new item contains unknown parts
  QSet<QByteArray> missingParts = localItem.loadedPayloadParts();
  missingParts.subtract( remoteItem.loadedPayloadParts() );
  if ( !missingParts.isEmpty() )
    return true;

  // ### FIXME SLOW!!!
  // If the available part identifiers don't differ, check
  // whether the content of the payload differs
  if ( localItem.payloadData() != remoteItem.payloadData() )
    return true;

  // check if remote attributes have been changed
  foreach ( Attribute* attr, remoteItem.attributes() ) {
    if ( !localItem.hasAttribute( attr->type() ) )
      return true;
    if ( attr->serialized() != localItem.attribute( attr->type() )->serialized() )
      return true;
  }

  return false;
}

void ItemSync::Private::slotLocalListDone( KJob * job )
{
  if ( job->error() )
    return;

  const Item::List list = static_cast<ItemFetchJob*>( job )->items();
  foreach ( const Item item, list ) {
    mLocalItemsById.insert( item.id(), item );
    mLocalItemsByRemoteId.insert( item.remoteId(), item );
    mUnprocessedLocalItems.insert( item );
  }
}

void ItemSync::Private::execute()
{
  // added / updated
  foreach ( const Item remoteItem, mRemoteItems ) {
#ifndef NDEBUG
    if ( remoteItem.remoteId().isEmpty() ) {
      kWarning( 5250 ) << "Item " << remoteItem.id() << " does not have a remote identifier";
    }
#endif

    Item localItem = mLocalItemsById.value( remoteItem.id() );
    if ( !localItem.isValid() )
      localItem = mLocalItemsByRemoteId.value( remoteItem.remoteId() );
    mUnprocessedLocalItems.remove( localItem );
    // missing locally
    if ( !localItem.isValid() ) {
      createLocalItem( remoteItem );
      continue;
    }

    bool needsUpdate = false;

    if ( mIncremental ) {
      /**
       * We know that this item has changed (as it is part of the
       * incremental changed list), so we just put it into the
       * storage.
       */
      needsUpdate = true;
    } else {
      needsUpdate = itemNeedsUpdate( localItem, remoteItem );
    }

    if ( needsUpdate ) {
      mPendingJobs++;

      Item i( remoteItem );
      i.setId( localItem.id() );
      i.setRemoteId( remoteItem.remoteId() );
      i.setRevision( localItem.revision() );
      i.setFlags( remoteItem.flags() );
      ItemModifyJob *mod = new ItemModifyJob( i, q );
      q->connect( mod, SIGNAL( result( KJob* ) ), q, SLOT( slotLocalChangeDone( KJob* ) ) );
    } else {
      mProgress++;
    }
  }

  // removed
  if ( !mIncremental )
    mRemovedRemoteItems = mUnprocessedLocalItems.toList();

  foreach ( const Item item, mRemovedRemoteItems ) {
    mPendingJobs++;
    ItemDeleteJob *job = new ItemDeleteJob( item, q );
    q->connect( job, SIGNAL( result( KJob* ) ), q, SLOT( slotLocalChangeDone( KJob* ) ) );
  }
  mLocalItemsById.clear();
  mLocalItemsByRemoteId.clear();

  checkDone();
}

void ItemSync::Private::slotLocalChangeDone( KJob * job )
{
  if ( job->error() )
    return;

  mPendingJobs--;
  mProgress++;

  checkDone();
}

#include "itemsync.moc"
