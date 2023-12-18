/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbconfigsqlite.h"
#include "akonadiserver_debug.h"
#include "utils.h"

#include <private/standarddirs_p.h>

#include <QDir>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

using namespace Akonadi;
using namespace Akonadi::Server;

static QString dataDir()
{
    QString akonadiHomeDir = StandardDirs::saveDir("data");
    if (!QDir(akonadiHomeDir).exists()) {
        if (!QDir().mkpath(akonadiHomeDir)) {
            qCCritical(AKONADISERVER_LOG) << "Unable to create" << akonadiHomeDir << "during database initialization";
            return QString();
        }
    }

    akonadiHomeDir += QDir::separator();

    return akonadiHomeDir;
}

static QString sqliteDataFile()
{
    const QString dir = dataDir();
    if (dir.isEmpty()) {
        return QString();
    }
    const QString akonadiPath = dir + QLatin1String("akonadi.db");
    if (!QFile::exists(akonadiPath)) {
        QFile file(akonadiPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCCritical(AKONADISERVER_LOG) << "Unable to create file" << akonadiPath << "during database initialization.";
            return QString();
        }
        file.close();
    }

    return akonadiPath;
}

DbConfigSqlite::DbConfigSqlite(const QString &configFile)
    : DbConfig(configFile)
{
}

QString DbConfigSqlite::driverName() const
{
    return QStringLiteral("QSQLITE");
}

QString DbConfigSqlite::databaseName() const
{
    return mDatabaseName;
}

bool DbConfigSqlite::init(QSettings &settings, bool storeSettings)
{
    // determine default settings depending on the driver
    const QString defaultDbName = sqliteDataFile();
    if (defaultDbName.isEmpty()) {
        return false;
    }

    // read settings for current driver
    settings.beginGroup(driverName());
    mDatabaseName = settings.value(QStringLiteral("Name"), defaultDbName).toString();
    mHostName = settings.value(QStringLiteral("Host")).toString();
    mUserName = settings.value(QStringLiteral("User")).toString();
    mPassword = settings.value(QStringLiteral("Password")).toString();
    mConnectionOptions = settings.value(QStringLiteral("Options")).toString();
    settings.endGroup();

    if (storeSettings) {
        // store back the default values
        settings.beginGroup(driverName());
        settings.setValue(QStringLiteral("Name"), mDatabaseName);
        settings.endGroup();
        settings.sync();
    }

    return true;
}

bool DbConfigSqlite::isAvailable(QSettings &settings)
{
    if (!QSqlDatabase::drivers().contains(driverName())) {
        return false;
    }

    if (!init(settings, false)) {
        return false;
    }

    return true;
}

void DbConfigSqlite::apply(QSqlDatabase &database)
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

bool DbConfigSqlite::useInternalServer() const
{
    return false;
}

bool DbConfigSqlite::setPragma(QSqlDatabase &db, QSqlQuery &query, const QString &pragma)
{
    if (!query.exec(QStringLiteral("PRAGMA %1").arg(pragma))) {
        qCCritical(AKONADISERVER_LOG) << "Could not set sqlite PRAGMA " << pragma;
        qCCritical(AKONADISERVER_LOG) << "Database: " << mDatabaseName;
        qCCritical(AKONADISERVER_LOG) << "Query error: " << query.lastError().text();
        qCCritical(AKONADISERVER_LOG) << "Database error: " << db.lastError().text();
        return false;
    }
    return true;
}

