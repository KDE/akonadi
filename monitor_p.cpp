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

// @cond PRIVATE

#include "monitor_p.h"

#include "collectionfetchjob.h"
#include "collectionstatistics.h"
#include "itemfetchjob.h"
#include "notificationmessage_p.h"
#include "session.h"

#include <kdebug.h>

using namespace Akonadi;

MonitorPrivate::MonitorPrivate(Monitor * parent) :
  q_ptr( parent ),
  nm( 0 ),
  monitorAll( false ),
  fetchCollection( false ),
  fetchCollectionStatistics( false )
{
}

bool MonitorPrivate::connectToNotificationManager()
{
  NotificationMessage::registerDBusTypes();

  if ( !nm )
    nm = new org::freedesktop::Akonadi::NotificationManager( QLatin1String( "org.freedesktop.Akonadi" ),
                                                     QLatin1String( "/notifications" ),
                                                     QDBusConnection::sessionBus(), q_ptr );
  else
    return true;

  if ( !nm ) {
    kWarning( 5250 ) << "Unable to connect to notification manager";
  } else {
    QObject::connect( nm, SIGNAL(notify(Akonadi::NotificationMessage::List)),
             q_ptr, SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
    return true;
  }
  return false;
}

bool MonitorPrivate::acceptNotification(const NotificationMessage & msg)
{
  if ( isSessionIgnored( msg.sessionId() ) )
    return false;
  switch ( msg.type() ) {
    case NotificationMessage::InvalidType:
      kWarning( 5250 ) << "Received invalid change notification!";
      return false;
    case NotificationMessage::Item:
      return isItemMonitored( msg.uid(), msg.parentCollection(), msg.parentDestCollection(), msg.mimeType(), msg.resource() )
          || isCollectionMonitored( msg.parentCollection(), msg.resource() )
          || isCollectionMonitored( msg.parentDestCollection(), msg.resource() );
    case NotificationMessage::Collection:
      return isCollectionMonitored( msg.uid(), msg.resource() );
  }
  Q_ASSERT( false );
  return false;
}

bool MonitorPrivate::processNotification(const NotificationMessage & msg)
{
  if ( !acceptNotification( msg ) )
    return false;

  if ( msg.type() == NotificationMessage::Item ) {
    notifyCollectionStatisticsWatchers( msg.parentCollection(), msg.resource() );
    if ( !mItemFetchScope.isEmpty() &&
          ( msg.operation() == NotificationMessage::Add || msg.operation() == NotificationMessage::Move ) ) {
      Item item( msg.uid() );
      item.setRemoteId( msg.remoteId() );

      ItemCollectionFetchJob *job = new ItemCollectionFetchJob( item, msg.parentCollection(), q_ptr );
      job->setFetchScope( mItemFetchScope );
      pendingJobs.insert( job, msg );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotItemJobFinished(KJob*)) );
      return true;
    }
    if ( !mItemFetchScope.isEmpty() && msg.operation() == NotificationMessage::Modify ) {
      Item item( msg.uid() );
      item.setRemoteId( msg.remoteId() );
      ItemFetchJob *job = new ItemFetchJob( item, q_ptr );
      job->setFetchScope( mItemFetchScope );
      pendingJobs.insert( job, msg );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotItemJobFinished(KJob*)) );
      return true;
    }
    emitItemNotification( msg );
    return true;

  } else if ( msg.type() == NotificationMessage::Collection ) {
    if ( msg.operation() != NotificationMessage::Remove && fetchCollection ) {
      Collection::List list;
      list << Collection( msg.uid() );
      if ( msg.operation() == NotificationMessage::Add )
        list << Collection( msg.parentCollection() );
      CollectionFetchJob *job = new CollectionFetchJob( list, q_ptr );
      pendingJobs.insert( job, msg );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotCollectionJobFinished(KJob*)) );
      return true;
    }
    if ( msg.operation() == NotificationMessage::Remove ) {
      // no need for statistics updates anymore
      recentlyChangedCollections.remove( msg.uid() );
    }
    emitCollectionNotification( msg );
    return true;

  } else {
    kWarning( 5250 ) << "Received unknown change notification!";
  }
  return false;
}

void MonitorPrivate::sessionDestroyed( QObject * object )
{
  Session* session = qobject_cast<Session*>( object );
  if ( session )
    sessions.removeAll( session->sessionId() );
}

void MonitorPrivate::slotStatisticsChangedFinished( KJob* job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Error on fetching collection statistics: " << job->errorText();
  } else {
    CollectionStatisticsJob *statisticsJob = static_cast<CollectionStatisticsJob*>( job );
    emit q_ptr->collectionStatisticsChanged( statisticsJob->collection().id(),
                                             statisticsJob->statistics() );
  }
}

void MonitorPrivate::slotFlushRecentlyChangedCollections()
{
  foreach( Collection::Id collection, recentlyChangedCollections ) {
    if ( fetchCollectionStatistics ) {
      fetchStatistics( collection );
    } else {
      static const CollectionStatistics dummyStatistics;
      emit q_ptr->collectionStatisticsChanged( collection, dummyStatistics );
    }
  }
  recentlyChangedCollections.clear();
}

void MonitorPrivate::slotNotify( const NotificationMessage::List &msgs )
{
  foreach ( const NotificationMessage &msg, msgs )
    processNotification( msg );
}

