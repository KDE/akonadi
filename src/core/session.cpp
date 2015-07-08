/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "session.h"
#include "session_p.h"

#include "job.h"
#include "job_p.h"
#include "servermanager.h"
#include "servermanager_p.h"
#include "protocolhelper_p.h"
#include <akonadi/private/xdgbasedirs_p.h>
#include <akonadi/private/protocol_p.h>

#include <QDebug>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QQueue>
#include <QtCore/QThreadStorage>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QSettings>

#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QApplication>

// ### FIXME pipelining got broken by switching result emission in JobPrivate::handleResponse to delayed emission
// in order to work around exec() deadlocks. As a result of that Session knows to late about a finished job and still
// sends responses for the next one to the already finished one
#define PIPELINE_LENGTH 0
//#define PIPELINE_LENGTH 2

using namespace Akonadi;

//@cond PRIVATE


void SessionPrivate::startNext()
{
    QTimer::singleShot(0, mParent, SLOT(doStartNext()));
}

void SessionPrivate::reconnect()
{
    QLocalSocket *localSocket = qobject_cast<QLocalSocket *>(socket);
    if (localSocket && (localSocket->state() == QLocalSocket::ConnectedState
                        || localSocket->state() == QLocalSocket::ConnectingState)) {
        // nothing to do, we are still/already connected
        return;
    }

    QTcpSocket *tcpSocket = qobject_cast<QTcpSocket *>(socket);
    if (tcpSocket && (tcpSocket->state() == QTcpSocket::ConnectedState
                      || tcpSocket->state() == QTcpSocket::ConnectingState)) {
        // same here, but for TCP
        return;
    }

    // try to figure out where to connect to
    QString serverAddress;
    quint16 port = 0;
    bool useTcp = false;

    // env var has precedence
    const QByteArray serverAddressEnvVar = qgetenv("AKONADI_SERVER_ADDRESS");
    if (!serverAddressEnvVar.isEmpty()) {
        const int pos = serverAddressEnvVar.indexOf(':');
        const QByteArray protocol = serverAddressEnvVar.left(pos);
        QMap<QString, QString> options;
        foreach (const QString &entry, QString::fromLatin1(serverAddressEnvVar.mid(pos + 1)).split(QLatin1Char(','))) {
            const QStringList pair = entry.split(QLatin1Char('='));
            if (pair.size() != 2) {
                continue;
            }
            options.insert(pair.first(), pair.last());
        }
        qDebug() << protocol << options;

        if (protocol == "tcp") {
            serverAddress = options.value(QStringLiteral("host"));
            port = options.value(QStringLiteral("port")).toUInt();
            useTcp = true;
        } else if (protocol == "unix") {
            serverAddress = options.value(QStringLiteral("path"));
        } else if (protocol == "pipe") {
            serverAddress = options.value(QStringLiteral("name"));
        }
    }

    // try config file next, fall back to defaults if that fails as well
    if (serverAddress.isEmpty()) {
        const QString connectionConfigFile = connectionFile();
        const QFileInfo fileInfo(connectionConfigFile);
        if (!fileInfo.exists()) {
            qDebug() << "Akonadi Client Session: connection config file '"
                     "akonadi/akonadiconnectionrc' can not be found in"
                     << XdgBaseDirs::homePath("config") << "nor in any of"
                     << XdgBaseDirs::systemPathList("config");
        }
        const QSettings connectionSettings(connectionConfigFile, QSettings::IniFormat);

#ifdef Q_OS_WIN  //krazy:exclude=cpp
        serverAddress = connectionSettings.value(QStringLiteral("Data/NamedPipe"), QStringLiteral("Akonadi")).toString();
#else
        const QString defaultSocketDir = Internal::xdgSaveDir("data");
        serverAddress = connectionSettings.value(QStringLiteral("Data/UnixPath"), QString(defaultSocketDir + QStringLiteral("/akonadiserver.socket"))).toString();
#endif
    }

    // create sockets if not yet done, note that this does not yet allow changing socket types on the fly
    // but that's probably not something we need to support anyway
    if (!socket) {
        if (!useTcp) {
            socket = localSocket = new QLocalSocket(mParent);
            mParent->connect(localSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), SLOT(socketError(QLocalSocket::LocalSocketError)));
        } else {
            socket = tcpSocket = new QTcpSocket(mParent);
            mParent->connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketError(QAbstractSocket::SocketError)));
        }
        mParent->connect(socket, SIGNAL(disconnected()), SLOT(socketDisconnected()));
        mParent->connect(socket, SIGNAL(readyRead()), SLOT(dataReceived()));
    }

    // actually do connect
    qDebug() << "connectToServer" << serverAddress;
    if (!useTcp) {
        localSocket->connectToServer(serverAddress);
    } else {
        tcpSocket->connectToHost(serverAddress, port);
    }

    emit mParent->reconnected();
}

