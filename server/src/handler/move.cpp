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

#include <akonadiconnection.h>
#include <entities.h>
#include <imapstreamparser.h>
#include <handlerhelper.h>
#include <storage/datastore.h>
#include <storage/itemretriever.h>
#include <storage/itemqueryhelper.h>
#include <storage/selectquerybuilder.h>
#include <storage/transaction.h>
#include <libs/imapset_p.h>

namespace Akonadi {

Move::Move( Scope::SelectionScope scope ) :
  mScope( scope )
{
}

bool Move::parseStream()
{
  mScope.parseScope( m_streamParser );
  const Collection destination = HandlerHelper::collectionFromIdOrName( m_streamParser->readString() );
  if ( !destination.isValid() )
    throw HandlerException( "Unknown destination collection" );
  const Resource destResource = destination.resource();

  // make sure all the items we want to move are in the cache
  ItemRetriever retriever( connection() );
  retriever.setScope( mScope );
  retriever.setRetrieveFullPayload( true );
  retriever.exec();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  SelectQueryBuilder<PimItem> qb;
  ItemQueryHelper::scopeToQuery( mScope, connection(), qb );
  qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::NotEquals, destination.id() );

  const QDateTime mtime = QDateTime::currentDateTime();

  if ( qb.exec() ) {
    const QList<PimItem> items = qb.result();
    if ( items.isEmpty() )
      throw HandlerException( "No items found" );
    foreach ( PimItem item, items ) {
      if ( !item.isValid() )
        throw HandlerException( "Invalid item in result set!?" );
      Q_ASSERT( item.collectionId() != destination.id() );
      const Collection source = item.collection();
      if ( !source.isValid() )
        throw HandlerException( "Item without collection found!?" );

      store->notificationCollector()->collectionChanged( source, QList<QByteArray>() ); // ### why?

      item.setCollectionId( destination.id() );
      item.setAtime( mtime );
      item.setDatetime( mtime );
      // if the resource moved itself, we assume it did so because the change happend in the backend
      if ( connection()->resourceContext().id() != destResource.id() )
        item.setDirty( true );

      if ( !item.update() )
        throw HandlerException( "Unable to update item" );

      store->notificationCollector()->itemMoved( item, source, destination );
    }
    store->notificationCollector()->collectionChanged( destination, QList<QByteArray>(), destResource.name().toUtf8() ); // ### why?
  } else {
    throw HandlerException( "Unable to execute query" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  return successResponse( "MOVE complete" );
}

}

#include "move.moc"
