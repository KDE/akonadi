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
#include "response.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "search/searchmanager.h"
#include "imapstreamparser.h"
#include "libs/protocol_p.h"
#include <akdebug.h>

#include <QtCore/QStringList>
#include <qdbusinterface.h>
#include <QtDBus/qdbusreply.h>

using namespace Akonadi;

SearchPersistent::SearchPersistent()
  : Handler()
{
}

SearchPersistent::~SearchPersistent()
{
}


bool SearchPersistent::parseStream()
{
  QString collectionName = m_streamParser->readUtf8String();
  if ( collectionName.isEmpty() )
    return failureResponse( "No name specified" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  const QString queryString = m_streamParser->readUtf8String();
  if ( queryString.isEmpty() )
    return failureResponse( "No query specified" );

  Collection col;
  col.setQueryString( queryString );
  col.setQueryLanguage( QLatin1String( "SPARQL" ) ); // TODO receive from client
  col.setRemoteId( queryString ); // ### remove, legacy compat
  col.setParentId( 1 ); // search root
  col.setResourceId( 1 ); // search resource
  col.setName( collectionName );
  if ( !db->appendCollection( col ) )
    return failureResponse( "Unable to create persistent search" );

  // work around the fact that we have no clue what might be in there
  MimeType::List mts = MimeType::retrieveAll();
  foreach ( const MimeType &mt, mts ) {
    if ( mt.name() == QLatin1String( "inode/directory" ) )
      continue;
    col.addMimeType( mt );
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction" );

  if ( !SearchManager::instance()->addSearch( col ) )
    return failureResponse( "Unable to add search to search manager" );

  const QByteArray b = HandlerHelper::collectionToByteArray( col );

  Response colResponse;
  colResponse.setUntagged();
  colResponse.setString( b );
  emit responseAvailable( colResponse );

  return successResponse( "SEARCH_STORE completed" );
}

#include "searchpersistent.moc"
