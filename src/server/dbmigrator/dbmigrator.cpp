/*
    SPDX-FileCopyrightText: 2024 g10 Code GmbH
    SPDX-FileContributor: Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbmigrator.h"
#include "ControlManager.h"
#include "akonadidbmigrator_debug.h"
#include "akonadifull-version.h"
#include "akonadischema.h"
#include "akranges.h"
#include "entities.h"
#include "private/dbus_p.h"
#include "private/standarddirs_p.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/dbconfig.h"
#include "storage/dbconfigmysql.h"
#include "storage/dbconfigpostgresql.h"
#include "storage/dbconfigsqlite.h"
#include "storage/dbintrospector.h"
#include "storage/querybuilder.h"
#include "storage/schematypes.h"
#include "storage/transaction.h"
#include "storagejanitor.h"
#include "utils.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>

#include <KLocalizedString>

#include <chrono>
#include <qdbusconnection.h>

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;
using namespace std::chrono_literals;

Q_DECLARE_METATYPE(UIDelegate::Result);

namespace
{

constexpr size_t maxTransactionSize = 1000; // Arbitary guess

class MigratorDataStoreFactory : public DataStoreFactory
{
public:
    explicit MigratorDataStoreFactory(DbConfig *config)
        : m_dbConfig(config)
    {
    }

    DataStore *createStore() override
    {
        class MigratorDataStore : public DataStore
        {
        public:
            explicit MigratorDataStore(DbConfig *config)
                : DataStore(config)
            {
            }
        };
        return new MigratorDataStore(m_dbConfig);
    }

private:
    DbConfig *const m_dbConfig;
};

class Rollback
{
    Q_DISABLE_COPY_MOVE(Rollback)
public:
    Rollback() = default;
    ~Rollback()
    {
        run();
    }

    void reset()
    {
        mRollbacks.clear();
    }

    void run()
    {
        // Run rollbacks in reverse order!
        for (auto it = mRollbacks.rbegin(); it != mRollbacks.rend(); ++it) {
            (*it)();
        }
        mRollbacks.clear();
    }

    template<typename T>
    void add(T &&rollback)
    {
        mRollbacks.push_back(std::forward<T>(rollback));
    }

private:
    std::vector<std::function<void()>> mRollbacks;
};

void stopDatabaseServer(DbConfig *config)
{
    config->stopInternalServer();
}

bool createDatabase(DbConfig *config)
{
    auto db = QSqlDatabase::addDatabase(config->driverName(), QStringLiteral("initConnection"));
    config->apply(db);
    db.setDatabaseName(config->databaseName());
    if (!db.isValid()) {
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Invalid database object during initial database connection";
        return false;
    }

    const auto closeDb = qScopeGuard([&db]() {
        db.close();
    });

    // Try to connect to the database and select the Akonadi DB
    if (db.open()) {
        return true;
    }

    // That failed - the database might not exist yet..
    qCCritical(AKONADIDBMIGRATOR_LOG) << "Failed to use database" << config->databaseName();
    qCCritical(AKONADIDBMIGRATOR_LOG) << "Database error:" << db.lastError().text();
    qCDebug(AKONADIDBMIGRATOR_LOG) << "Trying to create database now...";

    db.close();
    db.setDatabaseName(QString());
    // Try to just connect to the DB server without selecting a database
    if (!db.open()) {
        // Failed, DB server not running or broken
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Failed to connect to database!";
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Database error:" << db.lastError().text();
        return false;
    }

    // Try to create the database
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("CREATE DATABASE %1").arg(config->databaseName()))) {
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Failed to create database";
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Query error:" << query.lastError().text();
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Database error:" << db.lastError().text();
        return false;
    }
    return true;
}

std::unique_ptr<DataStore> prepareDatabase(DbConfig *config)
{
    if (config->useInternalServer()) {
        if (!config->startInternalServer()) {
            qCCritical(AKONADIDBMIGRATOR_LOG) << "Failed to start the database server";
            return {};
        }
    } else {
        if (!createDatabase(config)) {
            return {};
        }
    }

    config->setup();

    auto factory = std::make_unique<MigratorDataStoreFactory>(config);
    std::unique_ptr<DataStore> store{factory->createStore()};
    if (!store->database().isOpen()) {
        return {};
    }
    if (!store->init()) {
        return {};
    }

    return store;
}

void cleanupDatabase(DataStore *store, DbConfig *config)
{
    store->close();

    stopDatabaseServer(config);
}

bool syncAutoIncrementValue(DataStore *sourceStore, DataStore *destStore, const TableDescription &table)
{
    const auto idCol = std::find_if(table.columns.begin(), table.columns.end(), [](const auto &col) {
        return col.isPrimaryKey;
    });
    if (idCol == table.columns.end()) {
        return false;
    }

    const auto getAutoIncrementValue = [](DataStore *store, const QString &table, const QString &idCol) -> std::optional<qint64> {
        const auto db = store->database();
        const auto introspector = DbIntrospector::createInstance(db);

        QSqlQuery query(db);
        if (!query.exec(introspector->getAutoIncrementValueQuery(table, idCol))) {
            qCCritical(AKONADIDBMIGRATOR_LOG) << query.lastError().text();
            return {};
        }
        if (!query.next()) {
            // SQLite returns an empty result set for empty tables, so we assume the table is empty and
            // the counter starts at 1
            return 1;
        }

        return query.value(0).toLongLong();
    };

    const auto setAutoIncrementValue = [](DataStore *store, const QString &table, const QString &idCol, qint64 seq) -> bool {
        const auto db = store->database();
        const auto introspector = DbIntrospector::createInstance(db);

        QSqlQuery query(db);
        if (!query.exec(introspector->updateAutoIncrementValueQuery(table, idCol, seq))) {
            qCritical(AKONADIDBMIGRATOR_LOG) << query.lastError().text();
            return false;
        }

        return true;
    };

    const auto seq = getAutoIncrementValue(sourceStore, table.name, idCol->name);
    if (!seq) {
        return false;
    }

    return setAutoIncrementValue(destStore, table.name, idCol->name, *seq);
}

bool analyzeTable(const QString &table, DataStore *store)
{
    auto dbType = DbType::type(store->database());
    QString queryStr;
    switch (dbType) {
    case DbType::Sqlite:
    case DbType::PostgreSQL:
        queryStr = QLatin1StringView("ANALYZE ") + table;
        break;
    case DbType::MySQL:
        queryStr = QLatin1StringView("ANALYZE TABLE ") + table;
        break;
    case DbType::Unknown:
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Unknown database type";
        return false;
    }

    QSqlQuery query(store->database());
    if (!query.exec(queryStr)) {
        qCCritical(AKONADIDBMIGRATOR_LOG) << query.lastError().text();
        return false;
    }

    return true;
}

QString createTmpAkonadiServerRc(const QString &targetEngine)
{
    const auto origFileName = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
    const auto newFileName = QStringLiteral("%1.new").arg(origFileName);

    QSettings settings(newFileName, QSettings::IniFormat);
    settings.setValue(QStringLiteral("General/Driver"), targetEngine);

    return newFileName;
}

QString driverFromEngineName(const QString &engine)
{
    const auto enginelc = engine.toLower();
    if (enginelc == QLatin1StringView("sqlite")) {
        return QStringLiteral("QSQLITE");
    }
    if (enginelc == QLatin1StringView("mysql")) {
        return QStringLiteral("QMYSQL");
    }
    if (enginelc == QLatin1StringView("postgres")) {
        return QStringLiteral("QPSQL");
    }

    qCCritical(AKONADIDBMIGRATOR_LOG) << "Invalid engine:" << engine;
    return {};
}

std::unique_ptr<DbConfig> dbConfigFromServerRc(const QString &configFile, bool overrideDbPath = false)
{
    QSettings settings(configFile, QSettings::IniFormat);
    const auto driver = settings.value(QStringLiteral("General/Driver")).toString();
    std::unique_ptr<DbConfig> config;
    QString dbPathSuffix;
    if (driver == QLatin1StringView("QSQLITE") || driver == QLatin1StringView("QSQLITE3")) {
        config = std::make_unique<DbConfigSqlite>(configFile);
    } else if (driver == QLatin1StringView("QMYSQL")) {
        config = std::make_unique<DbConfigMysql>(configFile);
        dbPathSuffix = QStringLiteral("/db_data");
    } else if (driver == QLatin1StringView("QPSQL")) {
        config = std::make_unique<DbConfigPostgresql>(configFile);
        dbPathSuffix = QStringLiteral("/db_data");
    } else {
        qCCritical(AKONADIDBMIGRATOR_LOG) << "Invalid driver: " << driver;
        return {};
    }

    const auto dbPath = overrideDbPath ? StandardDirs::saveDir("data", QStringLiteral("db_migration%1").arg(dbPathSuffix)) : QString{};
    config->init(settings, true, dbPath);

    return config;
}

bool isValidDbPath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }

    const QFileInfo fi(path);
    return fi.exists() && (fi.isFile() || fi.isDir());
}

std::error_code restoreDatabaseFromBackup(const QString &backupPath, const QString &originalPath)
{
    std::error_code ec;
    if (QFileInfo(originalPath).exists()) {
        // Remove the original path, it could have been created by the new db
        std::filesystem::remove_all(originalPath.toStdString(), ec);
        return ec;
    }

    // std::filesystem doesn't care whether it's file or directory, unlike QFile vs QDir.
    std::filesystem::rename(backupPath.toStdString(), originalPath.toStdString(), ec);
    return ec;
}

bool akonadiIsRunning()
{
    auto sessionIface = QDBusConnection::sessionBus().interface();
    return sessionIface->isServiceRegistered(DBus::serviceName(DBus::ControlLock)) || sessionIface->isServiceRegistered(DBus::serviceName(DBus::Server));
}

bool stopAkonadi()
{
    static constexpr auto shutdownTimeout = 5s;

    org::freedesktop::Akonadi::ControlManager manager(DBus::serviceName(DBus::Control), QStringLiteral("/ControlManager"), QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        return false;
    }

    manager.shutdown();

    QElapsedTimer timer;
    timer.start();
    while (akonadiIsRunning() && timer.durationElapsed() <= shutdownTimeout) {
        QThread::msleep(100);
    }

    return timer.durationElapsed() <= shutdownTimeout && !akonadiIsRunning();
}

bool startAkonadi()
{
    QDBusConnection::sessionBus().interface()->startService(DBus::serviceName(DBus::Control));
    return true;
}

bool acquireAkonadiLock()
{
    auto connIface = QDBusConnection::sessionBus().interface();
    auto reply = connIface->registerService(DBus::serviceName(DBus::ControlLock));
    if (!reply.isValid() || reply != QDBusConnectionInterface::ServiceRegistered) {
        return false;
    }

    reply = connIface->registerService(DBus::serviceName(DBus::UpgradeIndicator));
    if (!reply.isValid() || reply != QDBusConnectionInterface::ServiceRegistered) {
        return false;
    }

    return true;
}

bool releaseAkonadiLock()
{
    auto connIface = QDBusConnection::sessionBus().interface();
    connIface->unregisterService(DBus::serviceName(DBus::ControlLock));
    connIface->unregisterService(DBus::serviceName(DBus::UpgradeIndicator));
    return true;
}

} // namespace

DbMigrator::DbMigrator(const QString &targetEngine, UIDelegate *delegate, QObject *parent)
    : QObject(parent)
    , m_targetEngine(targetEngine)
    , m_uiDelegate(delegate)
{
    qRegisterMetaType<UIDelegate::Result>();
}

DbMigrator::~DbMigrator()
{
    if (m_thread) {
        m_thread->wait();
    }
}

void DbMigrator::startMigration()
{
    m_thread.reset(QThread::create([this]() {
        bool restartAkonadi = false;
        if (akonadiIsRunning()) {
            emitInfo(i18nc("@info:status", "Stopping Akonadi service..."));
            restartAkonadi = true;
            if (!stopAkonadi()) {
                emitError(i18nc("@info:status", "Error: timeout while waiting for Akonadi to stop."));
                emitCompletion(false);
                return;
            }
        }

        if (!acquireAkonadiLock()) {
            emitError(i18nc("@info:status", "Error: couldn't acquire DBus lock for Akonadi."));
            emitCompletion(false);
            return;
        }

        const bool result = runMigrationThread();

        releaseAkonadiLock();
        if (restartAkonadi) {
            emitInfo(i18nc("@info:status", "Starting Akonadi service..."));
            startAkonadi();
        }

        emitCompletion(result);
    }));
    m_thread->start();
}

bool DbMigrator::runStorageJanitor(DbConfig *dbConfig)
{
    StorageJanitor janitor(dbConfig);
    connect(&janitor, &StorageJanitor::done, this, [this]() {
        emitInfo(i18nc("@info:status", "Database fsck completed"));
    });
    // Runs the janitor in the current thread
    janitor.check();

    return true;
}

bool DbMigrator::runMigrationThread()
{
    Rollback rollback;

    const auto driver = driverFromEngineName(m_targetEngine);
    if (driver.isEmpty()) {
        emitError(i18nc("@info:status", "Invalid database engine \"%1\" - valid values are \"sqlite\", \"mysql\" and \"postgres\".", m_targetEngine));
        return false;
    }

    // Create backup akonadiserverrc
    const auto sourceServerCfgFile = backupAkonadiServerRc();
    if (!sourceServerCfgFile) {
        return false;
    }
    rollback.add([file = *sourceServerCfgFile]() {
        QFile::remove(file);
    });

    // Create new akonadiserverrc with the new engine configuration
    const QString destServerCfgFile = createTmpAkonadiServerRc(driver);
    rollback.add([destServerCfgFile]() {
        QFile::remove(destServerCfgFile);
    });

    // Create DbConfig for the source DB
    auto sourceConfig = dbConfigFromServerRc(*sourceServerCfgFile);
    if (!sourceConfig) {
        emitError(i18nc("@info:shell", "Error: failed to configure source database server."));
        return false;
    }

    if (sourceConfig->driverName() == driver) {
        emitError(i18nc("@info:shell", "Source and destination database engines are the same."));
        return false;
    }

    // Check that we actually have valid source datbase path
    const auto sourceDbPath = sourceConfig->databasePath();
    if (!isValidDbPath(sourceDbPath)) {
        emitError(i18nc("@info:shell", "Error: failed to obtain path to source database data file or directory."));
        return false;
    }

    // Configure the new DB server
    auto destConfig = dbConfigFromServerRc(destServerCfgFile, /* overrideDbPath=*/true);
    if (!destConfig) {
        emitError(i18nc("@info:shell", "Error: failed to configure the new database server."));
        return false;
    }

    auto sourceStore = prepareDatabase(sourceConfig.get());
    if (!sourceStore) {
        emitError(i18nc("@info:shell", "Error: failed to open existing database to migrate data from."));
        return false;
    }
    auto destStore = prepareDatabase(destConfig.get());
    if (!destStore) {
        emitError(i18nc("@info:shell", "Error: failed to open new database to migrate data to."));
        return false;
    }

    // Run StorageJanitor on the existing database to ensure it's in a consistent state
    emitInfo(i18nc("@info:status", "Running fsck on the source database"));
    runStorageJanitor(sourceConfig.get());

    const bool migrationSuccess = migrateTables(sourceStore.get(), destStore.get(), destConfig.get());

    // Stop database servers and close connections. Make sure we always reach here, even if the migration failed
    cleanupDatabase(sourceStore.get(), sourceConfig.get());
    cleanupDatabase(destStore.get(), destConfig.get());

    if (!migrationSuccess) {
        return false;
    }

    // Remove the migrated database if the migration or post-migration steps fail
    rollback.add([this, dbPath = destConfig->databasePath()]() {
        std::error_code ec;
        std::filesystem::remove_all(dbPath.toStdString(), ec);
        if (ec) {
            emitError(i18nc("@info:status %2 is error message",
                            "Error: failed to remove temporary database directory %1: %2",
                            dbPath,
                            QString::fromStdString(ec.message())));
        }
    });

    // Move the old database into backup location
    emitInfo(i18nc("@info:shell", "Backing up original database..."));
    const auto backupPath = moveDatabaseToBackupLocation(sourceConfig.get());
    if (!backupPath.has_value()) {
        return false;
    }

    if (!backupPath->isEmpty()) {
        rollback.add([this, backupPath = *backupPath, sourceDbPath]() {
            emitInfo(i18nc("@info:status", "Restoring database from backup %1 to %2", backupPath, sourceDbPath));
            if (const auto ec = restoreDatabaseFromBackup(backupPath, sourceDbPath); ec) {
                emitError(i18nc("@info:status %1 is error message", "Error: failed to restore database from backup: %1", QString::fromStdString(ec.message())));
            }
        });
    }

    // Move the new database to the main location
    if (!moveDatabaseToMainLocation(destConfig.get(), destServerCfgFile)) {
        return false;
    }

    // Migration was success, nothing to roll back.
    rollback.reset();

    return true;
}

