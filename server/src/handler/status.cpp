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
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/countquerybuilder.h"

#include "response.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"

using namespace Akonadi;

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

  if ( !col.isValid() )
    return failureResponse( "No status for this folder" );

    // Responses:
    // REQUIRED untagged responses: STATUS

  qint64 itemCount, itemSize;
  if ( !HandlerHelper::itemStatistics( col, itemCount, itemSize ) )
    return failureResponse( "Failed to query statistics." );

    // build STATUS response
  QByteArray statusResponse;
    // MESSAGES - The number of messages in the mailbox
  if ( attributeList.contains( "MESSAGES" ) ) {
    statusResponse += "MESSAGES ";
    statusResponse += QByteArray::number( itemCount );
  }
    // RECENT - The number of messages with the \Recent flag set
  if ( attributeList.contains( "RECENT" ) ) {
    if ( !statusResponse.isEmpty() )
      statusResponse += " RECENT ";
    else
      statusResponse += "RECENT ";
    const int count = HandlerHelper::itemWithFlagCount( col, QLatin1String( "\\Recent" ) );
    if ( count < 0 )
      return failureResponse( "Could not determine recent item count" );
    statusResponse += QByteArray::number( count );
  }

  if ( attributeList.contains( "UNSEEN" ) ) {
    if ( !statusResponse.isEmpty() )
      statusResponse += " UNSEEN ";
    else
      statusResponse += "UNSEEN ";

    // itemWithFlagCount is twice as fast as itemWithoutFlagCount...
    const int count = HandlerHelper::itemWithFlagCount( col, QLatin1String( "\\SEEN" ) );
    if ( count < 0 )
      return failureResponse( "Unable to retrieve unread count" );
    statusResponse += QByteArray::number( itemCount - count );
  }
  if ( attributeList.contains( "SIZE" ) ) {
    if ( !statusResponse.isEmpty() )
      statusResponse += " SIZE ";
    else
      statusResponse += "SIZE ";
    statusResponse += QByteArray::number( itemSize );
  }

  Response response;
  response.setUntagged();
  response.setString( "STATUS \"" + HandlerHelper::pathForCollection( col ).toUtf8() + "\" (" + statusResponse + ')' );
  emit responseAvailable( response );

  response.setSuccess();
  response.setTag( tag() );
  response.setString( "STATUS completed" );
  emit responseAvailable( response );
  return true;
}

#include "status.moc"
