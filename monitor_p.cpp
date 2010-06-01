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

static const int PipelineSize = 5;

MonitorPrivate::MonitorPrivate(Monitor * parent) :
  q_ptr( parent ),
  nm( 0 ),
  monitorAll( false ),
  session( Session::defaultSession() ),
  collectionCache( 3*PipelineSize, session ), // needs to be at least 3x pipeline size for the collection move case
  itemCache( PipelineSize, session ), // needs to be at least 1x pipeline size
  fetchCollection( false ),
  fetchCollectionStatistics( false ),
  useRefCounting( false )
{
}

void MonitorPrivate::init()
{
  QObject::connect( &collectionCache, SIGNAL( dataAvailable() ), q_ptr, SLOT( dataAvailable() ) );
  QObject::connect( &itemCache, SIGNAL( dataAvailable() ), q_ptr, SLOT( dataAvailable() ) );
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
    kWarning() << "Unable to connect to notification manager";
  } else {
    QObject::connect( nm, SIGNAL( notify( Akonadi::NotificationMessage::List ) ),
                      q_ptr, SLOT( slotNotify( Akonadi::NotificationMessage::List ) ) );
    return true;
  }
  return false;
}

int MonitorPrivate::pipelineSize() const
{
  return PipelineSize;
}

bool MonitorPrivate::isLazilyIgnored( const NotificationMessage & msg ) const
{
  NotificationMessage::Operation op = msg.operation();

  if ( !fetchCollectionStatistics &&
       ( msg.type() == NotificationMessage::Item ) && ( ( op == NotificationMessage::Add && q_ptr->receivers( SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) ) == 0 )
    || ( op == NotificationMessage::Remove && q_ptr->receivers( SIGNAL( itemRemoved( const Akonadi::Item& ) ) ) == 0 )
    || ( op == NotificationMessage::Modify && q_ptr->receivers( SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) ) == 0 )
    || ( op == NotificationMessage::Move && q_ptr->receivers( SIGNAL( itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) ) ) == 0 )
    || ( op == NotificationMessage::Link && q_ptr->receivers( SIGNAL( itemLinked( const Akonadi::Item&, const Akonadi::Collection& ) ) ) == 0 )
    || ( op == NotificationMessage::Unlink && q_ptr->receivers( SIGNAL( itemUnlinked( const Akonadi::Item&, const Akonadi::Collection& ) ) ) == 0 ) ) )
  {
    return true;
  }

  if ( !useRefCounting )
    return false;

  if ( msg.type() == NotificationMessage::Collection )
    // Lazy fetching can only affects items.
    return false;

  Collection::Id parentCollectionId = msg.parentCollection();

  if ( ( op == NotificationMessage::Add )
    || ( op == NotificationMessage::Remove )
    || ( op == NotificationMessage::Modify )
    || ( op == NotificationMessage::Link )
    || ( op == NotificationMessage::Unlink ) )
  {
    if ( refCountMap.contains( parentCollectionId ) || m_buffer.isBuffered( parentCollectionId ) )
      return false;
  }


  if ( op == NotificationMessage::Move )
  {
    if ( !refCountMap.contains( parentCollectionId ) && !m_buffer.isBuffered( parentCollectionId ) )
      if ( !refCountMap.contains( msg.parentDestCollection() ) && !m_buffer.isBuffered( msg.parentDestCollection() ) )
        return true;
    // We can't ignore the move. It must be transformed later into a removal or insertion.
    return false;
  }
  return true;
}

bool MonitorPrivate::acceptNotification( const NotificationMessage & msg )
{
  if ( isSessionIgnored( msg.sessionId() ) )
    return false;

  if ( isLazilyIgnored( msg ) )
    return false;

  switch ( msg.type() ) {
    case NotificationMessage::InvalidType:
      kWarning() << "Received invalid change notification!";
      return false;
    case NotificationMessage::Item:
      return isItemMonitored( msg.uid(), msg.parentCollection(), msg.parentDestCollection(), msg.mimeType(), msg.resource() )
          || isCollectionMonitored( msg.parentCollection(), msg.resource() )
          || isCollectionMonitored( msg.parentDestCollection(), msg.resource() );
    case NotificationMessage::Collection:
      return isCollectionMonitored( msg.uid(), msg.resource() )
          || isCollectionMonitored( msg.parentCollection(), msg.resource() )
          || isCollectionMonitored( msg.parentDestCollection(), msg.resource() );
  }
  Q_ASSERT( false );
  return false;
}

