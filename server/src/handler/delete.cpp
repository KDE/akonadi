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

#include "delete.h"

#include <akonadiconnection.h>
#include <imapparser.h>
#include <response.h>
#include <storage/datastore.h>
#include <storage/entity.h>

Akonadi::Delete::Delete() : Handler()
{
}

Akonadi::Delete::~Delete()
{
}

bool Akonadi::Delete::handleLine(const QByteArray & line)
{
  int begin = line.indexOf( " DELETE" ) + 7;
  QByteArray collection;
  if ( line.length() > begin )
    PIM::ImapParser::parseQuotedString( line, collection, begin );

  // normalize collection name
  if ( collection.startsWith( '/' )  )
    collection = collection.right( collection.length() - 1 );
  if ( collection.endsWith( '/' ) )
    collection = collection.left( collection.length() - 1 );

  // prevent deletion of the root node
  if ( collection.isEmpty() )
    return failureResponse( "Deleting everything is not allowed." );

  // check if collection exists
  DataStore *db = connection()->storageBackend();
  Location location = db->locationByName( collection );
  if ( !location.isValid() )
    return failureResponse( "No such collection." );

  // delete all child collections
  QList<Location> locations = db->listLocations();
  collection += '/';
  foreach ( Location location, locations ) {
    if ( location.location().startsWith( collection ) )
      db->cleanupLocation( location );
  }

  db->cleanupLocation( location );

  Response response;
  response.setTag( tag() );
  response.setString( "DELETE completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}
