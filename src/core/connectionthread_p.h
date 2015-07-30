/*
 * Copyright 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CONNECTIONTHREAD_P_H
#define CONNECTIONTHREAD_P_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QTcpSocket>

#include <akonadi/private/protocol_p.h>

class QAbstractSocket;
class QFile;

namespace Akonadi
{

class ConnectionThread : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionThread(const QByteArray &sessionId, QObject *parent = Q_NULLPTR);
    ~ConnectionThread();

    Q_INVOKABLE void reconnect();
    void forceReconnect();
    void disconnect();
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
    void doDisconnect();
    void doSendCommand(qint64 tag, const Akonadi::Protocol::Command &command);

    void dataReceived();
    void socketError(QLocalSocket::LocalSocketError error);
    void socketError(QAbstractSocket::SocketError error);

private:
    bool handleCommand(qint64 tag, const Protocol::Command &cmd);

    QIODevice *mSocket;
    QFile *mLogFile;
    QByteArray mSessionId;
    QMutex mLock;
    struct Command {
        qint64 tag;
        Protocol::Command cmd;
    };
    QQueue<Command> mOutQueue;
};

}

#endif // CONNECTIONTHREAD_P_H
