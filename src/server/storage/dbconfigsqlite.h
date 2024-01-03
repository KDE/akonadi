/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbconfig.h"

namespace Akonadi
{
namespace Server
{
class DbConfigSqlite : public DbConfig
{
public:
    /**
     * Constructs a new DbConfig for SQLite reading configuration from the standard akonadiserverrc config file.
     */
    explicit DbConfigSqlite() = default;

    /**
     * Constructs a new DbConfig for MySQL reading configuration from the @p configFile.
     */
    explicit DbConfigSqlite(const QString &configFile);

    /**
     * Returns the name of the used driver.
     */
    QString driverName() const override;

    /**
     * Returns the database name.
     */
    QString databaseName() const override;

    /**
     * Returns path to the database file or directory.
     */
    QString databasePath() const override;

    /**
     * Sets path to the database file or directory.
     */
    void setDatabasePath(const QString &path, QSettings &settings) override;

    /**
     * This method is called whenever the Akonadi server is started
     * and before the initial database connection is set up.
     *
     * At this point the default settings should be determined, merged
     * with the given @p settings and written back if @p storeSettings is true.
     */
    bool init(QSettings &settings, bool storeSettings = true, const QString &dbPathOverride = {}) override;

    /**
     * This method checks if the requirements for this database connection are met
     * in the system (QSQLITE driver is available, object can be initialized, etc.).
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
     * Sets sqlite journal mode to WAL and synchronous mode to NORMAL
     */
    void setup() override;

    /// reimpl
    bool disableConstraintChecks(const QSqlDatabase &db) override;

    /// reimpl
    bool enableConstraintChecks(const QSqlDatabase &db) override;

private:
    bool setPragma(QSqlDatabase &db, QSqlQuery &query, const QString &pragma);

    QString mDatabaseName;
    QString mHostName;
    QString mUserName;
    QString mPassword;
    QString mConnectionOptions;
};

} // namespace Server
} // namespace Akonadi
