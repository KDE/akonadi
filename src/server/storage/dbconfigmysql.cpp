/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbconfigmysql.h"
#include "akonadiserver_debug.h"
#include "utils.h"

#include <private/standarddirs_p.h>

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>

using namespace Akonadi;
using namespace Akonadi::Server;

#define MYSQL_MIN_MAJOR 5
#define MYSQL_MIN_MINOR 1

#define MYSQL_VERSION_CHECK(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

static const QString s_mysqlSocketFileName = QStringLiteral("mysql.socket");
static const QString s_initConnection = QStringLiteral("initConnectionMysql");

DbConfigMysql::DbConfigMysql(const QString &configFile)
    : DbConfig(configFile)
{
}

DbConfigMysql::~DbConfigMysql() = default;

QString DbConfigMysql::driverName() const
{
    return QStringLiteral("QMYSQL");
}

QString DbConfigMysql::databaseName() const
{
    return mDatabaseName;
}

static QString findExecutable(const QString &bin)
{
    static const QStringList mysqldSearchPath = {
#ifdef MYSQLD_SCRIPTS_PATH
        QStringLiteral(MYSQLD_SCRIPTS_PATH),
#endif
        QStringLiteral("/usr/bin"),
        QStringLiteral("/usr/sbin"),
        QStringLiteral("/usr/local/sbin"),
        QStringLiteral("/usr/local/libexec"),
        QStringLiteral("/usr/libexec"),
        QStringLiteral("/opt/mysql/libexec"),
        QStringLiteral("/opt/local/lib/mysql5/bin"),
        QStringLiteral("/opt/mysql/sbin"),
    };
    QString path = QStandardPaths::findExecutable(bin);
    if (path.isEmpty()) { // No results in PATH; fall back to hardcoded list.
        path = QStandardPaths::findExecutable(bin, mysqldSearchPath);
    }
    return path;
}

