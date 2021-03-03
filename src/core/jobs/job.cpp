/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>
                  2006 - 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "job.h"
#include "akonadicore_debug.h"
#include "job_p.h"
#include "private/instance_p.h"
#include "private/protocol_p.h"
#include "session.h"
#include "session_p.h"
#include <QDBusConnection>
#include <QTime>

#include <KLocalizedString>

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QElapsedTimer>
#include <QTimer>

using namespace Akonadi;

static QDBusAbstractInterface *s_jobtracker = nullptr;

/// @cond PRIVATE
void JobPrivate::handleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_Q(Job);

    if (mCurrentSubJob) {
        mCurrentSubJob->d_ptr->handleResponse(tag, response);
        return;
    }

    if (tag == mTag) {
        if (response->isResponse()) {
            const auto &resp = Protocol::cmdCast<Protocol::Response>(response);
            if (resp.isError()) {
                q->setError(Job::Unknown);
                q->setErrorText(resp.errorMessage());
                q->emitResult();
                return;
            }
        }
    }

    if (mTag != tag) {
        qCWarning(AKONADICORE_LOG) << "Received response with a different tag!";
        qCDebug(AKONADICORE_LOG) << "Response tag:" << tag << ", response type:" << response->type();
        qCDebug(AKONADICORE_LOG) << "Job tag:" << mTag << ", job:" << q;
        return;
    }

    if (mStarted) {
        if (mReadingFinished) {
            qCWarning(AKONADICORE_LOG) << "Received response for a job that does not expect any more data, ignoring";
            qCDebug(AKONADICORE_LOG) << "Response tag:" << tag << ", response type:" << response->type();
            qCDebug(AKONADICORE_LOG) << "Job tag:" << mTag << ", job:" << q;
            Q_ASSERT(!mReadingFinished);
            return;
        }

        if (q->doHandleResponse(tag, response)) {
            mReadingFinished = true;
            QTimer::singleShot(0, q, [this]() {
                delayedEmitResult();
            });
        }
    }
}

void JobPrivate::init(QObject *parent)
{
    Q_Q(Job);

    mParentJob = qobject_cast<Job *>(parent);
    mSession = qobject_cast<Session *>(parent);

    if (!mSession) {
        if (!mParentJob) {
            mSession = Session::defaultSession();
        } else {
            mSession = mParentJob->d_ptr->mSession;
        }
    }

    if (!mParentJob) {
        mSession->d->addJob(q);
    } else {
        mParentJob->addSubjob(q);
    }
    publishJob();
}

void JobPrivate::publishJob()
{
    Q_Q(Job);
    // if there's a job tracker running, tell it about the new job
    if (!s_jobtracker) {
        // Let's only check for the debugging console every 3 seconds, otherwise every single job
        // makes a dbus call to the dbus daemon, doesn't help performance.
        static QElapsedTimer s_lastTime;
        if (!s_lastTime.isValid() || s_lastTime.elapsed() > 3000) {
            if (!s_lastTime.isValid()) {
                s_lastTime.start();
            }
            const QString suffix = Akonadi::Instance::identifier().isEmpty() ? QString() : QLatin1Char('-') + Akonadi::Instance::identifier();
            if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.akonadiconsole") + suffix)) {
                s_jobtracker = new QDBusInterface(QLatin1String("org.kde.akonadiconsole") + suffix,
                                                  QStringLiteral("/jobtracker"),
                                                  QStringLiteral("org.freedesktop.Akonadi.JobTracker"),
                                                  QDBusConnection::sessionBus(),
                                                  nullptr);
                mSession->d->publishOtherJobs(q);
            } else {
                s_lastTime.restart();
            }
        }
        // Note: we never reset s_jobtracker to 0 when a call fails; but if we did
        // then we should restart s_lastTime.
    }
    QMetaObject::invokeMethod(q, "signalCreationToJobTracker", Qt::QueuedConnection);
}

