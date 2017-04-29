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

#include "connection_p.h"
#include "session_p.h"
#include "servermanager_p.h"
#include "akonadicore_debug.h"

#include <QDataStream>
#include <QFile>
#include <QAbstractEventDispatcher>
#include <QFileInfo>
#include <QSettings>
#include <QElapsedTimer>
#include <QApplication>
#include <QDateTime>

#include <private/xdgbasedirs_p.h>
#include <private/protocol_exception_p.h>
#include <private/standarddirs_p.h>

using namespace Akonadi;

Connection::Connection(ConnectionType connType, const QByteArray &sessionId, QObject *parent)
    : QObject(parent)
    , mConnectionType(connType)
    , mSocket(nullptr)
    , mLogFile(nullptr)
    , mSessionId(sessionId)
{
    qRegisterMetaType<Protocol::CommandPtr>();
    qRegisterMetaType<QAbstractSocket::SocketState>();

    const QByteArray sessionLogFile = qgetenv("AKONADI_SESSION_LOGFILE");
    if (!sessionLogFile.isEmpty()) {
        mLogFile = new QFile(QStringLiteral("%1.%2.%3-%4").arg(QString::fromLatin1(sessionLogFile))
                             .arg(QString::number(QApplication::applicationPid()))
                             .arg(QString::fromLatin1(mSessionId.replace('/', '_')))
                             .arg(connType == CommandConnection ? QStringLiteral("Cmd") : QStringLiteral("Ntf")));
        if (!mLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(AKONADICORE_LOG) << "Failed to open Akonadi Session log file" << mLogFile->fileName();
            delete mLogFile;
            mLogFile = nullptr;
        }
    }
}

Connection::~Connection()
{
    delete mLogFile;
    if (mSocket) {
        mSocket->disconnect(this);
        mSocket->close();
        delete mSocket;
    }
}

void Connection::reconnect()
{
    const bool ok = QMetaObject::invokeMethod(this, "doReconnect", Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doReconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket && (mSocket->state() == QLocalSocket::ConnectedState
                    || mSocket->state() == QLocalSocket::ConnectingState)) {
        // nothing to do, we are still/already connected
        return;
    }

    // try to figure out where to connect to
    QString serverAddress;

    // env var has precedence
    const QByteArray serverAddressEnvVar = qgetenv("AKONADI_SERVER_ADDRESS");
    if (!serverAddressEnvVar.isEmpty()) {
        const int pos = serverAddressEnvVar.indexOf(':');
        const QByteArray protocol = serverAddressEnvVar.left(pos);
        QMap<QString, QString> options;
        const QStringList lst = QString::fromLatin1(serverAddressEnvVar.mid(pos + 1)).split(QLatin1Char(','));
        for (const QString &entry : lst) {
            const QStringList pair = entry.split(QLatin1Char('='));
            if (pair.size() != 2) {
                continue;
            }
            options.insert(pair.first(), pair.last());
        }
        qCDebug(AKONADICORE_LOG) << protocol << options;

        if (protocol == "unix") {
            serverAddress = options.value(QStringLiteral("path"));
        } else if (protocol == "pipe") {
            serverAddress = options.value(QStringLiteral("name"));
        }
    }

    // try config file next, fall back to defaults if that fails as well
    if (serverAddress.isEmpty()) {
        const QString connectionConfigFile = SessionPrivate::connectionFile();
        const QFileInfo fileInfo(connectionConfigFile);
        if (!fileInfo.exists()) {
            qCDebug(AKONADICORE_LOG) << "Akonadi Client Session: connection config file '"
                                     "akonadi/akonadiconnectionrc' can not be found in"
                                     << XdgBaseDirs::homePath("config") << "nor in any of"
                                     << XdgBaseDirs::systemPathList("config");
        }
        const QSettings connectionSettings(connectionConfigFile, QSettings::IniFormat);
        const QString defaultSocketDir = StandardDirs::saveDir("data");

        if (mConnectionType == CommandConnection) {
            const QString defaultSocketPath = defaultSocketDir % QStringLiteral("/akonadiserver-cmd.socket");
            serverAddress = connectionSettings.value(QStringLiteral("Data/UnixPath"), defaultSocketPath).toString();
        } else if (mConnectionType == NotificationConnection) {
            const QString defaultSocketPath = defaultSocketDir % QStringLiteral("/akonadiserver-ntf.socket");
            serverAddress = connectionSettings.value(QStringLiteral("Notifications/UnixPath"), defaultSocketPath).toString();
        }
    }

    // create sockets if not yet done, note that this does not yet allow changing socket types on the fly
    // but that's probably not something we need to support anyway
    if (!mSocket) {
        mSocket = new QLocalSocket(this);
        connect(mSocket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this,
        [this](QLocalSocket::LocalSocketError) {
            qCWarning(AKONADICORE_LOG) << mSocket->errorString() << mSocket->serverName();
            Q_EMIT socketError(mSocket->errorString());
            Q_EMIT socketDisconnected();
        });
        connect(mSocket, &QLocalSocket::disconnected, this, &Connection::socketDisconnected);
        connect(mSocket, &QLocalSocket::readyRead, this, &Connection::dataReceived);
    }

    // actually do connect
    qCDebug(AKONADICORE_LOG) << "connectToServer" << serverAddress;
    mSocket->connectToServer(serverAddress);

    Q_EMIT reconnected();
}

