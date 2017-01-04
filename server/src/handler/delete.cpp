/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "delete.h"

#include <connection.h>
#include <handlerhelper.h>
#include <response.h>
#include <storage/datastore.h>
#include <storage/entity.h>
#include <storage/transaction.h>
#include <search/searchmanager.h>
#include "imapstreamparser.h"
#include <storage/selectquerybuilder.h>
#include <storage/collectionqueryhelper.h>
#include "libs/protocol_p.h"
#include <akdebug.h>

#include <QDBusInterface>

using namespace Akonadi;
using namespace Akonadi::Server;

Delete::Delete( Scope scope )
  : Handler()
  , m_scope( scope )
{
}

bool Delete::deleteRecursive( Collection &col )
{
  Collection::List children = col.children();
  Q_FOREACH ( Collection child, children ) {
    if ( !deleteRecursive( child ) ) {
      return false;
    }
  }
  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );
  if (!db->cleanupCollection( col )) {
      return false;
  }
  if ( !transaction.commit() ) {
      return failureResponse( "Unable to commit transaction" );
  }
  return true;
}

bool Delete::parseStream()
{
  m_scope.parseScope( m_streamParser );
  connection()->context()->parseContext( m_streamParser );

  SelectQueryBuilder<Collection> qb;
  CollectionQueryHelper::scopeToQuery( m_scope, connection(), qb );
  if ( !qb.exec() ) {
    throw HandlerException( "Unable to execute collection query" );
  }
  const Collection::List collections = qb.result();
  if ( collections.isEmpty() ) {
    throw HandlerException( "No collection selected" );
  } else if ( collections.size() > 1 ) {
    throw HandlerException( "Deleting multiple collections is not yet implemented" );
  }

  // check if collection exists
  DataStore *db = connection()->storageBackend();

  Collection collection = collections.first();
  if ( !collection.isValid() ) {
    return failureResponse( "No such collection." );
  }

  // handle virtual folders
  if ( collection.resource().name() == QLatin1String( AKONADI_SEARCH_RESOURCE ) ) {
    // don't delete virtual root
    if ( collection.parentId() == 0 ) {
      return failureResponse( "Cannot delete virtual root collection" );
    }
  }

  if ( !deleteRecursive( collection ) ) {
    return failureResponse( "Unable to delete collection" );
  }

  Response response;
  response.setTag( tag() );
  response.setString( "DELETE completed" );
  Q_EMIT responseAvailable( response );
  return true;
}