void DbConfigSqlite::setup()
{
    const QLatin1String connectionName("initConnectionSqlite");

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(driverName(), connectionName);

        if (!db.isValid()) {
            qCCritical(AKONADISERVER_LOG) << "Invalid database for" << mDatabaseName << "with driver" << driverName();
            return;
        }

        QFileInfo finfo(mDatabaseName);
        if (!finfo.dir().exists()) {
            QDir dir;
            dir.mkpath(finfo.path());
        }

#ifdef Q_OS_LINUX
        QFile dbFile(mDatabaseName);
        // It is recommended to disable CoW feature when running on Btrfs to improve
        // database performance. It does not have any effect on non-empty files, so
        // we check, whether the database has not yet been initialized.
        if (dbFile.size() == 0) {
            if (Utils::getDirectoryFileSystem(mDatabaseName) == QLatin1String("btrfs")) {
                Utils::disableCoW(mDatabaseName);
            }
        }
#endif

        db.setDatabaseName(mDatabaseName);
        if (!db.open()) {
            qCCritical(AKONADISERVER_LOG) << "Could not open sqlite database" << mDatabaseName << "with driver" << driverName() << "for initialization";
            db.close();
            return;
        }

        apply(db);

        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("SELECT sqlite_version()"))) {
            qCCritical(AKONADISERVER_LOG) << "Could not query sqlite version";
            qCCritical(AKONADISERVER_LOG) << "Database: " << mDatabaseName;
            qCCritical(AKONADISERVER_LOG) << "Query error: " << query.lastError().text();
            qCCritical(AKONADISERVER_LOG) << "Database error: " << db.lastError().text();
            db.close();
            return;
        }

        if (!query.next()) { // should never occur
            qCCritical(AKONADISERVER_LOG) << "Could not query sqlite version";
            qCCritical(AKONADISERVER_LOG) << "Database: " << mDatabaseName;
            qCCritical(AKONADISERVER_LOG) << "Query error: " << query.lastError().text();
            qCCritical(AKONADISERVER_LOG) << "Database error: " << db.lastError().text();
            db.close();
            return;
        }

        const QString sqliteVersion = query.value(0).toString();
        qCDebug(AKONADISERVER_LOG) << "sqlite version is " << sqliteVersion;

        const QStringList list = sqliteVersion.split(QLatin1Char('.'));
        const int sqliteVersionMajor = list[0].toInt();
        const int sqliteVersionMinor = list[1].toInt();

        // set synchronous mode to NORMAL; see http://www.sqlite.org/pragma.html#pragma_synchronous
        if (!setPragma(db, query, QStringLiteral("synchronous=1"))) {
            db.close();
            return;
        }

        if (sqliteVersionMajor < 3 && sqliteVersionMinor < 7) {
            // wal mode is only supported with >= sqlite 3.7.0
            db.close();
            return;
        }

        // set write-ahead-log mode; see http://www.sqlite.org/wal.html
        if (!setPragma(db, query, QStringLiteral("journal_mode=wal"))) {
            db.close();
            return;
        }

        if (!query.next()) { // should never occur
            qCCritical(AKONADISERVER_LOG) << "Could not query sqlite journal mode";
            qCCritical(AKONADISERVER_LOG) << "Database: " << mDatabaseName;
            qCCritical(AKONADISERVER_LOG) << "Query error: " << query.lastError().text();
            qCCritical(AKONADISERVER_LOG) << "Database error: " << db.lastError().text();
            db.close();
            return;
        }

        const QString journalMode = query.value(0).toString();
        qCDebug(AKONADISERVER_LOG) << "sqlite journal mode is " << journalMode;

        // as of sqlite 3.12 this is default, previously was 1024.
        if (!setPragma(db, query, QStringLiteral("page_size=4096"))) {
            db.close();
            return;
        }

        // set cache_size to 100000 pages; see https://www.sqlite.org/pragma.html#pragma_cache_size
        if (!setPragma(db, query, QStringLiteral("cache_size=100000"))) {
            db.close();
            return;
        }

        // construct temporary tables in memory; see https://www.sqlite.org/pragma.html#pragma_temp_store
        if (!setPragma(db, query, QStringLiteral("temp_store=MEMORY"))) {
            db.close();
            return;
        }

        // enable foreign key support; see https://www.sqlite.org/pragma.html#pragma_foreign_keys
        if (!setPragma(db, query, QStringLiteral("foreign_keys=ON"))) {
            db.close();
            return;
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

bool DbConfigSqlite::disableConstraintChecks(const QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(QStringLiteral("PRAGMA ignore_check_constraints=ON"));
}

bool DbConfigSqlite::enableConstraintChecks(const QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(QStringLiteral("PRAGMA ignore_check_constraints=OFF"));
}
