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

#include "rename.h"
#include <akonadiconnection.h>
#include <handlerhelper.h>
#include <imapparser.h>
#include <response.h>
#include <storage/datastore.h>
#include <storage/entity.h>

using namespace Akonadi;

Akonadi::Rename::Rename()
{
}

Akonadi::Rename::~ Rename()
{
}

bool Akonadi::Rename::handleLine(const QByteArray & line)
{
  int pos = line.indexOf( ' ' ) + 1; // skip tag
  pos = line.indexOf( ' ', pos ); // skip command
  QByteArray oldName;
  QByteArray newName;
  if ( pos < 0 )
    return failureResponse( "Bad syntax" );

  pos = PIM::ImapParser::parseString( line, oldName, pos );
  oldName = HandlerHelper::normalizeCollectionName( oldName );
  PIM::ImapParser::parseString( line, newName, pos );
  newName = HandlerHelper::normalizeCollectionName( newName );

  if ( oldName.isEmpty() || newName.isEmpty() )
    return failureResponse( "Collection name must not be empty" );

  DataStore *db = connection()->storageBackend();
  Location location = db->locationByName( newName );
  if ( location.isValid() )
    return failureResponse( "Collection already exists" );
  location = db->locationByName( oldName );
  if ( !location.isValid() )
    return failureResponse( "No such collection" );

  // rename all child collections
  QList<Location> locations = db->listLocations();
  oldName += '/';
  foreach ( Location location, locations ) {
    if ( location.location().startsWith( oldName ) ) {
      // TODO: error handling
      QString name = location.location();
      name = name.replace( 0, oldName.length(), QString::fromUtf8(newName) + '/' );
      db->renameLocation( location, name );
    }
  }
  // TODO: error handling
  db->renameLocation( location, newName );

  Response response;
  response.setTag( tag() );
  response.setString( "RENAME done" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}