void Connection::forceReconnect()
{
    const bool ok = QMetaObject::invokeMethod(this, "doForceReconnect",
                    Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doForceReconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->disconnect(this, SIGNAL(socketDisconnected()));
        delete mSocket;
        mSocket = nullptr;
    }
    mSocket = nullptr;
}

void Connection::closeConnection()
{
    const bool ok = QMetaObject::invokeMethod(this, "doCloseConnection", Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doCloseConnection()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->close();
        mSocket->readAll();
    }
}

void Connection::dataReceived()
{
    Q_ASSERT(QThread::currentThread() == thread());

    QElapsedTimer timer;
    timer.start();
    while (mSocket->bytesAvailable() > 0) {
        QDataStream stream(mSocket);
        qint64 tag;
        // TODO: Verify the tag matches the last tag we sent
        stream >> tag;

        Protocol::CommandPtr cmd;
        try {
            cmd = Protocol::deserialize(mSocket);
        } catch (const Akonadi::ProtocolException &) {
            // cmd's type will be Invalid by default.
        }
        if (cmd->type() == Protocol::Command::Invalid) {
            qCWarning(AKONADICORE_LOG) << "Invalid command, the world is going to end!";
            mSocket->close();
            mSocket->readAll();
            reconnect();
            return;
        }

        if (mLogFile) {
            mLogFile->write("S: ");
            mLogFile->write(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz ")).toUtf8());
            mLogFile->write(QByteArray::number(tag));
            mLogFile->write(" ");
            mLogFile->write(Protocol::debugString(cmd).toUtf8());
            mLogFile->write("\n\n");
            mLogFile->flush();
        }

        if (cmd->type() == Protocol::Command::Hello) {
            Q_ASSERT(cmd->isResponse());
        }

        Q_EMIT commandReceived(tag, cmd);
        /*
        if (!handleCommand(tag, cmd)) {
            break;
        }
        */

        // FIXME: It happens often that data are arriving from the server faster
        // than we Jobs can process them which means, that we often process all
        // responses in single dataReceived() call and thus not returning to back
        // to QEventLoop, which breaks batch-delivery of ItemFetchJob (among other
        // things). To workaround that we force processing of events every
        // now and then.
        //
        // Longterm we want something better, like processing and parsing in
        // separate thread which would only post the parsed Protocol::Commands
        // to the jobs through event loop. That will be overall slower but should
        // result in much more responsive applications.
        if (timer.elapsed() > 50) {
            thread()->eventDispatcher()->processEvents(QEventLoop::ExcludeSocketNotifiers);
            timer.restart();
        }
    }
}

void Connection::sendCommand(qint64 tag, const Protocol::CommandPtr &cmd)
{
    const bool ok = QMetaObject::invokeMethod(this, "doSendCommand",
                    Qt::QueuedConnection,
                    Q_ARG(qint64, tag),
                    Q_ARG(Akonadi::Protocol::CommandPtr, cmd));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doSendCommand(qint64 tag, const Protocol::CommandPtr &cmd)
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mLogFile) {
        mLogFile->write("C: ");
        mLogFile->write(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz ")).toUtf8());
        mLogFile->write(QByteArray::number(tag));
        mLogFile->write(" ");
        mLogFile->write(Protocol::debugString(cmd).toUtf8());
        mLogFile->write("\n\n");
        mLogFile->flush();
    }

    if (mSocket && mSocket->isOpen()) {
        QDataStream stream(mSocket);
        stream << tag;
        try {
            Protocol::serialize(mSocket, cmd);
        } catch (const Akonadi::ProtocolException &e) {
            qCWarning(AKONADICORE_LOG) << "Protocol Exception:" << QString::fromUtf8(e.what());
            mSocket->close();
            mSocket->readAll();
            reconnect();
        }
    } else {
        // TODO: Queue the commands and resend on reconnect?
    }
}
