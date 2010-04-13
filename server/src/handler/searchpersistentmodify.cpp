/***************************************************************************
 *   Copyright (C) 2010 by Till Adam <adam@kde.org>                        *
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

#include "searchpersistentmodify.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "abstractsearchmanager.h"
#include "imapstreamparser.h"
#include "libs/protocol_p.h"
#include <akdebug.h>

#include <QtCore/QStringList>
#include <qdbusinterface.h>
#include <QtDBus/qdbusreply.h>

using namespace Akonadi;

SearchPersistentModify::SearchPersistentModify()
  : Handler()
{
}

SearchPersistentModify::~SearchPersistentModify()
{
}


bool SearchPersistentModify::parseStream()
{
  qint64 collectionId = m_streamParser->readNumber();
  if ( collectionId <= 0 )
    return failureResponse( "No collection id specified" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  QByteArray queryString = m_streamParser->readString();
  if ( queryString.isEmpty() )
    return failureResponse( "No query specified" );

  Collection existingCollection = Collection::retrieveById( collectionId );

  // FIXME for now this is implemented as 1) add a new collection 2) remove the original one
  // This changes the Id and is thus not desirable.
  Collection col;
  col.setRemoteId( QString::fromUtf8( queryString ) );
  col.setParentId( 1 ); // search root
  col.setResourceId( 1 ); // search resource
  col.setName( existingCollection.name() );

  // FIXME copy over everything else? Are attributes relevant?

  // TODO right now we have to remove, then add, otherwise the name will be
  // duplicated and the add will fail

  if ( !db->cleanupCollection( existingCollection ) )
    return failureResponse( "Unable to remove persistent search" );

  if ( !db->appendCollection( col ) )
    return failureResponse( "Unable to create persistent search" );

  // work around the fact that we have no clue what might be in there
  MimeType::List mts = MimeType::retrieveAll();
  foreach ( const MimeType &mt, mts ) {
    if ( mt.name() == QLatin1String( "inode/directory" ) )
      continue;
    col.addMimeType( mt );
  }


  if ( !AbstractSearchManager::instance()->removeSearch( existingCollection.id() ) )
    return failureResponse( "Unable to remove the original search from the search manager" );

  if ( !AbstractSearchManager::instance()->addSearch( col ) )
    return failureResponse( "Unable to add search to search manager" );

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction" );

  // distribute to remote search agents
  QDBusInterface agentMgr( QLatin1String( AKONADI_DBUS_CONTROL_SERVICE ),
                           QLatin1String( AKONADI_DBUS_AGENTMANAGER_PATH ),
                           QLatin1String( "org.freedesktop.Akonadi.AgentManagerInternal" ) );
  if ( agentMgr.isValid() ) {
    QList<QVariant> args;
    args << QString::fromUtf8( queryString );
    args << QString::fromLatin1( "SPARQL" );
    args << col.id();
    agentMgr.callWithArgumentList( QDBus::NoBlock, QLatin1String( "addSearch" ), args );
  } else {
    akError() << "Failed to connect to agent manager: " << agentMgr.lastError();
  }


  const QByteArray b = HandlerHelper::collectionToByteArray( col );

  Response colResponse;
  colResponse.setUntagged();
  colResponse.setString( b );
  emit responseAvailable( colResponse );

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( "SEARCH_MODIFY completed" );
  emit responseAvailable( response );
  return true;
}

#include "searchpersistentmodify.moc"
