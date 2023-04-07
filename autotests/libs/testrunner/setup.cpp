/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 * SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "setup.h"
#include "akonaditest_debug.h"
#include "config.h" //krazy:exclude=includes

#include <agentinstance.h>
#include <agentinstancecreatejob.h>
#include <private/standarddirs_p.h>
#include <resourcesynchronizationjob.h>

#include <KConfig>
#include <KConfigGroup>
#include <KProcess>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

bool SetupTest::startAkonadiDaemon()
{
    Q_ASSERT(Akonadi::ServerManager::hasInstanceIdentifier());

    if (!mAkonadiDaemonProcess) {
        mAkonadiDaemonProcess = std::make_unique<KProcess>();
        connect(mAkonadiDaemonProcess.get(), &KProcess::finished, this, &SetupTest::slotAkonadiDaemonProcessFinished);
    }

    mAkonadiDaemonProcess->setProgram(Akonadi::StandardDirs::findExecutable(QStringLiteral("akonadi_control")), {QStringLiteral("--instance"), instanceId()});
    mAkonadiDaemonProcess->start();
    const bool started = mAkonadiDaemonProcess->waitForStarted(5000);
    qCInfo(AKONADITEST_LOG) << "Started akonadi daemon with pid:" << mAkonadiDaemonProcess->processId();
    return started;
}

void SetupTest::stopAkonadiDaemon()
{
    if (!mAkonadiDaemonProcess) {
        return;
    }
    disconnect(mAkonadiDaemonProcess.get(), &KProcess::finished, this, nullptr);
    mAkonadiDaemonProcess->terminate();
    const bool finished = mAkonadiDaemonProcess->waitForFinished(5000);
    if (!finished) {
        qCDebug(AKONADITEST_LOG) << "Problem finishing process.";
    }
    mAkonadiDaemonProcess.reset();
}

void SetupTest::setupAgents()
{
    if (mAgentsCreated) {
        return;
    }
    mAgentsCreated = true;
    Config *config = Config::instance();
    const auto agents = config->agents();
    for (const auto &[instance, sync] : agents) {
        qCDebug(AKONADITEST_LOG) << "Creating agent" << instance << "...";
        ++mSetupJobCount;
        auto job = new Akonadi::AgentInstanceCreateJob(instance, this);
        job->setProperty("sync", sync);
        connect(job, &Akonadi::AgentInstanceCreateJob::result, this, &SetupTest::agentCreationResult);
        job->start();
    }

    checkSetupDone();
}

void SetupTest::agentCreationResult(KJob *job)
{
    qCDebug(AKONADITEST_LOG) << "Agent created";
    --mSetupJobCount;
    if (job->error()) {
        qCritical() << "Failed to create agent:" << job->errorString();
        setupFailed();
    } else {
        const bool needsSync = job->property("sync").toBool();
        const auto instance = qobject_cast<Akonadi::AgentInstanceCreateJob *>(job)->instance();
        qCDebug(AKONADITEST_LOG) << "Agent" << instance.identifier() << "created";
        if (needsSync) {
            ++mSetupJobCount;
            qCDebug(AKONADITEST_LOG) << "Scheduling Agent sync of" << instance.identifier();
            auto sync = new Akonadi::ResourceSynchronizationJob(instance, this);
            connect(sync, &Akonadi::ResourceSynchronizationJob::result, this, &SetupTest::synchronizationResult);
            sync->start();
        }
    }

    checkSetupDone();
}

void SetupTest::synchronizationResult(KJob *job)
{
    auto instance = qobject_cast<Akonadi::ResourceSynchronizationJob *>(job)->resource();
    qCDebug(AKONADITEST_LOG) << "Sync of" << instance.identifier() << "done";

    --mSetupJobCount;
    if (job->error()) {
        qCritical() << job->errorString();
        setupFailed();
    }

    checkSetupDone();
}

void SetupTest::serverStateChanged(Akonadi::ServerManager::State state)
{
    if (state == Akonadi::ServerManager::Running) {
        setupAgents();
    } else if (mShuttingDown && state == Akonadi::ServerManager::NotRunning) {
        shutdownHarder();
    }
}

