
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

#include "handler.h"
#include "response.h"
#include "handler/append.h"
#include "handler/capability.h"
#include "handler/create.h"
#include "handler/fetch.h"
#include "handler/list.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/noop.h"
#include "handler/search.h"
#include "handler/searchpersistent.h"
#include "handler/select.h"
#include "handler/status.h"
#include "handler/store.h"
#include "uid.h"

using namespace Akonadi;

Handler::Handler()
    :QObject()
    , m_connection( 0 )
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
    if ( command == "APPEND" )
        return new Append();
    if ( command == "CREATE" )
        return new Create();
    if ( command == "LIST" )
        return new List();
    if ( command == "SELECT" )
        return new Select();
    if ( command == "SEARCH" )
        return new Search();
    if ( command == "SEARCH_STORE" || command == "SEARCH_DELETE" || command == "SEARCH_DEBUG" )
        return new SearchPersistent();
    if ( command == "NOOP" )
        return new Noop();
    if ( command == "FETCH" )
        return new Fetch();
    if ( command == "UID" )
        return new Uid();
    if ( command == "STORE" )
        return new Store();
    if ( command == "STATUS" )
        return new Status();

    return 0;
}


void Akonadi::Handler::setConnection( AkonadiConnection* connection )
{
    m_connection = connection;
}


AkonadiConnection* Akonadi::Handler::connection()
{
    return m_connection;
}



QByteArray Akonadi::Handler::stripQuotes( const QByteArray &mailbox )
{
    if ( mailbox.startsWith('"') && mailbox.endsWith('"') )
       return mailbox.mid( 1, mailbox.size() - 2);
    return mailbox;
}
