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
#include "commandbuffer_p.h"
#include <private/instance_p.h>

#include <QDataStream>
#include <QFile>
#include <QAbstractEventDispatcher>
#include <QFileInfo>
#include <QSettings>
#include <QElapsedTimer>
#include <QApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QTimer>

#include <private/protocol_exception_p.h>
#include <private/standarddirs_p.h>

using namespace Akonadi;

Connection::Connection(ConnectionType connType, const QByteArray &sessionId,
                       CommandBuffer *commandBuffer, QObject *parent)
    : QObject(parent)
    , mConnectionType(connType)
    , mSessionId(sessionId)
    , mCommandBuffer(commandBuffer)
{
    qRegisterMetaType<Protocol::CommandPtr>();
    qRegisterMetaType<QAbstractSocket::SocketState>();

    const QByteArray sessionLogFile = qgetenv("AKONADI_SESSION_LOGFILE");
    if (!sessionLogFile.isEmpty()) {
        mLogFile = new QFile(QStringLiteral("%1.%2.%3.%4-%5").arg(QString::fromLatin1(sessionLogFile))
                             .arg(QString::number(QApplication::applicationPid()))
                             .arg(QString::number(reinterpret_cast<qulonglong>(this), 16))
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
        mSocket->disconnect();
        mSocket->disconnectFromServer();
        mSocket->close();
        mSocket.reset();
    }
}

void Connection::reconnect()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    const bool ok = QMetaObject::invokeMethod(this, &Connection::doReconnect, Qt::QueuedConnection);
#else
    const bool ok = QMetaObject::invokeMethod(this, "doReconnect", Qt::QueuedConnection);
#endif
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

QString Connection::defaultAddressForTypeAndMethod(ConnectionType type, const QString &method)
{
    if (method == QLatin1String("UnixPath")) {
        const QString defaultSocketDir = StandardDirs::saveDir("data");
        if (type == CommandConnection) {
            return defaultSocketDir % QStringLiteral("akonadiserver-cmd.socket");
        } else if (type == NotificationConnection) {
            return defaultSocketDir % QStringLiteral("akonadiserver-ntf.socket");
        }
   } else if (method == QLatin1String("NamedPipe")) {
        QString suffix;
        if (Instance::hasIdentifier()) {
            suffix += QStringLiteral("%1-").arg(Instance::identifier());
        }
        suffix += QString::fromUtf8(QUrl::toPercentEncoding(qApp->applicationDirPath()));
        if (type == CommandConnection) {
            return QStringLiteral("Akonadi-Cmd-") % suffix;
        } else if (type == NotificationConnection) {
            return QStringLiteral("Akonadi-Ntf-") % suffix;
       }
   }

   Q_UNREACHABLE();
}

void Connection::doReconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket && (mSocket->state() == QLocalSocket::ConnectedState
                    || mSocket->state() == QLocalSocket::ConnectingState)) {
        // nothing to do, we are still/already connected
        return;
    }

    if (ServerManager::self()->state() != ServerManager::Running) {
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
        const QString connectionConfigFile = StandardDirs::connectionConfigFile();
        const QFileInfo fileInfo(connectionConfigFile);
        if (!fileInfo.exists()) {
            qCDebug(AKONADICORE_LOG) << "Akonadi Client Session: connection config file '"
                                        "akonadi/akonadiconnectionrc' can not be found!";
        }

        QSettings connectionSettings(connectionConfigFile, QSettings::IniFormat);

        QString connectionType;
        if (mConnectionType == CommandConnection) {
            connectionType = QStringLiteral("Data");
        } else if (mConnectionType == NotificationConnection) {
            connectionType = QStringLiteral("Notifications");
        }

        connectionSettings.beginGroup(connectionType);
        const auto method = connectionSettings.value(QStringLiteral("Method"), QStringLiteral("UnixPath")).toString();
        serverAddress = connectionSettings.value(method, defaultAddressForTypeAndMethod(mConnectionType, method)).toString();
    }

    // create sockets if not yet done, note that this does not yet allow changing socket types on the fly
    // but that's probably not something we need to support anyway
    if (!mSocket) {
        mSocket.reset(new QLocalSocket(this));
        connect(mSocket.data(), static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this,
        [this](QLocalSocket::LocalSocketError) {
            qCWarning(AKONADICORE_LOG) << mSocket->errorString() << mSocket->serverName();
            Q_EMIT socketError(mSocket->errorString());
            Q_EMIT socketDisconnected();
        });
        connect(mSocket.data(), &QLocalSocket::disconnected, this, &Connection::socketDisconnected);
        connect(mSocket.data(), &QLocalSocket::readyRead, this, &Connection::handleIncomingData);
    }

    // actually do connect
    qCDebug(AKONADICORE_LOG) << "connectToServer" << serverAddress;
    mSocket->connectToServer(serverAddress);
    if (!mSocket->waitForConnected()) {
        qCWarning(AKONADICORE_LOG) << "Failed to connect to server!";
        Q_EMIT socketError(tr("Failed to connect to server!"));
        mSocket.reset();
        return;
    }

    QTimer::singleShot(0, this, &Connection::handleIncomingData);

    Q_EMIT reconnected();
}

void Connection::forceReconnect()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    const bool ok = QMetaObject::invokeMethod(this, &Connection::doForceReconnect,
                    Qt::QueuedConnection);
#else
    const bool ok = QMetaObject::invokeMethod(this, "doForceReconnect",
                    Qt::QueuedConnection);
#endif

    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doForceReconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->disconnect(this, SIGNAL(socketDisconnected()));
        mSocket->disconnectFromServer();
        mSocket.reset();
    }
}

void Connection::closeConnection()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    const bool ok = QMetaObject::invokeMethod(this, &Connection::doCloseConnection,
                    Qt::QueuedConnection);
#else
    const bool ok = QMetaObject::invokeMethod(this, "doCloseConnection", Qt::QueuedConnection);
#endif

    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doCloseConnection()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->close();
        mSocket->readAll();
        mSocket.reset();
    }
}

QLocalSocket *Connection::socket() const
{
    return mSocket.data();
}

void Connection::handleIncomingData()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (!mSocket) { // not connected yet
        return;
    }

    while (mSocket->bytesAvailable() >= int(sizeof(qint64))) {
        QDataStream stream(mSocket.data());
        qint64 tag;
        // TODO: Verify the tag matches the last tag we sent
        stream >> tag;

        Protocol::CommandPtr cmd;
        try {
            cmd = Protocol::deserialize(mSocket.data());
        } catch (const Akonadi::ProtocolException &e) {
            qCWarning(AKONADICORE_LOG) << "Protocol exception:" << e.what();
            // cmd's type will be Invalid by default, so fall-through
        }
        if (!cmd || (cmd->type() == Protocol::Command::Invalid)) {
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

        {
            CommandBufferLocker locker(mCommandBuffer);
            mCommandBuffer->enqueue(tag, cmd);
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
        QDataStream stream(mSocket.data());
        stream << tag;
        try {
            Protocol::serialize(mSocket.data(), cmd);
        } catch (const Akonadi::ProtocolException &e) {
            qCWarning(AKONADICORE_LOG) << "Protocol Exception:" << QString::fromUtf8(e.what());
            mSocket->close();
            mSocket->readAll();
            reconnect();
            return;
        }
        if (!mSocket->waitForBytesWritten()) {
            qCWarning(AKONADICORE_LOG) << "Socket write timeout";
            mSocket->close();
            mSocket->readAll();
            reconnect();
            return;
        }
    } else {
        // TODO: Queue the commands and resend on reconnect?
    }
}
