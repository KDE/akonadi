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

#include "dbconfigpostgresql.h"
#include "utils.h"
#include "akonadiserver_debug.h"

#include <private/standarddirs_p.h>

#include <QDir>
#include <QProcess>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace Akonadi::Server;

DbConfigPostgresql::DbConfigPostgresql()
    : mHostPort(0)
    , mInternalServer(true)
{
}

QString DbConfigPostgresql::driverName() const
{
    return QStringLiteral("QPSQL");
}

QString DbConfigPostgresql::databaseName() const
{
    return mDatabaseName;
}

bool DbConfigPostgresql::init(QSettings &settings)
{
    // determine default settings depending on the driver
    QString defaultHostName;
    QString defaultOptions;
    QString defaultServerPath;
    QString defaultInitDbPath;
    QString defaultPgData;

#ifndef Q_WS_WIN // We assume that PostgreSQL is running as service on Windows
    const bool defaultInternalServer = true;
#else
    const bool defaultInternalServer = false;
#endif

    mInternalServer = settings.value(QStringLiteral("QPSQL/StartServer"), defaultInternalServer).toBool();
    if (mInternalServer) {
        QStringList postgresSearchPath;

#ifdef POSTGRES_PATH
        const QString dir(QStringLiteral(POSTGRES_PATH));
        if (QDir(dir).exists()) {
            postgresSearchPath << QStringLiteral(POSTGRES_PATH);
        }
#endif
        postgresSearchPath << QStringLiteral("/usr/sbin")
                           << QStringLiteral("/usr/local/sbin");
        // Locale all versions in /usr/lib/postgresql (i.e. /usr/lib/postgresql/X.Y) in reversed
        // sorted order, so we search from the newest one to the oldest.
        QStringList postgresVersionedSearchPaths;
        QDir versionedDir(QStringLiteral("/usr/lib/postgresql"));
        if (versionedDir.exists()) {
            const auto versionedDirs = versionedDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
            for (const auto &path : versionedDirs) {
                // Don't break once PostgreSQL 10 is released, but something more future-proof will be needed
                if (path.fileName().startsWith(QLatin1String("10."))) {
                    postgresVersionedSearchPaths.prepend(path.absoluteFilePath() + QStringLiteral("/bin"));
                } else {
                    postgresVersionedSearchPaths.append(path.absoluteFilePath() + QStringLiteral("/bin"));
                }
            }
        }
        postgresSearchPath.append(postgresVersionedSearchPaths);
        defaultServerPath = QStandardPaths::findExecutable(QStringLiteral("pg_ctl"), postgresSearchPath);
        defaultInitDbPath = QStandardPaths::findExecutable(QStringLiteral("initdb"), postgresSearchPath);
        defaultHostName = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")));
        defaultPgData = StandardDirs::saveDir("data", QStringLiteral("db_data"));
    }

    // read settings for current driver
    settings.beginGroup(driverName());
    mDatabaseName = settings.value(QStringLiteral("Name"), defaultDatabaseName()).toString();
    if (mDatabaseName.isEmpty()) {
        mDatabaseName = defaultDatabaseName();
    }
    mHostName = settings.value(QStringLiteral("Host"), defaultHostName).toString();
    if (mHostName.isEmpty()) {
        mHostName = defaultHostName;
    }
    mHostPort = settings.value(QStringLiteral("Port")).toInt();
    // User, password and Options can be empty and still valid, so don't override them
    mUserName = settings.value(QStringLiteral("User")).toString();
    mPassword = settings.value(QStringLiteral("Password")).toString();
    mConnectionOptions = settings.value(QStringLiteral("Options"), defaultOptions).toString();
    mServerPath = settings.value(QStringLiteral("ServerPath"), defaultServerPath).toString();
    if (mInternalServer && mServerPath.isEmpty()) {
        mServerPath = defaultServerPath;
    }
    qCDebug(AKONADISERVER_LOG) << "Found pg_ctl:" << mServerPath;
    mInitDbPath = settings.value(QStringLiteral("InitDbPath"), defaultInitDbPath).toString();
    if (mInternalServer && mInitDbPath.isEmpty()) {
        mInitDbPath = defaultInitDbPath;
    }
    qCDebug(AKONADISERVER_LOG) << "Found initdb:" << mServerPath;
    mPgData = settings.value(QStringLiteral("PgData"), defaultPgData).toString();
    if (mPgData.isEmpty()) {
        mPgData = defaultPgData;
    }
    settings.endGroup();

    // store back the default values
    settings.beginGroup(driverName());
    settings.setValue(QStringLiteral("Name"), mDatabaseName);
    settings.setValue(QStringLiteral("Host"), mHostName);
    if (mHostPort) {
        settings.setValue(QStringLiteral("Port"), mHostPort);
    }
    settings.setValue(QStringLiteral("Options"), mConnectionOptions);
    settings.setValue(QStringLiteral("ServerPath"), mServerPath);
    settings.setValue(QStringLiteral("InitDbPath"), mInitDbPath);
    settings.setValue(QStringLiteral("StartServer"), mInternalServer);
    settings.endGroup();
    settings.sync();

    return true;
}

