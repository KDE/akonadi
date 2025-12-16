/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "session.h"
#include "session_p.h"

#include "job.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"
#include "servermanager.h"
#include "servermanager_p.h"
#include "sessionthread_p.h"

#include "akonadicore_debug.h"

#include <KLocalizedString>

#include <QPointer>
#include <QRandomGenerator>
#include <QThread>
#include <QThreadStorage>
#include <QTimer>

#include <QCoreApplication>
#include <QHostAddress>

// ### FIXME pipelining got broken by switching result emission in JobPrivate::handleResponse to delayed emission
// in order to work around exec() deadlocks. As a result of that Session knows to late about a finished job and still
// sends responses for the next one to the already finished one
#define PIPELINE_LENGTH 0
// #define PIPELINE_LENGTH 2

using namespace Akonadi;
using namespace std::chrono_literals;
/// @cond PRIVATE

void SessionPrivate::startNext()
{
    QTimer::singleShot(0, mParent, [this]() {
        doStartNext();
    });
}

void SessionPrivate::reconnect()
{
    if (!connection) {
        connection = new Connection(Connection::CommandConnection, sessionId, &mCommandBuffer);
        sessionThread()->addConnection(connection);
        mParent->connect(connection, &Connection::reconnected, mParent, &Session::reconnected, Qt::QueuedConnection);
        mParent->connect(
            connection,
            &Connection::socketDisconnected,
            mParent,
            [this]() {
                socketDisconnected();
            },
            Qt::QueuedConnection);
        mParent->connect(
            connection,
            &Connection::socketError,
            mParent,
            [this](const QString &error) {
                socketError(error);
            },
            Qt::QueuedConnection);
    }

    connection->reconnect();
}

void SessionPrivate::socketError(const QString &error)
{
    qCWarning(AKONADICORE_LOG) << "Socket error occurred:" << error;
    socketDisconnected();
}

void SessionPrivate::socketDisconnected()
{
    if (currentJob) {
        currentJob->d_ptr->lostConnection();
    }
    connected = false;
}

bool SessionPrivate::handleCommands()
{
    CommandBufferLocker lock(&mCommandBuffer);
    CommandBufferNotifyBlocker notify(&mCommandBuffer);
    while (!mCommandBuffer.isEmpty()) {
        const auto command = mCommandBuffer.dequeue();
        lock.unlock();
        const auto cmd = command.command;
        const auto tag = command.tag;

        // Handle Hello response -> send Login
        if (cmd->type() == Protocol::Command::Hello) {
            const auto &hello = Protocol::cmdCast<Protocol::HelloResponse>(cmd);
            if (hello.isError()) {
                qCWarning(AKONADICORE_LOG) << "Error when establishing connection with Akonadi server:" << hello.errorMessage();
                connection->closeConnection();
                QTimer::singleShot(1s, connection, &Connection::reconnect);
                return false;
            }

            qCDebug(AKONADICORE_LOG) << "Connected to" << hello.serverName() << ", using protocol version" << hello.protocolVersion();
            qCDebug(AKONADICORE_LOG) << "Server generation:" << hello.generation();
            qCDebug(AKONADICORE_LOG) << "Server says:" << hello.message();
            // Version mismatch is handled in SessionPrivate::startJob() so that
            // we can report the error out via KJob API
            protocolVersion = hello.protocolVersion();
            Internal::setServerProtocolVersion(protocolVersion);
            Internal::setGeneration(hello.generation());

            sendCommand(nextTag(), Protocol::LoginCommandPtr::create(sessionId));
        } else if (cmd->type() == Protocol::Command::Login) {
            const auto &login = Protocol::cmdCast<Protocol::LoginResponse>(cmd);
            if (login.isError()) {
                qCWarning(AKONADICORE_LOG) << "Unable to login to Akonadi server:" << login.errorMessage();
                connection->closeConnection();
                QTimer::singleShot(1s, mParent, [this]() {
                    reconnect();
                });
                return false;
            }

            connected = true;
            startNext();
        } else if (currentJob) {
            currentJob->d_ptr->handleResponse(tag, cmd);
        }

        lock.relock();
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
            job->setErrorText(
                i18n("Protocol version mismatch. Server version is older (%1) than ours (%2). "
                     "If you updated your system recently please restart the Akonadi server.",
                     protocolVersion,
                     Protocol::version()));
            qCWarning(AKONADICORE_LOG) << "Protocol version mismatch. Server version is older (" << protocolVersion << ") than ours (" << Protocol::version()
                                       << "). "
                                          "If you updated your system recently please restart the Akonadi server.";
        } else {
            job->setErrorText(
                i18n("Protocol version mismatch. Server version is newer (%1) than ours (%2). "
                     "If you updated your system recently please restart all KDE PIM applications.",
                     protocolVersion,
                     Protocol::version()));
            qCWarning(AKONADICORE_LOG) << "Protocol version mismatch. Server version is newer (" << protocolVersion << ") than ours (" << Protocol::version()
                                       << "). "
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
            currentJob = nullptr;
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
    Q_UNUSED(job)

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
    QObject::connect(job, &KJob::result, mParent, [this](KJob *job) {
        jobDone(job);
    });
    QObject::connect(job, &Job::writeFinished, mParent, [this](Job *job) {
        jobWriteFinished(job);
    });
    QObject::connect(job, &QObject::destroyed, mParent, [this](QObject *o) {
        jobDestroyed(o);
    });
    startNext();
}

void SessionPrivate::publishOtherJobs(Job *thanThisJob)
{
    int count = 0;
    for (const auto &job : std::as_const(queue)) {
        if (job != thanThisJob) {
            job->d_ptr->publishJob();
            ++count;
        }
    }
    if (count > 0) {
        qCDebug(AKONADICORE_LOG) << "published" << count << "pending jobs to the job tracker";
    }
    if (currentJob && currentJob != thanThisJob) {
        currentJob->d_ptr->signalStartedToJobTracker();
    }
}

qint64 SessionPrivate::nextTag()
{
    return theNextTag++;
}

void SessionPrivate::sendCommand(qint64 tag, const Protocol::CommandPtr &command)
{
    connection->sendCommand(tag, command);
}

void SessionPrivate::serverStateChanged(ServerManager::State state)
{
    if (state == ServerManager::Running && !connected) {
        reconnect();
    } else if (!connected && state == ServerManager::Broken) {
        // If the server is broken, cancel all pending jobs, otherwise they will be
        // blocked forever and applications waiting for them to finish would be stuck
        auto q = queue;
        for (Job *job : q) {
            job->setError(Job::ConnectionFailed);
            job->kill(KJob::EmitResult);
        }
    } else if (state == ServerManager::Stopping) {
        sessionThread()->destroyConnection(connection);
        connection = nullptr;
    }
}

void SessionPrivate::itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    // only deal with the queue, for the guys in the pipeline it's too late already anyway
    // and they shouldn't have gotten there if they depend on a preceding job anyway.
    for (Job *job : std::as_const(queue)) {
        job->d_ptr->updateItemRevision(itemId, oldRevision, newRevision);
    }
}

