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
#include "dbusconnectionpool.h"
#include "itemfetchjob.h"
#include "notificationmessagev2_p.h"
#include "notificationmanagerinterface.h"
#include "session.h"

#include <kdebug.h>
#include <kcomponentdata.h>
#include "changemediator_p.h"

using namespace Akonadi;

static const int PipelineSize = 5;

MonitorPrivate::MonitorPrivate( ChangeNotificationDependenciesFactory *dependenciesFactory_, Monitor * parent) :
  q_ptr( parent ),
  dependenciesFactory(dependenciesFactory_ ? dependenciesFactory_ : new ChangeNotificationDependenciesFactory),
  notificationSource( 0 ),
  monitorAll( false ),
  mFetchChangedOnly( false ),
  session( Session::defaultSession() ),
  collectionCache( 0 ),
  itemCache( 0 ),
  fetchCollection( false ),
  fetchCollectionStatistics( false ),
  collectionMoveTranslationEnabled( true ),
  useRefCounting( false )
{
}

void MonitorPrivate::init()
{
  // needs to be at least 3x pipeline size for the collection move case
  collectionCache = dependenciesFactory->createCollectionCache(3*PipelineSize, session);
  // needs to be at least 1x pipeline size
  itemCache = dependenciesFactory->createItemListCache(PipelineSize, session);

  QObject::connect( collectionCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()) );
  QObject::connect( itemCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()) );
  QObject::connect( ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
                    q_ptr, SLOT(serverStateChanged(Akonadi::ServerManager::State)) );

  NotificationMessageV2::registerDBusTypes();

  statisticsCompressionTimer.setSingleShot( true );
  statisticsCompressionTimer.setInterval( 500 );
  QObject::connect( &statisticsCompressionTimer, SIGNAL(timeout()), q_ptr, SLOT(slotFlushRecentlyChangedCollections()) );
}

bool MonitorPrivate::connectToNotificationManager()
{
  delete notificationSource;
  notificationSource = 0;

  notificationSource = dependenciesFactory->createNotificationSource(q_ptr);

  if (!notificationSource)
    return false;

  QObject::connect( notificationSource, SIGNAL(notifyV2(Akonadi::NotificationMessageV2::List)),
                    q_ptr, SLOT(slotNotify(Akonadi::NotificationMessageV2::List)) );

  return true;
}

void MonitorPrivate::serverStateChanged(ServerManager::State state)
{
  if ( state == ServerManager::Running )
    connectToNotificationManager();
}

void MonitorPrivate::invalidateCollectionCache( qint64 id )
{
  collectionCache->update( id, mCollectionFetchScope );
}

void MonitorPrivate::invalidateItemCache( qint64 id )
{
  itemCache->update( QList<Entity::Id>() << id, mItemFetchScope );
}

int MonitorPrivate::pipelineSize() const
{
  return PipelineSize;
}

