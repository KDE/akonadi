/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "dbconfigmysql.h"
#include "utils.h"
#include "akonadiserver_debug.h"

#include <private/standarddirs_p.h>
#include <private/xdgbasedirs_p.h>

#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QRegularExpression>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

using namespace Akonadi;
using namespace Akonadi::Server;

#define MYSQL_MIN_MAJOR 5
#define MYSQL_MIN_MINOR 1

#define MYSQL_VERSION_CHECK(major, minor, patch) ((major << 16) | (minor << 8) | patch)

DbConfigMysql::DbConfigMysql()
    : mInternalServer(true)
    , mDatabaseProcess(Q_NULLPTR)
{
}

QString DbConfigMysql::driverName() const
{
    return QStringLiteral("QMYSQL");
}

QString DbConfigMysql::databaseName() const
{
    return mDatabaseName;
}

bool DbConfigMysql::init(QSettings &settings)
{
    // determine default settings depending on the driver
    QString defaultHostName;
    QString defaultOptions;
    QString defaultServerPath;
    QString defaultCleanShutdownCommand;

#ifndef Q_OS_WIN
    const QString socketDirectory = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")));
#endif

    const bool defaultInternalServer = true;
#ifdef MYSQLD_EXECUTABLE
    if (QFile::exists(QStringLiteral(MYSQLD_EXECUTABLE))) {
        defaultServerPath = QStringLiteral(MYSQLD_EXECUTABLE);
    }
#endif
    const QStringList mysqldSearchPath = QStringList()
                                         << QStringLiteral("/usr/bin")
                                         << QStringLiteral("/usr/sbin")
                                         << QStringLiteral("/usr/local/sbin")
                                         << QStringLiteral("/usr/local/libexec")
                                         << QStringLiteral("/usr/libexec")
                                         << QStringLiteral("/opt/mysql/libexec")
                                         << QStringLiteral("/opt/local/lib/mysql5/bin")
                                         << QStringLiteral("/opt/mysql/sbin");
    if (defaultServerPath.isEmpty()) {
        defaultServerPath = XdgBaseDirs::findExecutableFile(QStringLiteral("mysqld"), mysqldSearchPath);
    }

    const QString mysqladminPath = XdgBaseDirs::findExecutableFile(QStringLiteral("mysqladmin"), mysqldSearchPath);
    if (!mysqladminPath.isEmpty()) {
#ifndef Q_OS_WIN
        defaultCleanShutdownCommand = QStringLiteral("%1 --defaults-file=%2/mysql.conf --socket=%3/mysql.socket shutdown")
                                      .arg(mysqladminPath, StandardDirs::saveDir("data"), socketDirectory);
#else
        defaultCleanShutdownCommand = QString::fromLatin1("%1 shutdown --shared-memory").arg(mysqladminPath);
#endif
    }

    mMysqlInstallDbPath = XdgBaseDirs::findExecutableFile(QStringLiteral("mysql_install_db"), mysqldSearchPath);
    qCDebug(AKONADISERVER_LOG) << "Found mysql_install_db: " << mMysqlInstallDbPath;

    mMysqlCheckPath = XdgBaseDirs::findExecutableFile(QStringLiteral("mysqlcheck"), mysqldSearchPath);
    qCDebug(AKONADISERVER_LOG) << "Found mysqlcheck: " << mMysqlCheckPath;

    mInternalServer = settings.value(QStringLiteral("QMYSQL/StartServer"), defaultInternalServer).toBool();
#ifndef Q_OS_WIN
    if (mInternalServer) {
        defaultOptions = QStringLiteral("UNIX_SOCKET=%1/mysql.socket").arg(socketDirectory);
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

    // apply temporary changes to the settings
    if (mInternalServer) {
        mHostName.clear();
        mUserName.clear();
        mPassword.clear();
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

    const QString akDir   = StandardDirs::saveDir("data");
    const QString dataDir = StandardDirs::saveDir("data", QStringLiteral("db_data"));
#ifndef Q_OS_WIN
    const QString socketDirectory = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")));
#endif

    // generate config file
    const QString globalConfig = XdgBaseDirs::findResourceFile("config", QStringLiteral("akonadi/mysql-global.conf"));
    const QString localConfig  = XdgBaseDirs::findResourceFile("config", QStringLiteral("akonadi/mysql-local.conf"));
    const QString actualConfig = StandardDirs::saveDir("data") + QLatin1String("/mysql.conf");
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
    qCDebug(AKONADISERVER_LOG).nospace() << "mysqld reports version " << (localVersion >> 16) << "." << ((localVersion >> 8) & 0x0000FF) << "." << (localVersion & 0x0000FF)
                                         << " (" << (isMariaDB ? "MariaDB" : "Oracle MySQL") << ")";


    bool confUpdate = false;
    QFile actualFile(actualConfig);
    // update conf only if either global (or local) is newer than actual
    if ((QFileInfo(globalConfig).lastModified() > QFileInfo(actualFile).lastModified()) ||
        (QFileInfo(localConfig).lastModified()  > QFileInfo(actualFile).lastModified())) {
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
            confUpdate = true;
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
    const QFile::Permissions allowedPerms = actualFile.permissions()
                                            & (QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther);
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
#endif

    // synthesize the mysqld command
    QStringList arguments;
    arguments << QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir);
    arguments << QStringLiteral("--datadir=%1/").arg(dataDir);
#ifndef Q_OS_WIN
    arguments << QStringLiteral("--socket=%1/mysql.socket").arg(socketDirectory);
#else
    arguments << QString::fromLatin1("--shared-memory");
#endif

    // If mysql.socket file does not exists, then we must start the server,
    // otherwise we reconnect to it
    if (!QFile::exists(QStringLiteral("%1/mysql.socket").arg(socketDirectory))) {
        // move mysql error log file out of the way
        const QFileInfo errorLog(dataDir + QDir::separator() + QLatin1String("mysql.err"));
        if (errorLog.exists()) {
            QFile logFile(errorLog.absoluteFilePath());
            QFile oldLogFile(dataDir + QDir::separator() + QLatin1String("mysql.err.old"));
            if (logFile.open(QFile::ReadOnly) && oldLogFile.open(QFile::Append)) {
                oldLogFile.write(logFile.readAll());
                oldLogFile.close();
                logFile.close();
                logFile.remove();
            } else {
                qCCritical(AKONADISERVER_LOG) << "Failed to open MySQL error log.";
            }
        }

        // first run, some MySQL versions need a mysql_install_db run for that
        const QString confFile = XdgBaseDirs::findResourceFile("config", QStringLiteral("akonadi/mysql-global.conf"));
        if (QDir(dataDir).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty() && !mMysqlInstallDbPath.isEmpty()) {
            if (isMariaDB) {
                initializeMariaDBDatabase(confFile, dataDir);
            } else if (localVersion >= MYSQL_VERSION_CHECK(5, 7, 6)) {
                initializeMySQL5_7_6Database(confFile, dataDir);
            } else {
                initializeMySQLDatabase(confFile, dataDir);
            }
        }

        // clear mysql ib_logfile's in case innodb_log_file_size option changed in last confUpdate
        if (confUpdate) {
            QFile(dataDir + QDir::separator() + QLatin1String("ib_logfile0")).remove();
            QFile(dataDir + QDir::separator() + QLatin1String("ib_logfile1")).remove();
        }

        qCDebug(AKONADISERVER_LOG) << "Executing:" << mMysqldPath << arguments.join(QLatin1Char(' '));
        mDatabaseProcess = new QProcess;
        mDatabaseProcess->start(mMysqldPath, arguments);
        if (!mDatabaseProcess->waitForStarted()) {
            qCCritical(AKONADISERVER_LOG) << "Could not start database server!";
            qCCritical(AKONADISERVER_LOG) << "executable:" << mMysqldPath;
            qCCritical(AKONADISERVER_LOG) << "arguments:" << arguments;
            qCCritical(AKONADISERVER_LOG) << "process error:" << mDatabaseProcess->errorString();
            return false;
        }

    #ifndef Q_OS_WIN
        // wait until mysqld has created the socket file (workaround for QTBUG-47475 in Qt5.5.0)
        QString socketFile = QStringLiteral("%1/mysql.socket").arg(socketDirectory);
        int counter = 50;  // avoid an endless loop in case mysqld terminated
        while ((counter-- > 0) && !QFileInfo::exists(socketFile)) {
            QThread::msleep(100);
        }
    #endif
    } else {
        qCDebug(AKONADISERVER_LOG) << "Found mysql.socket file, reconnecting to the database";
        mDatabaseProcess = new QProcess();
    }

    const QLatin1String initCon("initConnection");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), initCon);
        apply(db);

        db.setDatabaseName(QString());   // might not exist yet, then connecting to the actual db will fail
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
            if (mDatabaseProcess->waitForFinished(500)) {
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
                execute(mMysqlCheckPath, { QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir),
                                           QStringLiteral("--check-upgrade"),
                                           QStringLiteral("--auto-repair"),
#ifndef Q_OS_WIN
                                           QStringLiteral("--socket=%1/mysql.socket").arg(socketDirectory),
#endif
                                           mDatabaseName
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
                const QStringList versions = version.split(QLatin1Char('.'), QString::SkipEmptyParts);
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
                              << "(required" << QStringLiteral("%1.%2").arg(MYSQL_MIN_MAJOR).arg(MYSQL_MIN_MINOR)
                              << ", available" << QStringLiteral("%1.%2").arg(versions[0], versions[1]) << ")";
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

    QSqlDatabase::removeDatabase(initCon);
    return success;
}

void DbConfigMysql::stopInternalServer()
{
    if (!mDatabaseProcess) {
        return;
    }

    // first, try the nicest approach
    if (!mCleanServerShutdownCommand.isEmpty()) {
        QProcess::execute(mCleanServerShutdownCommand);
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
    mysqldProcess.start(mMysqldPath, { QStringLiteral("--version") });
    mysqldProcess.waitForFinished(10000 /* 10 secs */);

    const QString out = QString::fromLocal8Bit(mysqldProcess.readAllStandardOutput());
    QRegularExpression regexp(QStringLiteral("Ver ([0-9]+)\\.([0-9]+)\\.([0-9]+)"));
    auto match = regexp.match(out);
    if (!match.hasMatch()) {
        return 0;
    }

    return (match.capturedRef(1).toInt() << 16) | (match.capturedRef(2).toInt() << 8) | match.capturedRef(3).toInt();
}

bool DbConfigMysql::initializeMariaDBDatabase(const QString &confFile, const QString &dataDir) const
{
    QFileInfo fi(mMysqlInstallDbPath);
    QDir dir = fi.dir();
    dir.cdUp();
    const QString baseDir = dir.absolutePath();
    return 0 == execute(mMysqlInstallDbPath,
                        { QStringLiteral("--defaults-file=%1").arg(confFile),
                          QStringLiteral("--force"),
                          QStringLiteral("--basedir=%1").arg(baseDir),
                          QStringLiteral("--datadir=%1/").arg(dataDir) });
}

/**
 * As of MySQL 5.7.6 mysql_install_db is deprecated and mysqld --initailize should be used instead
 * See MySQL Reference Manual section 2.10.1.1 (Initializing the Data Directory Manually Using mysqld)
 */
bool DbConfigMysql::initializeMySQL5_7_6Database(const QString &confFile, const QString &dataDir) const
{
    return 0 == execute(mMysqldPath,
                        { QStringLiteral("--defaults-file=%1").arg(confFile),
                          QStringLiteral("--initialize"),
                          QStringLiteral("--datadir=%1/").arg(dataDir) });
}

bool DbConfigMysql::initializeMySQLDatabase(const QString &confFile, const QString &dataDir) const
{
    QFileInfo fi(mMysqlInstallDbPath);
    QDir dir = fi.dir();
    dir.cdUp();
    const QString baseDir = dir.absolutePath();

    // Don't use --force, it has been removed in MySQL 5.7.5
    return 0 == execute(mMysqlInstallDbPath,
                        {  QStringLiteral("--defaults-file=%1").arg(confFile),
                           QStringLiteral("--basedir=%1").arg(baseDir),
                           QStringLiteral("--datadir=%1/").arg(dataDir) });
}
