/*
 * Copyright 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef CONNECTION_P_H
#define CONNECTION_P_H

#include <QThread>
#include <QLocalSocket>
#include <QScopedPointer>

#include "private/protocol_p.h"

#include "akonadicore_export.h"

class QFile;

namespace Akonadi
{

class SessionThread;
class SessionPrivate;
class CommandBuffer;

class AKONADICORE_EXPORT Connection : public QObject
{
    Q_OBJECT

public:
    enum ConnectionType {
        CommandConnection,
        NotificationConnection
    };
    Q_ENUM(ConnectionType)

    explicit Connection(ConnectionType connType, const QByteArray &sessionId,
                        CommandBuffer *commandBuffer, QObject *parent = nullptr);
    ~Connection();

    void setSession(SessionPrivate *session);

    QLocalSocket *socket() const;

    Q_INVOKABLE void reconnect();
    void forceReconnect();
    void closeConnection();
    void sendCommand(qint64 tag, const Protocol::CommandPtr &command);

    void handleIncomingData();

Q_SIGNALS:
    void connected();
    void reconnected();
    void commandReceived(qint64 tag, const Akonadi::Protocol::CommandPtr &command);
    void socketDisconnected();
    void socketError(const QString &message);

private Q_SLOTS:
    void doReconnect();
    void doForceReconnect();
    void doCloseConnection();
    void doSendCommand(qint64 tag, const Akonadi::Protocol::CommandPtr &command);


private:
    QString defaultAddressForTypeAndMethod(ConnectionType type, const QString &method);
    bool handleCommand(qint64 tag, const Protocol::CommandPtr &cmd);

    ConnectionType mConnectionType;
    QScopedPointer<QLocalSocket> mSocket;
    QFile *mLogFile = nullptr;
    QByteArray mSessionId;
    CommandBuffer *mCommandBuffer;

    friend class Akonadi::SessionThread;
};

}

#endif // CONNECTION_P_H