bool MonitorPrivate::isLazilyIgnored( const NotificationMessageV2 & msg, bool allowModifyFlagsConversion ) const
{
  NotificationMessageV2::Operation op = msg.operation();

  if ( !fetchCollectionStatistics &&
       ( msg.type() == NotificationMessageV2::Items ) && ( ( op == NotificationMessageV2::Add && q_ptr->receivers( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) ) == 0 )
    || ( op == NotificationMessageV2::Remove && q_ptr->receivers( SIGNAL(itemRemoved(Akonadi::Item)) ) == 0
                                             && q_ptr->receivers( SIGNAL(itemsRemoved(Akonadi::Item::List)) ) == 0 )
    || ( op == NotificationMessageV2::Modify && q_ptr->receivers( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) ) == 0 )
    || ( op == NotificationMessageV2::ModifyFlags && q_ptr->receivers( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) ) == 0 )
        // Newly delivered ModifyFlags notifications will be converted to
        // itemChanged(item, "FLAGS") for legacy clients.
    || ( op == NotificationMessageV2::ModifyFlags && ( !allowModifyFlagsConversion || q_ptr->receivers( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) ) == 0 ) )
    || ( op == NotificationMessageV2::Move && q_ptr->receivers( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) ) == 0
                                           && q_ptr->receivers( SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection)) ) == 0 )
    || ( op == NotificationMessageV2::Link && q_ptr->receivers( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) ) == 0
                                           && q_ptr->receivers( SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection)) ) == 0 )
    || ( op == NotificationMessageV2::Unlink && q_ptr->receivers( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) ) == 0
                                             && q_ptr->receivers( SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection)) ) == 0 ) ) )
  {
    return true;
  }

  if ( !useRefCounting )
    return false;

  if ( msg.type() == NotificationMessageV2::Collections )
    // Lazy fetching can only affects items.
    return false;

  Collection::Id parentCollectionId = msg.parentCollection();

  if ( ( op == NotificationMessageV2::Add )
    || ( op == NotificationMessageV2::Remove )
    || ( op == NotificationMessageV2::Modify )
    || ( op == NotificationMessageV2::ModifyFlags )
    || ( op == NotificationMessageV2::Link )
    || ( op == NotificationMessageV2::Unlink ) )
  {
    if ( refCountMap.contains( parentCollectionId ) || m_buffer.isBuffered( parentCollectionId ) )
      return false;
  }


  if ( op == NotificationMessageV2::Move )
  {
    if ( !refCountMap.contains( parentCollectionId ) && !m_buffer.isBuffered( parentCollectionId ) )
      if ( !refCountMap.contains( msg.parentDestCollection() ) && !m_buffer.isBuffered( msg.parentDestCollection() ) )
        return true;
    // We can't ignore the move. It must be transformed later into a removal or insertion.
    return false;
  }
  return true;
}

void MonitorPrivate::checkBatchSupport(const NotificationMessageV2& msg, bool &needsSplit, bool &batchSupported) const
{
  const bool isBatch = (msg.entities().count() > 1);

  if ( msg.type() == NotificationMessageV2::Items ) {
    switch ( msg.operation() ) {
      case NotificationMessageV2::Add:
        needsSplit = isBatch;
        batchSupported = false;
        return;
      case NotificationMessageV2::Modify:
        needsSplit = isBatch;
        batchSupported = false;
        return;
      case NotificationMessageV2::ModifyFlags:
        batchSupported = q_ptr->receivers( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) ) > 0;
        needsSplit = isBatch && !batchSupported && q_ptr->receivers( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) ) > 0;
        return;
      case NotificationMessageV2::Move:
        needsSplit = isBatch && q_ptr->receivers( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) ) > 0;
        batchSupported = q_ptr->receivers( SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection)) ) > 0;
        return;
      case NotificationMessageV2::Remove:
        needsSplit = isBatch && q_ptr->receivers( SIGNAL(itemRemoved(Akonadi::Item)) ) > 0;
        batchSupported = q_ptr->receivers( SIGNAL(itemsRemoved(Akonadi::Item::List)) ) > 0;
        return;
      case NotificationMessageV2::Link:
        needsSplit = isBatch && q_ptr->receivers( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) ) > 0;
        batchSupported = q_ptr->receivers( SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection)) ) > 0;
        return;
      case NotificationMessageV2::Unlink:
        needsSplit = isBatch && q_ptr->receivers( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) ) > 0;
        batchSupported = q_ptr->receivers( SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection)) ) > 0;
        return;
      default:
        needsSplit = isBatch;
        batchSupported = false;
        kDebug() << "Unknown operation type" << msg.operation() << "in item change notification";
        return;
    }
  } else if ( msg.type() == NotificationMessageV2::Collections ) {
    needsSplit = isBatch;
    batchSupported = false;
  }
}

