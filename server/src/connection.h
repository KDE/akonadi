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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
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
class CollectionReferenceManager;

/**
    An Connection represents one connection of a client to the server.
*/
class Connection : public QObject
{
    Q_OBJECT
public:
    Connection(quintptr socketDescriptor, QObject *parent = 0);
    virtual ~Connection();

    static int protocolVersion();

    virtual DataStore *storageBackend();

    CollectionReferenceManager *collectionReferenceManager();

    CommandContext *context() const;

    /**
      Returns @c true if this connection belongs to the owning resource of @p item.
    */
    bool isOwnerResource(const PimItem &item) const;
    bool isOwnerResource(const Collection &collection) const;

    void addStatusMessage(const QByteArray &msg);
    void flushStatusMessageQueue();

    void setSessionId(const QByteArray &id);
    QByteArray sessionId() const;

    const ClientCapabilities &capabilities() const;
    void setCapabilities(const ClientCapabilities &capabilities);

    /** Returns @c true if permanent cache verification is enabled. */
    bool verifyCacheOnRetrieval() const;

Q_SIGNALS:
    void disconnected();

protected Q_SLOTS:
    /**
     * New data arrived from the client. Creates a handler for it and passes the data to the handler.
     */
    void slotNewData();
    void slotConnectionStateChange(ConnectionState state);

    virtual void slotResponseAvailable(const Akonadi::Server::Response &response);

protected:
    Connection(QObject *parent = 0); // used for testing

    void writeOut(const QByteArray &data);
    virtual Handler *findHandlerForCommand(const QByteArray &command);

protected:
    quintptr m_socketDescriptor;
    QIODevice *m_socket;
    QPointer<Handler> m_currentHandler;
    ConnectionState m_connectionState;
    mutable DataStore *m_backend;
    QList<QByteArray> m_statusMessageQueue;
    QString m_identifier;
    QByteArray m_sessionId;
    ImapStreamParser *m_streamParser;
    ClientCapabilities m_clientCapabilities;
    bool m_verifyCacheOnRetrieval;
    CommandContext m_context;

};

} // namespace Server
} // namespace Akonadi

#endif
