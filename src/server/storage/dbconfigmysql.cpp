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

#include <shared/akdebug.h>

#include <private/standarddirs_p.h>
#include <private/xdgbasedirs_p.h>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;
using namespace Akonadi::Server;

#define MYSQL_MIN_MAJOR 5
#define MYSQL_MIN_MINOR 1

DbConfigMysql::DbConfigMysql()
    : mInternalServer(true)
    , mDatabaseProcess(0)
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
                                      .arg(mysqladminPath)
                                      .arg(StandardDirs::saveDir("data"))
                                      .arg(socketDirectory);
#else
        defaultCleanShutdownCommand = QString::fromLatin1("%1 shutdown --shared-memory").arg(mysqladminPath);
#endif
    }

    mMysqlInstallDbPath = XdgBaseDirs::findExecutableFile(QStringLiteral("mysql_install_db"), mysqldSearchPath);
    akDebug() << "Found mysql_install_db: " << mMysqlInstallDbPath;

    mMysqlCheckPath = XdgBaseDirs::findExecutableFile(QStringLiteral("mysqlcheck"), mysqldSearchPath);
    akDebug() << "Found mysqlcheck: " << mMysqlCheckPath;

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
    mServerPath = settings.value(QStringLiteral("ServerPath"), defaultServerPath).toString();
    mCleanServerShutdownCommand = settings.value(QStringLiteral("CleanServerShutdownCommand"), defaultCleanShutdownCommand).toString();
    settings.endGroup();

    // verify settings and apply permanent changes (written out below)
    if (mInternalServer) {
        mConnectionOptions = defaultOptions;
        // intentionally not namespaced as we are the only one in this db instance when using internal mode
        mDatabaseName = QStringLiteral("akonadi");
    }
    if (mInternalServer && (mServerPath.isEmpty() || !QFile::exists(mServerPath))) {
        mServerPath = defaultServerPath;
    }

    // store back the default values
    settings.beginGroup(driverName());
    settings.setValue(QStringLiteral("Name"), mDatabaseName);
    settings.setValue(QStringLiteral("Host"), mHostName);
    settings.setValue(QStringLiteral("Options"), mConnectionOptions);
    if (!mServerPath.isEmpty()) {
        settings.setValue(QStringLiteral("ServerPath"), mServerPath);
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
    const QString mysqldPath = mServerPath;

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
        akError() << "Did not find MySQL server default configuration (mysql-global.conf)";
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
            akError() << "Unable to create MySQL server configuration file.";
            akError() << "This means that either the default configuration file (mysql-global.conf) was not readable";
            akError() << "or the target file (mysql.conf) could not be written.";
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
        akError() << "Akonadi server was not able to create database data directory";
        return false;
    }

    if (akDir.isEmpty()) {
        akError() << "Akonadi server was not able to create database log directory";
        return false;
    }

#ifndef Q_OS_WIN
    if (socketDirectory.isEmpty()) {
        akError() << "Akonadi server was not able to create database misc directory";
        return false;
    }

    // the socket path must not exceed 103 characters, so check for max dir length right away
    if (socketDirectory.length() >= 90) {
        akError() << "MySQL cannot deal with a socket path this long. Path was: " << socketDirectory;
        return false;
    }
#endif

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
            akError() << "Failed to open MySQL error log.";
        }
    }

    // first run, some MySQL versions need a mysql_install_db run for that
    const QString confFile = XdgBaseDirs::findResourceFile("config", QStringLiteral("akonadi/mysql-global.conf"));
    if (QDir(dataDir).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty() && !mMysqlInstallDbPath.isEmpty()) {
        const QStringList arguments = QStringList() << QStringLiteral("--force") << QStringLiteral("--defaults-file=%1").arg(confFile) << QStringLiteral("--datadir=%1/").arg(dataDir);
        QProcess::execute(mMysqlInstallDbPath, arguments);
    }

    // clear mysql ib_logfile's in case innodb_log_file_size option changed in last confUpdate
    if (confUpdate) {
        QFile(dataDir + QDir::separator() + QLatin1String("ib_logfile0")).remove();
        QFile(dataDir + QDir::separator() + QLatin1String("ib_logfile1")).remove();
    }

    // synthesize the mysqld command
    QStringList arguments;
    arguments << QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir);
    arguments << QStringLiteral("--datadir=%1/").arg(dataDir);
#ifndef Q_OS_WIN
    arguments << QStringLiteral("--socket=%1/mysql.socket").arg(socketDirectory);