NotificationMessageV2::List MonitorPrivate::splitMessage(const NotificationMessageV2& msg, bool legacy) const
{
  NotificationMessageV2::List list;

  NotificationMessageV2 baseMsg;
  baseMsg.setSessionId( msg.sessionId() );
  baseMsg.setType( msg.type() );
  if ( legacy && msg.operation() == NotificationMessageV2::ModifyFlags ) {
    baseMsg.setOperation( NotificationMessageV2::Modify );
    baseMsg.setItemParts( QSet<QByteArray>() << "FLAGS" );
  } else {
    baseMsg.setOperation( msg.operation() );
    baseMsg.setItemParts( msg.itemParts() );
  }
  baseMsg.setParentCollection( msg.parentCollection() );
  baseMsg.setParentDestCollection( msg.parentDestCollection() );
  baseMsg.setResource( msg.resource() );
  baseMsg.setDestinationResource( msg.destinationResource() );
  baseMsg.setAddedFlags( msg.addedFlags() );
  baseMsg.setRemovedFlags( msg.removedFlags() );

  Q_FOREACH( const NotificationMessageV2::Entity &entity, msg.entities() ) {
    NotificationMessageV2 copy = baseMsg;
    copy.addEntity(entity.id, entity.remoteId, entity.remoteRevision, entity.mimeType);

    list << copy;
  }

  return list;
}

bool MonitorPrivate::acceptNotification( const NotificationMessageV2 &msg, bool allowModifyFlagsConversion ) const
{
  // session is ignored
  if ( sessions.contains( msg.sessionId() ) )
    return false;

  if ( msg.entities().count() == 0 )
    return false;

  // corresponding signal is not connected
  if ( isLazilyIgnored( msg, allowModifyFlagsConversion ) ) {
    return false;
  }

  // user requested everything
  if ( monitorAll && msg.type() != NotificationMessageV2::InvalidType)
    return true;

  switch ( msg.type() ) {
    case NotificationMessageV2::InvalidType:
      kWarning() << "Received invalid change notification!";
      return false;

    case NotificationMessageV2::Items:
      // we have a resource or mimetype filter
      if ( !resources.isEmpty() || !mimetypes.isEmpty() ) {
        if ( resources.contains( msg.resource() ) || isMoveDestinationResourceMonitored( msg ) )
          return true;

        Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities() ) {
          if ( isMimeTypeMonitored( entity.mimeType ) )
            return true;
        }
        return false;
      }

      // we explicitly monitor that item or the collections it's in
      Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
        if ( items.contains( entity.id ) )
          return true;
      }

      return isCollectionMonitored( msg.parentCollection() )
          || isCollectionMonitored( msg.parentDestCollection() );

    case NotificationMessageV2::Collections:
      // we have a resource filter
      if ( !resources.isEmpty() ) {
        const bool resourceMatches = resources.contains( msg.resource() ) || isMoveDestinationResourceMonitored( msg );
        // a bit hacky, but match the behaviour from the item case,
        // if resource is the only thing we are filtering on, stop here, and if the resource filter matched, of course
        if ( mimetypes.isEmpty() || resourceMatches )
          return resourceMatches;
        // else continue
      }

      // we explicitly monitor that colleciton, or all of them
      Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
        if ( isCollectionMonitored( entity.id ) )
          return true;
      }
      return isCollectionMonitored( msg.parentCollection() )
          || isCollectionMonitored( msg.parentDestCollection() );
  }
  Q_ASSERT( false );
  return false;
}

void MonitorPrivate::cleanOldNotifications()
{
  bool erased = false;
  for ( QQueue<NotificationMessageV2>::iterator it = pipeline.begin(); it != pipeline.end(); ) {
    if ( !acceptNotification( *it ) ) {
      it = pipeline.erase( it );
      erased = true;
    } else {
      ++it;
    }
  }

  for ( QQueue<NotificationMessageV2>::iterator it = pendingNotifications.begin(); it != pendingNotifications.end(); ) {
    if ( !acceptNotification( *it ) ) {
      it = pendingNotifications.erase( it );
      erased = true;
    } else {
      ++it;
    }
  }
  if (erased)
    notificationsErased();
}