bool DbConfigMysql::init(QSettings &settings, bool storeSettings)
{
    // determine default settings depending on the driver
    QString defaultHostName;
    QString defaultOptions;
    QString defaultServerPath;
    QString defaultCleanShutdownCommand;

#ifndef Q_OS_WIN
    const QString socketDirectory = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")), s_mysqlSocketFileName.length());
#endif

    const bool defaultInternalServer = true;
#ifdef MYSQLD_EXECUTABLE
    if (QFile::exists(QStringLiteral(MYSQLD_EXECUTABLE))) {
        defaultServerPath = QStringLiteral(MYSQLD_EXECUTABLE);
    }
#endif
    if (defaultServerPath.isEmpty()) {
        defaultServerPath = findExecutable(QStringLiteral("mysqld"));
    }

    const QString mysqladminPath = findExecutable(QStringLiteral("mysqladmin"));
    if (!mysqladminPath.isEmpty()) {
#ifndef Q_OS_WIN
        defaultCleanShutdownCommand = QStringLiteral("%1 --defaults-file=%2/mysql.conf --socket=%3/%4 shutdown")
                                          .arg(mysqladminPath, StandardDirs::saveDir("data"), socketDirectory, s_mysqlSocketFileName);
#else
        defaultCleanShutdownCommand = QString::fromLatin1("%1 shutdown --shared-memory").arg(mysqladminPath);
#endif
    }

    mMysqlInstallDbPath = findExecutable(QStringLiteral("mysql_install_db"));
    qCDebug(AKONADISERVER_LOG) << "Found mysql_install_db: " << mMysqlInstallDbPath;

    mMysqlCheckPath = findExecutable(QStringLiteral("mysqlcheck"));
    qCDebug(AKONADISERVER_LOG) << "Found mysqlcheck: " << mMysqlCheckPath;

    mMysqlUpgradePath = findExecutable(QStringLiteral("mysql_upgrade"));
    qCDebug(AKONADISERVER_LOG) << "Found mysql_upgrade: " << mMysqlUpgradePath;

    mInternalServer = settings.value(QStringLiteral("QMYSQL/StartServer"), defaultInternalServer).toBool();
#ifndef Q_OS_WIN
    if (mInternalServer) {
        defaultOptions = QStringLiteral("UNIX_SOCKET=%1/%2").arg(socketDirectory, s_mysqlSocketFileName);
    }
#endif

    // read settings for current driver
    settings.beginGroup(driverName());
    mDatabaseName = settings.value(QStringLiteral("Name"), defaultDatabaseName()).toString();
    mHostName = settings.value(QStringLiteral("Host"), defaultHostName).toString();
    mUserName = settings.value(QStringLiteral("User")).toString();
    mPassword = settings.value(QStringLiteral("Password")).toString();
    mConnectionOptions = settings.value(QStringLiteral("Options"), defaultOptions).toString();
    mMysqldPath = settings.value(QStringLiteral("ServerPath"), defaultServerPath).toString();
    mCleanServerShutdownCommand = settings.value(QStringLiteral("CleanServerShutdownCommand"), defaultCleanShutdownCommand).toString();
    settings.endGroup();

    // verify settings and apply permanent changes (written out below)
    if (mInternalServer) {
        mConnectionOptions = defaultOptions;
        // intentionally not namespaced as we are the only one in this db instance when using internal mode
        mDatabaseName = QStringLiteral("akonadi");
    }
    if (mInternalServer && (mMysqldPath.isEmpty() || !QFile::exists(mMysqldPath))) {
        mMysqldPath = defaultServerPath;
    }

    qCDebug(AKONADISERVER_LOG) << "Using mysqld:" << mMysqldPath;

    if (storeSettings) {
        // store back the default values
        settings.beginGroup(driverName());
        settings.setValue(QStringLiteral("Name"), mDatabaseName);
        settings.setValue(QStringLiteral("Host"), mHostName);
        settings.setValue(QStringLiteral("Options"), mConnectionOptions);
        if (!mMysqldPath.isEmpty()) {
            settings.setValue(QStringLiteral("ServerPath"), mMysqldPath);
        }
        settings.setValue(QStringLiteral("StartServer"), mInternalServer);
        settings.endGroup();
        settings.sync();
    }

    // apply temporary changes to the settings
    if (mInternalServer) {
        mHostName.clear();
        mUserName.clear();
        mPassword.clear();
    }

    return true;
}

bool DbConfigMysql::isAvailable(QSettings &settings)
{
    if (!QSqlDatabase::drivers().contains(driverName())) {
        return false;
    }

    if (!init(settings, false)) {
        return false;
    }

    if (mInternalServer && (mMysqldPath.isEmpty() || !QFile::exists(mMysqldPath))) {
        return false;
    }

    return true;
}

void DbConfigMysql::apply(QSqlDatabase &database)
{
    if (!mDatabaseName.isEmpty()) {
        database.setDatabaseName(mDatabaseName);
    }
    if (!mHostName.isEmpty()) {
        database.setHostName(mHostName);
    }
    if (!mUserName.isEmpty()) {
        database.setUserName(mUserName);
    }
    if (!mPassword.isEmpty()) {
        database.setPassword(mPassword);
    }

    database.setConnectOptions(mConnectionOptions);

    // can we check that during init() already?
    Q_ASSERT(database.driver()->hasFeature(QSqlDriver::LastInsertId));
}

bool DbConfigMysql::useInternalServer() const
{
    return mInternalServer;
}

