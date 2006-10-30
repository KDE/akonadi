/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "modify.h"

#include <akonadiconnection.h>
#include <storage/datastore.h>
#include <storage/entity.h>
#include <storage/transaction.h>
#include <imapparser.h>
#include <handlerhelper.h>
#include <response.h>

using namespace Akonadi;

Akonadi::Modify::Modify()
{
}

Akonadi::Modify::~ Modify()
{
}

bool Akonadi::Modify::handleLine(const QByteArray & line)
{
  int pos = line.indexOf( ' ' ) + 1; // skip tag
  pos = line.indexOf( ' ', pos ); // skip command
  if ( pos < 0 )
    return failureResponse( "Invalid syntax" );

  QString collection;
  pos = ImapParser::parseString( line, collection, pos );
  collection = HandlerHelper::normalizeCollectionName( collection );

  if ( collection.isEmpty() )
    return failureResponse( "Cannot modify root collection." );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  Location location = Location::retrieveByName( collection );
  if ( !location.isValid() )
    return failureResponse( "No such collection." );

  while ( pos < line.length() ) {
    QByteArray type;
    pos = ImapParser::parseString( line, type, pos );
    if ( type == "MIMETYPES" ) {
      QList<QByteArray> mimeTypes;
      pos = ImapParser::parseParenthesizedList( line, mimeTypes, pos );
      if ( !db->removeMimeTypesForLocation( location.id() ) )
        return failureResponse( "Unable to modify collection mimetypes." );
      foreach ( QByteArray ba, mimeTypes )
        if ( !db->appendMimeTypeForLocation( location.id(), QString::fromUtf8(ba) ) )
          return failureResponse( "Unable to modify collection mimetypes." );
    } else
      return failureResponse( "Unknown modify type" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  Response response;
  response.setTag( tag() );
  response.setString( "MODIFY done" );
  emit responseAvailable( response );
  return true;
}
