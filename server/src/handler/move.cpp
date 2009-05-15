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

  // make sure all the items we want to move are in the cache
  ItemRetriever retriever( connection() );
  retriever.setScope( mScope );
  retriever.setRetrieveFullPayload( true );
  retriever.exec();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  SelectQueryBuilder<PimItem> qb;
  ItemQueryHelper::scopeToQuery( mScope, connection(), qb );

  if ( qb.exec() ) {
    const QList<PimItem> items = qb.result();
    if ( items.isEmpty() )
      throw HandlerException( "No items found" );
    foreach ( PimItem item, items ) {
      if ( !store->updatePimItem( item, destination ) ) // TODO inline this method here once it's no longer used in store.cpp
        throw HandlerException( "Unable to move item" );
    }
  } else {
    throw HandlerException( "Unable to execute query" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  return successResponse( "MOVE complete" );
}

}

#include "move.moc"
