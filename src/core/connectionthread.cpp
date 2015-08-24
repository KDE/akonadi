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

#include "connectionthread_p.h"
#include "session_p.h"
#include "servermanager_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>

#include <akonadi/private/xdgbasedirs_p.h>
#include <akonadi/private/protocol_exception_p.h>
#include <akonadi/private/standarddirs_p.h>

using namespace Akonadi;

ConnectionThread::ConnectionThread(const QByteArray &sessionId, QObject *parent)
    : QObject(parent)
    , mSocket(Q_NULLPTR)
    , mLogFile(Q_NULLPTR)
    , mSessionId(sessionId)
{
    qRegisterMetaType<Protocol::Command>();

    const QByteArray sessionLogFile = qgetenv("AKONADI_SESSION_LOGFILE");
    if (!sessionLogFile.isEmpty()) {
        mLogFile = new QFile(QStringLiteral("%1.%2.%3").arg(QString::fromLatin1(sessionLogFile))
                             .arg(QString::number(QApplication::applicationPid()))
                             .arg(QString::fromLatin1(mSessionId.replace('/', '_'))),
                             this);
        if (!mLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to open Akonadi Session log file" << mLogFile->fileName();
            delete mLogFile;
            mLogFile = Q_NULLPTR;
        }
    }
}

ConnectionThread::~ConnectionThread()
{
    if (mSocket) {
        mSocket->disconnect(this);
        mSocket->close();
    }
    delete mLogFile;
}

void ConnectionThread::quit()
{
    if (mSocket) {
        mSocket->disconnect(this);
    }
}

void ConnectionThread::reconnect()
{
    const bool ok = QMetaObject::invokeMethod(this, "doReconnect", Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void ConnectionThread::doReconnect()
{
    Q_ASSERT(QThread::currentThread() != qApp->thread());

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
        qDebug() << protocol << options;

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
            qDebug() << "Akonadi Client Session: connection config file '"
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
    qDebug() << "connectToServer" << serverAddress;
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
    Q_ASSERT(QThread::currentThread() != qApp->thread());

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
    Q_ASSERT(QThread::currentThread() != qApp->thread());

    if (mSocket) {
        mSocket->close();
        mSocket->readAll();
    }
}

void ConnectionThread::dataReceived()
{
    Q_ASSERT(QThread::currentThread() != qApp->thread());

    QElapsedTimer timer;
    timer.start();

    while (mSocket->bytesAvailable() > 0) {
        QDataStream stream(mSocket);
        qint64 tag;
        // TODO: Verify the tag matches the last tag we sent
        stream >> tag;

        Protocol::Command cmd = Protocol::deserialize(mSocket);
        if (cmd.type() == Protocol::Command::Invalid) {
            qWarning() << "Invalid command, the world is going to end!";
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
    Q_ASSERT(QThread::currentThread() != qApp->thread());

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
            qWarning() << "Protocol Exception:" << QString::fromUtf8(e.what());
            mSocket->close();
            mSocket->readAll();
            reconnect();
        }
    } else {
        // TODO: Queue the commands and resend on reconnect?
    }
}
