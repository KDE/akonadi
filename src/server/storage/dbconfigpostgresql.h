/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbconfig.h"

#include <optional>

namespace Akonadi
{
namespace Server
{
class DbConfigPostgresql : public DbConfig
{
public:
    /**
     * Constructs a new DbConfig for PostgreSQL reading configuration from the standard akonadiserverrc config file.
     */
    explicit DbConfigPostgresql() = default;

    /**
     * Constructs a new DbConfig for PostgreSQL reading configuration from the @p configFile.
     */
    explicit DbConfigPostgresql(const QString &configFile);

    /**
     * Returns the name of the used driver.
     */
    QString driverName() const override;

    /**
     * Returns the database name.
     */
    QString databaseName() const override;

    /**
     * This method is called whenever the Akonadi server is started
     * and before the initial database connection is set up.
     *
     * At this point the default settings should be determined, merged
     * with the given @p settings and written back if @p storeSettings is true.
     */
    bool init(QSettings &settings, bool storeSettings = true) override;

    /**
     * This method checks if the requirements for this database connection are
     * met in the system (QPOSTGRESQL driver is available, postgresql binary is
     * found, etc.).
     */
    bool isAvailable(QSettings &settings) override;

    /**
     * This method applies the configured settings to the QtSql @p database
     * instance.
     */
    void apply(QSqlDatabase &database) override;

    /**
     * Returns whether an internal server needs to be used.
     */
    bool useInternalServer() const override;

    /**
     * This method is called to start an external server.
     */
    bool startInternalServer() override;

    /**
     * This method is called to stop the external server.
     */
    void stopInternalServer() override;

    /// reimpl
    bool disableConstraintChecks(const QSqlDatabase &db) override;

    /// reimpl
    bool enableConstraintChecks(const QSqlDatabase &db) override;

protected:
    QStringList postgresSearchPaths(const QString &versionedPath) const;

private:
    struct Versions {
        int clusterVersion = 0;
        int pgServerVersion = 0;
    };
    std::optional<Versions> checkPgVersion() const;
    bool upgradeCluster(int clusterVersion);
    bool runInitDb(const QString &dbDataPath);

    bool checkServerIsRunning();

    QString mDatabaseName;
    QString mHostName;
    int mHostPort = 0;
    QString mUserName;
    QString mPassword;
    QString mConnectionOptions;
    QString mServerPath;
    QString mInitDbPath;
    QString mPgData;
    QString mPgUpgradePath;
    bool mInternalServer = true;
};

} // namespace Server
} // namespace Akonadi
