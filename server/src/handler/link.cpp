/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "link.h"

#include "akonadiconnection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "entities.h"

#include "imapstreamparser.h"

using namespace Akonadi;

Link::Link(bool create) : Handler(), mCreateLinks( create )
{
}

bool Link::parseStream()
{
  QByteArray tmp = m_streamParser->readString();
  const Collection collection = HandlerHelper::collectionFromIdOrName( tmp );
  if ( !collection.isValid() )
    return failureResponse( "No valid collection specified" );

  ImapSet set = m_streamParser->readSequenceSet();
  if ( set.isEmpty() )
    return failureResponse( "No valid sequence set specified" );

  SelectQueryBuilder<PimItem> qb;
  ItemQueryHelper::itemSetToQuery( set, qb );
  if ( !qb.exec() )
    return failureResponse( "Unable to execute item query" );

  PimItem::List items = qb.result();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  foreach ( const PimItem &item, items ) {
    const bool alreadyLinked = collection.relatesToPimItem( item );
    bool result = true;
    if ( mCreateLinks && !alreadyLinked ) {
      result = collection.addPimItem( item );
      store->notificationCollector()->itemLinked( item, collection );
    } else if ( !mCreateLinks && alreadyLinked ) {
      result = collection.removePimItem( item );
      store->notificationCollector()->itemUnlinked( item, collection );
    }
    if ( !result )
      return failureResponse( "Failed to modify item reference" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

  return successResponse( "LINK complete" );
}

#include "link.moc"
