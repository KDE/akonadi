/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "idleclient.h"
#include "idlemanager.h"
#include "libs/protocol_p.h"

using namespace Akonadi;

IdleClient::IdleClient( AkonadiConnection *connection, const QByteArray &clientId )
  : QObject()
  , mConnection( connection )
  , mClientId( clientId )
  , mRecordChanges( true )
  , mFrozen( true )
  , mMonitorAll( true )
{
}

IdleClient::~IdleClient()
{
}

AkonadiConnection *IdleClient::connection() const
{
  return mConnection;
}

void IdleClient::freeze()
{
  mFrozen = true;
}

void IdleClient::thaw()
{
  mFrozen = false;
}

bool IdleClient::isFrozen() const
{
  return mFrozen;
}

void IdleClient::done()
{
}

bool IdleClient::replayed( const ImapSet &set )
{
  return false;
}

bool IdleClient::record( const ImapSet &set )
{
  return false;
}

void IdleClient::setRecordChanges( bool recordChanges )
{
  mRecordChanges = recordChanges;
}

bool IdleClient::recordChanges()
{
  return mRecordChanges;
}

void IdleClient::setFetchScope( const FetchScope &fetchScope )
{
  IdleManager::self()->updateGlobalFetchScope( mFetchScope, fetchScope );
  mFetchScope = fetchScope;
}

const FetchScope &IdleClient::fetchScope() const
{
  return mFetchScope;
}

void IdleClient::setMonitoredItems( const QSet<qint64> &items )
{
  mMonitoredItems = items;
  updateMonitorAll();
}

void IdleClient::addMonitoredItems( const QSet<qint64> &items )
{
  mMonitoredItems += items;
  updateMonitorAll();
}

void IdleClient::removeMonitoredItems( const QSet<qint64> &items )
{
  mMonitoredItems -= items;
  updateMonitorAll();
}

const QSet<qint64> &IdleClient::monitoredItems() const
{
  return mMonitoredItems;
}

void IdleClient::setMonitoredCollections( const QSet<qint64> &collectons )
{
  mMonitoredCollections = collectons;
  updateMonitorAll();
}

void IdleClient::addMonitoredCollections( const QSet<qint64> &collections )
{
  mMonitoredCollections += collections;
  updateMonitorAll();
}

void IdleClient::removeMonitoredCollections( const QSet<qint64> &collections )
{
  mMonitoredCollections -= collections;
  updateMonitorAll();
}

const QSet<qint64> &IdleClient::monitoredCollections() const
{
  return mMonitoredCollections;
}

void IdleClient::setMonitoredMimeTypes( const QSet<QByteArray> &mimeTypes )
{
  mMonitoredMimeTypes = mimeTypes;
  updateMonitorAll();
}

void IdleClient::addMonitoredMimeTypes( const QSet<QByteArray> &mimeTypes )
{
  mMonitoredMimeTypes += mimeTypes;
  updateMonitorAll();
}

void IdleClient::removeMonitoredMimeTypes( const QSet<QByteArray> &mimeTypes )
{
  mMonitoredMimeTypes -= mimeTypes;
  updateMonitorAll();
}

const QSet<QByteArray> &IdleClient::monitoredMimeTypes() const
{
  return mMonitoredMimeTypes;
}

void IdleClient::setMonitoredResources( const QSet<QByteArray> &resources )
{
  mMonitoredResources = resources;
  updateMonitorAll();
}

void IdleClient::addMonitoredResources( const QSet<QByteArray> &resources )
{
  mMonitoredResources += resources;
  updateMonitorAll();
}

void IdleClient::removeMonitoredResources( const QSet<QByteArray> &resources )
{
  mMonitoredResources -= resources;
  updateMonitorAll();
}

const QSet<QByteArray> &IdleClient::monitoredResources() const
{
  return mMonitoredResources;
}

void IdleClient::setIgnoredSessions( const QSet<QByteArray> &sessions )
{
  mIgnoredSessions = sessions;
}

void IdleClient::addIgnoredSessions( const QSet<QByteArray> &sessions )
{
  mIgnoredSessions += sessions;
}

void IdleClient::removeIgnoredSessions( const QSet<QByteArray> &sessions )
{
  mIgnoredSessions -= sessions;
}

const QSet<QByteArray> &IdleClient::ignoredSessions() const
{
  return mIgnoredSessions;
}

void IdleClient::setMonitoredOperations( const QSet<QByteArray> &operations )
{
  mMonitoredOperations = operations;
  updateMonitorAll();
}

void IdleClient::addMonitoredOperations( const QSet<QByteArray> &operations )
{
  mMonitoredOperations += operations;
  updateMonitorAll();
}

void IdleClient::removeMonitoredOperations( const QSet<QByteArray> &operations )
{
  mMonitoredOperations -= operations;
  updateMonitorAll();
}

const QSet<QByteArray> &IdleClient::monitoredOperations() const
{
  return mMonitoredOperations;
  updateMonitorAll();
}