QString SessionPrivate::connectionFile()
{
    return Internal::xdgSaveDir("config") + QStringLiteral("/akonadiconnectionrc");
}

void SessionPrivate::socketError(QLocalSocket::LocalSocketError)
{
    Q_ASSERT(mParent->sender() == socket);
    qWarning() << "Socket error occurred:" << qobject_cast<QLocalSocket *>(socket)->errorString();
    socketDisconnected();
}

void SessionPrivate::socketError(QAbstractSocket::SocketError)
{
    Q_ASSERT(mParent->sender() == socket);
    qWarning() << "Socket error occurred:" << qobject_cast<QTcpSocket *>(socket)->errorString();
    socketDisconnected();
}

void SessionPrivate::socketDisconnected()
{
    if (currentJob) {
        currentJob->d_ptr->lostConnection();
    }
    connected = false;
}

bool SessionPrivate::handleCommand(qint64 tag, const Protocol::Command &cmd)
{
    // Handle Hello response -> send Login
    if (cmd.type() == Protocol::Command::Hello) {
        Protocol::HelloResponse hello(cmd);
        if (hello.isError()) {
            qWarning() << "Error when establishing connection with Akonadi server:" << hello.errorMessage();
            socket->close();
            QTimer::singleShot(1000, mParent, SLOT(reconnect()));
            return false;
        }

        qDebug() << "Connected to" << hello.serverName() << ", using protocol version" << hello.protocolVersion();
        qDebug() << "Server says:" << hello.message();
        // Version mismatch is handled in SessionPrivate::startJob() so that
        // we can report the error out via KJob API
        protocolVersion = hello.protocolVersion();

        Protocol::LoginCommand login(sessionId);
        sendCommand(nextTag(), login);
        return true;
    }

    // Login response
    if (cmd.type() == Protocol::Command::Login) {
        Protocol::LoginResponse login(cmd);
        if (login.isError()) {
            qWarning() << "Unable to login to Akonadi server:" << login.errorMessage();
            socket->close();
            QTimer::singleShot(1000, mParent, SLOT(reconnect()));
            return false;
        }

        connected = true;
        startNext();
        return true;
    }

    // work for the current job
    if (currentJob) {
        currentJob->d_ptr->handleResponse(tag, cmd);
    }

    return true;
}

void SessionPrivate::dataReceived()
{
    int iterations = 0;

    while (socket->bytesAvailable() > 0) {
        QDataStream stream(socket);
        qint64 tag;
        // TODO: Verify the tag matches the last tag we sent
        stream >> tag;

        Protocol::Command cmd = Protocol::deserialize(socket);
        if (cmd.type() == Protocol::Command::Invalid) {
            qWarning() << "Invalid command, the world is going to end!";
            socket->close();
            reconnect();
            return;
        }


        if (logFile) {
            logFile->write("S: " + cmd.debugString().toUtf8());
            logFile->write("\n\n");
            logFile->flush();
        }

        if (!handleCommand(tag, cmd)) {
            break;
        }

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
        if (++iterations == 1000) {
            qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
            iterations = 0;
        }
    }
}

bool SessionPrivate::canPipelineNext()
{
    if (queue.isEmpty() || pipeline.count() >= PIPELINE_LENGTH) {
        return false;
    }
    if (pipeline.isEmpty() && currentJob) {
        return currentJob->d_ptr->mWriteFinished;
    }
    if (!pipeline.isEmpty()) {
        return pipeline.last()->d_ptr->mWriteFinished;
    }
    return false;
}

