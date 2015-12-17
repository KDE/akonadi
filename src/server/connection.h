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
#include <QtCore/QTimer>
#include <QtNetwork/QLocalSocket>
#include <QtCore/QDataStream>

#include "akthread.h"
#include "entities.h"
#include "global.h"
#include "commandcontext.h"

#include <private/protocol_p.h>

namespace Akonadi {
namespace Server {

class Handler;
class Response;
class DataStore;
class Collection;
class CollectionReferenceManager;

/**
    An Connection represents one connection of a client to the server.
*/
class Connection : public AkThread
{
    Q_OBJECT
public:
    Connection(quintptr socketDescriptor, QObject *parent = 0);
    virtual ~Connection();

    virtual DataStore *storageBackend();

    CollectionReferenceManager *collectionReferenceManager();

    CommandContext *context() const;

    /**
      Returns @c true if this connection belongs to the owning resource of @p item.
    */
    bool isOwnerResource(const PimItem &item) const;
    bool isOwnerResource(const Collection &collection) const;

    void setSessionId(const QByteArray &id);
    QByteArray sessionId() const;

    void setIsNotificationBus(bool on);
    bool isNotificationBus() const;

    /** Returns @c true if permanent cache verification is enabled. */
    bool verifyCacheOnRetrieval() const;


    Protocol::Command readCommand();

public Q_SLOTS:
    virtual void sendResponse(const Protocol::Command &response);

Q_SIGNALS:
    void disconnected();

protected Q_SLOTS:
    /**
     * New data arrived from the client. Creates a handler for it and passes the data to the handler.
     */
    void slotNewData();
    void slotConnectionStateChange(ConnectionState state);
    void slotConnectionIdle();

    void slotSendHello();

protected:
    Connection(QObject *parent = 0); // used for testing

    virtual void init() Q_DECL_OVERRIDE;
    virtual void quit() Q_DECL_OVERRIDE;

    virtual Handler *findHandlerForCommand(Protocol::Command::Type cmd);

protected:
    quintptr m_socketDescriptor;
    QLocalSocket *m_socket;
    QPointer<Handler> m_currentHandler;
    ConnectionState m_connectionState;
    bool m_isNotificationBus;
    mutable DataStore *m_backend;
    QList<QByteArray> m_statusMessageQueue;
    QString m_identifier;
    QByteArray m_sessionId;
    bool m_verifyCacheOnRetrieval;
    CommandContext m_context;
    QTimer m_idleTimer;

    QTime m_time;
    qint64 m_totalTime;
    QHash<QString, qint64> m_totalTimeByHandler;
    QHash<QString, qint64> m_executionsByHandler;

private:
    void sendResponse(qint64 tag, const Protocol::Command &response);

    /** For debugging */
    void startTime();
    void stopTime(const QString &identifier);
    void reportTime() const;
    bool m_reportTime;

};

} // namespace Server
} // namespace Akonadi



#endif
