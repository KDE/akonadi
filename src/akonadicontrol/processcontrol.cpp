/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "processcontrol.h"
#include "akonadicontrol_debug.h"

#include <shared/akapplication.h>

#include <private/instance_p.h>
#include <private/standarddirs_p.h>

#include <QTimer>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#endif

using namespace Akonadi;
using namespace std::chrono_literals;

static const int s_maxCrashCount = 2;

ProcessControl::ProcessControl(QObject *parent)
    : QObject(parent)
    , mShutdownTimeout(1s)
{
    connect(&mProcess, &QProcess::errorOccurred, this, &ProcessControl::slotError);
    connect(&mProcess, &QProcess::finished, this, &ProcessControl::slotFinished);
    mProcess.setProcessChannelMode(QProcess::ForwardedChannels);

    if (Akonadi::Instance::hasIdentifier()) {
        QProcessEnvironment env = mProcess.processEnvironment();
        if (env.isEmpty()) {
            env = QProcessEnvironment::systemEnvironment();
        }
        env.insert(QStringLiteral("AKONADI_INSTANCE"), Akonadi::Instance::identifier());
        mProcess.setProcessEnvironment(env);
    }
}

ProcessControl::~ProcessControl()
{
    stop();
}

void ProcessControl::start(const QString &application, const QStringList &arguments, CrashPolicy policy)
{
    mFailedToStart = false;

    mApplication = application;
    mArguments = arguments;
    mPolicy = policy;

    start();
}

void ProcessControl::setCrashPolicy(CrashPolicy policy)
{
    mPolicy = policy;
}

void ProcessControl::stop()
{
    if (mProcess.state() != QProcess::NotRunning) {
        mProcess.waitForFinished(mShutdownTimeout.count());
        mProcess.terminate();
        mProcess.waitForFinished(std::chrono::milliseconds{10000}.count());
        mProcess.kill();
    }
}

void ProcessControl::slotError(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::Crashed:
        mCrashCount++;
        // do nothing, we'll respawn in slotFinished
        break;
    case QProcess::FailedToStart:
    default:
        mFailedToStart = true;
        break;
    }

    qCWarning(AKONADICONTROL_LOG) << "ProcessControl: Application" << mApplication << "stopped unexpectedly (" << mProcess.errorString() << ")";
}

void ProcessControl::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        if (mPolicy == RestartOnCrash) {
            // don't try to start an unstartable application
            if (!mFailedToStart && mCrashCount <= s_maxCrashCount) {
                qCWarning(AKONADICONTROL_LOG, "Application '%s' crashed! %d restarts left.", qPrintable(mApplication), s_maxCrashCount - mCrashCount);
                start();
                Q_EMIT restarted();
            } else {
                if (mFailedToStart) {
                    qCCritical(AKONADICONTROL_LOG, "Application '%s' failed to start!", qPrintable(mApplication));
                } else {
                    qCCritical(AKONADICONTROL_LOG, "Application '%s' crashed too often. Giving up!", qPrintable(mApplication));
                }
                mPolicy = StopOnCrash;
                Q_EMIT unableToStart();
                return;
            }
        } else {
            qCCritical(AKONADICONTROL_LOG, "Application '%s' crashed. No restart!", qPrintable(mApplication));
        }
    } else {
        if (exitCode != 0) {
            qCWarning(AKONADICONTROL_LOG,
                      "ProcessControl: Application '%s' returned with exit code %d (%s)",
                      qPrintable(mApplication),
                      exitCode,
                      qPrintable(mProcess.errorString()));
            if (mPolicy == RestartOnCrash) {
                if (mCrashCount > s_maxCrashCount) {
                    qCCritical(AKONADICONTROL_LOG) << mApplication << "crashed too often and will not be restarted!";
                    mPolicy = StopOnCrash;
                    Q_EMIT unableToStart();
                    return;
                }
                ++mCrashCount;
                QTimer::singleShot(std::chrono::seconds{60}, this, &ProcessControl::resetCrashCount);
                if (!mFailedToStart) { // don't try to start an unstartable application
                    start();
                    Q_EMIT restarted();
                }
            }
        } else {
            if (mRestartOnceOnExit) {
                mRestartOnceOnExit = false;
                qCInfo(AKONADICONTROL_LOG, "Restarting application '%s'.", qPrintable(mApplication));
                start();
            } else {
                qCInfo(AKONADICONTROL_LOG, "Application '%s' exited normally...", qPrintable(mApplication));
                Q_EMIT unableToStart();
            }
        }
    }
}

static bool listContains(const QStringList &list, const QString &pattern)
{
    for (const QString &s : list) {
        if (s.contains(pattern)) {
            return true;
        }
    }
    return false;
}