void SetupTest::copyXdgDirectory(const QString &src, const QString &dst)
{
    qCDebug(AKONADITEST_LOG) << "Copying" << src << "to" << dst;
    const QDir srcDir(src);
    const auto entries = srcDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (const auto &fi : entries) {
        if (fi.isDir()) {
            if (fi.fileName() == QLatin1String("akonadi")) {
                // namespace according to instance identifier
#ifdef Q_OS_WIN
                const bool isXdgConfig = src.contains(QLatin1String("/xdgconfig/"));
                copyDirectory(fi.absoluteFilePath(),
                              dst + QStringLiteral("/akonadi/") + (isXdgConfig ? QStringLiteral("config/") : QStringLiteral("data/"))
                                  + QStringLiteral("instance/") + instanceId());
#else
                copyDirectory(fi.absoluteFilePath(), dst + QStringLiteral("/akonadi/instance/") + instanceId());
#endif
            } else {
                copyDirectory(fi.absoluteFilePath(), dst + QLatin1Char('/') + fi.fileName());
            }
        } else {
            if (fi.fileName().startsWith(QLatin1String("akonadi_")) && fi.fileName().endsWith(QLatin1String("rc"))) {
                // namespace according to instance identifier
                const QString baseName = fi.fileName().left(fi.fileName().size() - 2);
                const QString dstPath = dst + QLatin1Char('/') + Akonadi::ServerManager::addNamespace(baseName) + QStringLiteral("rc");
                if (!QFile::copy(fi.absoluteFilePath(), dstPath)) {
                    qCWarning(AKONADITEST_LOG) << "Failed to copy" << fi.absoluteFilePath() << "to" << dstPath;
                }
            } else {
                const QString dstPath = dst + QLatin1Char('/') + fi.fileName();
                if (!QFile::copy(fi.absoluteFilePath(), dstPath)) {
                    qCWarning(AKONADITEST_LOG) << "Failed to copy" << fi.absoluteFilePath() << "to" << dstPath;
                }
            }
        }
    }
}

void SetupTest::copyDirectory(const QString &src, const QString &dst)
{
    const QDir srcDir(src);
    QDir::root().mkpath(dst);
    const auto entries = srcDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (const auto &fi : entries) {
        const QString dstPath = dst + QLatin1Char('/') + fi.fileName();
        if (fi.isDir()) {
            copyDirectory(fi.absoluteFilePath(), dstPath);
        } else {
            if (!QFile::copy(fi.absoluteFilePath(), dstPath)) {
                qCWarning(AKONADITEST_LOG) << "Failed to copy" << fi.absoluteFilePath() << "to" << dstPath;
            }
        }
    }
}

void SetupTest::createTempEnvironment()
{
    qCDebug(AKONADITEST_LOG) << "Creating test environment in" << basePath();

    const Config *config = Config::instance();
#ifdef Q_OS_WIN
    // Always copy the generic xdgconfig dir
    copyXdgDirectory(config->basePath() + QStringLiteral("/xdgconfig"), basePath());
    if (!config->xdgConfigHome().isEmpty()) {
        copyXdgDirectory(config->xdgConfigHome(), basePath());
    }
    copyXdgDirectory(config->xdgDataHome(), basePath());
    setEnvironmentVariable("XDG_DATA_HOME", basePath());
    setEnvironmentVariable("XDG_CONFIG_HOME", basePath());
    writeAkonadiserverrc(basePath() + QStringLiteral("/akonadi/config/instance/%1/akonadiserverrc").arg(instanceId()));
#else
    const QDir tmpDir(basePath());
    const QString testRunnerDataDir = QStringLiteral("data");
    const QString testRunnerConfigDir = QStringLiteral("config");
    const QString testRunnerTmpDir = QStringLiteral("tmp");

    tmpDir.mkpath(testRunnerConfigDir);
    tmpDir.mkpath(testRunnerDataDir);
    tmpDir.mkpath(testRunnerTmpDir);

    // Always copy the generic xdgconfig dir
    copyXdgDirectory(config->basePath() + QStringLiteral("/xdgconfig"), basePath() + testRunnerConfigDir);
    if (!config->xdgConfigHome().isEmpty()) {
        copyXdgDirectory(config->xdgConfigHome(), basePath() + testRunnerConfigDir);
    }
    copyXdgDirectory(config->xdgDataHome(), basePath() + testRunnerDataDir);

    setEnvironmentVariable("XDG_DATA_HOME", basePath() + testRunnerDataDir);
    setEnvironmentVariable("XDG_CONFIG_HOME", basePath() + testRunnerConfigDir);
    setEnvironmentVariable("TMPDIR", basePath() + testRunnerTmpDir);
    writeAkonadiserverrc(basePath() + testRunnerConfigDir + QStringLiteral("/akonadi/instance/%1/akonadiserverrc").arg(instanceId()));
#endif

    QString backend;
    if (Config::instance()->dbBackend() == QLatin1String("pgsql")) {
        backend = QStringLiteral("postgresql");
    } else {
        backend = Config::instance()->dbBackend();
    }
    setEnvironmentVariable("TESTRUNNER_DB_ENVIRONMENT", backend);
}

