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

Append::Append()
    : Handler(), m_size(-1)
{
}


Append::~Append()
{
}


bool Akonadi::Append::handleLine(const QByteArray& line )
{
    if ( m_size > -1 ) { // continuation
        m_data += line;
        bool done = false;
        m_size -= line.size();
        if (  m_size == 0 ) {
            commit();
            done = true;
            deleteLater();
        }
        return done;
    }
    
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        message literal
  
    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    int endOfMailbox = line.indexOf( ' ', startOfMailbox ) + 1;
    m_mailbox = stripQuotes( line.mid( startOfMailbox, endOfMailbox - startOfMailbox -1 ) );

    // FIXME parse optionals
    int startOfSize = line.indexOf('{') + 1;
    m_size = QString( line.mid( startOfSize, line.indexOf('}') - startOfSize ) ).toInt();
    
    Response response;
    response.setContinuation();
    response.setString( "Ready for literal data" );
    emit responseAvailable( response );
    return false;
}

void Akonadi::Append::commit()
{
    Response response;

    DataStore *db = connection()->storageBackend();
    Location l = db->getLocationByRawMailbox( m_mailbox );
    MimeType mimeType(0, "message/rfc822" );
    bool ok = db->appendPimItem( m_data, mimeType, l );
    response.setTag( tag() );
    if ( ok ) {
        response.setSuccess();
        response.setString( "Append completed" );
    } else {
        response.setFailure();
        response.setString( "Append failed" );
    }
    emit responseAvailable( response );
}

