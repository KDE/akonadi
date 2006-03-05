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
#include <QThread>

#include "akonadi.h"
#include "akonadiconnection.h"

#include "storagebackend.h"
#include "teststoragebackend.h"

#include "storage/datastore.h"
#include "storage/debug.h"

using namespace Akonadi;

static AkonadiServer *s_instance = 0;

AkonadiServer::AkonadiServer( QObject* parent )
    : QTcpServer( parent )
    , m_backend( 0 )
{
    s_instance = this;
    /* To see some output from the database access, you should start the server 
       in the directory where the prepared akonadi.db file is located and
       uncomment the following lines.
    */
    // debugMimeTypeList( *(DataStore::instance()->listMimeTypes()) );
    // debugCachePolicyList( *(DataStore::instance()->listCachePolicies()) );
    listen( QHostAddress::LocalHost, 4444 );
}


AkonadiServer::~AkonadiServer()
{
    delete m_backend;
}

void AkonadiServer::incomingConnection( int socketDescriptor )
{
    AkonadiConnection *thread = new AkonadiConnection(socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}


StorageBackend * Akonadi::AkonadiServer::storageBackend( )
{
    if ( !m_backend ) {
        m_backend = new TestStorageBackend();
    }
    return m_backend;
}

AkonadiServer * AkonadiServer::instance()
{
    if ( !s_instance )
        s_instance = new AkonadiServer();
    return s_instance;
}
