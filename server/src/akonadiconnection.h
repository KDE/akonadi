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
#ifndef AKONADICONNECTION_H
#define AKONADICONNECTION_H

#include <QThread>
#include <QtNetwork/QTcpSocket>
class QTcpSocket;

#include "global.h"

namespace Akonadi {
    class Handler;
    class Response;

/**
    An AkonadiConnection represents one connection of a client to the server.
*/
class AkonadiConnection : public QThread
{
    Q_OBJECT
public:
    AkonadiConnection( int socketDescriptor, QObject *parent );
    ~AkonadiConnection();
    void run();

signals:
    void error( QTcpSocket::SocketError socketError );

protected slots:
    void slotDisconnected();
    void slotNewData();
    void slotResponseAvailable( const Response& );
    void slotConnectionStateChange( ConnectionState );
    
protected:
    void writeOut( const char* );
    Handler* findHandlerForCommand( const QByteArray& command );

private:
    int m_socketDescriptor;
    QTcpSocket * m_tcpSocket;
    Handler *m_currentHandler;
    ConnectionState m_connectionState;
};

}

#endif
