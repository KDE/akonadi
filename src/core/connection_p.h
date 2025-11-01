/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QLocalSocket>
#include <QScopedPointer>
#include <QThread>

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
        NotificationConnection,
    };
    Q_ENUM(ConnectionType)

    explicit Connection(ConnectionType connType, const QByteArray &sessionId, CommandBuffer *commandBuffer, QObject *parent = nullptr);
    ~Connection() override;

    void setSession(SessionPrivate *session);

    QLocalSocket *socket() const;

    Q_INVOKABLE void reconnect();
    void forceReconnect();
    void closeConnection();
    void sendCommand(qint64 tag, const Protocol::CommandPtr &command);

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
    void handleIncomingData();
    QString defaultAddressForTypeAndMethod(ConnectionType type, const QString &method);
    bool handleCommand(qint64 tag, const Protocol::CommandPtr &cmd);

    ConnectionType mConnectionType;
    QScopedPointer<QLocalSocket> mSocket;
    QFile *mLogFile = nullptr;
    QByteArray mSessionId;
    CommandBuffer *mCommandBuffer;
    bool mIncomingDataRecursing = false;

    friend class Akonadi::SessionThread;
};

}