bool DbConfigMysql::startInternalServer()
{
    bool success = true;

    const QString akDir = StandardDirs::saveDir("data");
    const QString dataDir = StandardDirs::saveDir("data", QStringLiteral("db_data"));
#ifndef Q_OS_WIN
    const QString socketDirectory = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")), s_mysqlSocketFileName.length());
    const QString socketFile = QStringLiteral("%1/%2").arg(socketDirectory, s_mysqlSocketFileName);
    const QString pidFileName = QStringLiteral("%1/mysql.pid").arg(socketDirectory);
#endif

    // generate config file
    const QString globalConfig = StandardDirs::locateResourceFile("config", QStringLiteral("mysql-global.conf"));
    const QString localConfig = StandardDirs::locateResourceFile("config", QStringLiteral("mysql-local.conf"));
    const QString actualConfig = StandardDirs::saveDir("data") + QLatin1String("/mysql.conf");
    qCDebug(AKONADISERVER_LOG) << " globalConfig : " << globalConfig << " localConfig : " << localConfig << " actualConfig : " << actualConfig;
    if (globalConfig.isEmpty()) {
        qCCritical(AKONADISERVER_LOG) << "Did not find MySQL server default configuration (mysql-global.conf)";
        return false;
    }

#ifdef Q_OS_LINUX
    // It is recommended to disable CoW feature when running on Btrfs to improve
    // database performance. Disabling CoW only has effect on empty directory (since
    // it affects only new files), so we check whether MySQL has not yet been initialized.
    QDir dir(dataDir + QDir::separator() + QLatin1String("mysql"));
    if (!dir.exists()) {
        if (Utils::getDirectoryFileSystem(dataDir) == QLatin1String("btrfs")) {
            Utils::disableCoW(dataDir);
        }
    }
#endif

    if (mMysqldPath.isEmpty()) {
        qCCritical(AKONADISERVER_LOG) << "mysqld not found. Please verify your installation";
        return false;
    }

    // Get the version of the mysqld server that we'll be using.
    // MySQL (but not MariaDB) deprecates and removes command line options in
    // patch version releases, so we need to adjust the command line options accordingly
    // when running the helper utilities or starting the server
    const unsigned int localVersion = parseCommandLineToolsVersion();
    if (localVersion == 0x000000) {
        qCCritical(AKONADISERVER_LOG) << "Failed to detect mysqld version!";
    }
    // TODO: Parse "MariaDB" or "MySQL" from the version string instead of relying
    // on the version numbers
    const bool isMariaDB = localVersion >= MYSQL_VERSION_CHECK(10, 0, 0);
    qCDebug(AKONADISERVER_LOG).nospace() << "mysqld reports version " << (localVersion >> 16) << "." << ((localVersion >> 8) & 0x0000FF) << "."
                                         << (localVersion & 0x0000FF) << " (" << (isMariaDB ? "MariaDB" : "Oracle MySQL") << ")";

    QFile actualFile(actualConfig);
    // update conf only if either global (or local) is newer than actual
    if ((QFileInfo(globalConfig).lastModified() > QFileInfo(actualFile).lastModified())
        || (QFileInfo(localConfig).lastModified() > QFileInfo(actualFile).lastModified())) {
        QFile globalFile(globalConfig);
        QFile localFile(localConfig);
        if (globalFile.open(QFile::ReadOnly) && actualFile.open(QFile::WriteOnly)) {
            actualFile.write(globalFile.readAll());
            if (!localConfig.isEmpty()) {
                if (localFile.open(QFile::ReadOnly)) {
                    actualFile.write(localFile.readAll());
                    localFile.close();
                }
            }
            globalFile.close();
            actualFile.close();
        } else {
            qCCritical(AKONADISERVER_LOG) << "Unable to create MySQL server configuration file.";
            qCCritical(AKONADISERVER_LOG) << "This means that either the default configuration file (mysql-global.conf) was not readable";
            qCCritical(AKONADISERVER_LOG) << "or the target file (mysql.conf) could not be written.";
            return false;
        }
    }

    // MySQL doesn't like world writeable config files (which makes sense), but
    // our config file somehow ends up being world-writable on some systems for no
    // apparent reason nevertheless, so fix that
    const QFile::Permissions allowedPerms =
        actualFile.permissions() & (QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther);
    if (allowedPerms != actualFile.permissions()) {
        actualFile.setPermissions(allowedPerms);
    }

    if (dataDir.isEmpty()) {
        qCCritical(AKONADISERVER_LOG) << "Akonadi server was not able to create database data directory";
        return false;
    }

    if (akDir.isEmpty()) {
        qCCritical(AKONADISERVER_LOG) << "Akonadi server was not able to create database log directory";
        return false;
    }

#ifndef Q_OS_WIN
    if (socketDirectory.isEmpty()) {
        qCCritical(AKONADISERVER_LOG) << "Akonadi server was not able to create database misc directory";
        return false;
    }

    // the socket path must not exceed 103 characters, so check for max dir length right away
    if (socketDirectory.length() >= 90) {
        qCCritical(AKONADISERVER_LOG) << "MySQL cannot deal with a socket path this long. Path was: " << socketDirectory;
        return false;
    }

    // If mysql socket file exists, check if also the server process is still running,
    // else we can safely remove the socket file (cleanup after a system crash, etc.)
    QFile pidFile(pidFileName);
    if (QFile::exists(socketFile) && pidFile.open(QIODevice::ReadOnly)) {
        qCDebug(AKONADISERVER_LOG) << "Found a mysqld pid file, checking whether the server is still running...";
        QByteArray pid = pidFile.readLine().trimmed();
        QFile proc(QString::fromLatin1("/proc/" + pid + "/stat"));
        // Check whether the process with the PID from pidfile still exists and whether
        // it's actually still mysqld or, whether the PID has been recycled in the meanwhile.
        bool serverIsRunning = false;
        if (proc.open(QIODevice::ReadOnly)) {
            const QByteArray stat = proc.readAll();
            const QList<QByteArray> stats = stat.split(' ');
            if (stats.count() > 1) {
                // Make sure the PID actually belongs to mysql process

                // Linux trims executable name in /proc filesystem to 15 characters
                const QString expectedProcName = QFileInfo(mMysqldPath).fileName().left(15);
                if (QString::fromLatin1(stats[1]) == QStringLiteral("(%1)").arg(expectedProcName)) {
                    // Yup, our mysqld is actually running, so pretend we started the server
                    // and try to connect to it
                    qCWarning(AKONADISERVER_LOG) << "mysqld for Akonadi is already running, trying to connect to it.";
                    serverIsRunning = true;
                }
            }
            proc.close();
        }

        if (!serverIsRunning) {
            qCDebug(AKONADISERVER_LOG) << "No mysqld process with specified PID is running. Removing the pidfile and starting a new instance...";
            pidFile.close();
            pidFile.remove();
            QFile::remove(socketFile);
        }
    }
#endif

    // synthesize the mysqld command
    QStringList arguments;
    arguments << QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir);
    arguments << QStringLiteral("--datadir=%1/").arg(dataDir);