bool MonitorPrivate::ensureDataAvailable( const NotificationMessageV2& msg )
{
  bool allCached = true;
  if ( fetchCollection ) {
    if ( !collectionCache->ensureCached( msg.parentCollection(), mCollectionFetchScope ) )
      allCached = false;
    if ( msg.operation() == NotificationMessageV2::Move && !collectionCache->ensureCached( msg.parentDestCollection(), mCollectionFetchScope ) )
      allCached = false;
  }
  if ( msg.operation() == NotificationMessageV2::Remove )
    return allCached; // the actual object is gone already, nothing to fetch there

  if ( msg.type() == NotificationMessageV2::Items && !mItemFetchScope.isEmpty() ) {
    ItemFetchScope scope( mItemFetchScope );
    if ( mFetchChangedOnly && ( msg.operation() == NotificationMessageV2::Modify || msg.operation() == NotificationMessageV2::ModifyFlags ) ) {
      bool fullPayloadWasRequested = scope.fullPayload();
      scope.fetchFullPayload( false );
      QSet<QByteArray> requestedPayloadParts = scope.payloadParts();
      Q_FOREACH( const QByteArray &part, requestedPayloadParts ) {
        scope.fetchPayloadPart( part, false );
      }

      bool allAttributesWereRequested = scope.allAttributes();
      QSet<QByteArray> requestedAttrParts = scope.attributes();
      Q_FOREACH( const QByteArray &part, requestedAttrParts ) {
        scope.fetchAttribute( part, false );
      }

      QSet<QByteArray> changedParts = msg.itemParts();
      Q_FOREACH( const QByteArray &part, changedParts )  {
        if( part.startsWith( "PLD:" ) && //krazy:exclude=strings since QByteArray
            ( fullPayloadWasRequested || requestedPayloadParts.contains( part ) ) ) {
          scope.fetchPayloadPart( part.mid(4), true );;
        }
        if ( part.startsWith( "ATR:" ) && //krazy:exclude=strings since QByteArray
             ( allAttributesWereRequested || requestedAttrParts.contains( part ) ) ) {
          scope.fetchAttribute( part.mid(4), true );
        }
      }
    }
    if ( !itemCache->ensureCached( msg.uids(), scope ) ) {
        allCached = false;
    }
  } else if ( msg.type() == NotificationMessageV2::Collections && fetchCollection ) {
    Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
      if ( !collectionCache->ensureCached( entity.id, mCollectionFetchScope ) )
        allCached = false;
    }
  }
  return allCached;
}

bool MonitorPrivate::emitNotification( const NotificationMessageV2& msg )
{
  const Collection parent = collectionCache->retrieve( msg.parentCollection() );
  Collection destParent;
  if ( msg.operation() == NotificationMessageV2::Move )
    destParent = collectionCache->retrieve( msg.parentDestCollection() );

  bool someoneWasListening = false;
  if ( msg.type() == NotificationMessageV2::Collections ) {
    Collection col;
    Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
      col = collectionCache->retrieve( entity.id );
      if ( emitCollectionNotification( msg, col, parent, destParent ) && !someoneWasListening )
        someoneWasListening = true;
    }
  } else if ( msg.type() == NotificationMessageV2::Items ) {
    Item::List items = itemCache->retrieve( msg.uids() );
    someoneWasListening = emitItemsNotification( msg, items, parent, destParent );
  }

  if ( !someoneWasListening )
    cleanOldNotifications(); // probably someone disconnected a signal in the meantime, get rid of the no longer interesting stuff

  return someoneWasListening;
}

