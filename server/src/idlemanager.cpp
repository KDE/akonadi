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

#include "idlemanager.h"
#include "idleclient.h"
#include "akonadiconnection.h"
#include "storage/selectquerybuilder.h"
#include "storage/queryhelper.h"
#include "storage/itemqueryhelper.h"
#include "libs/protocol_p.h"
#include "akdebug.h"

#include <QMutex>
#include <QHash>

using namespace Akonadi;

IdleManager *IdleManager::s_instance = 0;

IdleManager* IdleManager::self()
{
  static QMutex instanceLock;
  QMutexLocker locker( &instanceLock );

  if ( s_instance == 0 ) {
    s_instance = new IdleManager();
  }

  return s_instance;
}

IdleManager::IdleManager()
  : QObject()
{
  mFetchScope.setAllAttrs( true );
  mFetchScope.setAncestorDepth( -1 );
  mFetchScope.setCacheOnly( true );
  mFetchScope.setChangedOnly( false );
  mFetchScope.setCheckCachedPayloadPartsOnly( false );
  mFetchScope.setExternalPayloadSupported( true );
  mFetchScope.setFlagsRequested( true );
  mFetchScope.setFullPayload( true );
  mFetchScope.setGidRequested( true );
  mFetchScope.setIgnoreErrors( true );
  mFetchScope.setMTimeRequested( true );
  mFetchScope.setRemoteIdRequested( true );
  mFetchScope.setRemoteRevisionRequested( true );
  mFetchScope.setSizeRequested( true );
  mFetchScope.setRequestedParts( QVector<QByteArray>() << AKONADI_PARAM_PLD_RFC822 );
}

IdleManager::~IdleManager()
{
}

void IdleManager::registerClient( IdleClient *client )
{
  QWriteLocker locker( &mRegistrarLock );
  if ( mClientsRegistrar.contains( client->connection()->sessionId() ) ) {
    throw IdleException( "IDLE session for this connection already exists" );
  }

  mClientsRegistrar.insert( client->connection()->sessionId(), client );
}

void IdleManager::unregisterClient( IdleClient *client )
{
  QWriteLocker locker( &mRegistrarLock );
  if ( mClientsRegistrar.remove( client->connection()->sessionId() ) == 0 ) {
    throw IdleException( "No such client");
  }
}

IdleClient *IdleManager::clientForConnection( AkonadiConnection *connection )
{
  QReadLocker locker( &mRegistrarLock );
  return mClientsRegistrar.value( connection->sessionId() );
}

void IdleManager::notify( NotificationMessageV2::List &msgs )
{
  QReadLocker locker( &mRegistrarLock );

  Collection collection;
  QTime timer;
  timer.start();
  Q_FOREACH ( const NotificationMessageV2 &msg, msgs ) {
    FetchHelper *helper = 0;
    QSqlQuery itemsQuery;

    if ( msg.type() == NotificationMessageV2::Collections ) {
      collection = fetchCollection( msg );
    } else if ( msg.type() == Akonadi::NotificationMessageV2::Items ) {
      if ( msg.operation() != NotificationMessageV2::Remove ) {
        ImapSet set;
        set.add( msg.uids().toVector() );
        Scope scope( Scope::Uid );
        scope.setUidSet( set );

        helper = new FetchHelper( scope, mFetchScope );
        itemsQuery = helper->buildItemQuery();
        /*
        QSqlQuery partQuery = helper.buildPartQuery( QVector<QByteArray>(), true, true );
        QSqlQuery flagQuery = helper.buildFlagQuery();
        */
      }
    }
    akDebug() << "Fetched and extracted" << itemsQuery.size() << "items in" << timer.elapsed() << "ms";

    timer.start();
    int acceptingClients = 0;
    Q_FOREACH ( IdleClient *client, mClientsRegistrar ) {
      if ( client->acceptsNotification( msg ) ) {
        ++acceptingClients;
        if ( msg.type() == NotificationMessageV2::Collections ) {
          client->dispatchNotification( msg, collection );
        } else if ( msg.type() == NotificationMessageV2::Items ) {
          client->dispatchNotification( msg, *helper, itemsQuery );
        }
      }
    }
    akDebug() << "Dispatched notification to" << acceptingClients << "clients in" << timer.elapsed() << "ms";
  }
}

Collection IdleManager::fetchCollection( const NotificationMessageV2 &msg )
{
  // Remove this once (if ever) we support batch operations on collections
  Q_ASSERT( msg.uids().size() == 1 );

  // Makes use of internal Collection cache to avoid repetitive queries
  return Collection::retrieveById( msg.uids().first() );
}

void IdleManager::updateGlobalFetchScope( const FetchScope &oldFetchScope,
                                          const FetchScope &newFetchScope )
{
  // TODO
}