void MonitorPrivate::dispatchNotifications()
{
  while ( pipeline.size() < pipelineSize() && !pendingNotifications.isEmpty() ) {
    const NotificationMessage msg = pendingNotifications.dequeue();
    if ( ensureDataAvailable( msg ) && pipeline.isEmpty() )
      emitNotification( msg );
    else
      pipeline.enqueue( msg );
  }
}

void MonitorPrivate::cleanOldNotifications()
{
  foreach ( const NotificationMessage &msg, pipeline ) {
    if ( !acceptNotification( msg ) ) {
      pipeline.removeOne( msg );
    }
  }

  foreach ( const NotificationMessage &msg, pendingNotifications ) {
    if ( !acceptNotification( msg ) ) {
      pendingNotifications.removeOne( msg );
    }
  }
}

bool MonitorPrivate::ensureDataAvailable( const NotificationMessage &msg )
{
  bool allCached = true;
  if ( fetchCollection ) {
    if ( !collectionCache.ensureCached( msg.parentCollection(), mCollectionFetchScope ) )
      allCached = false;
    if ( msg.operation() == NotificationMessage::Move && !collectionCache.ensureCached( msg.parentDestCollection(), mCollectionFetchScope ) )
      allCached = false;
  }
  if ( msg.operation() == NotificationMessage::Remove )
    return allCached; // the actual object is gone already, nothing to fetch there

  if ( msg.type() == NotificationMessage::Item && !mItemFetchScope.isEmpty() ) {
    if ( !itemCache.ensureCached( msg.uid(), mItemFetchScope ) )
      allCached = false;
  } else if ( msg.type() == NotificationMessage::Collection && fetchCollection ) {
    if ( !collectionCache.ensureCached( msg.uid(), mCollectionFetchScope ) )
      allCached = false;
  }
  return allCached;
}

void MonitorPrivate::emitNotification( const NotificationMessage &msg )
{
  const Collection parent = collectionCache.retrieve( msg.parentCollection() );
  Collection destParent;
  if ( msg.operation() == NotificationMessage::Move )
    destParent = collectionCache.retrieve( msg.parentDestCollection() );

  if ( msg.type() == NotificationMessage::Collection ) {
    const Collection col = collectionCache.retrieve( msg.uid() );
    emitCollectionNotification( msg, col, parent, destParent );
  } else if ( msg.type() == NotificationMessage::Item ) {
    const Item item = itemCache.retrieve( msg.uid() );
    emitItemNotification( msg, item, parent, destParent );
  }
}

void MonitorPrivate::dataAvailable()
{
  while ( !pipeline.isEmpty() ) {
    const NotificationMessage msg = pipeline.head();
    if ( ensureDataAvailable( msg ) ) {
      // dequeue should be before emit, otherwise stuff might happen (like dataAvailable
      // being called again) and we end up dequeuing an empty pipeline
      pipeline.dequeue();
      emitNotification( msg );
    } else {
      break;
    }
  }
  dispatchNotifications();
}

void MonitorPrivate::updatePendingStatistics( const NotificationMessage &msg )
{
  if ( msg.type() == NotificationMessage::Item ) {
    notifyCollectionStatisticsWatchers( msg.parentCollection(), msg.resource() );
    // FIXME use the proper resource of the target collection, for cross resource moves
    notifyCollectionStatisticsWatchers( msg.parentDestCollection(), msg.resource() );
  } else if ( msg.type() == NotificationMessage::Collection && msg.operation() == NotificationMessage::Remove ) {
    // no need for statistics updates anymore
    recentlyChangedCollections.remove( msg.uid() );
  }
}

void MonitorPrivate::slotSessionDestroyed( QObject * object )
{
  Session* objectSession = qobject_cast<Session*>( object );
  if ( objectSession )
    sessions.removeAll( objectSession->sessionId() );
}