void DbConfigPostgresql::apply(QSqlDatabase &database)
{
    if (!mDatabaseName.isEmpty()) {
        database.setDatabaseName(mDatabaseName);
    }
    if (!mHostName.isEmpty()) {
        database.setHostName(mHostName);
    }
    if (mHostPort > 0 && mHostPort < 65535) {
        database.setPort(mHostPort);
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

bool DbConfigPostgresql::useInternalServer() const
{
    return mInternalServer;
}

bool DbConfigPostgresql::startInternalServer()
{
    // We defined the mHostName to the socket directory, during init
    const QString socketDir = mHostName;
    bool success = true;

    // Make sure the path exists, otherwise pg_ctl fails
    if (!QFile::exists(socketDir)) {
        QDir().mkpath(socketDir);
    }

// TODO Windows support
#ifndef Q_WS_WIN
    // If postmaster.pid exists, check whether the postgres process still exists too,
    // because normally we shouldn't be able to get this far if Akonadi is already
    // running. If postgres is not running, then the pidfile was left after a system
    // crash or something similar and we can remove it (otherwise pg_ctl won't start)
    QFile postmaster(QStringLiteral("%1/postmaster.pid").arg(mPgData));
    if (postmaster.exists() && postmaster.open(QIODevice::ReadOnly)) {
        qCDebug(AKONADISERVER_LOG) << "Found a postmaster.pid pidfile, checking whether the server is still running...";
        QByteArray pid = postmaster.readLine();
        // Remvoe newline character
        pid.truncate(pid.size() - 1);
        QFile proc(QString::fromLatin1("/proc/" + pid + "/stat"));
        // Check whether the process with the PID from pidfile still exists and whether
        // it's actually still postgres or, whether the PID has been recycled in the
        // meanwhile.
        if (proc.open(QIODevice::ReadOnly)) {
            const QByteArray stat = proc.readAll();
            const QList<QByteArray> stats = stat.split(' ');
            if (stats.count() > 1) {
                // Make sure the PID actually belongs to postgres process
                if (stats[1] == "(postgres)") {
                    // Yup, our PostgreSQL is actually running, so pretend we started the server
                    // and try to connect to it
                    qCWarning(AKONADISERVER_LOG) << "PostgreSQL for Akonadi is already running, trying to connect to it.";
                    return true;
                }
            }
            proc.close();
        }

        qCDebug(AKONADISERVER_LOG) << "No postgres process with specified PID is running. Removing the pidfile and starting a new Postgres instance...";
        postmaster.close();
        postmaster.remove();
    }
#endif

    // postgres data directory not initialized yet, so call initdb on it
    if (!QFile::exists(QStringLiteral("%1/PG_VERSION").arg(mPgData))) {
#ifdef Q_OS_LINUX
        // It is recommended to disable CoW feature when running on Btrfs to improve
        // database performance. This only has effect when done on empty directory,
        // so we only call this before calling initdb
        if (Utils::getDirectoryFileSystem(mPgData) == QLatin1String("btrfs")) {
            Utils::disableCoW(mPgData);
        }
#endif

        // call 'initdb --pgdata=/home/user/.local/share/akonadi/data_db'
        execute(mInitDbPath, { QStringLiteral("--pgdata=%1").arg(mPgData),
                               QStringLiteral("--locale=en_US.UTF-8") // TODO: check locale
                             });
    }

    // synthesize the postgres command
    QStringList arguments;
    arguments << QStringLiteral("start")
              << QStringLiteral("-w")
              << QStringLiteral("--timeout=10")   // default is 60 seconds.
              << QStringLiteral("--pgdata=%1").arg(mPgData)
              // These options are passed to postgres
              //  -k - directory for unix domain socket communication
              //  -h - disable listening for TCP/IP
              << QStringLiteral("-o \"-k%1\" -h ''").arg(socketDir);

    qCDebug(AKONADISERVER_LOG) << "Executing:" << mServerPath << arguments.join(QLatin1Char(' '));
    QProcess pgCtl;
    pgCtl.start(mServerPath, arguments);
    if (!pgCtl.waitForStarted()) {
        qCCritical(AKONADISERVER_LOG) << "Could not start database server!";
        qCCritical(AKONADISERVER_LOG) << "executable:" << mServerPath;
        qCCritical(AKONADISERVER_LOG) << "arguments:" << arguments;
        qCCritical(AKONADISERVER_LOG) << "process error:" << pgCtl.errorString();
        return false;
    }

    const QLatin1String initCon("initConnection");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), initCon);
        apply(db);

        // use the default database that is always available
        db.setDatabaseName(QStringLiteral("postgres"));

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

            if (pgCtl.waitForFinished(500) && pgCtl.exitCode()) {
                qCCritical(AKONADISERVER_LOG) << "Database process exited unexpectedly during initial connection!";
                qCCritical(AKONADISERVER_LOG) << "executable:" << mServerPath;
                qCCritical(AKONADISERVER_LOG) << "arguments:" << arguments;
                qCCritical(AKONADISERVER_LOG) << "stdout:" << pgCtl.readAllStandardOutput();
                qCCritical(AKONADISERVER_LOG) << "stderr:" << pgCtl.readAllStandardError();
                qCCritical(AKONADISERVER_LOG) << "exit code:" << pgCtl.exitCode();
                qCCritical(AKONADISERVER_LOG) << "process error:" << pgCtl.errorString();
                return false;
            }
        }

        if (opened) {
            {
                QSqlQuery query(db);

                // check if the 'akonadi' database already exists
                query.exec(QStringLiteral("SELECT 1 FROM pg_catalog.pg_database WHERE datname = '%1'").arg(mDatabaseName));

                // if not, create it
                if (!query.first()) {
                    if (!query.exec(QStringLiteral("CREATE DATABASE %1").arg(mDatabaseName))) {
                        qCCritical(AKONADISERVER_LOG) << "Failed to create database";
                        qCCritical(AKONADISERVER_LOG) << "Query error:" << query.lastError().text();
                        qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
                        success = false;
                    }
                }
            } // make sure query is destroyed before we close the db
            db.close();
        }
    }
    // Make sure pg_ctl has returned
    pgCtl.waitForFinished();

    QSqlDatabase::removeDatabase(initCon);
    return success;
}

