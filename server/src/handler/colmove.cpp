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

#include "colmove.h"
#include <handlerhelper.h>
#include <storage/datastore.h>
#include <akonadiconnection.h>
#include <storage/itemretriever.h>
#include <imapstreamparser.h>
#include <storage/transaction.h>

namespace Akonadi {

ColMove::ColMove(Scope::SelectionScope scope) :
  m_scope( scope )
{
}


bool ColMove::parseStream()
{
  // TODO: use m_scope here to support collection sets and RID based operations
  Collection source = HandlerHelper::collectionFromIdOrName( m_streamParser->readString() );
  if ( !source.isValid() )
    return failureResponse( "No valid source specified" );

  qint64 target = m_streamParser->readNumber();
  if ( target < 0 )
    return failureResponse( "No valid destination specified" );

  if ( source.parentId() == target )
    return successResponse( "COLMOVE complete - nothing to do" );

  // retrieve all not yet cached items of the source
  ItemRetriever retriever( connection() );
  retriever.setCollection( source, true );
  retriever.setRetrieveFullPayload( true );
  retriever.exec();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  if ( !store->renameCollection( source, target, source.name() ) )
    return failureResponse( "Unable to reparent collection" );

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

  return successResponse( "COLMOVE complete" );
}

}

#include "colmove.moc"
