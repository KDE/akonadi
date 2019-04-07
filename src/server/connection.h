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
#include "tracer.h"

#include <private/protocol_p.h>
#include <private/datastream_p_p.h>

#include <memory>

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

    void setState(ConnectionState state);

    template<typename T>
    inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type
    sendResponse(T &&response);

    void sendResponse(qint64 tag, const Protocol::CommandPtr &response);

Q_SIGNALS:
    void disconnected();
    void connectionClosing();

protected Q_SLOTS:
    void handleIncomingData();

    void slotConnectionIdle();
    void slotSocketDisconnected();
    void slotSendHello();

protected:
    Connection(QObject *parent = nullptr); // used for testing

    void init() override;
    void quit() override;

    Handler *findHandlerForCommand(Protocol::Command::Type cmd);

    qint64 currentTag() const;

protected:
    quintptr m_socketDescriptor = {};
    QLocalSocket *m_socket = nullptr;
    std::unique_ptr<Handler> m_currentHandler;
    ConnectionState m_connectionState = NonAuthenticated;
    mutable DataStore *m_backend = nullptr;
    QList<QByteArray> m_statusMessageQueue;
    QString m_identifier;
    QByteArray m_sessionId;
    bool m_verifyCacheOnRetrieval = false;
    CommandContext m_context;
    QTimer *m_idleTimer = nullptr;

    QTime m_time;
    qint64 m_totalTime = 0;
    QHash<QString, qint64> m_totalTimeByHandler;
    QHash<QString, qint64> m_executionsByHandler;

    bool m_connectionClosing = false;

private:
    void parseStream(const Protocol::CommandPtr &cmd);
    template<typename T>
    inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type
    sendResponse(qint64 tag, T &&response);

    /** For debugging */
    void startTime();
    void stopTime(const QString &identifier);
    void reportTime() const;
    bool m_reportTime = false;

};

template<typename T>
inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type
Connection::sendResponse(T &&response)
{
    sendResponse<T>(currentTag(), std::move(response));
}

template<typename T>
inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type
Connection::sendResponse(qint64 tag, T &&response)
{
    if (Tracer::self()->currentTracer() != QLatin1String("null")) {
        Tracer::self()->connectionOutput(m_identifier, tag, response);
    }
    Protocol::DataStream stream(m_socket);
    stream << tag;
    stream << std::move(response);
    if (!m_socket->waitForBytesWritten()) {
        if (m_socket->state() == QLocalSocket::ConnectedState) {
            throw ProtocolException("Server write timeout");
        } else {
            // The client has disconnected before we managed to send our response,
            // which is not an error
        }
    }
}

} // namespace Server
} // namespace Akonadi

#endif
