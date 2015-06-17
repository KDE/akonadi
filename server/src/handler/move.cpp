/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "move.h"

#include <connection.h>
#include <entities.h>
#include <imapstreamparser.h>
#include <handlerhelper.h>
#include <cachecleaner.h>
#include <storage/datastore.h>
#include <storage/itemretriever.h>
#include <storage/itemqueryhelper.h>
#include <storage/selectquerybuilder.h>
#include <storage/transaction.h>
#include <storage/collectionqueryhelper.h>

using namespace Akonadi::Server;

Move::Move( Scope::SelectionScope scope )
  : mScope( scope )
{
}

bool Move::parseStream()
{
  mScope.parseScope( m_streamParser );

  Scope destScope( mScope.scope() );
  destScope.parseScope( m_streamParser );
  const Collection destination = CollectionQueryHelper::singleCollectionFromScope( destScope, connection() );
  const Resource destResource = destination.resource();

  if ( destination.isVirtual() ) {
      return failureResponse( "Moving items into virtual collection is not allowed" );
  }

  Collection source;
  if ( !m_streamParser->atCommandEnd() ) {
    Scope sourceScope( mScope.scope() );
    sourceScope.parseScope( m_streamParser );
    source = CollectionQueryHelper::singleCollectionFromScope( sourceScope, connection() );
  }

  if ( mScope.scope() == Scope::Rid && !source.isValid() ) {
    throw HandlerException( "RID move requires valid source collection" );
  }
  connection()->context()->setCollection( source );

  CacheCleanerInhibitor inhibitor;

  // make sure all the items we want to move are in the cache
  ItemRetriever retriever( connection() );
  retriever.setScope( mScope );
  retriever.setRetrieveFullPayload( true );
  if ( !retriever.exec() ) {
    return failureResponse( retriever.lastError() );
  }

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  SelectQueryBuilder<PimItem> qb;
  ItemQueryHelper::scopeToQuery( mScope, connection()->context(), qb );
  qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::NotEquals, destination.id() );

  const QDateTime mtime = QDateTime::currentDateTime();

  if ( qb.exec() ) {
    const QVector<PimItem> items = qb.result();
    if ( items.isEmpty() ) {
      return successResponse( "MOVE complete" );
    }

    // Split the list by source collection
    QMap<Entity::Id /* collection */, PimItem> toMove;
    QMap<Entity::Id /* collection */, Collection> sources;
    Q_FOREACH ( /*sic!*/ PimItem item, items ) {
      const Collection source = items.first().collection();
      if ( !source.isValid() ) {
        throw HandlerException( "Item without collection found!?" );
      }
      if ( !sources.contains( source.id() ) ) {
        sources.insert( source.id(), source );
      }

      if ( !item.isValid() ) {
        throw HandlerException( "Invalid item in result set!?" );
      }
      Q_ASSERT( item.collectionId() != destination.id() );

      item.setCollectionId( destination.id() );
      item.setAtime( mtime );
      item.setDatetime( mtime );
      // if the resource moved itself, we assume it did so because the change happend in the backend
      if ( connection()->context()->resource().id() != destResource.id() ) {
        item.setDirty( true );
      }

      toMove.insertMulti( source.id(), item );
    }

    // Emit notification for each source collection separately
    Q_FOREACH ( const Entity::Id &sourceId, toMove.uniqueKeys() ) {
      const PimItem::List &itemsToMove = toMove.values( sourceId ).toVector();
      const Collection &source = sources.value( sourceId );
      store->notificationCollector()->itemsMoved( itemsToMove, source, destination );

      Q_FOREACH ( PimItem moved, toMove.values( sourceId ) ) {
        // reset RID on inter-resource moves, but only after generating the change notification
        // so that this still contains the old one for the source resource
        const bool isInterResourceMove = moved.collection().resource().id() != source.resource().id();
        if ( isInterResourceMove ) {
          moved.setRemoteId( QString() );
        }

        // FIXME Could we aggregate the changes to a single SQL query?
        if ( !moved.update() ) {
          throw HandlerException( "Unable to update item" );
        }
      }
    }
  } else {
    throw HandlerException( "Unable to execute query" );
  }

  if ( !transaction.commit() ) {
    return failureResponse( "Unable to commit transaction." );
  }

  return successResponse( "MOVE complete" );
}