void SetupTest::writeAkonadiserverrc(const QString &path)
{
    QString backend;
    if (Config::instance()->dbBackend() == QLatin1String("sqlite")) {
        backend = QStringLiteral("QSQLITE");
    } else if (Config::instance()->dbBackend() == QLatin1String("mysql")) {
        backend = QStringLiteral("QMYSQL");
    } else if (Config::instance()->dbBackend() == QLatin1String("pgsql")) {
        backend = QStringLiteral("QPSQL");
    } else {
        qCCritical(AKONADITEST_LOG, "Invalid backend name %s", qPrintable(backend));
        return;
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("General"));
    settings.setValue(QStringLiteral("Driver"), backend);
    settings.endGroup();
    settings.beginGroup(QStringLiteral("Search"));
    settings.setValue(QStringLiteral("Manager"), QStringLiteral("Dummy"));
    settings.endGroup();
    settings.beginGroup(QStringLiteral("Debug"));
    settings.setValue(QStringLiteral("Tracer"), QStringLiteral("null"));
    settings.endGroup();
    qCDebug(AKONADITEST_LOG) << "Written akonadiserverrc to" << settings.fileName();
}

void SetupTest::cleanTempEnvironment() const
{
#ifdef Q_OS_WIN
    QDir(basePath() + QStringLiteral("akonadi/config/instance/") + instanceId()).removeRecursively();
    QDir(basePath() + QStringLiteral("akonadi/data/instance/") + instanceId()).removeRecursively();
#else
    QDir(basePath()).removeRecursively();
#endif
}

SetupTest::SetupTest()
    : mAkonadiDaemonProcess(nullptr)
    , mShuttingDown(false)
    , mAgentsCreated(false)
    , mTrackAkonadiProcess(true)
    , mSetupJobCount(0)
    , mExitCode(0)
{
    setupInstanceId();
    cleanTempEnvironment();
    createTempEnvironment();

    // switch off agent auto-starting by default, can be re-enabled if really needed inside the config.xml
    setEnvironmentVariable("AKONADI_DISABLE_AGENT_AUTOSTART", QStringLiteral("true"));
    setEnvironmentVariable("AKONADI_TESTRUNNER_PID", QString::number(QCoreApplication::instance()->applicationPid()));
    // enable all debugging, so we get some useful information when test fails
    setEnvironmentVariable("QT_LOGGING_RULES",
                           QStringLiteral("* = true\n"
                                          "qt.* = false\n"
                                          "kf5.coreaddons.desktopparser.debug = false"));

    setEnvironmentVariable("KIO_DISABLE_CACHE_CLEANER", QStringLiteral("yes"));

    QHashIterator<QString, QString> iter(Config::instance()->envVars());
    while (iter.hasNext()) {
        iter.next();
        qCDebug(AKONADITEST_LOG) << "Setting environment variable" << iter.key() << "=" << iter.value();
        setEnvironmentVariable(iter.key().toLocal8Bit(), iter.value());
    }

    // No kres-migrator please
    KConfig migratorConfig(basePath() + QStringLiteral("config/kres-migratorrc"));
    KConfigGroup migrationCfg(&migratorConfig, "Migration");
    migrationCfg.writeEntry("Enabled", false);

    connect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged, this, &SetupTest::serverStateChanged);

    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Akonadi.Testrunner-")
                                                  + QString::number(QCoreApplication::instance()->applicationPid()));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), this, QDBusConnection::ExportScriptableSlots);
}

SetupTest::~SetupTest()
{
    cleanTempEnvironment();
}