void JobPrivate::signalCreationToJobTracker()
{
    Q_Q(Job);
    if (s_jobtracker) {
        // We do these dbus calls manually, so as to avoid having to install (or copy) the console's
        // xml interface document. Since this is purely a debugging aid, that seems preferable to
        // publishing something not intended for public consumption.
        // WARNING: for any signature change here, apply it to resourcescheduler.cpp too
        const QList<QVariant> argumentList = QList<QVariant>() << QLatin1String(mSession->sessionId()) << QString::number(reinterpret_cast<quintptr>(q), 16)
                                                               << (mParentJob ? QString::number(reinterpret_cast<quintptr>(mParentJob), 16) : QString())
                                                               << QString::fromLatin1(q->metaObject()->className()) << jobDebuggingString();
        QDBusPendingCall call = s_jobtracker->asyncCallWithArgumentList(QStringLiteral("jobCreated"), argumentList);

        auto watcher = new QDBusPendingCallWatcher(call, s_jobtracker);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, s_jobtracker, [](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<void> reply = *w;
            if (reply.isError() && s_jobtracker) {
                qDebug() << reply.error().name() << reply.error().message();
                s_jobtracker->deleteLater();
                s_jobtracker = nullptr;
            }
            w->deleteLater();
        });
    }
}

void JobPrivate::signalStartedToJobTracker()
{
    Q_Q(Job);
    if (s_jobtracker) {
        // if there's a job tracker running, tell it a job started
        const QList<QVariant> argumentList = {QString::number(reinterpret_cast<quintptr>(q), 16)};
        s_jobtracker->callWithArgumentList(QDBus::NoBlock, QStringLiteral("jobStarted"), argumentList);
    }
}

void JobPrivate::aboutToFinish()
{
    // Dummy
}

void JobPrivate::delayedEmitResult()
{
    Q_Q(Job);
    if (q->hasSubjobs()) {
        // We still have subjobs, wait for them to finish
        mFinishPending = true;
    } else {
        aboutToFinish();
        q->emitResult();
    }
}

void JobPrivate::startQueued()
{
    Q_Q(Job);
    mStarted = true;

    Q_EMIT q->aboutToStart(q);
    q->doStart();
    QTimer::singleShot(0, q, [this]() {
        startNext();
    });
    QMetaObject::invokeMethod(q, "signalStartedToJobTracker", Qt::QueuedConnection);
}

void JobPrivate::lostConnection()
{
    Q_Q(Job);

    if (mCurrentSubJob) {
        mCurrentSubJob->d_ptr->lostConnection();
    } else {
        q->setError(Job::ConnectionFailed);
        q->emitResult();
    }
}

void JobPrivate::slotSubJobAboutToStart(Job *job)
{
    Q_ASSERT(mCurrentSubJob == nullptr);
    mCurrentSubJob = job;
}

void JobPrivate::startNext()
{
    Q_Q(Job);

    if (mStarted && !mCurrentSubJob && q->hasSubjobs()) {
        Job *job = qobject_cast<Akonadi::Job *>(q->subjobs().at(0));
        Q_ASSERT(job);
        job->d_ptr->startQueued();
    } else if (mFinishPending && !q->hasSubjobs()) {
        // The last subjob we've been waiting for has finished, emitResult() finally
        QTimer::singleShot(0, q, [this]() {
            delayedEmitResult();
        });
    }
}

qint64 JobPrivate::newTag()
{
    if (mParentJob) {
        mTag = mParentJob->d_ptr->newTag();
    } else {
        mTag = mSession->d->nextTag();
    }
    return mTag;
}

qint64 JobPrivate::tag() const
{
    return mTag;
}

void JobPrivate::sendCommand(qint64 tag, const Protocol::CommandPtr &cmd)
{
    if (mParentJob) {
        mParentJob->d_ptr->sendCommand(tag, cmd);
    } else {
        mSession->d->sendCommand(tag, cmd);
    }
}

void JobPrivate::sendCommand(const Protocol::CommandPtr &cmd)
{
    sendCommand(newTag(), cmd);
}

void JobPrivate::itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    mSession->d->itemRevisionChanged(itemId, oldRevision, newRevision);
}