#ifndef Q_OS_WIN
    arguments << QStringLiteral("--socket=%1").arg(socketFile);
    arguments << QStringLiteral("--pid-file=%1").arg(pidFileName);
#else
    arguments << QString::fromLatin1("--shared-memory");
#endif

#ifndef Q_OS_WIN
    // If mysql socket file does not exists, then we must start the server,
    // otherwise we reconnect to it
    if (!QFile::exists(socketFile)) {
        // move mysql error log file out of the way
        const QFileInfo errorLog(dataDir + QDir::separator() + QLatin1String("mysql.err"));
        if (errorLog.exists()) {
            QFile logFile(errorLog.absoluteFilePath());
            QFile oldLogFile(dataDir + QDir::separator() + QLatin1String("mysql.err.old"));
            if (logFile.open(QFile::ReadOnly) && oldLogFile.open(QFile::WriteOnly)) {
                oldLogFile.write(logFile.readAll());
                oldLogFile.close();
                logFile.close();
                logFile.remove();
            } else {
                qCCritical(AKONADISERVER_LOG) << "Failed to open MySQL error log.";
            }
        }

        // first run, some MySQL versions need a mysql_install_db run for that
        const QString confFile = StandardDirs::locateResourceFile("config", QStringLiteral("mysql-global.conf"));
        if (QDir(dataDir).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty()) {
            if (isMariaDB) {
                initializeMariaDBDatabase(confFile, dataDir);
            } else if (localVersion >= MYSQL_VERSION_CHECK(5, 7, 6)) {
                initializeMySQL5_7_6Database(confFile, dataDir);
            } else {
                initializeMySQLDatabase(confFile, dataDir);
            }
        }

        qCDebug(AKONADISERVER_LOG) << "Executing:" << mMysqldPath << arguments.join(QLatin1Char(' '));
        mDatabaseProcess = std::make_unique<QProcess>();
        mDatabaseProcess->start(mMysqldPath, arguments);
        if (!mDatabaseProcess->waitForStarted()) {
            qCCritical(AKONADISERVER_LOG) << "Could not start database server!";
            qCCritical(AKONADISERVER_LOG) << "executable:" << mMysqldPath;
            qCCritical(AKONADISERVER_LOG) << "arguments:" << arguments;
            qCCritical(AKONADISERVER_LOG) << "process error:" << mDatabaseProcess->errorString();
            return false;
        }

        connect(mDatabaseProcess.get(), &QProcess::finished, this, &DbConfigMysql::processFinished);

        // wait until mysqld has created the socket file (workaround for QTBUG-47475 in Qt5.5.0)
        int counter = 50; // avoid an endless loop in case mysqld terminated
        while ((counter-- > 0) && !QFileInfo::exists(socketFile)) {
            QThread::msleep(100);
        }
    } else {
        qCDebug(AKONADISERVER_LOG) << "Found " << qPrintable(s_mysqlSocketFileName) << " file, reconnecting to the database";
    }