bool DbMigrator::migrateTables(DataStore *sourceStore, DataStore *destStore, DbConfig *destConfig)
{
    // Disable foreign key constraint checks
    if (!destConfig->disableConstraintChecks(destStore->database())) {
        return false;
    }

    AkonadiSchema schema;
    const int totalTables = schema.tables().size() + schema.relations().size();
    int doneTables = 0;

    // Copy regular tables
    for (const auto &table : schema.tables()) {
        ++doneTables;
        emitProgress(table.name, doneTables, totalTables);
        if (!copyTable(sourceStore, destStore, table)) {
            emitError(i18nc("@info:shell", "Error has occurred while migrating table %1", table.name));
            return false;
        }
    }

    // Copy relational tables
    for (const auto &relation : schema.relations()) {
        const RelationTableDescription table{relation};
        ++doneTables;
        emitProgress(table.name, doneTables, totalTables);
        if (!copyTable(sourceStore, destStore, table)) {
            emitError(i18nc("@info:shell", "Error has occurred while migrating table %1", table.name));
            return false;
        }
    }

    // Re-enable foreign key constraint checks
    if (!destConfig->enableConstraintChecks(destStore->database())) {
        return false;
    }

    return true;
}

std::optional<QString> DbMigrator::moveDatabaseToBackupLocation(DbConfig *config)
{
    const std::filesystem::path dbPath = config->databasePath().toStdString();

    QDir backupDir = StandardDirs::saveDir("data", QStringLiteral("migration_backup"));
    if (!backupDir.isEmpty()) {
        const auto result = questionYesNoSkip(i18nc("@label", "Backup directory already exists. Do you want to overwrite the previous backup?"));
        if (result == UIDelegate::Result::Skip) {
            return QString{};
        }
        if (result == UIDelegate::Result::No) {
            emitError(i18nc("@info:shell", "Cannot proceed without backup. Migration interrupted."));
            return {};
        }

        if (!backupDir.removeRecursively()) {
            emitError(i18nc("@info:shell", "Failed to remove previous backup directory."));
            return {};
        }
    }

    backupDir.mkpath(QStringLiteral("."));

    std::error_code ec;
    std::filesystem::path backupPath = backupDir.absolutePath().toStdString();
    // /path/to/akonadi.sql -> /path/to/backup/akonadi.sql
    // /path/to/db_data -> /path/to/backup/db_data
    std::filesystem::rename(dbPath, backupPath / *(--dbPath.end()), ec);
    if (ec) {
        emitError(i18nc("@info:shell", "Failed to move database to backup location: %1", QString::fromStdString(ec.message())));
        return {};
    }

    return backupDir.absolutePath();
}

