/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "subscribe.h"

#include "imapstreamparser.h"
#include <protocol_p.h>
#include <handlerhelper.h>
#include <connection.h>
#include <storage/datastore.h>
#include <storage/transaction.h>

using namespace Akonadi::Server;

Subscribe::Subscribe( bool subscribe )
  : mSubscribe( subscribe )
{
}

bool Subscribe::parseStream()
{
  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  QByteArray buffer;
  while ( !m_streamParser->atCommandEnd() ) {
    buffer = m_streamParser->readString();
    if ( buffer.isEmpty() ) {
      break;
    }
    Collection col = HandlerHelper::collectionFromIdOrName( buffer );
    if ( !col.isValid() ) {
      return failureResponse( "Invalid collection" );
    }
    if ( col.enabled() == mSubscribe ) {
      continue;
    }
    // TODO do all changes in one db operation
    col.setEnabled( mSubscribe );
    if ( !col.update() ) {
      return failureResponse( "Unable to change subscription" );
    }
    store->notificationCollector()->collectionChanged( col, QList<QByteArray>() << AKONADI_PARAM_ENABLED );
    if ( mSubscribe ) {
      store->notificationCollector()->collectionSubscribed( col );
    } else {
      store->notificationCollector()->collectionUnsubscribed( col );
    }
  }

  if ( !transaction.commit() ) {
    return failureResponse( "Cannot commit transaction." );
  }

  return successResponse( "Completed" );
}
