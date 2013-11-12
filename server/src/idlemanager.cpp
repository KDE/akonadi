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
#include "response.h"
#include "libs/protocol_p.h"
#include "libs/imapparser_p.h"
#include "akdebug.h"

#include <QMutex>
#include <QHash>
#include <boost/concept_check.hpp>

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
    client->done();
    throw IdleException( "IDLE session for this connection already exists" );
  }

  mClientsRegistrar.insert( client->connection()->sessionId(), client );
  akDebug() << "Registered IDLE client" << client->clientId() << "for session" << client->connection()->sessionId();
}

void IdleManager::unregisterClient( IdleClient *client )
{
  QWriteLocker locker( &mRegistrarLock );
  if ( mClientsRegistrar.remove( client->connection()->sessionId() ) == 0 ) {
    throw IdleException( "No such client");
  }

  akDebug() << "Unregistered IDLE client" << client->clientId();
}

IdleClient *IdleManager::clientForConnection( AkonadiConnection *connection )
{
  QReadLocker locker( &mRegistrarLock );
  return mClientsRegistrar.value( connection->sessionId() );
}

QByteArray IdleManager::notificationOperationName( NotificationMessageV2::Operation operation ) const
{
  switch ( operation ) {
    case NotificationMessageV2::Add:
      return "ADD";
    case NotificationMessageV2::Modify:
      return "MODIFY";
    case NotificationMessageV2::ModifyFlags:
      return "MODIFYFLAGS";
    case NotificationMessageV2::Move:
      return "MOVE";
    case NotificationMessageV2::Remove:
      return "REMOVE";
    case NotificationMessageV2::Link:
      return "LINK";
    case NotificationMessageV2::Unlink:
      return "UNLINK";
    }

    Q_ASSERT( !"Invalid operation" );
    return QByteArray();
}


void IdleManager::notify( NotificationMessageV2::List &msgs )
{

  Collection collection;
  QTime timer;
  timer.start();
  Q_FOREACH ( const NotificationMessageV2 &msg, msgs ) {
    QByteArray command = "NOTIFY ";
    command += notificationOperationName( msg.operation() );

    if ( msg.type() == NotificationMessageV2::Collections ) {
      command += " COLLECTION";

      collection = fetchCollection( msg );
      mRegistrarLock.lockForRead();
      Q_FOREACH ( IdleClient *client, mClientsRegistrar ) {
        if ( client->acceptsNotification( msg ) ) {
            client->dispatchNotification( msg, collection );
        }
      }
      mRegistrarLock.unlock();
    } else if ( msg.type() == Akonadi::NotificationMessageV2::Items ) {
      command += " ITEM";

      switch ( msg.operation() ) {
      case NotificationMessageV2::Add:
      case NotificationMessageV2::Link:
      case NotificationMessageV2::Unlink:
        command += " DESTINATION " + QByteArray::number( msg.parentCollection() );
        break;
      case NotificationMessageV2::ModifyFlags:
        if ( !msg.addedFlags().isEmpty() ) {
          command += " ADDED (" + ImapParser::join( msg.addedFlags(), "," ) + ")";
        }
        if ( !msg.removedFlags().isEmpty() ) {
          command += " REMOVED (" + ImapParser::join( msg.removedFlags(), "," ) + ")";
        }
        break;
      case NotificationMessageV2::Modify:
        command += " PARTS (" + ImapParser::join( msg.itemParts(), "," ) + ")";
        break;
      case NotificationMessageV2::Move:
        command += " SOURCE " + QByteArray::number( msg.parentCollection() ) +
                   " DESTINATION " + QByteArray::number( msg.parentDestCollection() ) +
                   " RESOURCE " + msg.resource() +
                   " DESTRESOURCE " + msg.destinationResource();
        break;
      default:
        break;
      }

      ImapSet set;
      set.add( msg.uids().toVector() );
      Scope scope( Scope::Uid );
      scope.setUidSet( set );

      FetchHelper fetchHelper( scope );
      fetchHelper.setProperty( "NotificationMessageV2", QVariant::fromValue( msg ) );
      // FIXME: We assume, that if client supports IDLE, it has to support NOPAYLOADPATH
      fetchHelper.setResolvePayloadPath( false );
      connect( &fetchHelper, SIGNAL(responseAvailable(Akonadi::Response)),
                this, SLOT(fetchHelperResponseAvailable(Akonadi::Response)) );
      FetchScope fetchScope;
      if ( msg.operation() == NotificationMessageV2::Remove ) {
        fetchScope.setRemoteIdRequested( true );
        fetchScope.setRemoteRevisionRequested( true );
        fetchScope.setCacheOnly( true );
      } else {
        fetchScope = mFetchScope;
      }

      int count = fetchHelper.parseStream( command, fetchScope );

      Response response;
      response.setTag( "+" );
      response.setString( "IDLE" );
      fetchHelperResponseAvailable( response );
    }
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

void IdleManager::fetchHelperResponseAvailable( const Response &response )
{
  QReadLocker locker( &mRegistrarLock );

  // HACK: Called by FetchHelper, or directly?
  if ( sender() ) {
    const NotificationMessageV2 &msg = sender()->property( "NotificationMessageV2" ).value<NotificationMessageV2>();
    Q_FOREACH ( IdleClient *client, mClientsRegistrar ) {
      if ( client->acceptsNotification( msg ) ) {
        client->dispatchNotification( response );
      }
    }
  } else {
    Q_FOREACH ( IdleClient *client, mClientsRegistrar ) {
      client->dispatchNotification( response );
    }
  }
}