void MonitorPrivate::emitItemNotification( const NotificationMessage &msg, const Item &item,
                                             const Collection &collection, const Collection &collectionDest  )
{
  Q_ASSERT( msg.type() == NotificationMessage::Item );
  Collection col = collection;
  Collection colDest = collectionDest;
  if ( !col.isValid() ) {
    col = Collection( msg.parentCollection() );
    col.setResource( QString::fromUtf8( msg.resource() ) );
  }
  if ( !colDest.isValid() ) {
    colDest = Collection( msg.parentDestCollection() );
    // FIXME setResource here required ?
  }
  Item it = item;
  if ( !it.isValid() ) {
    it = Item( msg.uid() );
    it.setRemoteId( msg.remoteId() );
    it.setMimeType( msg.mimeType() );
  }
  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit q_ptr->itemAdded( it, col );
      break;
    case NotificationMessage::Modify:
      emit q_ptr->itemChanged( it, msg.itemParts() );
      break;
    case NotificationMessage::Move:
      emit q_ptr->itemMoved( it, col, colDest );
      break;
    case NotificationMessage::Remove:
      emit q_ptr->itemRemoved( it );
      break;
    default:
      Q_ASSERT_X( false, "MonitorPrivate::emitItemNotification()", "Invalid enum value" );
  }
}

void MonitorPrivate::emitCollectionNotification( const NotificationMessage &msg, const Collection &col,
                                                   const Collection &par )
{
  Q_ASSERT( msg.type() == NotificationMessage::Collection );
  Collection collection = col;
  if ( !collection.isValid() ) {
    collection = Collection( msg.uid() );
    collection.setParent( msg.parentCollection() );
    collection.setResource( QString::fromUtf8( msg.resource() ) );
    collection.setRemoteId( msg.remoteId() );
  }
  Collection parent = par;
  if ( !parent.isValid() )
    parent = Collection( msg.parentCollection() );
  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit q_ptr->collectionAdded( collection, parent );
      break;
    case NotificationMessage::Modify:
      emit q_ptr->collectionChanged( collection );
      break;
    case NotificationMessage::Remove:
      emit q_ptr->collectionRemoved( collection );
      break;
    default:
      Q_ASSERT_X( false, "MonitorPrivate::emitCollectionNotification", "Invalid enum value" );
  }
}

void MonitorPrivate::slotItemJobFinished( KJob* job )
{
  if ( !pendingJobs.contains( job ) ) {
    kWarning( 5250 ) << "Unknown job - wtf is going on here?";
    return;
  }
  NotificationMessage msg = pendingJobs.take( job );
  if ( job->error() ) {
    kWarning( 5250 ) << "Error on fetching item:" << job->errorText();
  } else {
    Item item;
    Collection col;
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
    if ( fetchJob && fetchJob->items().count() > 0 )
      item = fetchJob->items().first();
    ItemCollectionFetchJob *cfjob = qobject_cast<ItemCollectionFetchJob*>( job );
    if ( cfjob ) {
      item = cfjob->item();
      col = cfjob->collection();
    }
    emitItemNotification( msg, item, col );
  }
}

void MonitorPrivate::slotCollectionJobFinished( KJob* job )
{
  if ( !pendingJobs.contains( job ) ) {
    kWarning( 5250 ) << "Unknown job - wtf is going on here?";
    return;
  }
  NotificationMessage msg = pendingJobs.take( job );
  if ( job->error() ) {
    kWarning( 5250 ) << "Error on fetching collection:" << job->errorText();
  } else {
    Collection col, parent;
    CollectionFetchJob *listJob = qobject_cast<CollectionFetchJob*>( job );
    if ( listJob && listJob->collections().count() > 0 )
      col = listJob->collections().first();
    if ( listJob && listJob->collections().count() > 1 && msg.operation() == NotificationMessage::Add ) {
      parent = listJob->collections().at( 1 );
      if ( col.id() != msg.uid() )
        qSwap( col, parent );
    }
    emitCollectionNotification( msg, col, parent );
  }
}


ItemCollectionFetchJob::ItemCollectionFetchJob( const Item &item, Collection::Id collectionId, QObject *parent )
  : Job( parent ),
    mReferenceItem( item ), mCollectionId( collectionId )
{
}

ItemCollectionFetchJob::~ItemCollectionFetchJob()
{
}

Item ItemCollectionFetchJob::item() const
{
  return mItem;
}

Collection ItemCollectionFetchJob::collection() const
{
  return mCollection;
}

void ItemCollectionFetchJob::setFetchScope( const ItemFetchScope &fetchScope )
{
  mFetchScope = fetchScope;
}

void ItemCollectionFetchJob::doStart()
{
  CollectionFetchJob *listJob = new CollectionFetchJob( Collection( mCollectionId ), CollectionFetchJob::Base, this );
  connect( listJob, SIGNAL( result( KJob* ) ), SLOT( collectionJobDone( KJob* ) ) );
  addSubjob( listJob );

  ItemFetchJob *fetchJob = new ItemFetchJob( mReferenceItem, this );
  fetchJob->setFetchScope( mFetchScope );
  connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( itemJobDone( KJob* ) ) );
  addSubjob( fetchJob );
}

void ItemCollectionFetchJob::collectionJobDone( KJob* job )
{
  if ( !job->error() ) {
    CollectionFetchJob *listJob = qobject_cast<CollectionFetchJob*>( job );
    if ( listJob->collections().isEmpty() ) {
      setError( 1 );
      setErrorText( QLatin1String( "No collection found" ) );
    } else
      mCollection = listJob->collections().first();
  }
}

void ItemCollectionFetchJob::itemJobDone( KJob* job )
{
  if ( !job->error() ) {
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
    if ( fetchJob->items().isEmpty() ) {
      setError( 2 );
      setErrorText( QLatin1String( "No item found" ) );
    } else
      mItem = fetchJob->items().first();

    emitResult();
  }
}

// @endcond

#include "monitor_p.moc"