void SessionPrivate::doStartNext()
{
    if (!connected || (queue.isEmpty() && pipeline.isEmpty())) {
        return;
    }
    if (canPipelineNext()) {
        Akonadi::Job *nextJob = queue.dequeue();
        pipeline.enqueue(nextJob);
        startJob(nextJob);
    }
    if (jobRunning) {
        return;
    }
    jobRunning = true;
    if (!pipeline.isEmpty()) {
        currentJob = pipeline.dequeue();
    } else {
        currentJob = queue.dequeue();
        startJob(currentJob);
    }
}

void SessionPrivate::startJob(Job *job)
{
    if (protocolVersion != clientProtocolVersion()) {
        job->setError(Job::ProtocolVersionMismatch);
        if (protocolVersion < SessionPrivate::clientProtocolVersion()) {
            job->setErrorText(i18n("Protocol version mismatch. Server version is newer (%1) than ours (%2). "
                                    "If you updated your system recently please restart the Akonadi server.",
                                    protocolVersion, clientProtocolVersion()));
            qWarning() << "Protocol version mismatch. Server version is newer (" << protocolVersion << ") than ours (" << clientProtocolVersion() << "). "
                            "If you updated your system recently please restart the Akonadi server.";
        } else {
            job->setErrorText(i18n("Protocol version mismatch. Server version is older (%1) than ours (%2). "
                                    "If you updated your system recently please restart all KDE PIM applications.",
                                    protocolVersion, clientProtocolVersion()));
            qWarning() << "Protocol version mismatch. Server version is older (" << protocolVersion << ") than ours (" << clientProtocolVersion() << "). "
                            "If you updated your system recently please restart all KDE PIM applications.";
        }
        job->emitResult();
    } else {
        job->d_ptr->startQueued();
    }
}

void SessionPrivate::endJob(Job *job)
{
    job->emitResult();
}

void SessionPrivate::jobDone(KJob *job)
{
    // ### careful, this method can be called from the QObject dtor of job (see jobDestroyed() below)
    // so don't call any methods on job itself
    if (job == currentJob) {
        if (pipeline.isEmpty()) {
            jobRunning = false;
            currentJob = 0;
        } else {
            currentJob = pipeline.dequeue();
        }
        startNext();
    } else {
        // non-current job finished, likely canceled while still in the queue
        queue.removeAll(static_cast<Akonadi::Job *>(job));
        // ### likely not enough to really cancel already running jobs
        pipeline.removeAll(static_cast<Akonadi::Job *>(job));
    }
}

void SessionPrivate::jobWriteFinished(Akonadi::Job *job)
{
    Q_ASSERT((job == currentJob && pipeline.isEmpty()) || (job = pipeline.last()));
    Q_UNUSED(job);

    startNext();
}

void SessionPrivate::jobDestroyed(QObject *job)
{
    // careful, accessing non-QObject methods of job will fail here already
    jobDone(static_cast<KJob *>(job));
}

void SessionPrivate::addJob(Job *job)
{
    queue.append(job);
    QObject::connect(job, SIGNAL(result(KJob*)), mParent, SLOT(jobDone(KJob*)));
    QObject::connect(job, SIGNAL(writeFinished(Akonadi::Job*)), mParent, SLOT(jobWriteFinished(Akonadi::Job*)));
    QObject::connect(job, SIGNAL(destroyed(QObject*)), mParent, SLOT(jobDestroyed(QObject*)));
    startNext();
}

qint64 SessionPrivate::nextTag()
{
    return theNextTag++;
}

void SessionPrivate::sendCommand(qint64 tag, const Protocol::Command &command)
{
    if (logFile) {
        logFile->write("C: " + command.debugString().toUtf8());
        logFile->write("\n\n");
        logFile->flush();
    }

    if (socket && socket->isOpen()) {
        QDataStream stream(socket);
        stream << tag;
        Protocol::serialize(socket, command);
    } else {
        // TODO: Queue the commands and resend on reconnect?
    }
}

void SessionPrivate::serverStateChanged(ServerManager::State state)
{
    if (state == ServerManager::Running && !connected) {
        reconnect();
    } else if (!connected && state == ServerManager::Broken) {
        // If the server is broken, cancel all pending jobs, otherwise they will be
        // blocked forever and applications waiting for them to finish would be stuck
        Q_FOREACH (Job *job, queue) {
            job->setError(Job::ConnectionFailed);
            job->kill(KJob::EmitResult);
        }
    }
}

