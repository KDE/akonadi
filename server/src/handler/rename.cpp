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
#include <akonadi/private/imapparser.h>
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
  QByteArray oldName;
  QString newName;
  if ( pos < 0 )
    return failureResponse( "Bad syntax" );

  pos = ImapParser::parseString( line, oldName, pos );
  ImapParser::parseString( line, newName, pos );

  if ( oldName.isEmpty() || newName.isEmpty() )
    return failureResponse( "Collection name must not be empty" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  Location location = HandlerHelper::collectionFromIdOrName( newName.toUtf8() );
  if ( location.isValid() )
    return failureResponse( "Collection already exists" );
  location = HandlerHelper::collectionFromIdOrName( oldName );
  if ( !location.isValid() )
    return failureResponse( "No such collection" );

  QString parentPath;
  int index = newName.lastIndexOf( QLatin1Char('/') );
  if ( index > 0 )
    parentPath = newName.mid( index + 1 );
  Location parent = HandlerHelper::collectionFromIdOrName( parentPath.toUtf8() );
  newName = newName.left( index );
  int parentId = 0;
  if ( parent.isValid() )
    parentId = parent.id();

  if ( !db->renameLocation( location, parentId, newName ) )
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
