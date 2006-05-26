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
    // parse out the reference name and mailbox name
    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    QByteArray mailbox = stripQuotes( line.right( line.size() - startOfMailbox ) );
    mailbox.prepend( connection()->selectedCollection() );

    // Responses:  REQUIRED untagged responses: FLAGS, EXISTS, RECENT
    // OPTIONAL OK untagged responses: UNSEEN, PERMANENTFLAGS
    Response response;
    response.setUntagged();
    response.setString( "FLAGS (Answered)");
    emit responseAvailable( response );

    DataStore *db = connection()->storageBackend();
    int secondSlash = mailbox.indexOf( '/', 2 ) + 1;
    //qDebug() << "Select: " << mailbox.mid( secondSlash, mailbox.size() - secondSlash )
    //         << " in resource: " << mailbox.mid( 1, secondSlash - 2 );
    Resource resource = db->getResourceByName( mailbox.mid( 1, secondSlash - 2 ) );
    Location l = db->getLocationByName( resource, mailbox.mid( secondSlash -1 , mailbox.size() - (secondSlash-1) ) );
    if ( !l.isValid() ) {
        response.setFailure();
        response.setString( "Cannot list this folder");
        emit responseAvailable( response );
    }

    response.setString( QString::number(l.getExists()) + " EXISTS" );
    emit responseAvailable( response );

    response.setString( QString::number(l.getRecent()) + " RECENT" );
    emit responseAvailable( response );

    int firstUnseen = 4;

    response.setString( "OK [UNSEEN " + QString::number(l.getUnseen()) + "] Message "
            + QString::number(l.getFirstUnseen() ) + " is first unseen" );
    emit responseAvailable( response );

    response.setString( "OK [UIDVALIDITY " + QString::number( l.getUidValidity() ) + "] UIDs valid" );
    emit responseAvailable( response );

    response.setSuccess();
    response.setTag( tag() );
    response.setString( "Completed" );
    emit responseAvailable( response );
    
    connection()->setSelectedCollection( mailbox );
    deleteLater();
    return true;
}

