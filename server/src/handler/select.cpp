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
#include <QtCore/QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "handlerhelper.h"
#include "imapparser.h"

#include "select.h"
#include "response.h"

using namespace Akonadi;

Select::Select(): Handler()
{
}


Select::~Select()
{
}


bool Select::handleLine(const QByteArray& line )
{
    // as per rfc, even if the following select fails, we need to reset
    connection()->setSelectedCollection( 0 );

    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    QByteArray mailbox;
    ImapParser::parseString( line, mailbox, startOfMailbox );

    Location l = HandlerHelper::collectionFromIdOrName( mailbox );

    if ( !l.isValid() )
        return failureResponse( "Cannot list this folder" );

    // Responses:  REQUIRED untagged responses: FLAGS, EXISTS, RECENT
    // OPTIONAL OK untagged responses: UNSEEN, PERMANENTFLAGS
    Response response;
    response.setUntagged();

    response.setString( Flag::joinByName<Flag>( Flag::retrieveAll(), QLatin1String(" ") ) );
    emit responseAvailable( response );

    response.setString( QByteArray::number(l.existCount()) + " EXISTS" );
    emit responseAvailable( response );

    response.setString( QByteArray::number(l.recentCount()) + " RECENT" );
    emit responseAvailable( response );

    response.setString( "OK [UNSEEN " + QByteArray::number(l.unseenCount()) + "] Message "
            + QByteArray::number(l.firstUnseen() ) + " is first unseen" );
    emit responseAvailable( response );

    response.setString( "OK [UIDVALIDITY " + QByteArray::number( (qlonglong)l.uidValidity() ) + "] UIDs valid" );
    emit responseAvailable( response );

    response.setSuccess();
    response.setTag( tag() );
    response.setString( "Completed" );
    emit responseAvailable( response );

    connection()->setSelectedCollection( l.id() );
    deleteLater();
    return true;
}