void MonitorPrivate::updatePendingStatistics( const NotificationMessageV2& msg )
{
  if ( msg.type() == NotificationMessageV2::Items ) {
    notifyCollectionStatisticsWatchers( msg.parentCollection(), msg.resource() );
    // FIXME use the proper resource of the target collection, for cross resource moves
    notifyCollectionStatisticsWatchers( msg.parentDestCollection(), msg.destinationResource() );
  } else if ( msg.type() == NotificationMessageV2::Collections && msg.operation() == NotificationMessageV2::Remove ) {
    // no need for statistics updates anymore
    Q_FOREACH( const NotificationMessageV2::Entity &entity, msg.entities() ) {
      recentlyChangedCollections.remove( entity.id );
    }
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

int MonitorPrivate::translateAndCompress( QQueue<NotificationMessageV2> &notificationQueue, const NotificationMessageV2 &msg  )
{
  // We have to split moves into insert or remove if the source or destination
  // is not monitored.
  if ( msg.operation() != NotificationMessageV2::Move ) {
    return NotificationMessageV2::appendAndCompress( notificationQueue, msg ) ? 1 : 0;
  }

  bool sourceWatched = false;
  bool destWatched = false;

  if ( useRefCounting && msg.type() == NotificationMessageV2::Items ) {
    sourceWatched = refCountMap.contains( msg.parentCollection() ) || m_buffer.isBuffered( msg.parentCollection() );
    destWatched = refCountMap.contains( msg.parentDestCollection() ) || m_buffer.isBuffered( msg.parentDestCollection() );
  } else {
    if ( !resources.isEmpty() ) {
      sourceWatched = resources.contains( msg.resource() );
      destWatched = isMoveDestinationResourceMonitored( msg );
    }
    if ( !sourceWatched )
      sourceWatched = isCollectionMonitored( msg.parentCollection() );
    if ( !destWatched )
      destWatched = isCollectionMonitored( msg.parentDestCollection() );
  }

  if ( !sourceWatched && !destWatched ) {
    return 0;
  }

  if ( ( sourceWatched && destWatched ) || ( !collectionMoveTranslationEnabled && msg.type() == NotificationMessageV2::Collections ) ) {
    return NotificationMessageV2::appendAndCompress( notificationQueue, msg ) ? 1 : 0;
  }

  if ( sourceWatched )
  {
    // Transform into a removal
    NotificationMessageV2 removalMessage = msg;
    removalMessage.setOperation( NotificationMessageV2::Remove );
    removalMessage.setParentDestCollection( -1 );
    return NotificationMessageV2::appendAndCompress( notificationQueue, removalMessage ) ? 1 : 0;
  }

  // Transform into an insertion
  NotificationMessageV2 insertionMessage = msg;
  insertionMessage.setOperation( NotificationMessageV2::Add );
  insertionMessage.setParentCollection( msg.parentDestCollection() );
  insertionMessage.setParentDestCollection( -1 );
  // We don't support batch insertion, so we have to do it one by one
  const NotificationMessageV2::List split = splitMessage( insertionMessage, false );
  int appended = 0;
  Q_FOREACH (const NotificationMessageV2 &insertion, split ) {
    if ( NotificationMessageV2::appendAndCompress( notificationQueue, insertion ) ) {
      ++appended;
    }
  }
  return appended;
}

/*

  server notification --> ?accepted --> pendingNotifications --> ?dataAvailable --> emit
                                  |                                           |
                                  x --> discard                               x --> pipeline

  fetchJobDone --> pipeline ?dataAvailable --> emit
 */

void MonitorPrivate::slotNotify( const NotificationMessageV2::List &msgs )
{
  int appendedMessages = 0;
  int modifiedMessages = 0;
  int erasedMessages = 0;
  foreach ( const NotificationMessageV2 &msg, msgs ) {
    invalidateCaches( msg );
    updatePendingStatistics( msg );
    if ( acceptNotification( msg, true ) ) {
      bool needsSplit = true;
      bool supportsBatch = false;

      checkBatchSupport( msg, needsSplit, supportsBatch );

      if ( supportsBatch
          || ( !needsSplit && !supportsBatch && msg.operation() != NotificationMessageV2::ModifyFlags )
          || msg.type() == NotificationMessageV2::Collections ) {
        // Make sure the batch msg is always queued before the split notifications
        const int oldSize = pendingNotifications.size();
        const int appended = translateAndCompress( pendingNotifications, msg );
        if ( appended > 0 ) {
          appendedMessages += appended;
          // translateAndCompress can remove an existing "modify" when msg is a "delete". We need to detect that, for ChangeRecorder.
          if ( pendingNotifications.count() != oldSize + 1 )
            ++erasedMessages;
        } else
          ++modifiedMessages;
      } else if ( needsSplit ) {
        // If it's not queued at least make sure we fetch all the items from split
        // notifications in one go.
        itemCache->ensureCached( msg.uids(), mItemFetchScope );
      }

      // if the message contains more items, but we need to emit single-item notification,
      // split the message into one message per item and queue them
      // if the message contains only one item, but batches are not supported
      // (and thus neither is flagsModified), splitMessage() will convert the
      // notification to regular Modify with "FLAGS" part changed
      if ( needsSplit || ( !needsSplit && !supportsBatch && msg.operation() == Akonadi::NotificationMessageV2::ModifyFlags ) ) {
        const NotificationMessageV2::List split = splitMessage( msg, !supportsBatch );
        pendingNotifications << split.toList();
        appendedMessages += split.count();
      }
    }
  }

  // tell ChangeRecorder (even if 0 appended, the compression could have made changes to existing messages)
  if ( appendedMessages > 0 || modifiedMessages > 0 || erasedMessages > 0 ) {
    if ( erasedMessages > 0 )
      notificationsErased();
    else
      notificationsEnqueued( appendedMessages );
  }

  dispatchNotifications();
}

void MonitorPrivate::flushPipeline()
{
  while ( !pipeline.isEmpty() ) {
    const NotificationMessageV2 msg = pipeline.head();
    if ( ensureDataAvailable( msg ) ) {
      // dequeue should be before emit, otherwise stuff might happen (like dataAvailable
      // being called again) and we end up dequeuing an empty pipeline
      pipeline.dequeue();
      emitNotification( msg );
    } else {
      break;
    }
  }
}

void MonitorPrivate::dataAvailable()
{
  flushPipeline();
  dispatchNotifications();
}

void MonitorPrivate::dispatchNotifications()
{
  // Note that this code is not used in a ChangeRecorder (pipelineSize==0)
  while ( pipeline.size() < pipelineSize() && !pendingNotifications.isEmpty() ) {
    const NotificationMessageV2 msg = pendingNotifications.dequeue();
    if ( ensureDataAvailable( msg ) && pipeline.isEmpty() ) {
      emitNotification( msg );
    } else {
      pipeline.enqueue( msg );
    }
  }
}

bool MonitorPrivate::emitItemsNotification( const NotificationMessageV2 &msg, const Item::List &items,
                                            const Collection &collection, const Collection &collectionDest  )
{
  Q_ASSERT( msg.type() == NotificationMessageV2::Items );
  Collection col = collection;
  Collection colDest = collectionDest;
  if ( !col.isValid() ) {
    col = Collection( msg.parentCollection() );
    col.setResource( QString::fromUtf8( msg.resource() ) );
  }
  if ( !colDest.isValid() ) {
    colDest = Collection( msg.parentDestCollection() );
    // HACK: destination resource is delivered in the parts field...
    if ( !msg.itemParts().isEmpty() )
      colDest.setResource( QString::fromLatin1( *(msg.itemParts().begin()) ) );
  }

  QMap<NotificationMessageV2::Id,NotificationMessageV2::Entity> msgEntities = msg.entities();
  Item::List its = items;
  QMutableListIterator<Item> iter( its );
  while ( iter.hasNext() ) {
    Item it = iter.next();
    if ( it.isValid() ) {
      const NotificationMessageV2::Entity msgEntity = msgEntities[ it.id() ];
      if ( msg.operation() == NotificationMessageV2::Remove ) {
        it.setRemoteId( msgEntity.remoteId );
        it.setRemoteRevision( msgEntity.remoteRevision );
        it.setMimeType( msgEntity.mimeType );
      }

      if ( !it.parentCollection().isValid() ) {
        if ( msg.operation() == NotificationMessageV2::Move ) {
          it.setParentCollection( colDest );
        } else {
          it.setParentCollection( col );
        }
      } else {
        // item has a valid parent collection, most likely due to retrieved ancestors
        // still, collection might contain extra info, so inject that
        if ( it.parentCollection() == col ) {
          const Collection oldParent = it.parentCollection();
          if ( oldParent.parentCollection().isValid() && !col.parentCollection().isValid() )
            col.setParentCollection( oldParent.parentCollection() ); // preserve ancestor chain
          it.setParentCollection( col );
        } else {
          // If one client does a modify followed by a move we have to make sure that the
          // AgentBase::itemChanged() in another client always sees the parent collection
          // of the item before it has been moved.
          if ( msg.operation() != NotificationMessageV2::Move )
            it.setParentCollection( col );
        }
      }
      iter.setValue( it );
      msgEntities.remove( it.id() );
    } else {
      // remove the invalid item
      iter.remove();
    }
  }


  // Now reconstruct any items there were left in msgItems
  Q_FOREACH( const NotificationMessageV2::Entity &msgItem, msgEntities ) {
    Item it( msgItem.id );
    it.setRemoteId( msgItem.remoteId );
    it.setRemoteRevision( msgItem.remoteRevision );
    it.setMimeType( msgItem.mimeType );
    if ( msg.operation() == NotificationMessageV2::Move ) {
      it.setParentCollection( colDest );
    } else {
      it.setParentCollection( col );
    }
    its << it;
  }

  bool handled = false;
  switch ( msg.operation() ) {
    case NotificationMessageV2::Add:
      if ( q_ptr->receivers( SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) ) > 0 ) {
        Q_ASSERT( its.count() == 1 );
        emit q_ptr->itemAdded( its.first(), col );
        return true;
      }
      return false;
    case NotificationMessageV2::Modify:
      if ( q_ptr->receivers( SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)) ) > 0 ) {
        Q_ASSERT( its.count() == 1 );
        emit q_ptr->itemChanged( its.first(), msg.itemParts() );
        return true;
      }
      return false;
    case NotificationMessageV2::ModifyFlags:
      if ( q_ptr->receivers( SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>)) ) > 0 ) {
        emit q_ptr->itemsFlagsChanged( its, msg.addedFlags(), msg.removedFlags() );
        handled = true;
      }
      return handled;
    case NotificationMessageV2::Move:
      if ( q_ptr->receivers( SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) ) > 0 ) {
        Q_ASSERT( its.count() == 1 );
        emit q_ptr->itemMoved( its.first(), col, colDest );
        handled = true;
      }
      if ( q_ptr->receivers( SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection)) ) > 0 ) {
        emit q_ptr->itemsMoved( its, col, colDest );
        handled = true;
      }
      return handled;
    case NotificationMessageV2::Remove:
      if ( q_ptr->receivers( SIGNAL(itemRemoved(Akonadi::Item)) ) > 0 ) {
        Q_ASSERT( its.count() == 1 );
        emit q_ptr->itemRemoved( its.first() );
        handled = true;
      }
      if ( q_ptr->receivers( SIGNAL(itemsRemoved(Akonadi::Item::List)) ) > 0 ) {
        emit q_ptr->itemsRemoved( its );
        handled = true;
      }
      return handled;
    case NotificationMessageV2::Link:
      if ( q_ptr->receivers( SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) ) > 0 ) {
        Q_ASSERT( its.count() == 1 );
        emit q_ptr->itemLinked( its.first(), col );
        handled = true;
      }
      if ( q_ptr->receivers( SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection)) ) > 0 ) {
        emit q_ptr->itemsLinked( its, col );
        handled = true;
      }
      return handled;
    case NotificationMessageV2::Unlink:
      if ( q_ptr->receivers( SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) ) > 0 ) {
        Q_ASSERT( its.count() == 1 );
        emit q_ptr->itemUnlinked( its.first(), col );
        handled = true;
      }
      if ( q_ptr->receivers( SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection)) ) > 0 ) {
        emit q_ptr->itemsUnlinked( its, col );
        handled = true;
      }
      return handled;
    default:
      kDebug() << "Unknown operation type" << msg.operation() << "in item change notification";
  }

  return false;
}

