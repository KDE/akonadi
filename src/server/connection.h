/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QElapsedTimer>
#include <QLocalSocket>
#include <QThread>
#include <QTimer>

#include "akonadi.h"
#include "akthread.h"
#include "commandcontext.h"
#include "entities.h"
#include "global.h"
#include "tracer.h"

#include <private/datastream_p_p.h>
#include <private/protocol_p.h>

#include <memory>

namespace Akonadi
{
namespace Server
{
class Handler;
class Response;
class DataStore;
class Collection;

/**
    An Connection represents one connection of a client to the server.
*/
class Connection : public AkThread
{
    Q_OBJECT
public:
    explicit Connection(quintptr socketDescriptor, AkonadiServer &akonadi);
    ~Connection() override;

    virtual DataStore *storageBackend();

    const CommandContext &context() const;
    void setContext(const CommandContext &context);

    AkonadiServer &akonadi() const
    {
        return m_akonadi;
    }

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

    template<typename T> inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type sendResponse(T &&response);

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
    Connection(AkonadiServer &akonadi); // used for testing

    void init() override;
    void quit() override;

    std::unique_ptr<Handler> findHandlerForCommand(Protocol::Command::Type cmd);

    qint64 currentTag() const;

protected:
    quintptr m_socketDescriptor = {};
    AkonadiServer &m_akonadi;
    std::unique_ptr<QLocalSocket> m_socket;
    std::unique_ptr<Handler> m_currentHandler;
    std::unique_ptr<QTimer> m_idleTimer;

    ConnectionState m_connectionState = NonAuthenticated;

    mutable DataStore *m_backend = nullptr;
    QList<QByteArray> m_statusMessageQueue;
    QString m_identifier;
    QByteArray m_sessionId;
    bool m_verifyCacheOnRetrieval = false;
    CommandContext m_context;

    QElapsedTimer m_time;
    qint64 m_totalTime = 0;
    QHash<QString, qint64> m_totalTimeByHandler;
    QHash<QString, qint64> m_executionsByHandler;

    bool m_connectionClosing = false;

private:
    void parseStream(const Protocol::CommandPtr &cmd);
    template<typename T> inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type sendResponse(qint64 tag, T &&response);

    /** For debugging */
    void startTime();
    void stopTime(const QString &identifier);
    void reportTime() const;
    bool m_reportTime = false;
};

template<typename T> inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type Connection::sendResponse(T &&response)
{
    sendResponse<T>(currentTag(), std::move(response));
}

template<typename T> inline typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type Connection::sendResponse(qint64 tag, T &&response)
{
    if (m_akonadi.tracer().currentTracer() != QLatin1String("null")) {
        m_akonadi.tracer().connectionOutput(m_identifier, tag, response);
    }
    Protocol::DataStream stream(m_socket.get());
    stream << tag;
    stream << std::move(response);
    stream.flush();
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

