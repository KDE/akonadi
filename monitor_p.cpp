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

#include "monitor_p.h"

#include "collectionlistjob.h"
#include "itemfetchjob.h"
#include "notificationmessage.h"
#include "session.h"

#include <kdebug.h>

using namespace Akonadi;

MonitorPrivate::MonitorPrivate(Monitor * parent) :
  q_ptr( parent ),
  nm( 0 ),
  monitorAll( false ),
  fetchCollection( false ),
  fetchCollectionStatus( false ),
  fetchAllParts( false )
{
}

bool MonitorPrivate::connectToNotificationManager()
{
  NotificationMessage::registerDBusTypes();

  if ( !nm )
    nm = new org::kde::Akonadi::NotificationManager( QLatin1String( "org.kde.Akonadi" ),
                                                     QLatin1String( "/notifications" ),
                                                     QDBusConnection::sessionBus(), q_ptr );
  else
    return true;

  if ( !nm ) {
    qWarning() << "Unable to connect to notification manager";
  } else {
    QObject::connect( nm, SIGNAL(notify(Akonadi::NotificationMessage::List)),
             q_ptr, SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
    return true;
  }
  return false;
}

bool MonitorPrivate::processNotification(const NotificationMessage & msg)
{
  if ( isSessionIgnored( msg.sessionId() ) )
    return false;

  if ( msg.type() == NotificationMessage::Item ) {
    notifyCollectionStatusWatchers( msg.parentCollection(), msg.resource() );
    if ( !isItemMonitored( msg.uid(), msg.parentCollection(), msg.parentDestCollection(), msg.mimeType(), msg.resource() ) )
      return false;
    if ( (!mFetchParts.isEmpty() || fetchAllParts) &&
          ( msg.operation() == NotificationMessage::Add || msg.operation() == NotificationMessage::Move ) ) {
      ItemCollectionFetchJob *job = new ItemCollectionFetchJob( DataReference( msg.uid(), msg.remoteId() ),
                                                                msg.parentCollection(), q_ptr );
      foreach( QString part, mFetchParts )
        job->addFetchPart( part );
      if ( fetchAllParts )
        job->fetchAllParts();
      pendingJobs.insert( job, msg );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotItemJobFinished(KJob*)) );
      return true;
    }
    if ( (!mFetchParts.isEmpty() || fetchAllParts) && msg.operation() == NotificationMessage::Modify ) {
      ItemFetchJob *job = new ItemFetchJob( DataReference( msg.uid(), msg.remoteId() ), q_ptr );
      foreach( QString part, mFetchParts )
        job->addFetchPart( part );
      if ( fetchAllParts )
        job->fetchAllParts();
      pendingJobs.insert( job, msg );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotItemJobFinished(KJob*)) );
      return true;
    }
    emitItemNotification( msg );
    return true;

  } else if ( msg.type() == NotificationMessage::Collection ) {
    if ( !isCollectionMonitored( msg.uid(), msg.resource() ) )
      return false;
    if ( msg.operation() != NotificationMessage::Remove && fetchCollection ) {
      Collection::List list;
      list << Collection( msg.uid() );
      if ( msg.operation() == NotificationMessage::Add )
        list << Collection( msg.parentCollection() );
      CollectionListJob *job = new CollectionListJob( list, q_ptr );
      pendingJobs.insert( job, msg );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotCollectionJobFinished(KJob*)) );
      return true;
    }
    if ( msg.operation() == NotificationMessage::Remove ) {
      // no need for status updates anymore
      recentlyChangedCollections.remove( msg.uid() );
    }
    emitCollectionNotification( msg );
    return true;

  } else {
    qWarning() << "Received unknown change notification!";
  }
  return false;
}

void MonitorPrivate::sessionDestroyed( QObject * object )
{
  Session* session = qobject_cast<Session*>( object );
  if ( session )
    sessions.removeAll( session->sessionId() );
}

void MonitorPrivate::slotStatusChangedFinished( KJob* job )
{
  if ( job->error() ) {
    qWarning() << "Error on fetching collection status: " << job->errorText();
  } else {
    CollectionStatusJob *statusJob = static_cast<CollectionStatusJob*>( job );
    emit q_ptr->collectionStatusChanged( statusJob->collection().id(), statusJob->status() );
  }
}

void MonitorPrivate::slotFlushRecentlyChangedCollections()
{
  foreach( int collection, recentlyChangedCollections ) {
    if ( fetchCollectionStatus ) {
      fetchStatus( collection );
    } else {
      static const CollectionStatus dummyStatus;
      emit q_ptr->collectionStatusChanged( collection, dummyStatus );
    }
  }
  recentlyChangedCollections.clear();
}

void MonitorPrivate::slotNotify( const NotificationMessage::List &msgs )
{
  foreach ( const NotificationMessage msg, msgs )
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
    it = Item( DataReference( msg.uid(), msg.remoteId() ) );
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
      emit q_ptr->itemRemoved( it.reference() );
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
      emit q_ptr->collectionRemoved( collection.id(), collection.remoteId() );
      break;
    default:
      Q_ASSERT_X( false, "MonitorPrivate::emitCollectionNotification", "Invalid enum value" );
  }
}

void MonitorPrivate::slotItemJobFinished( KJob* job )
{
  if ( !pendingJobs.contains( job ) ) {
    kWarning() <<"unknown job - wtf is going on here?";
    return;
  }
  NotificationMessage msg = pendingJobs.take( job );
  if ( job->error() ) {
    kWarning() <<"Error on fetching item:" << job->errorText();
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
    kWarning() <<"unknown job - wtf is going on here?";
    return;
  }
  NotificationMessage msg = pendingJobs.take( job );
  if ( job->error() ) {
    kWarning() <<"Error on fetching collection:" << job->errorText();
  } else {
    Collection col, parent;
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );
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


ItemCollectionFetchJob::ItemCollectionFetchJob( const DataReference &reference, int collectionId, QObject *parent )
  : Job( parent ),
    mReference( reference ), mCollectionId( collectionId ), mFetchAllParts( false )
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

void ItemCollectionFetchJob::doStart()
{
  CollectionListJob *listJob = new CollectionListJob( Collection( mCollectionId ), CollectionListJob::Local, this );
  connect( listJob, SIGNAL( result( KJob* ) ), SLOT( collectionJobDone( KJob* ) ) );
  addSubjob( listJob );

  ItemFetchJob *fetchJob = new ItemFetchJob( mReference, this );
  foreach( QString part, mFetchParts )
    fetchJob->addFetchPart( part );
  if ( mFetchAllParts )
    fetchJob->fetchAllParts();
  connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( itemJobDone( KJob* ) ) );
  addSubjob( fetchJob );
}

void ItemCollectionFetchJob::collectionJobDone( KJob* job )
{
  if ( !job->error() ) {
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );
    if ( listJob->collections().count() == 0 ) {
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
    if ( fetchJob->items().count() == 0 ) {
      setError( 2 );
      setErrorText( QLatin1String( "No item found" ) );
    } else
      mItem = fetchJob->items().first();

    emitResult();
  }
}

void ItemCollectionFetchJob::addFetchPart( const QString &identifier )
{
  if ( !mFetchParts.contains( identifier ) )
    mFetchParts.append( identifier );
}

void ItemCollectionFetchJob::fetchAllParts()
{
  mFetchAllParts = true;
}

#include "monitor_p.moc"