bool MonitorPrivate::emitCollectionNotification( const NotificationMessageV2 &msg, const Collection &col,
                                                 const Collection &par, const Collection &dest )
{
  Q_ASSERT( msg.type() == NotificationMessageV2::Collections );
  Collection parent = par;
  if ( !parent.isValid() )
    parent = Collection( msg.parentCollection() );
  Collection destination = dest;
  if ( !destination.isValid() )
    destination = Collection( msg.parentDestCollection() );

  Collection collection = col;
  NotificationMessageV2::Entity msgEntities = msg.entities().values().first();
  if ( !collection.isValid() || msg.operation() == NotificationMessageV2::Remove ) {
    collection = Collection( msgEntities.id );
    collection.setResource( QString::fromUtf8( msg.resource() ) );
    collection.setRemoteId( msgEntities.remoteId );
  }

  if ( !collection.parentCollection().isValid() ) {
    if ( msg.operation() == NotificationMessageV2::Move )
      collection.setParentCollection( destination );
    else
      collection.setParentCollection( parent );
  }

  switch ( msg.operation() ) {
    case NotificationMessageV2::Add:
      if ( q_ptr->receivers( SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) ) == 0 )
        return false;
      emit q_ptr->collectionAdded( collection, parent );
      return true;
    case NotificationMessageV2::Modify:
      if ( q_ptr->receivers( SIGNAL(collectionChanged(Akonadi::Collection)) ) == 0
        && q_ptr->receivers( SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)) ) == 0 )
        return false;
      emit q_ptr->collectionChanged( collection );
      emit q_ptr->collectionChanged( collection, msg.itemParts() );
      return true;
    case NotificationMessageV2::Move:
      if ( q_ptr->receivers( SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) ) == 0 )
        return false;
      emit q_ptr->collectionMoved( collection, parent, destination );
      return true;
    case NotificationMessageV2::Remove:
      if ( q_ptr->receivers( SIGNAL(collectionRemoved(Akonadi::Collection)) ) == 0 )
        return false;
      emit q_ptr->collectionRemoved( collection );
      return true;
    case NotificationMessageV2::Subscribe:
      if ( q_ptr->receivers( SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)) ) == 0 )
        return false;
      if ( !monitorAll ) // ### why??
        emit q_ptr->collectionSubscribed( collection, parent );
      return true;
    case NotificationMessageV2::Unsubscribe:
      if ( q_ptr->receivers( SIGNAL(collectionUnsubscribed(Akonadi::Collection)) ) == 0 )
        return false;
      if ( !monitorAll ) // ### why??
        emit q_ptr->collectionUnsubscribed( collection );
      return true;
    default:
      kDebug() << "Unknown operation type" << msg.operation() << "in collection change notification";
  }

  return false;
}

