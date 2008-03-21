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
#include <akonadi/private/imapparser_p.h>
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

  QByteArray collection;
  pos = ImapParser::parseString( line, collection, pos );
  Location location = HandlerHelper::collectionFromIdOrName( collection );
  if ( !location.isValid() )
    return failureResponse( "No such collection" );
  if ( location.id() == 0 )
    return failureResponse( "Cannot modify root collection" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  while ( pos < line.length() ) {
    QByteArray type;
    pos = ImapParser::parseString( line, type, pos );
    if ( type == "MIMETYPE" ) {
      QList<QByteArray> mimeTypes;
      pos = ImapParser::parseParenthesizedList( line, mimeTypes, pos );
      if ( !db->removeMimeTypesForLocation( location.id() ) )
        return failureResponse( "Unable to modify collection mimetypes." );
      QStringList mts;
      foreach ( QByteArray ba, mimeTypes )
        mts << QString::fromLatin1(ba);
      if ( !db->appendMimeTypeForLocation( location.id(), mts ) )
        return failureResponse( "Unable to modify collection mimetypes." );
    } else if ( type == "CACHEPOLICY" ) {
      pos = HandlerHelper::parseCachePolicy( line, location, pos );
      db->notificationCollector()->collectionChanged( location );
      if ( !location.update() )
        return failureResponse( "Unable to change cache policy" );
    } else if ( type == "NAME" ) {
      QString newName;
      pos = ImapParser::parseString( line, newName, pos );
      if ( !db->renameLocation( location, location.parentId(), newName ) )
        return failureResponse( "Unable to rename collection" );
    } else if ( type == "PARENT" ) {
      int newParent;
      bool ok = false;
      pos = ImapParser::parseNumber( line, newParent, &ok, pos );
      if ( !ok )
        return failureResponse( "Invalid syntax" );
      if ( !db->renameLocation( location, newParent, location.name() ) )
        return failureResponse( "Unable to reparent colleciton" );
    } else if ( type == "REMOTEID" ) {
      // FIXME: missing change notification
      QString rid;
      pos = ImapParser::parseString( line, rid, pos );
      if ( rid == location.remoteId() )
        continue;
      location.setRemoteId( rid );
      if ( !location.update() )
        return failureResponse( "Unable to change remote identifier" );
    } else if ( type.isEmpty() ) {
      break; // input end
    } else {
      // custom attribute
      bool removeOnly = false;
      if ( type.startsWith( '-' ) ) {
        type = type.mid( 1 );
        removeOnly = true;
      }
      if ( !db->removeCollectionAttribute( location, type ) )
        return failureResponse( "Unable to remove custom collection attribute" );
      if ( ! removeOnly ) {
        QByteArray value;
        pos = ImapParser::parseString( line, value, pos );
        if ( !db->addCollectionAttribute( location, type, value ) )
          return failureResponse( "Unable to add custom collection attribute" );
      }
    }
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  Response response;
  response.setTag( tag() );
  response.setString( "MODIFY done" );
  emit responseAvailable( response );
  return true;
}
