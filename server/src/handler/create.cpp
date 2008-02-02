/***************************************************************************
 *   Copyright (C) 2006 by Ingo Kloecker <kloecker@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "create.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"

#include "imapparser.h"
#include "response.h"

using namespace Akonadi;

Create::Create(): Handler()
{
}


Create::~Create()
{
}


bool Create::handleLine(const QByteArray& line )
{
  int pos = line.indexOf( ' ' ); // skip tag
  pos = line.indexOf( ' ', pos + 1 ); // skip command

  QString name;
  pos = ImapParser::parseString( line, name, pos );

  bool ok = false;
  int parentId = -1;
  Location parent;
  pos = ImapParser::parseNumber( line, parentId, &ok, pos );
  if ( !ok ) { // RFC 3501 compat
    QString parentPath;
    int index = name.lastIndexOf( QLatin1Char('/') );
    if ( index > 0 ) {
      parentPath = name.left( index );
      name = name.mid( index + 1 );
      parent = HandlerHelper::collectionFromIdOrName( parentPath.toUtf8() );
    } else
      parentId = 0;
  } else {
    if ( parentId > 0 )
      parent = Location::retrieveById( parentId );
  }

  if ( parentId != 0 && !parent.isValid() )
    return failureResponse( "Parent collection not found" );

  if ( name.isEmpty() )
    return failureResponse( "Invalid collection name" );

  int resourceId = 0;
  MimeType::List parentContentTypes;
  if ( parent.isValid() ) {
    // check if parent can contain a sub-folder
    parentContentTypes = parent.mimeTypes();
    bool found = false;
    foreach ( const MimeType mt, parentContentTypes ) {
      if ( mt.name() == QLatin1String( "inode/directory" ) ) {
        found = true;
        break;
      }
    }
    if ( !found )
      return failureResponse( "Parent collection can not contain sub-collections" );

    // inherit resource
    resourceId = parent.resourceId();
  } else {
    // deduce owning resource from current session id
    QString sessionId = QString::fromUtf8( connection()->sessionId() );
    Resource res = Resource::retrieveByName( sessionId );
    if ( !res.isValid() )
      return failureResponse( "Cannot create top-level collection" );
    resourceId = res.id();
  }

  Location location;
  location.setParentId( parentId );
  location.setName( name );
  location.setResourceId( resourceId );

  // attributes
  QList<QByteArray> attributes;
  QList<QByteArray> mimeTypes;
  QList< QPair<QByteArray, QByteArray> > userDefAttrs;
  bool mimeTypesSet = false;
  pos = ImapParser::parseParenthesizedList( line, attributes, pos );
  for ( int i = 0; i < attributes.count() - 1; i += 2 ) {
    const QByteArray key = attributes.at( i );
    const QByteArray value = attributes.at( i + 1 );
    if ( key == "REMOTEID" ) {
      location.setRemoteId( QString::fromUtf8( value ) );
    } else if ( key == "MIMETYPE" ) {
      ImapParser::parseParenthesizedList( value, mimeTypes );
      mimeTypesSet = true;
    } else if ( key == "CACHEPOLICY" ) {
      HandlerHelper::parseCachePolicy( value, location );
    } else {
      userDefAttrs << qMakePair( key, value );
    }
  }

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  if ( !db->appendLocation( location ) )
    return failureResponse( "Could not create collection" );

  QStringList effectiveMimeTypes;
  if ( mimeTypesSet ) {
    foreach ( const QByteArray b, mimeTypes )
      effectiveMimeTypes << QString::fromUtf8( b );
  } else {
    foreach ( const MimeType mt, parentContentTypes )
      effectiveMimeTypes << mt.name();
  }
  if ( !db->appendMimeTypeForLocation( location.id(), effectiveMimeTypes ) )
    return failureResponse( "Unable to append mimetype for collection." );

  // store user defined attributes
  typedef QPair<QByteArray,QByteArray> QByteArrayPair;
  foreach ( const QByteArrayPair attr, userDefAttrs ) {
    if ( !db->addCollectionAttribute( location, attr.first, attr.second ) )
      return failureResponse( "Unable to add collection attribute." );
  }

  Response response;
  response.setUntagged();

   // write out collection details
  QByteArray b = QByteArray::number( location.id() ) + ' '
               + QByteArray::number( location.parentId() ) + " ()"; // TODO: add attributes
  response.setString( b );
  emit responseAvailable( response );

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  response.setSuccess();
  response.setString( "CREATE completed" );
  response.setTag( tag() );
  emit responseAvailable( response );

  deleteLater();
  return true;
}