std::optional<QString> DbMigrator::backupAkonadiServerRc()
{
    const auto origFileName = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
    const auto bkpFileName = QStringLiteral("%1.bkp").arg(origFileName);
    std::error_code ec;

    if (QFile::exists(bkpFileName)) {
        const auto result = questionYesNo(i18nc("@label", "Backup file %1 already exists. Overwrite?", bkpFileName));
        if (result != UIDelegate::Result::Yes) {
            emitError(i18nc("@info:status", "Cannot proceed without backup. Migration interrupted."));
            return {};
        }

        std::filesystem::remove(bkpFileName.toStdString(), ec);
        if (ec) {
            emitError(i18nc("@info:status", "Error: Failed to remove existing backup file %1: %2", bkpFileName, QString::fromStdString(ec.message())));
            return {};
        }
    }

    std::filesystem::copy(origFileName.toStdString(), bkpFileName.toStdString(), ec);
    if (ec) {
        emitError(i18nc("@info:status", "Failed to back up Akonadi Server configuration: %1", QString::fromStdString(ec.message())));
        return {};
    }

    return bkpFileName;
}

bool DbMigrator::moveDatabaseToMainLocation(DbConfig *destConfig, const QString &destServerCfgFile)
{
    std::error_code ec;
    // /path/to/db_migration/akonadi.db -> /path/to/akonadi.db
    // /path/to/db_migration/db_data -> /path/to/db_data
    const std::filesystem::path dbSrcPath = destConfig->databasePath().toStdString();
    const auto dbDestPath = dbSrcPath.parent_path().parent_path() / *(--dbSrcPath.end());
    std::filesystem::rename(dbSrcPath, dbDestPath, ec);
    if (ec) {
        emitError(i18nc("@info:status %1 is error message",
                        "Error: failed to move migrated database to the primary location: %1",
                        QString::fromStdString(ec.message())));
        return false;
    }

    // Adjust the db path stored in new akonadiserverrc to point to the primary location
    {
        QSettings settings(destServerCfgFile, QSettings::IniFormat);
        destConfig->setDatabasePath(QString::fromStdString(dbDestPath.string()), settings);
    }

    // Remove the - now empty - db_migration directory
    // We don't concern ourselves too much with this failing.
    std::filesystem::remove(dbSrcPath.parent_path(), ec);

    // Turn the new temporary akonadiserverrc int othe main one so that
    // Akonadi starts with the new DB configuration.
    std::filesystem::remove(StandardDirs::serverConfigFile(StandardDirs::ReadWrite).toStdString(), ec);
    if (ec) {
        emitError(i18nc("@info:status %1 is error message", "Error: failed to remove original akonadiserverrc: %1", QString::fromStdString(ec.message())));
        return false;
    }
    std::filesystem::rename(destServerCfgFile.toStdString(), StandardDirs::serverConfigFile(StandardDirs::ReadWrite).toStdString(), ec);
    if (ec) {
        emitError(i18nc("@info:status %1 is error message",
                        "Error: failed to move new akonadiserverrc to the primary location: %1",
                        QString::fromStdString(ec.message())));
        return false;
    }

    return true;
}