#endif

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), s_initConnection);
        apply(db);

        db.setDatabaseName(QString()); // might not exist yet, then connecting to the actual db will fail
        if (!db.isValid()) {
            qCCritical(AKONADISERVER_LOG) << "Invalid database object during database server startup";
            return false;
        }

        bool opened = false;
        for (int i = 0; i < 120; ++i) {
            opened = db.open();
            if (opened) {
                break;
            }
            if (mDatabaseProcess && mDatabaseProcess->waitForFinished(500)) {
                qCCritical(AKONADISERVER_LOG) << "Database process exited unexpectedly during initial connection!";
                qCCritical(AKONADISERVER_LOG) << "executable:" << mMysqldPath;
                qCCritical(AKONADISERVER_LOG) << "arguments:" << arguments;
                qCCritical(AKONADISERVER_LOG) << "stdout:" << mDatabaseProcess->readAllStandardOutput();
                qCCritical(AKONADISERVER_LOG) << "stderr:" << mDatabaseProcess->readAllStandardError();
                qCCritical(AKONADISERVER_LOG) << "exit code:" << mDatabaseProcess->exitCode();
                qCCritical(AKONADISERVER_LOG) << "process error:" << mDatabaseProcess->errorString();
                return false;
            }
        }

        if (opened) {
            if (!mMysqlCheckPath.isEmpty()) {
                execute(mMysqlCheckPath,
                        {QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir),
                         QStringLiteral("--check-upgrade"),
                         QStringLiteral("--auto-repair"),
#ifndef Q_OS_WIN
                         QStringLiteral("--socket=%1/%2").arg(socketDirectory, s_mysqlSocketFileName),
#endif
                         mDatabaseName});
            }

            if (!mMysqlUpgradePath.isEmpty()) {
                execute(mMysqlUpgradePath,
                        {QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir)
#ifndef Q_OS_WIN
                             ,
                         QStringLiteral("--socket=%1/%2").arg(socketDirectory, s_mysqlSocketFileName)
#endif
                        });
            }

            // Verify MySQL version
            {
                QSqlQuery query(db);
                if (!query.exec(QStringLiteral("SELECT VERSION()")) || !query.first()) {
                    qCCritical(AKONADISERVER_LOG) << "Failed to verify database server version";
                    qCCritical(AKONADISERVER_LOG) << "Query error:" << query.lastError().text();
                    qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
                    return false;
                }

                const QString version = query.value(0).toString();
                const QStringList versions = version.split(QLatin1Char('.'), Qt::SkipEmptyParts);
                if (versions.count() < 3) {
                    qCCritical(AKONADISERVER_LOG) << "Invalid database server version: " << version;
                    return false;
                }

                if (versions[0].toInt() < MYSQL_MIN_MAJOR || (versions[0].toInt() == MYSQL_MIN_MAJOR && versions[1].toInt() < MYSQL_MIN_MINOR)) {
                    qCCritical(AKONADISERVER_LOG) << "Unsupported MySQL version:";
                    qCCritical(AKONADISERVER_LOG) << "Current version:" << QStringLiteral("%1.%2").arg(versions[0], versions[1]);
                    qCCritical(AKONADISERVER_LOG) << "Minimum required version:" << QStringLiteral("%1.%2").arg(MYSQL_MIN_MAJOR).arg(MYSQL_MIN_MINOR);
                    qCCritical(AKONADISERVER_LOG) << "Please update your MySQL database server";
                    return false;
                } else {
                    qCDebug(AKONADISERVER_LOG) << "MySQL version OK"
                                               << "(required" << QStringLiteral("%1.%2").arg(MYSQL_MIN_MAJOR).arg(MYSQL_MIN_MINOR) << ", available"
                                               << QStringLiteral("%1.%2").arg(versions[0], versions[1]) << ")";
                }
            }

            {
                QSqlQuery query(db);
                if (!query.exec(QStringLiteral("USE %1").arg(mDatabaseName))) {
                    qCDebug(AKONADISERVER_LOG) << "Failed to use database" << mDatabaseName;
                    qCDebug(AKONADISERVER_LOG) << "Query error:" << query.lastError().text();
                    qCDebug(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
                    qCDebug(AKONADISERVER_LOG) << "Trying to create database now...";
                    if (!query.exec(QStringLiteral("CREATE DATABASE akonadi"))) {
                        qCCritical(AKONADISERVER_LOG) << "Failed to create database";
                        qCCritical(AKONADISERVER_LOG) << "Query error:" << query.lastError().text();
                        qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
                        success = false;
                    }
                }
            } // make sure query is destroyed before we close the db
            db.close();
        } else {
            qCCritical(AKONADISERVER_LOG) << "Failed to connect to database!";
            qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
            success = false;
        }
    }

    return success;
}

