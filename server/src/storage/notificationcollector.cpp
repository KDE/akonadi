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

#include "notificationcollector.h"
#include "storage/datastore.h"
#include "storage/entity.h"

#include <QDebug>

Akonadi::NotificationItem::NotificationItem(int uid, const QByteArray & collection,
                                            const QByteArray & mimeType,
                                            const QByteArray & resource) :
  mType( Item ),
  mUid( uid ),
  mCollection( collection ),
  mMimeType( mimeType ),
  mResource( resource )
{
}

Akonadi::NotificationItem::NotificationItem(const QByteArray & collection,
                                            const QByteArray & resource) :
  mType( Collection ),
  mCollection( collection ),
  mResource( resource )
{
}

bool Akonadi::NotificationItem::isComplete() const
{
  if ( mCollection.isEmpty() || mResource.isEmpty() )
    return false;

  if ( mType == Item && mMimeType.isEmpty() )
    return false;

  return true;
}

bool Akonadi::NotificationItem::operator ==(const NotificationItem & item) const
{
  if ( mType != item.mType )
    return false;
  if ( mType == Item )
    return mUid == item.mUid;
  if ( mType == Collection )
    return mCollection == item.mCollection;
  return false;
}



Akonadi::NotificationCollector::NotificationCollector(DataStore * db) :
  QObject( db ),
  mDb( db )
{
  connect( db, SIGNAL(transactionCommitted()), SLOT(transactionCommitted()) );
  connect( db, SIGNAL(transactionRolledBack()), SLOT(transactionRolledBack()) );
}

Akonadi::NotificationCollector::~NotificationCollector()
{
}

void Akonadi::NotificationCollector::itemAdded( int uid, const QByteArray & collection,
                                                const QByteArray & mimeType,
                                                const QByteArray & resource )
{
  NotificationItem ni( uid, collection, mimeType, resource );
  if ( mDb->inTransaction() )
    mAddedItems.append( ni );
  else {
    completeItem( ni );
    emit itemAddedNotification( ni.uid(), ni.collection(), ni.mimeType(), ni.resource() );
  }
}

void Akonadi::NotificationCollector::itemChanged( int uid, const QByteArray & collection,
                                                  const QByteArray & mimeType,
                                                  const QByteArray & resource )
{
  NotificationItem ni( uid, collection, mimeType, resource );
  if ( mDb->inTransaction() )
    mChangedItems.append( ni );
  else {
    completeItem( ni );
    emit itemChangedNotification( ni.uid(), ni.collection(), ni.mimeType(), ni.resource() );
  }
}

void Akonadi::NotificationCollector::itemRemoved( int uid, const QByteArray & collection,
                                                  const QByteArray & mimeType,
                                                  const QByteArray & resource )
{
  if ( mDb->inTransaction() )
    mRemovedItems.append( NotificationItem( uid, collection, mimeType, resource ) );
  else
    emit itemRemovedNotification( uid, collection, mimeType, resource );
}

void Akonadi::NotificationCollector::collectionAdded( const QByteArray &collection,
                                                      const QByteArray &resource )
{
  NotificationItem ni( collection, resource );
  if ( mDb->inTransaction() )
    mAddedCollections.append( ni );
  else {
    completeItem( ni );
    emit collectionAddedNotification( ni.collection(), ni.resource() );
  }
}

void Akonadi::NotificationCollector::collectionChanged( const QByteArray &collection,
                                                        const QByteArray &resource )
{
  NotificationItem ni( collection, resource );
  if ( mDb->inTransaction() )
    mChangedCollections.append( ni );
  else {
    completeItem( ni );
    emit collectionChangedNotification( ni.collection(), ni.resource() );
  }
}

void Akonadi::NotificationCollector::collectionRemoved( const QByteArray &collection,
                                                        const QByteArray &resource )
{
  if ( mDb->inTransaction() )
    mRemovedCollections.append( NotificationItem( collection, resource ) );
  else
    emit collectionRemovedNotification( collection, resource );
}

void Akonadi::NotificationCollector::completeItem(NotificationItem & item)
{
  if ( item.isComplete() )
    return;

  if ( item.type() == NotificationItem::Collection ) {
    Location loc = mDb->locationByName( item.collection() );
    Resource res = mDb->resourceById( loc.resourceId() );
    item.setResource( res.resource().toLatin1() );
  }

  if ( item.type() == NotificationItem::Item ) {
    PimItem pi = mDb->pimItemById( item.uid() );
    if ( !pi.isValid() )
      return;
    Location loc;
    if ( item.collection().isEmpty() ) {
      loc = mDb->locationById( pi.locationId() );
      item.setCollection( loc.location().toLatin1() );
    }
    if ( item.mimeType().isEmpty() ) {
      MimeType mt = mDb->mimeTypeById( pi.mimeTypeId() );
      item.setMimeType( mt.mimeType().toLatin1() );
    }
    if ( item.resource().isEmpty() ) {
      if ( !loc.isValid() )
        loc = mDb->locationById( pi.locationId() );
      Resource res = mDb->resourceById( loc.resourceId() );
      item.setResource( res.resource().toLatin1() );
    }
  }
}

void Akonadi::NotificationCollector::transactionCommitted()
{
  // TODO: remove duplicates from the lists

  foreach ( NotificationItem ni, mAddedCollections ) {
    completeItem( ni );
    emit collectionAddedNotification( ni.collection(), ni.resource() );
    // no change notifications for new collections
    mChangedCollections.removeAll( ni );
  }

  foreach ( NotificationItem ni, mRemovedCollections ) {
    emit collectionRemovedNotification( ni.collection(), ni.resource() );
    // no change notifications for removed collections
    mChangedCollections.removeAll( ni );
  }

  foreach ( NotificationItem ni, mChangedCollections ) {
    completeItem( ni );
    emit collectionChangedNotification( ni.collection(), ni.resource() );
  }


  foreach ( NotificationItem ni, mAddedItems ) {
    completeItem( ni );
    emit itemAddedNotification( ni.uid(), ni.collection(), ni.mimeType(), ni.resource() );
    // no change notifications for new items
    mChangedItems.removeAll( ni );
  }

  foreach ( NotificationItem ni, mRemovedItems ) {
    emit itemRemovedNotification( ni.uid(), ni.collection(), ni.mimeType(), ni.resource() );
    // no change notifications for removed items
    mChangedItems.removeAll( ni );
  }

  foreach ( NotificationItem ni, mChangedItems ) {
    completeItem( ni );
    emit itemChangedNotification( ni.uid(), ni.collection(), ni.mimeType(), ni.resource() );
  }
}

void Akonadi::NotificationCollector::transactionRolledBack()
{
  mAddedItems.clear();
  mChangedItems.clear();
  mRemovedItems.clear();
  mAddedCollections.clear();
  mChangedCollections.clear();
  mRemovedCollections.clear();
}

#include "notificationcollector.moc"