bool DbMigrator::copyTable(DataStore *sourceStore, DataStore *destStore, const TableDescription &table)
{
    const auto columns = table.columns | Views::transform([](const auto &tbl) {
                             return tbl.name;
                         })
        | Actions::toQList;

    // Count number of items in the current table
    const auto totalRows = [](DataStore *store, const QString &table) {
        CountQueryBuilder countQb(store, table);
        countQb.exec();
        return countQb.result();
    }(sourceStore, table.name);

    // Fetch *everything* from the current able
    QueryBuilder sourceQb(sourceStore, table.name);
    sourceQb.addColumns(columns);
    sourceQb.exec();
    auto sourceQuery = sourceQb.query();

    // Clean the destination table (from data pre-inserted by DbInitializer)
    {
        QueryBuilder clearQb(destStore, table.name, QueryBuilder::Delete);
        clearQb.exec();
    }

    // Begin insertion transaction
    Transaction transaction(destStore, QStringLiteral("Migrator"));
    size_t trxSize = 0;
    size_t processed = 0;

    // Loop over source resluts
    while (sourceQuery.next()) {
        // Insert the current row into the new database
        QueryBuilder destQb(destStore, table.name, QueryBuilder::Insert);
        destQb.setIdentificationColumn({});
        for (int col = 0; col < table.columns.size(); ++col) {
            QVariant value;
            if (table.columns[col].type == QLatin1StringView("QDateTime")) {
                value = sourceQuery.value(col).toDateTime();
            } else if (table.columns[col].type == QLatin1StringView("bool")) {
                value = sourceQuery.value(col).toBool();
            } else if (table.columns[col].type == QLatin1StringView("QByteArray")) {
                value = Utils::variantToByteArray(sourceQuery.value(col));
            } else if (table.columns[col].type == QLatin1StringView("QString")) {
                value = Utils::variantToString(sourceQuery.value(col));
            } else {
                value = sourceQuery.value(col);
            }
            destQb.setColumnValue(table.columns[col].name, value);
        }
        if (!destQb.exec()) {
            qCWarning(AKONADIDBMIGRATOR_LOG) << "Failed to insert row into table" << table.name << ":" << destQb.query().lastError().text();
            return false;
        }

        // Commit the transaction after every "maxTransactionSize" inserts to make it reasonably fast
        if (++trxSize > maxTransactionSize) {
            if (!transaction.commit()) {
                qCWarning(AKONADIDBMIGRATOR_LOG) << "Failed to commit transaction:" << destStore->database().lastError().text();
                return false;
            }
            trxSize = 0;
            transaction.begin();
        }

        emitTableProgress(table.name, ++processed, totalRows);
    }

    // Commit whatever is left in the transaction
    if (!transaction.commit()) {
        qCWarning(AKONADIDBMIGRATOR_LOG) << "Failed to commit transaction:" << destStore->database().lastError().text();
        return false;
    }

    // Synchronize next autoincrement value (if the table has one)
    if (const auto cnt = std::count_if(table.columns.begin(),
                                       table.columns.end(),
                                       [](const auto &col) {
                                           return col.isAutoIncrement;
                                       });
        cnt == 1) {
        if (!syncAutoIncrementValue(sourceStore, destStore, table)) {
            emitError(i18nc("@info:status", "Error: failed to update autoincrement value for table %1", table.name));
            return false;
        }
    }

    emitInfo(i18nc("@info:status", "Optimizing table %1...", table.name));
    // Optimize the new table
    if (!analyzeTable(table.name, destStore)) {
        emitError(i18nc("@info:status", "Error: failed to optimize table %1", table.name));
        return false;
    }

    return true;
}

