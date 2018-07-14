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

#include <QPointer>
#include <QThread>
#include <QTimer>
#include <QLocalSocket>
#include <QTime>

#include "akthread.h"
#include "entities.h"
#include "global.h"
#include "commandcontext.h"

#include <private/protocol_p.h>

class QEventLoop;

namespace Akonadi
{
namespace Server
{

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
    explicit Connection(quintptr socketDescriptor, QObject *parent = nullptr);
    ~Connection() override;

    static Connection *self();

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

    /** Returns @c true if permanent cache verification is enabled. */
    bool verifyCacheOnRetrieval() const;

    Protocol::CommandPtr readCommand();

public Q_SLOTS:
    virtual void sendResponse(const Protocol::CommandPtr &response);

Q_SIGNALS:
    void disconnected();
    void connectionClosing();

protected Q_SLOTS:
    void handleIncomingData();

    void slotConnectionStateChange(ConnectionState state);
    void slotConnectionIdle();
    void slotSocketDisconnected();
    void slotSendHello();

protected:
    Connection(QObject *parent = nullptr); // used for testing

    void init() override;
    void quit() override;

    Handler *findHandlerForCommand(Protocol::Command::Type cmd);

protected:
    quintptr m_socketDescriptor = {};
    QLocalSocket *m_socket = nullptr;
    QPointer<Handler> m_currentHandler;
    ConnectionState m_connectionState = NonAuthenticated;
    mutable DataStore *m_backend = nullptr;
    QList<QByteArray> m_statusMessageQueue;
    QString m_identifier;
    QByteArray m_sessionId;
    bool m_verifyCacheOnRetrieval = false;
    CommandContext m_context;
    QTimer *m_idleTimer = nullptr;
    QEventLoop *m_waitLoop = nullptr;

    QTime m_time;
    qint64 m_totalTime = 0;
    QHash<QString, qint64> m_totalTimeByHandler;
    QHash<QString, qint64> m_executionsByHandler;

    bool m_connectionClosing = false;

private:
    void sendResponse(qint64 tag, const Protocol::CommandPtr &response);

    /** For debugging */
    void startTime();
    void stopTime(const QString &identifier);
    void reportTime() const;
    bool m_reportTime = false;

};

} // namespace Server
} // namespace Akonadi

#endif
