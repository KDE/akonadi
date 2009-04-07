/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "select.h"

#include <QtCore/QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "storage/selectquerybuilder.h"

#include "response.h"

using namespace Akonadi;

Select::Select( Scope::SelectionScope scope ) :
  Handler(),
  mScope( scope )
{
}


Select::~Select()
{
}


bool Select::parseStream()
{
  // as per rfc, even if the following select fails, we need to reset
  connection()->setSelectedCollection( 0 );

  QByteArray buffer = m_streamParser->readString();

  bool silent = false;
  if ( buffer == "SILENT" ) {
    silent = true;
    buffer = m_streamParser->readString();
  }

  // collection
  Collection col;

  if ( mScope == Scope::None || mScope == Scope::Uid ) {
    col = HandlerHelper::collectionFromIdOrName( buffer );
    if ( !col.isValid() ) {
      bool ok = false;
      if ( buffer.toLongLong( &ok ) == 0 && ok )
        silent = true;
      else
        return failureResponse( "Cannot select this collection" );
    }
  } else if ( mScope == Scope::Rid ) {
    if ( buffer.isEmpty() ) {
      silent = true; // unselect
    } else {
      if ( !connection()->resourceContext().isValid() )
        throw HandlerException( "Cannot select based on remote identifier without a resource scope" );
      SelectQueryBuilder<Collection> qb;
      qb.addValueCondition( Collection::remoteIdColumn(), Query::Equals, buffer );
      qb.addValueCondition( Collection::resourceIdColumn(), Query::Equals, connection()->resourceContext().id() );
      if ( !qb.exec() )
        throw HandlerException( "Failed to select collection" );
      Collection::List results = qb.result();
      if ( results.count() != 1 )
        throw HandlerException( QByteArray::number( results.count() ) + " collections found" );
      col = results.first();
    }
  }

    // Responses:  REQUIRED untagged responses: FLAGS, EXISTS, RECENT
    // OPTIONAL OK untagged responses: UNSEEN, PERMANENTFLAGS
  Response response;
  if ( !silent ) {
    response.setUntagged();
    response.setString( Flag::joinByName<Flag>( Flag::retrieveAll(), QLatin1String(" ") ) );
    emit responseAvailable( response );

    int count = HandlerHelper::itemCount( col );
    if ( count < 0 )
      return failureResponse( "Unable to determine item count" );
    response.setString( QByteArray::number( count ) + " EXISTS" );
    emit responseAvailable( response );

    count = HandlerHelper::itemWithFlagCount( col, QLatin1String( "\\Recent" ) );
    if ( count < 0 )
      return failureResponse( "Unable to determine recent count" );
    response.setString( QByteArray::number( count ) + " RECENT" );
    emit responseAvailable( response );

    count = HandlerHelper::itemWithoutFlagCount( col, QLatin1String( "\\Seen" ) );
    if ( count < 0 )
      return failureResponse( "Unable to retrieve unseen count" );
    response.setString( "OK [UNSEEN " + QByteArray::number( count ) + "] Message 0 is first unseen" );
    emit responseAvailable( response );

    response.setString( "OK [UIDVALIDITY 1] UIDs valid" );
    emit responseAvailable( response );
  }

  response.setSuccess();
  response.setTag( tag() );
  response.setString( "Completed" );
  emit responseAvailable( response );

  connection()->setSelectedCollection( col.id() );
  deleteLater();
  return true;
}



#include "select.moc"
