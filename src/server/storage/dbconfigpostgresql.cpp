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
#include <shared/akoptional.h>
#include <shared/akranges.h>

#include <QDir>
#include <QProcess>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <chrono>

using namespace std::chrono_literals;

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

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

namespace {

struct VersionCompare {

    bool operator()(const QFileInfo &lhsFi, const QFileInfo &rhsFi) const
    {
        const auto lhs = parseVersion(lhsFi.fileName());
        if (!lhs.has_value()) {
            return false;
        }
        const auto rhs = parseVersion(rhsFi.fileName());
        if (!rhs.has_value()) {
            return true;
        }

        return std::tie(lhs->major, lhs->minor) < std::tie(rhs->major, rhs->minor);
    }

private:
    struct Version {
        int major;
        int minor;
    };
    akOptional<Version> parseVersion(const QString &name) const
    {
        const auto dotIdx = name.indexOf(QLatin1Char('.'));
        if (dotIdx == -1) {
            return {};
        }
        bool ok = false;
        const auto major = name.leftRef(dotIdx).toInt(&ok);
        if (!ok) {
            return {};
        }
        const auto minor = name.midRef(dotIdx + 1).toInt(&ok);
        if (!ok) {
            return {};
        }
        return Version{major, minor};
    }
};

}

QStringList DbConfigPostgresql::postgresSearchPaths(const QString &versionedPath) const
{
    QStringList paths;

#ifdef POSTGRES_PATH
    const QString dir(QStringLiteral(POSTGRES_PATH));
    if (QDir(dir).exists()) {
        paths.push_back(QStringLiteral(POSTGRES_PATH));
    }
#endif
    paths << QStringLiteral("/usr/bin")
          << QStringLiteral("/usr/sbin")
          << QStringLiteral("/usr/local/sbin");

    // Locate all versions in /usr/lib/postgresql (i.e. /usr/lib/postgresql/X.Y) in reversed
    // sorted order, so we search from the newest one to the oldest.
    QDir versionedDir(versionedPath);
    if (versionedDir.exists()) {
        auto versionedDirs = versionedDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        qDebug() << versionedDirs;
        std::sort(versionedDirs.begin(), versionedDirs.end(), VersionCompare());
        std::reverse(versionedDirs.begin(), versionedDirs.end());
        paths += versionedDirs | Views::transform([](const auto &dir) -> QString {
                                    return dir.absoluteFilePath() + QStringLiteral("/bin"); })
                               | Actions::toQList;
    }

    return paths;
}