void DbMigrator::emitInfo(const QString &message)
{
    QMetaObject::invokeMethod(
        this,
        [this, message]() {
            Q_EMIT info(message);
        },
        Qt::QueuedConnection);
}

void DbMigrator::emitError(const QString &message)
{
    QMetaObject::invokeMethod(
        this,
        [this, message]() {
            Q_EMIT error(message);
        },
        Qt::QueuedConnection);
}

void DbMigrator::emitProgress(const QString &table, int done, int total)
{
    QMetaObject::invokeMethod(
        this,
        [this, table, done, total]() {
            Q_EMIT progress(table, done, total);
        },
        Qt::QueuedConnection);
}

void DbMigrator::emitTableProgress(const QString &table, int done, int total)
{
    QMetaObject::invokeMethod(
        this,
        [this, table, done, total]() {
            Q_EMIT tableProgress(table, done, total);
        },
        Qt::QueuedConnection);
}

void DbMigrator::emitCompletion(bool success)
{
    QMetaObject::invokeMethod(
        this,
        [this, success]() {
            Q_EMIT migrationCompleted(success);
        },
        Qt::QueuedConnection);
}

UIDelegate::Result DbMigrator::questionYesNo(const QString &question)
{
    UIDelegate::Result answer;
    QMetaObject::invokeMethod(
        this,
        [this, question, &answer]() {
            if (m_uiDelegate) {
                answer = m_uiDelegate->questionYesNo(question);
            }
        },
        Qt::BlockingQueuedConnection);
    return answer;
}

UIDelegate::Result DbMigrator::questionYesNoSkip(const QString &question)
{
    UIDelegate::Result answer;
    QMetaObject::invokeMethod(
        this,
        [this, question, &answer]() {
            if (m_uiDelegate) {
                answer = m_uiDelegate->questionYesNoSkip(question);
            }
        },
        Qt::BlockingQueuedConnection);
    return answer;
}
#include "moc_dbmigrator.cpp"
