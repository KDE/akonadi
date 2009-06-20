/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
#include "imapstreamparser.h"
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

bool Akonadi::Rename::parseStream()
{
  QByteArray oldName = m_streamParser->readString();
  QByteArray newName = m_streamParser->readString();

  if ( oldName.isEmpty() || newName.isEmpty() )
    return failureResponse( "Collection name must not be empty" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  Collection collection = HandlerHelper::collectionFromIdOrName( newName );
  if ( collection.isValid() )
    return failureResponse( "Collection already exists" );
  collection = HandlerHelper::collectionFromIdOrName( oldName );
  if ( !collection.isValid() )
    return failureResponse( "No such collection" );

  QByteArray parentPath;
  int index = newName.lastIndexOf( '/' );
  if ( index > 0 )
    parentPath = newName.mid( index + 1 );
  Collection parent = HandlerHelper::collectionFromIdOrName( parentPath );
  newName = newName.left( index );
  qint64 parentId = 0;
  if ( parent.isValid() )
    parentId = parent.id();

  if ( !db->renameCollection( collection, parentId, newName ) )
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

#include "rename.moc"