bool DbConfigPostgresql::init(QSettings &settings)
{
    // determine default settings depending on the driver
    QString defaultHostName;
    QString defaultOptions;
    QString defaultServerPath;
    QString defaultInitDbPath;
    QString defaultPgUpgradePath;
    QString defaultPgData;

#ifndef Q_WS_WIN // We assume that PostgreSQL is running as service on Windows
    const bool defaultInternalServer = true;
#else
    const bool defaultInternalServer = false;
#endif

    mInternalServer = settings.value(QStringLiteral("QPSQL/StartServer"), defaultInternalServer).toBool();
    if (mInternalServer) {
        const auto paths = postgresSearchPaths(QStringLiteral("/usr/lib/postgresql"));

        defaultServerPath = QStandardPaths::findExecutable(QStringLiteral("pg_ctl"), paths);
        defaultInitDbPath = QStandardPaths::findExecutable(QStringLiteral("initdb"), paths);
        defaultHostName = Utils::preferredSocketDirectory(StandardDirs::saveDir("data", QStringLiteral("db_misc")));
        defaultPgUpgradePath = QStandardPaths::findExecutable(QStringLiteral("pg_upgrade"), paths);
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
    mPgUpgradePath = settings.value(QStringLiteral("UpgradePath"), defaultPgUpgradePath).toString();
    if (mInternalServer && mPgUpgradePath.isEmpty()) {
        mPgUpgradePath = defaultPgUpgradePath;
    }
    qCDebug(AKONADISERVER_LOG) << "Found pg_upgrade:" << mPgUpgradePath;
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

akOptional<DbConfigPostgresql::Versions> DbConfigPostgresql::checkPgVersion() const
{
    // Contains major version of Postgres that creted the cluster
    QFile pgVersionFile(QStringLiteral("%1/PG_VERSION").arg(mPgData));
    if (!pgVersionFile.open(QIODevice::ReadOnly)) {
        return nullopt;
    }
    const auto clusterVersion = pgVersionFile.readAll().toInt();

    QProcess pgctl;
    pgctl.start(mServerPath, { QStringLiteral("--version") }, QIODevice::ReadOnly);
    if (!pgctl.waitForFinished()) {
        return nullopt;
    }
    // Looks like "pg_ctl (PostgreSQL) 11.2"
    const auto output = QString::fromUtf8(pgctl.readAll());

    // Get the major version from major.minor
    QRegularExpression re(QStringLiteral("\\(PostgreSQL\\) ([0-9]+).[0-9]+"));
    const auto match = re.match(output);
    if (!match.hasMatch()) {
        return nullopt;
    }
    const auto serverVersion = match.captured(1).toInt();

    qDebug(AKONADISERVER_LOG) << "Detected psql versions - cluster:" << clusterVersion << ", server:" << serverVersion;
    return {{ clusterVersion, serverVersion }};
}

bool DbConfigPostgresql::runInitDb(const QString &newDbPath)
{
    // Make sure the cluster directory exists
    if (!QDir(newDbPath).exists()) {
        if (!QDir().mkpath(newDbPath)) {
            return false;
        }
    }

#ifdef Q_OS_LINUX
    // It is recommended to disable CoW feature when running on Btrfs to improve
    // database performance. This only has effect when done on empty directory,
    // so we only call this before calling initdb
    if (Utils::getDirectoryFileSystem(newDbPath) == QLatin1String("btrfs")) {
        Utils::disableCoW(newDbPath);
    }
#endif

    // call 'initdb --pgdata=/home/user/.local/share/akonadi/data_db'
    return execute(mInitDbPath, { QStringLiteral("--pgdata=%1").arg(newDbPath),
                                  QStringLiteral("--locale=en_US.UTF-8") /* TODO: check locale */ }) == 0;
}

namespace {

akOptional<QString> findBinPathForVersion(int version)
{
    // First we need to find where the previous PostgreSQL version binaries are available
    const auto oldBinSearchPaths = {
        QStringLiteral("/usr/lib64/pgsql/postgresql-%1/bin").arg(version), // Fedora & friends
        QStringLiteral("/usr/lib/pgsql/postgresql-%1/bin").arg(version),
        QStringLiteral("/usr/lib/postgresql/%1/bin").arg(version), // Debian-based
        QStringLiteral("/opt/pgsql-%1/bin").arg(version), // Arch Linux
        // TODO: Check other distros as well, they might do things differently.
    };

    for (const auto &path : oldBinSearchPaths) {
        if (QDir(path).exists()) {
            return path;
        }
    }

    return nullopt;
}

bool checkAndRemoveTmpCluster(const QDir &baseDir, const QString &clusterName)
{
    if (baseDir.exists(clusterName)) {
        qCInfo(AKONADISERVER_LOG) << "Postgres cluster update:" << clusterName << "cluster already exists, trying to remove it first";
        if (!QDir(baseDir.path() + QDir::separator() + clusterName).removeRecursively()) {
            qCWarning(AKONADISERVER_LOG) << "Postgres cluster update: failed to remove" << clusterName << "cluster from some previous run, not performing auto-upgrade";
            return false;
        }
    }
    return true;
}

bool runPgUpgrade(const QString &pgUpgrade, const QDir &baseDir, const QString &oldBinPath, const QString &newBinPath, const QString &oldDbData, const QString &newDbData)
{
    QProcess process;
    const QStringList args = { QString(QStringLiteral("--old-bindir=%1").arg(oldBinPath)),
                               QString(QStringLiteral("--new-bindir=%1").arg(newBinPath)),
                               QString(QStringLiteral("--old-datadir=%1").arg(oldDbData)),
                               QString(QStringLiteral("--new-datadir=%1").arg(newDbData)) };
    qCInfo(AKONADISERVER_LOG) << "Postgres cluster update: starting pg_upgrade to upgrade your Akonadi DB cluster";
    qCDebug(AKONADISERVER_LOG) << "Executing pg_upgrade" << QStringList(args);
    process.setWorkingDirectory(baseDir.path());
    process.start(pgUpgrade, args);
    process.waitForFinished(std::chrono::milliseconds(1h).count());
    if (process.exitCode() != 0) {
        qCWarning(AKONADISERVER_LOG) << "Postgres cluster update: pg_upgrade finished with exit code" << process.exitCode() << ", please run migration manually.";
        return false;
    }

    qCDebug(AKONADISERVER_LOG) << "Postgres cluster update: pg_upgrade finished successfully.";
    return true;
}

bool swapClusters(QDir &baseDir, const QString &oldDbDataCluster, const QString &newDbDataCluster)
{
    // If everything went fine, swap the old and new clusters
    if (!baseDir.rename(QStringLiteral("db_data"), oldDbDataCluster)) {
        qCWarning(AKONADISERVER_LOG) << "Postgres cluster update: failed to rename old db_data to" << oldDbDataCluster;
        return false;
    }
    if (!baseDir.rename(newDbDataCluster, QStringLiteral("db_data"))) {
        qCWarning(AKONADISERVER_LOG) << "Postgres cluster update: failed to rename" << newDbDataCluster << "to db_data, rolling back";
        if (!baseDir.rename(oldDbDataCluster, QStringLiteral("db_data"))) {
            qCWarning(AKONADISERVER_LOG) << "Postgres cluster update: failed to roll back from" << oldDbDataCluster << "to db_data.";
            return false;
        }
        qCDebug(AKONADISERVER_LOG) << "Postgres cluster update: rollback successful.";
        return false;
    }

    return true;
}

}

bool DbConfigPostgresql::upgradeCluster(int clusterVersion)
{
    const auto oldDbDataCluster = QStringLiteral("old_db_data");
    const auto newDbDataCluster = QStringLiteral("new_db_data");

    QDir baseDir(mPgData); // db_data
    baseDir.cdUp(); // move to its parent folder

    const auto oldBinPath = findBinPathForVersion(clusterVersion);
    if (!oldBinPath.has_value()) {
        qCDebug(AKONADISERVER_LOG) << "Postgres cluster update: failed to find Postgres server for version" << clusterVersion;
        return false;
    }
    const auto newBinPath = QFileInfo(mServerPath).path();

    if (!checkAndRemoveTmpCluster(baseDir, oldDbDataCluster)) {
        return false;
    }
    if (!checkAndRemoveTmpCluster(baseDir, newDbDataCluster)) {
        return false;
    }

    // Next, initialize a new cluster
    const QString newDbData = baseDir.path() + QDir::separator() + newDbDataCluster;
    qCInfo(AKONADISERVER_LOG) << "Postgres cluster upgrade: creating a new cluster for current Postgres server";
    if (!runInitDb(newDbData)) {
        qCWarning(AKONADISERVER_LOG) << "Postgres cluster update: failed to initialize new db cluster";
        return false;
    }

    // Now migrate the old cluster from the old version into the new cluster
    if (!runPgUpgrade(mPgUpgradePath, baseDir, *oldBinPath, newBinPath, mPgData, newDbData)) {
        return false;
    }

    if (!swapClusters(baseDir, oldDbDataCluster, newDbDataCluster)) {
        return false;
    }

    // Drop the old cluster
    if (!QDir(baseDir.path() + QDir::separator() + oldDbDataCluster).removeRecursively()) {
        qCInfo(AKONADISERVER_LOG) << "Postgres cluster update: failed to remove" << oldDbDataCluster << "cluster (not an issue, continuing)";
    }

    return true;
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
        // Remove newline character
        pid.chop(1);
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
        // database performance. This only has effect when done on an empty directory,
        // so we call this before calling initdb.
        if (Utils::getDirectoryFileSystem(mPgData) == QLatin1String("btrfs")) {
            Utils::disableCoW(mPgData);
        }
#endif
        // call 'initdb --pgdata=/home/user/.local/share/akonadi/db_data'
        execute(mInitDbPath, { QStringLiteral("--pgdata=%1").arg(mPgData),
                               QStringLiteral("--locale=en_US.UTF-8") // TODO: check locale
                             });
    } else {
        const auto versions = checkPgVersion();
        if (versions.has_value() && (versions->clusterVersion < versions->pgServerVersion)) {
            qCInfo(AKONADISERVER_LOG) << "Cluster PG_VERSION is" << versions->clusterVersion << ", PostgreSQL server is version " << versions->pgServerVersion << ", will attempt to upgrade the cluster";
            if (upgradeCluster(versions->clusterVersion)) {
                qCInfo(AKONADISERVER_LOG) << "Successfully upgraded db cluster from Postgres" << versions->clusterVersion << "to" << versions->pgServerVersion;
            } else {
                qCWarning(AKONADISERVER_LOG) << "Postgres db cluster upgrade failed, Akonadi will fail to start. Sorry.";
            }
        }
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
    const QString command = mServerPath;
    QStringList arguments;
    arguments << QStringLiteral("status")
              << QStringLiteral("--pgdata=%1").arg(mPgData);

    QProcess pgCtl;
    pgCtl.start(command, arguments, QIODevice::ReadOnly);
    if (!pgCtl.waitForFinished(3000)) {
        // Error?
        return false;
    }

    // "pg_ctl status" exits with 0 when server is running and a non-zero code when not.
    return pgCtl.exitCode() == 0;
}