void MonitorPrivate::slotStatisticsChangedFinished( KJob* job )
{
  if ( job->error() ) {
    kWarning() << "Error on fetching collection statistics: " << job->errorText();
  } else {
    CollectionStatisticsJob *statisticsJob = static_cast<CollectionStatisticsJob*>( job );
    Q_ASSERT( statisticsJob->collection().isValid() );
    emit q_ptr->collectionStatisticsChanged( statisticsJob->collection().id(),
                                             statisticsJob->statistics() );
  }
}

void MonitorPrivate::slotFlushRecentlyChangedCollections()
{
  foreach ( Collection::Id collection, recentlyChangedCollections ) {
    Q_ASSERT( collection >= 0 );
    if ( fetchCollectionStatistics ) {
      fetchStatistics( collection );
    } else {
      static const CollectionStatistics dummyStatistics;
      emit q_ptr->collectionStatisticsChanged( collection, dummyStatistics );
    }
  }
  recentlyChangedCollections.clear();
}

void MonitorPrivate::appendAndCompress( const NotificationMessage &msg  )
{
  if ( !useRefCounting || msg.operation() != NotificationMessage::Move || msg.type() != NotificationMessage::Item )
    return NotificationMessage::appendAndCompress( pendingNotifications, msg );

  bool sourceWatched = refCountMap.contains( msg.parentCollection() ) || m_buffer.isBuffered( msg.parentCollection() );
  bool destWatched = refCountMap.contains( msg.parentDestCollection() ) || m_buffer.isBuffered( msg.parentDestCollection() );

  if ( sourceWatched && destWatched )
    return NotificationMessage::appendAndCompress( pendingNotifications, msg );

  if ( sourceWatched )
  {
    // Transform into a removal
    NotificationMessage removalMessage = msg;
    removalMessage.setOperation( NotificationMessage::Remove );
    removalMessage.setParentDestCollection( -1 );
    return NotificationMessage::appendAndCompress( pendingNotifications, removalMessage );
  }

  // Transform into an insertion
  NotificationMessage insertionMessage = msg;
  insertionMessage.setOperation( NotificationMessage::Add );
  insertionMessage.setParentCollection( msg.parentDestCollection() );
  insertionMessage.setParentDestCollection( -1 );
  NotificationMessage::appendAndCompress( pendingNotifications, insertionMessage );
}

void MonitorPrivate::slotNotify( const NotificationMessage::List &msgs )
{
  foreach ( const NotificationMessage &msg, msgs ) {
    invalidateCaches( msg );
    if ( acceptNotification( msg ) ) {
      updatePendingStatistics( msg );
      appendAndCompress( msg );
    }
  }

  dispatchNotifications();
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
  if ( !it.isValid() || msg.operation() == NotificationMessage::Remove ) {
    it = Item( msg.uid() );
    it.setRemoteId( msg.remoteId() );
    it.setMimeType( msg.mimeType() );
  }
  if ( !it.parentCollection().isValid() ) {
    if ( msg.operation() == NotificationMessage::Move )
      it.setParentCollection( colDest );
    else
      it.setParentCollection( col );
  }

  // HACK: We have the remoteRevision stored in the itemParts set
  //       for delete operations to avoid protocol breakage
  if ( msg.operation() == NotificationMessage::Remove ) {
    if ( !msg.itemParts().isEmpty() ) { //TODO: investigate why it could be empty
      const QString remoteRevision = QString::fromUtf8( msg.itemParts().toList().first() );
      it.setRemoteRevision( remoteRevision );
    }
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
    case NotificationMessage::Link:
      emit q_ptr->itemLinked( it, col );
      break;
    case NotificationMessage::Unlink:
      emit q_ptr->itemUnlinked( it, col );
      break;
    default:
      kDebug() << "Unknown operation type" << msg.operation() << "in item change notification";
      break;
  }
}