void SetupTest::shutdown()
{
    if (mShuttingDown) {
        return;
    }
    mShuttingDown = true;

    switch (Akonadi::ServerManager::self()->state()) {
    case Akonadi::ServerManager::Running:
    case Akonadi::ServerManager::Starting:
    case Akonadi::ServerManager::Upgrading:
        qCInfo(AKONADITEST_LOG) << "Shutting down Akonadi control...";
        Akonadi::ServerManager::self()->stop();
        // safety timeout
        QTimer::singleShot(30s, this, &SetupTest::shutdownHarder);
        break;
    case Akonadi::ServerManager::NotRunning:
    case Akonadi::ServerManager::Broken:
        shutdownHarder();
        break;
    case Akonadi::ServerManager::Stopping:
        // safety timeout
        QTimer::singleShot(30s, this, &SetupTest::shutdownHarder);
        break;
    }
}

void SetupTest::shutdownHarder()
{
    qCDebug(AKONADITEST_LOG) << "Forcing akonaditest shutdown";
    mShuttingDown = false;
    stopAkonadiDaemon();
    QCoreApplication::instance()->exit(mExitCode);
}

void SetupTest::restartAkonadiServer()
{
    qCDebug(AKONADITEST_LOG) << "Restarting Akonadi";
    disconnect(mAkonadiDaemonProcess.get(), &KProcess::finished, this, nullptr);
    Akonadi::ServerManager::self()->stop();
    const bool shutdownResult = mAkonadiDaemonProcess->waitForFinished();
    if (!shutdownResult) {
        qCWarning(AKONADITEST_LOG) << "Akonadi control did not shut down in time, killing it.";
        mAkonadiDaemonProcess->kill();
    }
    // we don't use Control::start() since we want to be able to kill
    // it forcefully, if necessary, and know the pid
    startAkonadiDaemon();
    // from here on, the server exiting is an error again
    connect(mAkonadiDaemonProcess.get(), &KProcess::finished, this, &SetupTest::slotAkonadiDaemonProcessFinished);
}

QString SetupTest::basePath() const
{
#ifdef Q_OS_WIN
    // On Windows we are forced to share the same data directory as production instances
    // because there's no way to override QStandardPaths like we can on Unix.
    // This means that on Windows we rely on Instances providing us the necessary isolation
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
#else
    QString sysTempDirPath = QDir::tempPath();
#ifdef Q_OS_UNIX
    // QDir::tempPath() makes sure to use the fully sym-link exploded
    // absolute path to the temp dir. That is nice, but on OSX it makes
    // that path really long. MySQL chokes on this, for it's socket path,
    // so work around that
    sysTempDirPath = QStringLiteral("/tmp");
#endif

    const QDir sysTempDir(sysTempDirPath);
    const QString tempDir = QStringLiteral("/aktestrunner-%1/").arg(QCoreApplication::instance()->applicationPid());
    if (!sysTempDir.exists(tempDir)) {
        sysTempDir.mkdir(tempDir);
    }
    return sysTempDirPath + tempDir;
#endif
}

void SetupTest::slotAkonadiDaemonProcessFinished(int exitCode)
{
    if (mTrackAkonadiProcess || exitCode != EXIT_SUCCESS) {
        qCWarning(AKONADITEST_LOG) << "Akonadi server process was terminated externally!";
        Q_EMIT serverExited(exitCode);
    }
    mAkonadiDaemonProcess.reset();
}

void SetupTest::trackAkonadiProcess(bool track)
{
    mTrackAkonadiProcess = track;
}

QString SetupTest::instanceId() const
{
    return QStringLiteral("testrunner-") + QString::number(QCoreApplication::instance()->applicationPid());
}

void SetupTest::setupInstanceId()
{
    setEnvironmentVariable("AKONADI_INSTANCE", instanceId());
}

void SetupTest::checkSetupDone()
{
    qCDebug(AKONADITEST_LOG) << "checkSetupDone: pendingJobs =" << mSetupJobCount << ", exitCode =" << mExitCode;
    if (mSetupJobCount == 0) {
        if (mExitCode != 0) {
            qCInfo(AKONADITEST_LOG) << "Setup has failed, aborting test.";
            shutdown();
        } else {
            qCInfo(AKONADITEST_LOG) << "Setup successful";
            Q_EMIT setupDone();
        }
    }
}

void SetupTest::setupFailed()
{
    mExitCode = 1;
}

void SetupTest::setEnvironmentVariable(const QByteArray &name, const QString &value)
{
    mEnvVars.push_back(qMakePair(name, value.toLocal8Bit()));
    qputenv(name.constData(), value.toLatin1());
}

QVector<SetupTest::EnvVar> SetupTest::environmentVariables() const
{
    return mEnvVars;
}
