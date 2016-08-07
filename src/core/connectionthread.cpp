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

#include "connectionthread_p.h"
#include "session_p.h"
#include "servermanager_p.h"
#include "akonadicore_debug.h"

#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>

#include <private/xdgbasedirs_p.h>
#include <private/protocol_exception_p.h>
#include <private/standarddirs_p.h>

using namespace Akonadi;

ConnectionThread::ConnectionThread(const QByteArray &sessionId, QObject *parent)
    : QObject(parent)
    , mSocket(Q_NULLPTR)
    , mLogFile(Q_NULLPTR)
    , mSessionId(sessionId)
{
    qRegisterMetaType<Protocol::Command>();
    qRegisterMetaType<QAbstractSocket::SocketState>();

    QThread *thread = new QThread();
    moveToThread(thread);
    thread->start();

    const QByteArray sessionLogFile = qgetenv("AKONADI_SESSION_LOGFILE");
    if (!sessionLogFile.isEmpty()) {
        mLogFile = new QFile(QStringLiteral("%1.%2.%3").arg(QString::fromLatin1(sessionLogFile),
                                                            QString::number(QApplication::applicationPid()),
                                                            QString::fromLatin1(mSessionId.replace('/', '_'))));
        if (!mLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(AKONADICORE_LOG) << "Failed to open Akonadi Session log file" << mLogFile->fileName();
            delete mLogFile;
            mLogFile = Q_NULLPTR;
        }
    }
}

ConnectionThread::~ConnectionThread()
{
    if (QCoreApplication::instance()) {
        QMetaObject::invokeMethod(this, "doThreadQuit");
    } else {
        // QCoreApplication already destroyed -> invokeMethod would just not get the message delivered
        // We leak the socket, but at least we don't block for 10s
        qWarning() << "Akonadi ConnectionThread deleted after QCoreApplication is destroyed. Clean up your sessions earlier!";
        thread()->quit();
    }
    if (!thread()->wait(10 * 1000)) {
        thread()->terminate();
        // Make sure to wait until it's done, otherwise it can crash when the pthread callback is called
        thread()->wait();
    }
    delete mLogFile;
    delete thread();
}

void ConnectionThread::doThreadQuit()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (mSocket) {
        mSocket->disconnect(this);
        mSocket->close();
        delete mSocket;
    }
    thread()->quit();
}

void ConnectionThread::reconnect()
{
    const bool ok = QMetaObject::invokeMethod(this, "doReconnect", Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void ConnectionThread::doReconnect()
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
        Q_FOREACH (const QString &entry, QString::fromLatin1(serverAddressEnvVar.mid(pos + 1)).split(QLatin1Char(','))) {
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
        serverAddress = connectionSettings.value(QStringLiteral("Data/UnixPath"), QString(defaultSocketDir + QStringLiteral("/akonadiserver.socket"))).toString();
    }

    // create sockets if not yet done, note that this does not yet allow changing socket types on the fly
    // but that's probably not something we need to support anyway
    if (!mSocket) {
        mSocket = new QLocalSocket(this);
        connect(mSocket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this,
                [this](QLocalSocket::LocalSocketError) {
                    Q_EMIT socketError(mSocket->errorString());
                    Q_EMIT socketDisconnected();
                });
        connect(mSocket, &QLocalSocket::disconnected, this, &ConnectionThread::socketDisconnected);
        connect(mSocket, &QLocalSocket::readyRead, this, &ConnectionThread::dataReceived);
    }

    // actually do connect
    qCDebug(AKONADICORE_LOG) << "connectToServer" << serverAddress;
    mSocket->connectToServer(serverAddress);

    Q_EMIT reconnected();
}

void ConnectionThread::forceReconnect()
{
    const bool ok = QMetaObject::invokeMethod(this, "doForceReconnect",
                                              Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void ConnectionThread::doForceReconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->disconnect(this, SIGNAL(socketDisconnected()));
        delete mSocket;
        mSocket = Q_NULLPTR;
    }
    mSocket = Q_NULLPTR;
}

void ConnectionThread::disconnect()
{
    const bool ok = QMetaObject::invokeMethod(this, "doDisconnect", Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void ConnectionThread::doDisconnect()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mSocket) {
        mSocket->close();
        mSocket->readAll();
    }
}

void ConnectionThread::dataReceived()
{
    Q_ASSERT(QThread::currentThread() == thread());

    QElapsedTimer timer;
    timer.start();

    while (mSocket->bytesAvailable() > 0) {
        QDataStream stream(mSocket);
        qint64 tag;
        // TODO: Verify the tag matches the last tag we sent
        stream >> tag;

        Protocol::Command cmd;
        try {
            cmd = Protocol::deserialize(mSocket);
        } catch (const Akonadi::ProtocolException &) {
            // cmd's type will be Invalid by default.
        }
        if (cmd.type() == Protocol::Command::Invalid) {
            qCWarning(AKONADICORE_LOG) << "Invalid command, the world is going to end!";
            mSocket->close();
            mSocket->readAll();
            reconnect();
            return;
        }

        if (mLogFile) {
            mLogFile->write("S: " + cmd.debugString().toUtf8());
            mLogFile->write("\n\n");
            mLogFile->flush();
        }

        if (cmd.type() == Protocol::Command::Hello)
            Q_ASSERT(cmd.isResponse());

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
            QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::ExcludeSocketNotifiers);
            timer.restart();
        }
    }
}

void ConnectionThread::sendCommand(qint64 tag, const Protocol::Command &cmd)
{
    const bool ok = QMetaObject::invokeMethod(this, "doSendCommand",
                                              Qt::QueuedConnection,
                                              Q_ARG(qint64, tag),
                                              Q_ARG(Akonadi::Protocol::Command, cmd));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void ConnectionThread::doSendCommand(qint64 tag, const Protocol::Command &cmd)
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mLogFile) {
        mLogFile->write("C: " + cmd.debugString().toUtf8());
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
