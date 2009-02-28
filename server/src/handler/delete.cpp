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
#include <handlerhelper.h>
#include "../../libs/imapparser_p.h"
#include <response.h>
#include <storage/datastore.h>
#include <storage/entity.h>
#include <storage/transaction.h>
#include "abstractsearchmanager.h"

using namespace Akonadi;

Delete::Delete() : Handler()
{
}

Delete::~Delete()
{
}

bool Delete::handleLine(const QByteArray & line)
{
  int begin = line.indexOf( " DELETE" ) + 7;
  QByteArray collectionByteArray;
  if ( line.length() > begin )
    ImapParser::parseString( line, collectionByteArray, begin );

  // prevent deletion of the root node
  if ( collectionByteArray.isEmpty() )
    return failureResponse( "Deleting everything is not allowed." );

  // check if collection exists
  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  Collection collection = HandlerHelper::collectionFromIdOrName( collectionByteArray );
  if ( !collection.isValid() )
    return failureResponse( "No such collection." );

  // handle virtual folders
  if ( collection.resource().name() == QLatin1String("akonadi_search_resource") ) {
    // don't delete virtual root
    if ( collection.parentId() == 0 )
      return failureResponse( "Cannot delete virtual root collection" );

    if ( !AbstractSearchManager::instance()->removeSearch( collection.id() ) )
      return failureResponse( "Failed to remove search from search manager" );
  }

  if ( !deleteRecursive( collection ) )
    return failureResponse( "Unable to delete collection" );

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  Response response;
  response.setTag( tag() );
  response.setString( "DELETE completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

bool Delete::deleteRecursive(Collection & col)
{
  Collection::List children = col.children();
  foreach ( Collection child, children ) {
    if ( !deleteRecursive( child ) )
      return false;
  }
  DataStore *db = connection()->storageBackend();
  return db->cleanupCollection( col );
}

#include "delete.moc"