void DbConfigMysql::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    qCCritical(AKONADISERVER_LOG) << "database server stopped unexpectedly";

#ifndef Q_OS_WIN
    // when the server stopped unexpectedly, make sure to remove the stale socket file since otherwise
    // it can not be started again
    const QString socketDirectory = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")), s_mysqlSocketFileName.length());
    const QString socketFile = QStringLiteral("%1/%2").arg(socketDirectory, s_mysqlSocketFileName);
    QFile::remove(socketFile);
#endif

    QCoreApplication::quit();
}

void DbConfigMysql::stopInternalServer()
{
    if (!mDatabaseProcess) {
        return;
    }

    // closing initConnection this late to work around QTBUG-63108
    QSqlDatabase::removeDatabase(s_initConnection);

    disconnect(mDatabaseProcess.get(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &DbConfigMysql::processFinished);

    // first, try the nicest approach
    if (!mCleanServerShutdownCommand.isEmpty()) {
        QProcess::execute(mCleanServerShutdownCommand, QStringList());
        if (mDatabaseProcess->waitForFinished(3000)) {
            return;
        }
    }

    mDatabaseProcess->terminate();
    const bool result = mDatabaseProcess->waitForFinished(3000);
    // We've waited nicely for 3 seconds, to no avail, let's be rude.
    if (!result) {
        mDatabaseProcess->kill();
    }
}

void DbConfigMysql::initSession(const QSqlDatabase &database)
{
    QSqlQuery query(database);
    query.exec(QStringLiteral("SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED"));
}

int DbConfigMysql::parseCommandLineToolsVersion() const
{
    QProcess mysqldProcess;
    mysqldProcess.start(mMysqldPath, {QStringLiteral("--version")});
    mysqldProcess.waitForFinished(10000 /* 10 secs */);

    const QString out = QString::fromLocal8Bit(mysqldProcess.readAllStandardOutput());
    QRegularExpression regexp(QStringLiteral("Ver ([0-9]+)\\.([0-9]+)\\.([0-9]+)"));
    auto match = regexp.match(out);
    if (!match.hasMatch()) {
        return 0;
    }

    return (match.capturedView(1).toInt() << 16) | (match.capturedView(2).toInt() << 8) | match.capturedView(3).toInt();
}

bool DbConfigMysql::initializeMariaDBDatabase(const QString &confFile, const QString &dataDir) const
{
    // KDE Neon (and possible others) don't ship mysql_install_db, but it seems
    // that MariaDB can initialize itself automatically on first start, it only
    // needs that the datadir directory exists
    if (mMysqlInstallDbPath.isEmpty()) {
        return QDir().mkpath(dataDir);
    }

    QFileInfo fi(mMysqlInstallDbPath);
    QDir dir = fi.dir();
    dir.cdUp();
    const QString baseDir = dir.absolutePath();
    return 0
        == execute(mMysqlInstallDbPath,
                   {QStringLiteral("--defaults-file=%1").arg(confFile),
                    QStringLiteral("--force"),
                    QStringLiteral("--basedir=%1").arg(baseDir),
                    QStringLiteral("--datadir=%1/").arg(dataDir)});
}

/**
 * As of MySQL 5.7.6 mysql_install_db is deprecated and mysqld --initailize should be used instead
 * See MySQL Reference Manual section 2.10.1.1 (Initializing the Data Directory Manually Using mysqld)
 */
bool DbConfigMysql::initializeMySQL5_7_6Database(const QString &confFile, const QString &dataDir) const
{
    return 0
        == execute(mMysqldPath,
                   {QStringLiteral("--defaults-file=%1").arg(confFile), QStringLiteral("--initialize"), QStringLiteral("--datadir=%1/").arg(dataDir)});
}

bool DbConfigMysql::initializeMySQLDatabase(const QString &confFile, const QString &dataDir) const
{
    // On FreeBSD MySQL 5.6 is also installed without mysql_install_db, so this
    // might do the trick there as well.
    if (mMysqlInstallDbPath.isEmpty()) {
        return QDir().mkpath(dataDir);
    }

    QFileInfo fi(mMysqlInstallDbPath);
    QDir dir = fi.dir();
    dir.cdUp();
    const QString baseDir = dir.absolutePath();

    // Don't use --force, it has been removed in MySQL 5.7.5
    return 0
        == execute(
               mMysqlInstallDbPath,
               {QStringLiteral("--defaults-file=%1").arg(confFile), QStringLiteral("--basedir=%1").arg(baseDir), QStringLiteral("--datadir=%1/").arg(dataDir)});
}

bool DbConfigMysql::disableConstraintChecks(const QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(QStringLiteral("SET FOREIGN_KEY_CHECKS=0"));
}

bool DbConfigMysql::enableConstraintChecks(const QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(QStringLiteral("SET FOREIGN_KEY_CHECKS=1"));
}

#include "moc_dbconfigmysql.cpp"