void MonitorPrivate::emitCollectionNotification( const NotificationMessage &msg, const Collection &col,
                                                   const Collection &par, const Collection &dest )
{
  Q_ASSERT( msg.type() == NotificationMessage::Collection );
  Collection collection = col;
  if ( !collection.isValid() || msg.operation() == NotificationMessage::Remove ) {
    collection = Collection( msg.uid() );
    collection.setResource( QString::fromUtf8( msg.resource() ) );
    collection.setRemoteId( msg.remoteId() );
  }

  Collection parent = par;
  if ( !parent.isValid() )
    parent = Collection( msg.parentCollection() );
  Collection destination = dest;
  if ( !destination.isValid() )
    destination = Collection( msg.parentDestCollection() );

  if ( !collection.parentCollection().isValid() ) {
    if ( msg.operation() == NotificationMessage::Move )
      collection.setParentCollection( destination );
    else
      collection.setParentCollection( parent );
  }

  // HACK: We have the remoteRevision stored in the itemParts set
  //       for delete operations to avoid protocol breakage
  if ( msg.operation() == NotificationMessage::Remove ) {
    if ( !msg.itemParts().isEmpty() ) { //TODO: investigate why it could be empty
      const QString remoteRevision = QString::fromUtf8( msg.itemParts().toList().first() );
      collection.setRemoteRevision( remoteRevision );
    }
  }

  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit q_ptr->collectionAdded( collection, parent );
      break;
    case NotificationMessage::Modify:
      emit q_ptr->collectionChanged( collection );
      emit q_ptr->collectionChanged( collection, msg.itemParts() );
      break;
    case NotificationMessage::Move:
      emit q_ptr->collectionMoved( collection, parent, destination );
      break;
    case NotificationMessage::Remove:
      emit q_ptr->collectionRemoved( collection );
      break;
    default:
      kDebug() << "Unknown operation type" << msg.operation() << "in collection change notification";
  }
}

void MonitorPrivate::invalidateCaches( const NotificationMessage &msg )
{
  // remove invalidates
  if ( msg.operation() == NotificationMessage::Remove ) {
    if ( msg.type() == NotificationMessage::Collection ) {
      collectionCache.invalidate( msg.uid() );
    } else if ( msg.type() == NotificationMessage::Item ) {
      itemCache.invalidate( msg.uid() );
    }
  }

  // modify removes the cache entry, as we need to re-fetch
  if ( msg.operation() == NotificationMessage::Modify || msg.operation() == NotificationMessage::Move ) {
    if ( msg.type() == NotificationMessage::Collection ) {
      collectionCache.update( msg.uid(), mCollectionFetchScope );
    } else if ( msg.type() == NotificationMessage::Item ) {
      itemCache.update( msg.uid(), mItemFetchScope );
    }
  }
}

void MonitorPrivate::invalidateCache( const Collection &col )
{
  collectionCache.update( col.id(), mCollectionFetchScope );
}

void MonitorPrivate::ref( Collection::Id id )
{
  if ( !refCountMap.contains( id ) )
  {
    refCountMap.insert( id, 0 );
  }
  ++refCountMap[ id ];

  if ( m_buffer.isBuffered( id ) )
    m_buffer.purge( id );
}

Collection::Id MonitorPrivate::deref( Collection::Id id )
{
  Q_ASSERT( refCountMap.contains( id ) );
  if ( --refCountMap[ id ] == 0 )
  {
    refCountMap.remove( id );
  }
  return m_buffer.buffer( id );
}

void MonitorPrivate::PurgeBuffer::purge( Collection::Id id )
{
  int idx = m_buffer.indexOf( id, 0 );
  while ( idx <= m_index )
  {
    if ( idx < 0 )
      break;
    m_buffer.removeAt( idx );
    if ( m_index > 0 )
      --m_index;
    idx = m_buffer.indexOf( id, idx );
  }
  while ( int idx = m_buffer.indexOf( id, m_index ) > -1 )
  {
    m_buffer.removeAt( idx );
  }
}

Collection::Id MonitorPrivate::PurgeBuffer::buffer( Collection::Id id )
{
  if ( m_index == MAXBUFFERSIZE )
  {
    m_index = 0;
  }
  Collection::Id bumpedId = -1;
  if ( m_buffer.size() == MAXBUFFERSIZE )
  {
    bumpedId = m_buffer.takeAt( m_index );
  }

  // Ensure that we don't put a duplicate @p id into the buffer.
  purge( id );

  m_buffer.insert( m_index, id );
  ++m_index;

  return bumpedId;
}

// @endcond
