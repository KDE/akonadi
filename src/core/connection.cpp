/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "akonadicore_debug.h"
#include "commandbuffer_p.h"
#include "connection_p.h"
#include "servermanager_p.h"
#include "session_p.h"
#include <private/instance_p.h>

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>

#include <private/datastream_p_p.h>
#include <private/protocol_exception_p.h>
#include <private/standarddirs_p.h>

using namespace Akonadi;

Connection::Connection(ConnectionType connType, const QByteArray &sessionId, CommandBuffer *commandBuffer, QObject *parent)
    : QObject(parent)
    , mConnectionType(connType)
    , mSessionId(sessionId)
    , mCommandBuffer(commandBuffer)
{
    qRegisterMetaType<Protocol::CommandPtr>();
    qRegisterMetaType<QAbstractSocket::SocketState>();

    const QByteArray sessionLogFile = qgetenv("AKONADI_SESSION_LOGFILE");
    if (!sessionLogFile.isEmpty()) {
        mLogFile = new QFile(QStringLiteral("%1.%2.%3.%4-%5")
                                 .arg(QString::fromLatin1(sessionLogFile))
                                 .arg(QApplication::applicationPid())
                                 .arg(QString::number(reinterpret_cast<qulonglong>(this), 16),
                                      QString::fromLatin1(mSessionId.replace('/', '_')),
                                      connType == CommandConnection ? QStringLiteral("Cmd") : QStringLiteral("Ntf")));
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
    const bool ok = QMetaObject::invokeMethod(this, &Connection::doReconnect, Qt::QueuedConnection);
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

    if (mSocket && (mSocket->state() == QLocalSocket::ConnectedState || mSocket->state() == QLocalSocket::ConnectingState)) {
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
            qCWarning(AKONADICORE_LOG) << "Akonadi Client Session: connection config file '"
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

    mSocket.reset(new QLocalSocket(this));
    connect(mSocket.data(), &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError /*unused*/) {
        qCWarning(AKONADICORE_LOG) << mSocket->errorString() << mSocket->serverName();
        Q_EMIT socketError(mSocket->errorString());
        Q_EMIT socketDisconnected();
    });
    connect(mSocket.data(), &QLocalSocket::disconnected, this, &Connection::socketDisconnected);
    // note: we temporarily disconnect from readyRead-signal inside handleIncomingData()
    connect(mSocket.data(), &QLocalSocket::readyRead, this, &Connection::handleIncomingData);

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
    const bool ok = QMetaObject::invokeMethod(this, &Connection::doForceReconnect, Qt::QueuedConnection);

    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doForceReconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        disconnect(mSocket.get(), &QLocalSocket::disconnected, this, &Connection::socketDisconnected);
        mSocket->disconnectFromServer();
        mSocket.reset();
    }
}

void Connection::closeConnection()
{
    const bool ok = QMetaObject::invokeMethod(this, &Connection::doCloseConnection, Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void Connection::doCloseConnection()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->close();
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
        Protocol::DataStream stream(mSocket.data());
        qint64 tag;
        stream >> tag;

        // temporarily disconnect from readyRead-signal to avoid re-entering this function when we
        // call waitForData() deep inside Protocol::deserialize
        disconnect(mSocket.data(), &QLocalSocket::readyRead, this, &Connection::handleIncomingData);

        Protocol::CommandPtr cmd;
        try {
            cmd = Protocol::deserialize(mSocket.data());
        } catch (const Akonadi::ProtocolException &e) {
            qCWarning(AKONADICORE_LOG) << "Protocol exception:" << e.what();
            // cmd's type will be Invalid by default, so fall-through
        }

        // reconnect to the signal again
        connect(mSocket.data(), &QLocalSocket::readyRead, this, &Connection::handleIncomingData);

        if (!cmd || (cmd->type() == Protocol::Command::Invalid)) {
            qCWarning(AKONADICORE_LOG) << "Invalid command, the world is going to end!";
            mSocket->close();
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
    const bool ok = QMetaObject::invokeMethod(this, "doSendCommand", Qt::QueuedConnection, Q_ARG(qint64, tag), Q_ARG(Akonadi::Protocol::CommandPtr, cmd));
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
        Protocol::DataStream stream(mSocket.data());
        try {
            stream << tag;
            Protocol::serialize(stream, cmd);
            stream.flush();
        } catch (const Akonadi::ProtocolException &e) {
            qCWarning(AKONADICORE_LOG) << "Protocol Exception:" << QString::fromUtf8(e.what());
            mSocket->close();
            reconnect();
            return;
        }
        if (!mSocket->waitForBytesWritten()) {
            qCWarning(AKONADICORE_LOG) << "Socket write timeout";
            mSocket->close();
            reconnect();
            return;
        }
    } else {
        // TODO: Queue the commands and resend on reconnect?
    }
}
