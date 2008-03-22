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

#include "aklist.h"

#include <QtCore/QDebug>
#include <akonadi/private/imapparser_p.h>

#include "storage/datastore.h"
#include "storage/entity.h"

#include "akonadiconnection.h"
#include "response.h"
#include "handlerhelper.h"

using namespace Akonadi;

AkList::AkList():
    Handler(),
    mOnlySubscribed( false )
{}

AkList::~AkList() {}

bool AkList::handleLine(const QByteArray& line )
{
  // parse out the reference name and mailbox name
  int pos = line.indexOf( ' ' ) + 1; // skip tag
  QByteArray tmp;

  // command
  pos = ImapParser::parseString( line, tmp, pos );
  if ( tmp == "X-AKLSUB" )
    mOnlySubscribed = true;

  qint64 baseCollection;
  bool ok = false;
  pos = ImapParser::parseNumber( line, baseCollection, &ok, pos );
  if ( !ok )
    return failureResponse( "Invalid base collection" );

  int depth;
  pos = ImapParser::parseString( line, tmp, pos );
  if ( tmp.isEmpty() )
    return failureResponse( "Specify listing depth" );
  if ( tmp == "INF" )
    depth = INT_MAX;
  else
    depth = tmp.toInt();

  QList<QByteArray> filter;
  pos = ImapParser::parseParenthesizedList( line, filter, pos );

  for ( int i = 0; i < filter.count() - 1; i += 2 ) {
    if ( filter.at( i ) == "RESOURCE" ) {
      mResource = Resource::retrieveByName( QString::fromLatin1( filter.at(i + 1) ) );
      if ( !mResource.isValid() )
        return failureResponse( "Unknown resource" );
    } else
      return failureResponse( "Invalid filter parameter" );
  }

  Location::List locations;
  if ( baseCollection != 0 ) { // not root
    Location loc = Location::retrieveById( baseCollection );
    if ( !loc.isValid() )
      return failureResponse( "Collection " + QByteArray::number( baseCollection ) + " does not exist" );
    if ( depth == 0 )
      locations << loc;
    else {
      locations << loc.children();
      --depth;
    }
  } else {
    if ( depth != 0 ) {
      Location::List list = Location::retrieveFiltered( Location::parentIdColumn(), 0 );
      locations << list;
    }
    --depth;
  }

  foreach ( const Location &loc, locations )
    listCollection( loc, depth );

  Response response;
  response.setSuccess();
  response.setTag( tag() );
  response.setString( "List completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

bool AkList::listCollection(const Location & root, int depth )
{
  // recursive listing of child collections
  bool childrenFound = false;
  if ( depth > 0 ) {
    Location::List children = root.children();
    foreach ( const Location loc, children ) {
      if ( listCollection( loc, depth - 1 ) )
        childrenFound = true;
    }
  }

  // filter if this node isn't needed by it's children
  bool hidden = (mResource.isValid() && root.resourceId() != mResource.id()) || (mOnlySubscribed && !root.subscribed());
  if ( !childrenFound && hidden )
    return false;

  // write out collection details
  Location dummy = root;
  DataStore *db = connection()->storageBackend();
  db->activeCachePolicy( dummy );
  const QByteArray b = HandlerHelper::collectionToByteArray( dummy, hidden );

  Response response;
  response.setUntagged();
  response.setString( b );
  emit responseAvailable( response );

  return true;
}