void MonitorPrivate::invalidateCaches( const NotificationMessageV2& msg )
{
  // remove invalidates
  if ( msg.operation() == NotificationMessageV2::Remove ) {
    if ( msg.type() == NotificationMessageV2::Collections ) {
      Q_FOREACH( qint64 uid, msg.uids() )
        collectionCache->invalidate( uid );
    } else if ( msg.type() == NotificationMessageV2::Items ) {
      itemCache->invalidate( msg.uids() );
    }
  }

  // modify removes the cache entry, as we need to re-fetch
  // And subscription modify the visibility of the collection by the collectionFetchScope.
  if ( msg.operation() == NotificationMessageV2::Modify
        || msg.operation() == NotificationMessageV2::ModifyFlags
        || msg.operation() == NotificationMessageV2::Move
        || msg.operation() == NotificationMessageV2::Subscribe ) {
    if ( msg.type() == NotificationMessageV2::Collections ) {
      Q_FOREACH( quint64 uid, msg.uids() )
        collectionCache->update( uid, mCollectionFetchScope );
    } else if ( msg.type() == NotificationMessageV2::Items ) {
      itemCache->update( msg.uids(), mItemFetchScope );
    }
  }
}

void MonitorPrivate::invalidateCache( const Collection &col )
{
  collectionCache->update( col.id(), mCollectionFetchScope );
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

Akonadi::Collection::Id MonitorPrivate::deref( Collection::Id id )
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
  m_buffer.removeOne(id);
}

Akonadi::Collection::Id MonitorPrivate::PurgeBuffer::buffer( Collection::Id id )
{
  // Ensure that we don't put a duplicate @p id into the buffer.
  purge( id );

  Collection::Id bumpedId = -1;
  if ( m_buffer.size() == MAXBUFFERSIZE )
  {
    bumpedId = m_buffer.dequeue();
    purge( bumpedId );
  }

  m_buffer.enqueue( id );

  return bumpedId;
}

void MonitorPrivate::notifyCollectionStatisticsWatchers(Entity::Id collection, const QByteArray& resource) {
  if ( collection > 0 && (monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) ) ) {
    recentlyChangedCollections.insert( collection );
    if ( !statisticsCompressionTimer.isActive() )
      statisticsCompressionTimer.start();
  }
}

// @endcond
