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
#ifndef AKONADI_CONNECTION_H
#define AKONADI_CONNECTION_H

#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtNetwork/QLocalSocket>

#include "entities.h"
#include "global.h"
#include "clientcapabilities.h"
#include "commandcontext.h"

namespace Akonadi {
namespace Server {

class Handler;
class Response;
class DataStore;
class Collection;
class ImapStreamParser;

/**
    An AkonadiConnection represents one connection of a client to the server.
*/
class Connection : public QThread
{
    Q_OBJECT
public:
    Connection( quintptr socketDescriptor, QObject *parent );
    virtual ~Connection();
    void run();

    virtual DataStore *storageBackend();

    CommandContext *context() const;

    Resource resourceContext() const;
    void setResourceContext( const Resource &res );
    /**
      Returns @c true if this connection belongs to the owning resource of @p item.
    */
    bool isOwnerResource( const PimItem &item ) const;
    bool isOwnerResource( const Collection &collection ) const;

    void addStatusMessage( const QByteArray &msg );
    void flushStatusMessageQueue();

    void setSessionId( const QByteArray &id );
    QByteArray sessionId() const;

    const ClientCapabilities &capabilities() const;
    void setCapabilities( const ClientCapabilities &capabilities );

    /** Returns @c true if permanent cache verification is enabled. */
    bool verifyCacheOnRetrieval() const;

protected Q_SLOTS:
    void slotDisconnected();
    /**
     * New data arrived from the client. Creates a handler for it and passes the data to the handler.
     */
    void slotNewData();
    void slotResponseAvailable( const Akonadi::Server::Response &response );
    void slotConnectionStateChange( ConnectionState );

protected:
    Connection() {} // used for testing
    void writeOut( const QByteArray &data );
    Handler *findHandlerForCommand( const QByteArray &command );

private:
    quintptr m_socketDescriptor;
    QLocalSocket *m_socket;
    QPointer<Handler> m_currentHandler;
    ConnectionState m_connectionState;
    mutable DataStore *m_backend;
    QList<QByteArray> m_statusMessageQueue;
    QString m_identifier;
    QByteArray m_sessionId;
    ImapStreamParser *m_streamParser;
    Resource m_resourceContext;
    ClientCapabilities m_clientCapabilities;
    bool m_verifyCacheOnRetrieval;
    CommandContext m_context;

};

} // namespace Server
} // namespace Akonadi

#endif