#else
    arguments << QString::fromLatin1("--shared-memory");
#endif

    if (mysqldPath.isEmpty()) {
        akError() << "mysqld not found. Please verify your installation";
        return false;
    }
    mDatabaseProcess = new QProcess;
    mDatabaseProcess->start(mysqldPath, arguments);
    if (!mDatabaseProcess->waitForStarted()) {
        akError() << "Could not start database server!";
        akError() << "executable:" << mysqldPath;
        akError() << "arguments:" << arguments;
        akError() << "process error:" << mDatabaseProcess->errorString();
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

    const QLatin1String initCon("initConnection");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), initCon);
        apply(db);

        db.setDatabaseName(QString());   // might not exist yet, then connecting to the actual db will fail
        if (!db.isValid()) {
            akError() << "Invalid database object during database server startup";
            return false;
        }

        bool opened = false;
        for (int i = 0; i < 120; ++i) {
            opened = db.open();
            if (opened) {
                break;
            }
            if (mDatabaseProcess->waitForFinished(500)) {
                akError() << "Database process exited unexpectedly during initial connection!";
                akError() << "executable:" << mysqldPath;
                akError() << "arguments:" << arguments;
                akError() << "stdout:" << mDatabaseProcess->readAllStandardOutput();
                akError() << "stderr:" << mDatabaseProcess->readAllStandardError();
                akError() << "exit code:" << mDatabaseProcess->exitCode();
                akError() << "process error:" << mDatabaseProcess->errorString();
                return false;
            }
        }

        if (opened) {
            if (!mMysqlCheckPath.isEmpty()) {
                const QStringList arguments = QStringList() << QStringLiteral("--defaults-file=%1/mysql.conf").arg(akDir)
                                              << QStringLiteral("--check-upgrade")
                                              << QStringLiteral("--all-databases")
                                              << QStringLiteral("--auto-repair")
#ifndef Q_OS_WIN
                                              << QStringLiteral("--socket=%1/mysql.socket").arg(socketDirectory)
#endif
                                              ;
                QProcess::execute(mMysqlCheckPath, arguments);
            }

            // Verify MySQL version
            {
                QSqlQuery query(db);
                if (!query.exec(QStringLiteral("SELECT VERSION()")) || !query.first()) {
                    akError() << "Failed to verify database server version";
                    akError() << "Query error:" << query.lastError().text();
                    akError() << "Database error:" << db.lastError().text();
                    return false;
                }

                const QString version = query.value(0).toString();
                const QStringList versions = version.split(QLatin1Char('.'), QString::SkipEmptyParts);
                if (versions.count() < 3) {
                    akError() << "Invalid database server version: " << version;
                    return false;
                }

                if (versions[0].toInt() < MYSQL_MIN_MAJOR || (versions[0].toInt() == MYSQL_MIN_MAJOR && versions[1].toInt() < MYSQL_MIN_MINOR)) {
                    akError() << "Unsupported MySQL version:";
                    akError() << "Current version:" << QStringLiteral("%1.%2").arg(versions[0], versions[1]);
                    akError() << "Minimum required version:" << QStringLiteral("%1.%2").arg(MYSQL_MIN_MAJOR).arg(MYSQL_MIN_MINOR);
                    akError() << "Please update your MySQL database server";
                    return false;
                } else {
                    akDebug() << "MySQL version OK"
                              << "(required" << QStringLiteral("%1.%2").arg(MYSQL_MIN_MAJOR).arg(MYSQL_MIN_MINOR)
                              << ", available" << QStringLiteral("%1.%2").arg(versions[0], versions[1]) << ")";
                }
            }

            {
                QSqlQuery query(db);
                if (!query.exec(QStringLiteral("USE %1").arg(mDatabaseName))) {
                    akDebug() << "Failed to use database" << mDatabaseName;
                    akDebug() << "Query error:" << query.lastError().text();
                    akDebug() << "Database error:" << db.lastError().text();
                    akDebug() << "Trying to create database now...";
                    if (!query.exec(QStringLiteral("CREATE DATABASE akonadi"))) {
                        akError() << "Failed to create database";
                        akError() << "Query error:" << query.lastError().text();
                        akError() << "Database error:" << db.lastError().text();
                        success = false;
                    }
                }
            } // make sure query is destroyed before we close the db
            db.close();
        } else {
            akError() << "Failed to connect to database!";
            akError() << "Database error:" << db.lastError().text();
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
