/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbconfig.h"
#include <QObject>
#include <QProcess>

class QSqlDatabase;

namespace Akonadi
{
namespace Server
{
class DbConfigMysql : public QObject, public DbConfig
{
    Q_OBJECT

public:
    /**
     * Constructs a new DbConfig for MySQL reading configuration from the standard akonadiserverrc config file.
     */
    explicit DbConfigMysql() = default;
    /**
     * Constructs a new DbConfig for MySQL reading configuration from the @p configFile.
     */
    explicit DbConfigMysql(const QString &configFile);

    /**
     * Destructor.
     */
    ~DbConfigMysql() override;

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
     * This method checks if the requirements for this database connection are met
     * in the system (QMYSQL driver is available, mysqld binary is found, etc.).
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
    void initSession(const QSqlDatabase &database) override;

    /// reimpl
    bool disableConstraintChecks(const QSqlDatabase &db) override;

    /// reimpl
    bool enableConstraintChecks(const QSqlDatabase &db) override;

private Q_SLOTS:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    int parseCommandLineToolsVersion() const;

    bool initializeMariaDBDatabase(const QString &confFile, const QString &dataDir) const;
    bool initializeMySQL5_7_6Database(const QString &confFile, const QString &dataDir) const;
    bool initializeMySQLDatabase(const QString &confFile, const QString &dataDir) const;

    QString mDatabaseName;
    QString mHostName;
    QString mUserName;
    QString mPassword;
    QString mConnectionOptions;
    QString mMysqldPath;
    QString mCleanServerShutdownCommand;
    QString mMysqlInstallDbPath;
    QString mMysqlCheckPath;
    QString mMysqlUpgradePath;
    bool mInternalServer = true;
    std::unique_ptr<QProcess> mDatabaseProcess;
};

} // namespace Server
} // namespace Akonadi
