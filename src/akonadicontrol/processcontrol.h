/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QObject>
#include <QProcess>

#include <chrono>

namespace AkonadiControl
{
/**
 * This class starts and observes a process. Depending on the
 * policy it also restarts the process when it crashes.
 */
class ProcessControl : public QObject
{
    Q_OBJECT

public:
    /**
     * These enums describe the behaviour when the observed
     * application crashed.
     *
     * @li StopOnCrash    - The application won't be restarted.
     * @li RestartOnCrash - The application is restarted with the same arguments.
     */
    enum CrashPolicy {
        StopOnCrash,
        RestartOnCrash,
    };

    /**
     * Creates a new process control.
     *
     * @param parent The parent object.
     */
    explicit ProcessControl(QObject *parent = nullptr);

    /**
     * Destroys the process control.
     */
    ~ProcessControl() override;

    /**
     * Starts the @p application with the given list of @p arguments.
     */
    void start(const QString &application, const QStringList &arguments = QStringList(), CrashPolicy policy = RestartOnCrash);

    /**
     * Starts the process with the previously set application and arguments.
     */
    void start();

    /**
     * Stops the currently running application.
     */
    void stop();

    /**
     * Sets the crash policy.
     */
    void setCrashPolicy(CrashPolicy policy);

    /**
     * Restart the application the next time it exits normally.
     */
    void restartOnceWhenFinished()
    {
        mRestartOnceOnExit = true;
    }

    /**
     * Returns true if the process is currently running.
     */
    Q_REQUIRED_RESULT bool isRunning() const;

    /**
     * Sets the time (in msecs) we wait for the process to shut down before we send terminate/kill signals.
     * Default is 1 second.
     * Note that it is your responsiblility to ask the process to quit, otherwise this is just
     * pointless waiting.
     */
    void setShutdownTimeout(std::chrono::milliseconds timeout);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the observed application
     * writes something to stderr.
     *
     * @param errorMsg The error output of the observed application.
     */
    void processErrorMessages(const QString &errorMsg);

    /**
     * This signal is emitted when the server is restarted after a crash.
     */
    void restarted();

    /**
     * Emitted if the process could not be started since it terminated
     * too often.
     */
    void unableToStart();

private Q_SLOTS:
    void slotError(QProcess::ProcessError);
    void slotFinished(int, QProcess::ExitStatus);
    void resetCrashCount();

private:
    QProcess mProcess;
    QString mApplication;
    QStringList mArguments;
    CrashPolicy mPolicy = RestartOnCrash;
    bool mFailedToStart = false;
    int mCrashCount = 0;
    bool mRestartOnceOnExit = false;
    std::chrono::milliseconds mShutdownTimeout;
};

}