void JobPrivate::updateItemRevision(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    Q_Q(Job);
    const auto &subjobs = q->subjobs();
    for (KJob *j : subjobs) {
        auto job = qobject_cast<Akonadi::Job *>(j);
        if (job) {
            job->d_ptr->updateItemRevision(itemId, oldRevision, newRevision);
        }
    }
    doUpdateItemRevision(itemId, oldRevision, newRevision);
}

void JobPrivate::doUpdateItemRevision(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    Q_UNUSED(itemId)
    Q_UNUSED(oldRevision)
    Q_UNUSED(newRevision)
}

int JobPrivate::protocolVersion() const
{
    return mSession->d->protocolVersion;
}
/// @endcond

Job::Job(QObject *parent)
    : KCompositeJob(parent)
    , d_ptr(new JobPrivate(this))
{
    d_ptr->init(parent);
}

Job::Job(JobPrivate *dd, QObject *parent)
    : KCompositeJob(parent)
    , d_ptr(dd)
{
    d_ptr->init(parent);
}

Job::~Job()
{
    // if there is a job tracer listening, tell it the job is done now
    if (s_jobtracker) {
        const QList<QVariant> argumentList = {QString::number(reinterpret_cast<quintptr>(this), 16), errorString()};
        s_jobtracker->callWithArgumentList(QDBus::NoBlock, QStringLiteral("jobEnded"), argumentList);
    }

    delete d_ptr;
}

void Job::start()
{
}

bool Job::doKill()
{
    Q_D(Job);
    if (d->mStarted) {
        // the only way to cancel an already started job is reconnecting to the server
        d->mSession->d->forceReconnect();
    }
    d->mStarted = false;
    return true;
}

QString Job::errorString() const
{
    QString str;
    switch (error()) {
    case NoError:
        break;
    case ConnectionFailed:
        str = i18n("Cannot connect to the Akonadi service.");
        break;
    case ProtocolVersionMismatch:
        str = i18n("The protocol version of the Akonadi server is incompatible. Make sure you have a compatible version installed.");
        break;
    case UserCanceled:
        str = i18n("User canceled operation.");
        break;
    case Unknown:
        return errorText();
    case UserError:
        str = i18n("Unknown error.");
        break;
    }
    if (!errorText().isEmpty()) {
        str += QStringLiteral(" (%1)").arg(errorText());
    }
    return str;
}

bool Job::addSubjob(KJob *job)
{
    bool rv = KCompositeJob::addSubjob(job);
    if (rv) {
        connect(qobject_cast<Job *>(job), &Job::aboutToStart, this, [this](Job *job) {
            d_ptr->slotSubJobAboutToStart(job);
        });
        QTimer::singleShot(0, this, [this]() {
            d_ptr->startNext();
        });
    }
    return rv;
}

bool Job::removeSubjob(KJob *job)
{
    bool rv = KCompositeJob::removeSubjob(job);
    if (job == d_ptr->mCurrentSubJob) {
        d_ptr->mCurrentSubJob = nullptr;
        QTimer::singleShot(0, this, [this]() {
            d_ptr->startNext();
        });
    }
    return rv;
}

bool Akonadi::Job::doHandleResponse(qint64 tag, const Akonadi::Protocol::CommandPtr &response)
{
    qCDebug(AKONADICORE_LOG) << this << "Unhandled response: " << tag << Protocol::debugString(response);
    setError(Unknown);
    setErrorText(i18n("Unexpected response"));
    emitResult();
    return true;
}

void Job::slotResult(KJob *job)
{
    if (d_ptr->mCurrentSubJob == job) {
        // current job finished, start the next one
        d_ptr->mCurrentSubJob = nullptr;
        KCompositeJob::slotResult(job);
        if (!job->error()) {
            QTimer::singleShot(0, this, [this]() {
                d_ptr->startNext();
            });
        }
    } else {
        // job that was still waiting for execution finished, probably canceled,
        // so just remove it from the queue and move on without caring about
        // its error code
        KCompositeJob::removeSubjob(job);
    }
}

void Job::emitWriteFinished()
{
    d_ptr->mWriteFinished = true;
    Q_EMIT writeFinished(this);
}

#include "moc_job.cpp"
