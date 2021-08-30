/*
 * SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "testrunner.h"
#include "akonaditest_debug.h"

#include <KProcess>

TestRunner::TestRunner(const QStringList &args, QObject *parent)
    : QObject(parent)
    , mArguments(args)
    , mExitCode(0)
    , mProcess(nullptr)
{
}

int TestRunner::exitCode() const
{
    return mExitCode;
}

void TestRunner::run()
{
    qCDebug(AKONADITEST_LOG) << "Starting test" << mArguments;
    mProcess = new KProcess(this);
    mProcess->setProgram(mArguments);
    connect(mProcess, qOverload<int, QProcess::ExitStatus>(&KProcess::finished), this, &TestRunner::processFinished);
    connect(mProcess, &KProcess::errorOccurred, this, &TestRunner::processError);
    // environment setup seems to have been done by setuptest globally already
    mProcess->start();
    if (!mProcess->waitForStarted()) {
        qCWarning(AKONADITEST_LOG) << mArguments << "failed to start!";
        mExitCode = 255;
        Q_EMIT finished();
    }
}

void TestRunner::triggerTermination(int exitCode)
{
    processFinished(exitCode);
}

void TestRunner::processFinished(int exitCode)
{
    // Only update the exit code when it is 0. This prevents overwriting a non-zero
    // value with 0. This can happen when multiple processes finish or triggerTermination
    // is called after a process has finished.
    if (mExitCode == 0) {
        mExitCode = exitCode;
        qCInfo(AKONADITEST_LOG) << "Test finished with exist code" << exitCode;
    }
    Q_EMIT finished();
}

void TestRunner::processError(QProcess::ProcessError error)
{
    qCWarning(AKONADITEST_LOG) << mArguments << "exited with an error:" << error;
    mExitCode = 255;
    Q_EMIT finished();
}

void TestRunner::terminate()
{
    if (mProcess) {
        mProcess->terminate();
    }
}