void ProcessControl::start()
{
    // Prefer akonadiserver from the builddir
    mApplication = StandardDirs::findExecutable(mApplication);

#ifdef Q_OS_UNIX
    QString agentValgrind = akGetEnv("AKONADI_VALGRIND");
    if (!agentValgrind.isEmpty() && (mApplication.contains(agentValgrind) || listContains(mArguments, agentValgrind))) {
        mArguments.prepend(mApplication);
        const QString originalArguments = mArguments.join(QString::fromLocal8Bit(" "));
        mApplication = QString::fromLocal8Bit("valgrind");

        const QString valgrindSkin = akGetEnv("AKONADI_VALGRIND_SKIN", QString::fromLocal8Bit("memcheck"));
        mArguments.prepend(QLatin1StringView("--tool=") + valgrindSkin);

        const QString valgrindOptions = akGetEnv("AKONADI_VALGRIND_OPTIONS");
        if (!valgrindOptions.isEmpty()) {
            mArguments = valgrindOptions.split(QLatin1Char(' '), Qt::SkipEmptyParts) << mArguments;
        }

        qCDebug(AKONADICONTROL_LOG);
        qCDebug(AKONADICONTROL_LOG) << "============================================================";
        qCDebug(AKONADICONTROL_LOG) << "ProcessControl: Valgrinding process" << originalArguments;
        if (!valgrindSkin.isEmpty()) {
            qCDebug(AKONADICONTROL_LOG) << "ProcessControl: Valgrind skin:" << valgrindSkin;
        }
        if (!valgrindOptions.isEmpty()) {
            qCDebug(AKONADICONTROL_LOG) << "ProcessControl: Additional Valgrind options:" << valgrindOptions;
        }
        qCDebug(AKONADICONTROL_LOG) << "============================================================";
        qCDebug(AKONADICONTROL_LOG);
    }

    const QString agentHeaptrack = akGetEnv("AKONADI_HEAPTRACK");
    if (!agentHeaptrack.isEmpty() && (mApplication.contains(agentHeaptrack) || listContains(mArguments, agentHeaptrack))) {
        mArguments.prepend(mApplication);
        const QString originalArguments = mArguments.join(QLatin1Char(' '));
        mApplication = QStringLiteral("heaptrack");

        qCDebug(AKONADICONTROL_LOG);
        qCDebug(AKONADICONTROL_LOG) << "============================================================";
        qCDebug(AKONADICONTROL_LOG) << "ProcessControl: Heaptracking process" << originalArguments;
        qCDebug(AKONADICONTROL_LOG) << "============================================================";
        qCDebug(AKONADICONTROL_LOG);
    }

    const QString agentPerf = akGetEnv("AKONADI_PERF");
    if (!agentPerf.isEmpty() && (mApplication.contains(agentPerf) || listContains(mArguments, agentPerf))) {
        mArguments.prepend(mApplication);
        const QString originalArguments = mArguments.join(QLatin1Char(' '));
        mApplication = QStringLiteral("perf");

        mArguments = QStringList{QStringLiteral("record"), QStringLiteral("--call-graph"), QStringLiteral("dwarf"), QStringLiteral("--")} + mArguments;

        qCDebug(AKONADICONTROL_LOG);
        qCDebug(AKONADICONTROL_LOG) << "============================================================";
        qCDebug(AKONADICONTROL_LOG) << "ProcessControl: Perf-recording process" << originalArguments;
        qCDebug(AKONADICONTROL_LOG) << "============================================================";
        qCDebug(AKONADICONTROL_LOG);
    }
#endif

    mProcess.start(mApplication, mArguments);
    if (!mProcess.waitForStarted()) {
        qCWarning(AKONADICONTROL_LOG, "ProcessControl: Unable to start application '%s' (%s)", qPrintable(mApplication), qPrintable(mProcess.errorString()));
        Q_EMIT unableToStart();
        return;
    } else {
        QString agentDebug = QString::fromLocal8Bit(qgetenv("AKONADI_DEBUG_WAIT"));
        auto pid = mProcess.processId();
        if (!agentDebug.isEmpty() && mApplication.contains(agentDebug)) {
            qCDebug(AKONADICONTROL_LOG);
            qCDebug(AKONADICONTROL_LOG) << "============================================================";
            qCDebug(AKONADICONTROL_LOG) << "ProcessControl: Suspending process" << mApplication;
#ifdef Q_OS_UNIX
            qCDebug(AKONADICONTROL_LOG) << "'gdb --pid" << pid << "' to debug";
            qCDebug(AKONADICONTROL_LOG) << "'kill -SIGCONT" << pid << "' to continue";
            kill(pid, SIGSTOP);
#endif
#ifdef Q_OS_WIN
            qCDebug(AKONADICONTROL_LOG) << "PID:" << pid;
            qCDebug(AKONADICONTROL_LOG) << "Process is waiting for a debugger...";
            // the agent process will wait for a debugger to be attached in AgentBase::debugAgent()
#endif
            qCDebug(AKONADICONTROL_LOG) << "============================================================";
            qCDebug(AKONADICONTROL_LOG);
        }
    }
}

void ProcessControl::resetCrashCount()
{
    mCrashCount = 0;
}

bool ProcessControl::isRunning() const
{
    return mProcess.state() != QProcess::NotRunning;
}

void ProcessControl::setShutdownTimeout(std::chrono::milliseconds timeout)
{
    mShutdownTimeout = timeout;
}

#include "moc_processcontrol.cpp"
