/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "searchpersistent.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include <akonadi/private/imapparser_p.h>
#include "response.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "xesammanager.h"

#include <QtCore/QStringList>

using namespace Akonadi;

SearchPersistent::SearchPersistent()
  : Handler()
{
}

SearchPersistent::~SearchPersistent()
{
}

bool SearchPersistent::handleLine( const QByteArray& line )
{
  int pos = line.indexOf( ' ' ) + 1; // skip tag
  pos = line.indexOf( ' ', pos ) + 1; // skip command

  QByteArray collectionName;
  pos = ImapParser::parseString( line, collectionName, pos );
  if ( collectionName.isEmpty() )
    return failureResponse( "No name specified" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  QByteArray queryString;
  ImapParser::parseString( line, queryString, pos );
  if ( queryString.isEmpty() )
    return failureResponse( "No query specified" );

  Location l;
  l.setRemoteId( QString::fromUtf8( queryString ) );
  l.setParentId( 1 ); // search root
  l.setResourceId( 1 ); // search resource
  l.setName( QString::fromUtf8( collectionName ) );
  if ( !db->appendLocation( l ) )
    return failureResponse( "Unable to create persistent search" );

  if ( !XesamManager::instance()->addSearch( l ) )
    return failureResponse( "Unable to start XESAM search" );

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction" );

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( "SEARCH_STORE completed" );
  emit responseAvailable( response );

  deleteLater();
  return true;
}
