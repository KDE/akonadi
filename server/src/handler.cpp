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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <QDebug>

#include "handler.h"
#include "response.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/capability.h"
#include "handler/list.h"


using namespace Akonadi;

Handler::Handler()
    :QObject()
{
}


Handler::~Handler()
{
}

bool Handler::handleLine( const QByteArray & command )
{
    Response response;
    response.setError();
    response.setTag( m_tag );
    response.setString( "Unrecognized command: " + command );

    emit responseAvailable( response );
    deleteLater();
    return true;
}

Handler * Handler::findHandlerForCommandNonAuthenticated( const QByteArray & command )
{
    // allowed are LOGIN and AUTHENTICATE
    if ( command == "LOGIN" )
        return new Login();

    return 0;
}

Handler * Handler::findHandlerForCommandAlwaysAllowed( const QByteArray & command )
{
    // allowed commands CAPABILITY, NOOP, and LOGOUT
    if ( command == "LOGOUT" )
        return new Logout();
    if ( command == "CAPABILITY" )
        return new Capability();
    return 0;
}

void Handler::setTag( const QByteArray & tag )
{
    m_tag = tag;
}

QByteArray Handler::tag( ) const
{
    return m_tag;
}

Handler * Handler::findHandlerForCommandAuthenticated( const QByteArray & command )
{
    // allowd commands are SELECT, EXAMINE, CREATE, DELETE, RENAME,
    // SUBSCRIBE, UNSUBSCRIBE, LIST, LSUB, and APPEND.
    if ( command == "LIST" )
        return new List();
    return 0;
}
