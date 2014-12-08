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
#include "status.h"

#include <QtCore/QDebug>

#include "akonadi.h"
#include "connection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/countquerybuilder.h"
#include "storage/collectionstatistics.h"

#include "response.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"

#include <libs/protocol_p.h>

using namespace Akonadi::Server;

Status::Status(): Handler()
{
}

Status::~Status()
{
}

bool Status::parseStream()
{
    // Arguments: mailbox name
    //            status data item names

    // Syntax:
    // status     = "STATUS" SP mailbox SP "(" status-att *(SP status-att) ")"
    // status-att = "MESSAGES" / "RECENT" / "UNSEEN" / "SIZE"

  const QByteArray mailbox = m_streamParser->readString();
  QList<QByteArray> attributeList = m_streamParser->readParenthesizedList();
  const Collection col = HandlerHelper::collectionFromIdOrName( mailbox );

  if ( !col.isValid() ) {
    return failureResponse( "No status for this folder" );
  }

    // Responses:
    // REQUIRED untagged responses: STATUS

  const CollectionStatistics::Statistics &stats = CollectionStatistics::self()->statistics(col);
  if (stats.count == -1) {
      return failureResponse( "Failed to query statistics." );
  }

    // build STATUS response
  QByteArray statusResponse;
    // MESSAGES - The number of messages in the mailbox
  if ( attributeList.contains( AKONADI_ATTRIBUTE_MESSAGES ) ) {
    statusResponse += AKONADI_ATTRIBUTE_MESSAGES " ";
    statusResponse += QByteArray::number( stats.count );
  }

  if ( attributeList.contains( AKONADI_ATTRIBUTE_UNSEEN ) ) {
    if ( !statusResponse.isEmpty() ) {
      statusResponse += " ";
    }
    statusResponse += AKONADI_ATTRIBUTE_UNSEEN " ";
    statusResponse += QByteArray::number( stats.count - stats.read );
  }
  if ( attributeList.contains( AKONADI_PARAM_SIZE ) ) {
    if ( !statusResponse.isEmpty() ) {
      statusResponse += " ";
    }
    statusResponse += AKONADI_PARAM_SIZE " ";
    statusResponse += QByteArray::number( stats.size );
  }

  Response response;
  response.setUntagged();
  response.setString( "STATUS \"" + HandlerHelper::pathForCollection( col ).toUtf8() + "\" (" + statusResponse + ')' );
  Q_EMIT responseAvailable( response );

  response.setSuccess();
  response.setTag( tag() );
  response.setString( "STATUS completed" );
  Q_EMIT responseAvailable( response );
  return true;
}