void IdleClient::updateMonitorAll() const
{
  if ( !mMonitoredItems.isEmpty() ) {
    mMonitorAll = false;
    return;
  }

  if ( !mMonitoredCollections.isEmpty() ) {
    mMonitorAll = false;
    return;
  }

  if ( !mMonitoredMimeTypes.isEmpty() ) {
    mMonitorAll = false;
    return;
  }

  if ( !mMonitoredResources.isEmpty() ) {
    mMonitorAll = false;
    return;
  }

  if ( !mMonitoredOperations.isEmpty() ) {
    mMonitorAll = false;
    return;
  }

  // Ignored sessions have no impact on monitorAll

  mMonitorAll = true;
}

bool IdleClient::isCollectionMonitored( Entity::Id id ) const
{
  if ( id < 0 ) {
    return false;
  } else if ( mMonitoredCollections.contains( id ) ) {
    return true;
  } else if ( mMonitoredCollections.contains( 0 ) ) {
    return true;
  }
  return false;
}

bool IdleClient::isMimeTypeMonitored( const QString &mimeType ) const
{
  // FIXME: Handle mimetype aliases

  return mMonitoredMimeTypes.contains( mimeType.toLatin1() );
}

bool IdleClient::isMoveDestinationResourceMonitored( const NotificationMessageV2 &msg ) const
{
  if ( msg.operation() != NotificationMessageV2::Move ) {
    return false;
  }
  return mMonitoredResources.contains( msg.destinationResource() );
}

bool IdleClient::acceptsNotification( const NotificationMessageV2 &msg )
{
  // session is ignored
  if ( mIgnoredSessions.contains( msg.sessionId() ) ) {
    return false;
  }

  if ( msg.entities().count() == 0 ) {
    return false;
  }

  // user requested everything
  if ( mMonitorAll && msg.type() != NotificationMessageV2::InvalidType ) {
    return true;
  }

  if ( ( msg.operation() == NotificationMessageV2::Add && !mMonitoredOperations.contains( AKONADI_OPERATION_ADD ) )
      || ( msg.operation() == NotificationMessageV2::Modify && !mMonitoredOperations.contains( AKONADI_OPERATION_MODIFY ) )
      || ( msg.operation() == NotificationMessageV2::ModifyFlags && !mMonitoredOperations.contains( AKONADI_OPERATION_MODIFYFLAGS ) )
      || ( msg.operation() == NotificationMessageV2::Remove && !mMonitoredOperations.contains( AKONADI_OPERATION_REMOVE ) )
      || ( msg.operation() == NotificationMessageV2::Move && !mMonitoredOperations.contains( AKONADI_OPERATION_MOVE ) )
      || ( msg.operation() == NotificationMessageV2::Link && !mMonitoredOperations.contains( AKONADI_OPERATION_LINK ) )
      || ( msg.operation() == NotificationMessageV2::Unlink && !mMonitoredOperations.contains( AKONADI_OPERATION_UNLINK ) )
      || ( msg.operation() == NotificationMessageV2::Subscribe && !mMonitoredOperations.contains( AKONADI_OPERATION_SUBSCRIBE ) )
      || ( msg.operation() == NotificationMessageV2::Unsubscribe && !mMonitoredOperations.contains( AKONADI_OPERATION_UNSUBSCRIBE ) ) ) {
        return false;
  }

  switch ( msg.type() ) {
  case NotificationMessageV2::InvalidType:
    akDebug() << "Received invalid change notification!";
    return false;

  case NotificationMessageV2::Items:
    // we have a resource or mimetype filter
    if ( !mMonitoredResources.isEmpty() || !mMonitoredMimeTypes.isEmpty() ) {
      if ( mMonitoredResources.contains( msg.resource() ) ) {
        return true;
      }

      if ( isMoveDestinationResourceMonitored( msg ) ) {
        return true;
      }

      Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
        if ( isMimeTypeMonitored( entity.mimeType ) ) {
          return true;
        }
      }

      return false;
    }

    // we explicitly monitor that item or the collections it's in
    Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
      if ( mMonitoredItems.contains( entity.id ) ) {
        return true;
      }
    }

    return isCollectionMonitored( msg.parentCollection() )
        || isCollectionMonitored( msg.parentDestCollection() );

  case NotificationMessageV2::Collections:
    // we have a resource filter
    if ( !mMonitoredResources.isEmpty() ) {
      const bool resourceMatches = mMonitoredResources.contains( msg.resource() )
                                  || isMoveDestinationResourceMonitored( msg );

      // a bit hacky, but match the behaviour from the item case,
      // if resource is the only thing we are filtering on, stop here, and if the resource filter matched, of course
      if ( mMonitoredMimeTypes.isEmpty() || resourceMatches ) {
        return resourceMatches;
      }
      // else continue
    }

    // we explicitly monitor that colleciton, or all of them
    Q_FOREACH ( const NotificationMessageV2::Entity &entity, msg.entities() ) {
      if ( isCollectionMonitored( entity.id ) ) {
        return true;
      }
    }

    return isCollectionMonitored( msg.parentCollection() )
        || isCollectionMonitored( msg.parentDestCollection() );
  }

  return false;
}

void IdleClient::dispatchNotification( const NotificationMessageV2 &msg,
                                       const QSqlQuery &itemsQuery,
                                       const QSqlQuery &partsQuery,
                                       const QSqlQuery &flagsQuery )
{

}

void IdleClient::dispatchNotification( const NotificationMessageV2 &msg,
                                       const Collection &collection )
{

}



