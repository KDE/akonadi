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
#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/imapset_p.h>

#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

using namespace Akonadi;

bool Copy::handleLine(const QByteArray & line)
{
  QByteArray tmp;
  int pos = ImapParser::parseString( line, tmp ); // skip tag
  pos = ImapParser::parseString( line, tmp, pos ); // skip command

  ImapSet set;
  pos = ImapParser::parseSequenceSet( line, set, pos );
  if ( set.isEmpty() )
    return failureResponse( "No items specified" );

  ImapParser::parseString( line, tmp, pos );
  const Location loc = HandlerHelper::collectionFromIdOrName( tmp );
  if ( !loc.isValid() )
    return failureResponse( "No valid target specified" );

  SelectQueryBuilder<PimItem> qb;
  imapSetToQuery( set, true, qb );
  if ( !qb.exec() )
    return failureResponse( "Unable to retrieve items" );
  PimItem::List items = qb.result();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  foreach ( const PimItem item, items ) {
    if ( !copyItem( item, loc ) )
      return failureResponse( "Unable to copy item" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

  return successResponse( "COPY complete" );
}

bool Copy::copyItem(const PimItem & item, const Location & target)
{
  DataStore *store = connection()->storageBackend();
  PimItem newItem = item;
  newItem.setId( -1 );
  newItem.setRemoteId( QByteArray() );
  newItem.setLocationId( target.id() );
  Part::List parts;
  foreach ( const Part part, item.parts() ) {
    Part newPart( part );
    newPart.setPimItemId( -1 );
    parts << newPart;
  }
  return store->appendPimItem( parts, item.mimeType(), target, QDateTime::currentDateTime(), QByteArray(), newItem );
}
