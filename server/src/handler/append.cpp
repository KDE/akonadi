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
#include <QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storagebackend.h"
#include "storage/datastore.h"
#include "storage/entity.h"

#include "append.h"
#include "response.h"

using namespace Akonadi;

Append::Append(): Handler()
{
}


Append::~Append()
{
}


bool Append::handleLine(const QByteArray& line )
{
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        message literal
  
    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    QByteArray mailbox = stripQuotes( line.right( line.size() - startOfMailbox ) );
    QByteArray body;

    Response response;
    response.setUntagged();

    DataStore *db = connection()->storageBackend();
    Location l = db->getLocationByRawMailbox( mailbox );
    MimeType mimeType(0, "message/rfc820" );
    bool ok = db->appendPimItem( body, mimeType, l );
    if ( ok )
        response.setSuccess();
    else
        response.setFailure();
    response.setTag( tag() );
    response.setString( "Append completed" );
    emit responseAvailable( response );
    deleteLater();
    return true;
}

