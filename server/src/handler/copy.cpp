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

#include "copy.h"

#include "akonadiconnection.h"
#include "handlerhelper.h"

#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretriever.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"
#include "storage/parthelper.h"

#include "libs/imapset_p.h"
#include "imapstreamparser.h"

using namespace Akonadi;

bool Copy::copyItem(const PimItem & item, const Collection & target)
{
  qDebug() << "Copy::copyItem";
  DataStore *store = connection()->storageBackend();
  PimItem newItem = item;
  newItem.setId( -1 );
  newItem.setRev( 0 );
  newItem.setDatetime( QDateTime::currentDateTime() );
  newItem.setAtime( QDateTime::currentDateTime() );
  newItem.setRemoteId( QString() );
  newItem.setCollectionId( target.id() );
  Part::List parts;
  foreach ( const Part &part, item.parts() ) {
    Part newPart( part );
    newPart.setData(PartHelper::translateData(newPart.id(), newPart.data(), part.external()));
    newPart.setPimItemId( -1 );
    parts << newPart;
  }
  return store->appendPimItem( parts, item.mimeType(), target, QDateTime::currentDateTime(), QString(), newItem );
}

bool Copy::parseStream()
{
  ImapSet set = m_streamParser->readSequenceSet();
  if ( set.isEmpty() )
    return failureResponse( "No items specified" );

  ItemRetriever retriever( connection() );
  retriever.setItemSet( set );
  retriever.setRetrieveFullPayload( true );
  retriever.exec();

  const QByteArray tmp = m_streamParser->readString();
  const Collection col = HandlerHelper::collectionFromIdOrName( tmp );
  if ( !col.isValid() )
    return failureResponse( "No valid target specified" );

  SelectQueryBuilder<PimItem> qb;
  ItemQueryHelper::itemSetToQuery( set, qb );
  if ( !qb.exec() )
    return failureResponse( "Unable to retrieve items" );
  PimItem::List items = qb.result();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  foreach ( const PimItem &item, items ) {
    if ( !copyItem( item, col ) )
      return failureResponse( "Unable to copy item" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

  return successResponse( "COPY complete" );
}

#include "copy.moc"
