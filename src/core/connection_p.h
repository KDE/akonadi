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

#ifndef CONNECTIONTHREAD_P_H
#define CONNECTIONTHREAD_P_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QLocalSocket>

#include <private/protocol_p.h>

#include "akonadicore_export.h"

class QAbstractSocket;
class QFile;

namespace Akonadi
{

class SessionThread;

class AKONADICORE_EXPORT Connection : public QObject
{
    Q_OBJECT

public:
    enum ConnectionType {
        CommandConnection,
        NotificationConnection
    };
    Q_ENUM(ConnectionType)

    explicit Connection(ConnectionType connType, const QByteArray &sessionId, QObject *parent = nullptr);
    ~Connection();

    Q_INVOKABLE void reconnect();
    void forceReconnect();
    void closeConnection();
    void sendCommand(qint64 tag, const Protocol::Command &command);

Q_SIGNALS:
    void connected();
    void reconnected();
    void commandReceived(qint64 tag, const Akonadi::Protocol::Command &command);
    void socketDisconnected();
    void socketError(const QString &message);

private Q_SLOTS:
    void doReconnect();
    void doForceReconnect();
    void doCloseConnection();
    void doSendCommand(qint64 tag, const Akonadi::Protocol::Command &command);

    void dataReceived();

private:

    bool handleCommand(qint64 tag, const Protocol::Command &cmd);

    ConnectionType mConnectionType;
    QLocalSocket *mSocket;
    QFile *mLogFile;
    QByteArray mSessionId;
    QMutex mLock;
    struct Command {
        qint64 tag;
        Protocol::Command cmd;
    };
    QQueue<Command> mOutQueue;

    friend class Akonadi::SessionThread;
};

}

#endif // CONNECTIONTHREAD_P_H