/// @endcond

SessionPrivate::SessionPrivate(Session *parent)
    : mParent(parent)
    , mSessionThread(new SessionThread)
    , connection(nullptr)
    , protocolVersion(0)
    , mCommandBuffer(parent, "handleCommands")
    , currentJob(nullptr)
{
    // Shutdown the thread before QCoreApplication event loop quits - the
    // thread()->wait() mechanism in Connection dtor crashes sometimes
    // when called from QCoreApplication destructor
    connThreadCleanUp = QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [this]() {
        socketDisconnected();
        connection = nullptr;

        delete mSessionThread;
        mSessionThread = nullptr;
    });
}

SessionPrivate::~SessionPrivate()
{
    QObject::disconnect(connThreadCleanUp);
    delete mSessionThread;
}

void SessionPrivate::init(const QByteArray &id)
{
    if (!id.isEmpty()) {
        sessionId = id;
    } else {
        sessionId = QCoreApplication::instance()->applicationName().toUtf8() + '-' + QByteArray::number(QRandomGenerator::global()->generate());
    }

    qCDebug(AKONADICORE_LOG) << "Initializing session with ID" << id;

    connected = false;
    theNextTag = 2;
    jobRunning = false;

    if (ServerManager::state() == ServerManager::NotRunning) {
        ServerManager::start();
    }
    QObject::connect(ServerManager::self(), &ServerManager::stateChanged, mParent, [this](ServerManager::State state) {
        serverStateChanged(state);
    });
    reconnect();
}

void SessionPrivate::forceReconnect()
{
    jobRunning = false;
    connected = false;
    if (connection) {
        connection->forceReconnect();
    }
    QMetaObject::invokeMethod(
        mParent,
        [this]() {
            reconnect();
        },
        Qt::QueuedConnection);
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
    d->clear(false);
}

QByteArray Session::sessionId() const
{
    return d->sessionId;
}

Q_GLOBAL_STATIC(QThreadStorage<QPointer<Session>>, instances) // NOLINT(readability-redundant-member-init)

void SessionPrivate::createDefaultSession(const QByteArray &sessionId)
{
    Q_ASSERT_X(!sessionId.isEmpty(), "SessionPrivate::createDefaultSession", "You tried to create a default session with empty session id!");
    Q_ASSERT_X(!instances()->hasLocalData(), "SessionPrivate::createDefaultSession", "You tried to create a default session twice!");

    auto session = new Session(sessionId);
    setDefaultSession(session);
}

void SessionPrivate::setDefaultSession(Session *session)
{
    instances()->setLocalData({session});
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, []() {
        instances()->setLocalData({});
    });
}

Session *Session::defaultSession()
{
    if (!instances()->hasLocalData()) {
        auto session = new Session();
        SessionPrivate::setDefaultSession(session);
    }
    return instances()->localData().data();
}

void Session::clear()
{
    d->clear(true);
}

void SessionPrivate::clear(bool forceReconnect)
{
    auto q = queue;
    for (Job *job : q) {
        job->kill(KJob::EmitResult); // safe, not started yet
    }
    queue.clear();
    auto p = pipeline;
    for (Job *job : p) {
        job->d_ptr->mStarted = false; // avoid killing/reconnect loops
        job->kill(KJob::EmitResult);
    }
    pipeline.clear();
    if (currentJob) {
        currentJob->d_ptr->mStarted = false; // avoid killing/reconnect loops
        currentJob->kill(KJob::EmitResult);
    }

    if (forceReconnect) {
        this->forceReconnect();
    }
}

#include "moc_session.cpp"
