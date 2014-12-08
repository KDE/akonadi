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
#include "connection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionstatistics.h"
#include "commandcontext.h"

#include "response.h"

#include <libs/protocol_p.h>

using namespace Akonadi::Server;

Select::Select( Scope::SelectionScope scope )
  : Handler()
  , mScope( scope )
{
}

bool Select::parseStream()
{
  // as per rfc, even if the following select fails, we need to reset
  connection()->context()->setCollection( Collection() );

  QByteArray buffer = m_streamParser->readString();

  bool silent = false;
  if ( buffer == AKONADI_PARAM_SILENT ) {
    silent = true;
    buffer = m_streamParser->readString();
  }

  // collection
  Collection col;

  if ( mScope == Scope::None || mScope == Scope::Uid ) {
    col = HandlerHelper::collectionFromIdOrName( buffer );
    if ( !col.isValid() ) {
      bool ok = false;
      if ( buffer.toLongLong( &ok ) == 0 && ok ) {
        silent = true;
      } else {
        return failureResponse( "Cannot select this collection" );
      }
    }
  } else if ( mScope == Scope::Rid ) {
    if ( buffer.isEmpty() ) {
      silent = true; // unselect
    } else {
      if ( !connection()->context()->resource().isValid() ) {
        throw HandlerException( "Cannot select based on remote identifier without a resource scope" );
      }
      SelectQueryBuilder<Collection> qb;
      qb.addValueCondition( Collection::remoteIdColumn(), Query::Equals, QString::fromUtf8( buffer ) );
      qb.addValueCondition( Collection::resourceIdColumn(), Query::Equals, connection()->context()->resource().id() );
      if ( !qb.exec() ) {
        throw HandlerException( "Failed to select collection" );
      }
      Collection::List results = qb.result();
      if ( results.count() != 1 ) {
        throw HandlerException( QByteArray::number( results.count() ) + " collections found" );
      }
      col = results.first();
    }
  }

    // Responses:  REQUIRED untagged responses: FLAGS, EXISTS, RECENT
    // OPTIONAL OK untagged responses: UNSEEN, PERMANENTFLAGS
  Response response;
  if ( !silent ) {
    response.setUntagged();
    response.setString( "FLAGS (" + Flag::joinByName( Flag::retrieveAll(), QLatin1String( " " ) ).toLatin1() + ")" );
    Q_EMIT responseAvailable( response );

    const CollectionStatistics::Statistics stats = CollectionStatistics::self()->statistics(col);
    if ( stats.count == -1 ) {
      return failureResponse( "Unable to determine item count" );
    }
    response.setString( QByteArray::number( stats.count ) + " EXISTS" );
    Q_EMIT responseAvailable( response );

    response.setString( "OK [UNSEEN " + QByteArray::number( stats.count - stats.read ) + "] Message 0 is first unseen" );
    Q_EMIT responseAvailable( response );
  }

  response.setSuccess();
  response.setTag( tag() );
  response.setString( "Completed" );
  Q_EMIT responseAvailable( response );

  connection()->context()->setCollection( col );
  return true;
}