void SessionPrivate::itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    // only deal with the queue, for the guys in the pipeline it's too late already anyway
    // and they shouldn't have gotten there if they depend on a preceding job anyway.
    foreach (Job *job, queue) {
        job->d_ptr->updateItemRevision(itemId, oldRevision, newRevision);
    }
}

//@endcond

SessionPrivate::SessionPrivate(Session *parent)
    : mParent(parent)
    , socket(0)
    , protocolVersion(0)
    , currentJob(0)
    , logFile(0)
{
}

SessionPrivate::~SessionPrivate()
{
}

void SessionPrivate::init(const QByteArray &id)
{
    qDebug() << id;

    if (!id.isEmpty()) {
        sessionId = id;
    } else {
        sessionId = QCoreApplication::instance()->applicationName().toUtf8()
                    + '-' + QByteArray::number(qrand());
    }
    connected = false;
    theNextTag = 2;
    jobRunning = false;

    if (ServerManager::state() == ServerManager::NotRunning) {
        ServerManager::start();
    }
    mParent->connect(ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
                     SLOT(serverStateChanged(Akonadi::ServerManager::State)));

    const QByteArray sessionLogFile = qgetenv("AKONADI_SESSION_LOGFILE");
    if (!sessionLogFile.isEmpty()) {
        logFile = new QFile(QString::fromLatin1("%1.%2.%3").arg(QString::fromLatin1(sessionLogFile))
                            .arg(QString::number(QApplication::applicationPid()))
                            .arg(QString::fromLatin1(sessionId.replace('/', '_'))),
                            mParent);
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to open Akonadi Session log file" << logFile->fileName();
            delete logFile;
            logFile = 0;
        }
    }

    reconnect();
}

void SessionPrivate::forceReconnect()
{
    jobRunning = false;
    connected = false;
    if (socket) {
        socket->disconnect(mParent);   // prevent signal emitted from close() causing mayhem - we might be called from ~QThreadStorage!
        delete socket;
    }
    socket = 0;
    QMetaObject::invokeMethod(mParent, "reconnect", Qt::QueuedConnection);   // avoids reconnecting in the dtor
}

Session::Session(const QByteArray &sessionId, QObject *parent)
    : QObject(parent)
    , d(new SessionPrivate(this))
{
    d->init(sessionId);
}

Session::Session(SessionPrivate *dd, const QByteArray &sessionId, QObject *parent)
    : QObject(parent)
    , d(dd)
{
    d->mParent = this;
    d->init(sessionId);
}

Session::~Session()
{
    clear();
    delete d;
}

QByteArray Session::sessionId() const
{
    return d->sessionId;
}

static QThreadStorage<Session *> instances;

void SessionPrivate::createDefaultSession(const QByteArray &sessionId)
{
    Q_ASSERT_X(!sessionId.isEmpty(), "SessionPrivate::createDefaultSession",
               "You tried to create a default session with empty session id!");
    Q_ASSERT_X(!instances.hasLocalData(), "SessionPrivate::createDefaultSession",
               "You tried to create a default session twice!");

    instances.setLocalData(new Session(sessionId));
}

void SessionPrivate::setDefaultSession(Session *session)
{
    instances.setLocalData(session);
}

Session *Session::defaultSession()
{
    if (!instances.hasLocalData()) {
        instances.setLocalData(new Session());
    }
    return instances.localData();
}

void Session::clear()
{
    foreach (Job *job, d->queue) {
        job->kill(KJob::EmitResult);   // safe, not started yet
    }
    d->queue.clear();
    foreach (Job *job, d->pipeline) {
        job->d_ptr->mStarted = false; // avoid killing/reconnect loops
        job->kill(KJob::EmitResult);
    }
    d->pipeline.clear();
    if (d->currentJob) {
        d->currentJob->d_ptr->mStarted = false; // avoid killing/reconnect loops
        d->currentJob->kill(KJob::EmitResult);
    }
    d->forceReconnect();
}

#include "moc_session.cpp"
