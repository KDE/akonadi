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
#include "connectionthread_p.h"
#include <akonadi/private/standarddirs_p.h>
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
    connThread->reconnect();
}

QString SessionPrivate::connectionFile()
{
    return StandardDirs::saveDir("config") + QStringLiteral("/akonadiconnectionrc");
}

void SessionPrivate::socketError(const QString &error)
{
    qWarning() << "Socket error occurred:" << error;
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
            connThread->disconnect();
            QTimer::singleShot(1000, connThread, &ConnectionThread::reconnect);
            return false;
        }

        qDebug() << "Connected to" << hello.serverName() << ", using protocol version" << hello.protocolVersion();
        qDebug() << "Server says:" << hello.message();
        // Version mismatch is handled in SessionPrivate::startJob() so that
        // we can report the error out via KJob API
        protocolVersion = hello.protocolVersion();
        Internal::setServerProtocolVersion(protocolVersion);

        Protocol::LoginCommand login(sessionId);
        sendCommand(nextTag(), login);
        return true;
    }

    // Login response
    if (cmd.type() == Protocol::Command::Login) {
        Protocol::LoginResponse login(cmd);
        if (login.isError()) {
            qWarning() << "Unable to login to Akonadi server:" << login.errorMessage();
            connThread->disconnect();
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
    if (protocolVersion != Protocol::version()) {
        job->setError(Job::ProtocolVersionMismatch);
        if (protocolVersion < Protocol::version()) {
            job->setErrorText(i18n("Protocol version mismatch. Server version is newer (%1) than ours (%2). "
                                   "If you updated your system recently please restart the Akonadi server.",
                                   protocolVersion, Protocol::version()));
            qWarning() << "Protocol version mismatch. Server version is newer (" << protocolVersion << ") than ours (" << Protocol::version() << "). "
                       "If you updated your system recently please restart the Akonadi server.";
        } else {
            job->setErrorText(i18n("Protocol version mismatch. Server version is older (%1) than ours (%2). "
                                   "If you updated your system recently please restart all KDE PIM applications.",
                                   protocolVersion, Protocol::version()));
            qWarning() << "Protocol version mismatch. Server version is older (" << protocolVersion << ") than ours (" << Protocol::version() << "). "
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
    connThread->sendCommand(tag, command);
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
    Q_FOREACH (Job *job, queue) {
        job->d_ptr->updateItemRevision(itemId, oldRevision, newRevision);
    }
}

//@endcond

SessionPrivate::SessionPrivate(Session *parent)
    : mParent(parent)
    , thread(0)
    , connThread(0)
    , protocolVersion(0)
    , currentJob(0)
{
}

SessionPrivate::~SessionPrivate()
{
    delete connThread;
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

    connThread = new ConnectionThread(sessionId);
    mParent->connect(connThread, &ConnectionThread::reconnected, mParent, &Session::reconnected,
                     Qt::QueuedConnection);
    mParent->connect(connThread, SIGNAL(commandReceived(qint64,Akonadi::Protocol::Command)),
                     mParent, SLOT(handleCommand(qint64, Akonadi::Protocol::Command)),
                     Qt::QueuedConnection);
    mParent->connect(connThread, SIGNAL(socketDisconnected()), mParent, SLOT(socketDisconnected()),
                     Qt::QueuedConnection);
    mParent->connect(connThread, SIGNAL(socketError(QString)), mParent, SLOT(socketError(QString)),
                     Qt::QueuedConnection);


    if (ServerManager::state() == ServerManager::NotRunning) {
        ServerManager::start();
    }
    mParent->connect(ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
                     SLOT(serverStateChanged(Akonadi::ServerManager::State)));

    reconnect();
}

void SessionPrivate::forceReconnect()
{
    jobRunning = false;
    connected = false;
    if (connThread) {
        connThread->forceReconnect();
    }
    QMetaObject::invokeMethod(mParent, "reconnect", Qt::QueuedConnection);
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

Q_GLOBAL_STATIC(QThreadStorage<Session *>, instances)

void SessionPrivate::createDefaultSession(const QByteArray &sessionId)
{
    Q_ASSERT_X(!sessionId.isEmpty(), "SessionPrivate::createDefaultSession",
               "You tried to create a default session with empty session id!");
    Q_ASSERT_X(!instances()->hasLocalData(), "SessionPrivate::createDefaultSession",
               "You tried to create a default session twice!");

    instances()->setLocalData(new Session(sessionId));
}

void SessionPrivate::setDefaultSession(Session *session)
{
    instances()->setLocalData(session);
}

Session *Session::defaultSession()
{
    if (!instances()->hasLocalData()) {
        instances()->setLocalData(new Session());
    }
    return instances()->localData();
}

void Session::clear()
{
    Q_FOREACH(Job *job, d->queue) {
        job->kill(KJob::EmitResult);   // safe, not started yet
    }
    d->queue.clear();
    Q_FOREACH(Job *job, d->pipeline) {
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
