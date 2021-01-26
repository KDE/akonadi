/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SETUP_H
#define SETUP_H

#include <servermanager.h>

#include <QObject>
#include <QPair>
#include <QVector>

#include <memory>

class KProcess;
class KJob;

class SetupTest : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Testrunner")

public:
    SetupTest();
    ~SetupTest();

    /**
      Sets the instance identifier for the Akonadi session.
      Call this before using any other Akonadi API!
    */
    void setupInstanceId();
    bool startAkonadiDaemon();
    void stopAkonadiDaemon();
    QString basePath() const;

    /// Identifier used for the Akonadi session
    QString instanceId() const;

    /// set an environment variable
    void setEnvironmentVariable(const QByteArray &name, const QString &value);

    /// retrieve all modified environment variables, for writing the shell script
    typedef QPair<QByteArray, QByteArray> EnvVar;
    QVector<EnvVar> environmentVariables() const;

public Q_SLOTS:
    Q_SCRIPTABLE void shutdown();
    Q_SCRIPTABLE void shutdownHarder();
    /** Synchronously restarts the server. */
    Q_SCRIPTABLE void restartAkonadiServer();
    Q_SCRIPTABLE void trackAkonadiProcess(bool track);

Q_SIGNALS:
    void setupDone();
    void serverExited(int exitCode);

private Q_SLOTS:
    void serverStateChanged(Akonadi::ServerManager::State state);
    void slotAkonadiDaemonProcessFinished(int exitCode);
    void agentCreationResult(KJob *job);
    void synchronizationResult(KJob *job);

private:
    void setupAgents();
    void copyXdgDirectory(const QString &src, const QString &dst);
    void copyDirectory(const QString &src, const QString &dst);
    void createTempEnvironment();
    void cleanTempEnvironment() const;
    void setupFailed();
    void writeAkonadiserverrc(const QString &path);
    void checkSetupDone();

private:
    std::unique_ptr<KProcess> mAkonadiDaemonProcess;
    bool mShuttingDown;
    bool mAgentsCreated;
    bool mTrackAkonadiProcess;
    int mSetupJobCount;
    int mExitCode;
    QVector<EnvVar> mEnvVars;
};

#endif