void DbConfigPostgresql::stopInternalServer()
{
    if (!checkServerIsRunning()) {
        qCDebug(AKONADISERVER_LOG) << "Database is no longer running";
        return;
    }

    // first, try a FAST shutdown
    execute(mServerPath, { QStringLiteral("stop"),
                           QStringLiteral("--pgdata=%1").arg(mPgData),
                           QStringLiteral("--mode=fast")
                         });
    if (!checkServerIsRunning()) {
        return;
    }

    // second, try an IMMEDIATE shutdown
    execute(mServerPath, { QStringLiteral("stop"),
                           QStringLiteral("--pgdata=%1").arg(mPgData),
                           QStringLiteral("--mode=immediate")
                         });
    if (!checkServerIsRunning()) {
        return;
    }

    // third, pg_ctl couldn't terminate all the postgres processes, we have to
    // kill the master one. We don't want to do that, but we've passed the last
    // call. pg_ctl is used to send the kill signal (safe when kill is not
    // supported by OS)
    const QString pidFileName = QStringLiteral("%1/postmaster.pid").arg(mPgData);
    QFile pidFile(pidFileName);
    if (pidFile.open(QIODevice::ReadOnly)) {
        QString postmasterPid = QString::fromUtf8(pidFile.readLine(0).trimmed());
        qCCritical(AKONADISERVER_LOG) << "The postmaster is still running. Killing it.";

        execute(mServerPath, { QStringLiteral("kill"),
                               QStringLiteral("ABRT"),
                               postmasterPid
                             });
    }
}

bool DbConfigPostgresql::checkServerIsRunning()
{
    const QString command = QStringLiteral("%1").arg(mServerPath);
    QStringList arguments;
    arguments << QStringLiteral("status")
              << QStringLiteral("--pgdata=%1").arg(mPgData);

    QProcess pgCtl;
    pgCtl.start(command, arguments, QIODevice::ReadOnly);
    if (!pgCtl.waitForFinished(3000)) {
        // Error?
        return false;
    }

    const QByteArray output = pgCtl.readAllStandardOutput();
    return output.startsWith("pg_ctl: server is running");
}
