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
#include <storage/transaction.h>

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
  QString oldName;
  QString newName;
  if ( pos < 0 )
    return failureResponse( "Bad syntax" );

  pos = ImapParser::parseString( line, oldName, pos );
  oldName = HandlerHelper::normalizeCollectionName( oldName );
  ImapParser::parseString( line, newName, pos );
  newName = HandlerHelper::normalizeCollectionName( newName );

  if ( oldName.isEmpty() || newName.isEmpty() )
    return failureResponse( "Collection name must not be empty" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  Location location = db->locationByName( newName );
  if ( location.isValid() )
    return failureResponse( "Collection already exists" );
  location = db->locationByName( oldName );
  if ( !location.isValid() )
    return failureResponse( "No such collection" );

  // rename all child collections
  QList<Location> locations = db->listLocations();
  oldName += QLatin1Char('/');
  foreach ( Location location, locations ) {
    if ( location.location().startsWith( oldName ) ) {
      QString name = location.location();
      name = name.replace( 0, oldName.length(), newName + QLatin1Char('/') );
      if ( !db->renameLocation( location, name ) )
        return failureResponse( "Failed to rename collection." );
    }
  }
  if ( !db->renameLocation( location, newName ) )
    return failureResponse( "Failed to rename collection." );

  if ( !transaction.commit() )
    return failureResponse( "Failed to commit transaction." );

  Response response;
  response.setTag( tag() );
  response.setString( "RENAME done" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}
