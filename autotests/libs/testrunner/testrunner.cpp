/*
 * Copyright (c) 2009  Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
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
    connect(mProcess, QOverload<int>::of(&KProcess::finished), this, &TestRunner::processFinished);
    connect(mProcess, &KProcess::errorOccurred,
            this, &TestRunner::processError);
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
