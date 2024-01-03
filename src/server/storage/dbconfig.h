/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSettings>
#include <QSqlDatabase>

namespace Akonadi
{
namespace Server
{
/**
 * A base class that provides an unique access layer to configuration
 * and initialization of different database backends.
 */
class DbConfig
{
public:
    virtual ~DbConfig();

    /**
     * Returns whether database have been configured.
     */
    static bool isConfigured();

    /**
     * Returns the DbConfig instance for the database the user has
     * configured.
     */
    static DbConfig *configuredDatabase();

    /**
     * Returns the name of the used driver.
     */
    virtual QString driverName() const = 0;

    /**
     * Returns the database name.
     */
    virtual QString databaseName() const = 0;

    /**
     * Returns path to the database file or directory.
     */
    virtual QString databasePath() const = 0;

    /**
     * Set the path to the database file or directory.
     */
    virtual void setDatabasePath(const QString &path, QSettings &settings) = 0;

    /**
     * This method is called whenever the Akonadi server is started
     * and before the initial database connection is set up.
     *
     * At this point the default settings should be determined, merged
     * with the given @p settings and written back if @p storeSettings is true.
     *
     * When overrideDbPath is specified, the database will be stored inside this
     * directory instead of the default location.
     */
    virtual bool init(QSettings &settings, bool storeSettings = true, const QString &overrideDbPath = {}) = 0;

    /**
     * This method checks if the requirements for this database connection are met
     * in the system (i.e. QMYSQL driver is available, mysqld binary is found, etc.).
     */
    virtual bool isAvailable(QSettings &settings) = 0;

    /**
     * This method applies the configured settings to the QtSql @p database
     * instance.
     */
    virtual void apply(QSqlDatabase &database) = 0;

    /**
     * Do session setup/initialization work on @p database.
     * An example would be to run some SQL commands on every new session,
     * typically stuff like setting encodings, transaction isolation levels, etc.
     */
    virtual void initSession(const QSqlDatabase &database);

    /**
     * Returns whether an internal server needs to be used.
     */
    virtual bool useInternalServer() const = 0;

    /**
     * This method is called to start an external server.
     */
    virtual bool startInternalServer();

    /**
     * This method is called to stop the external server.
     */
    virtual void stopInternalServer();

    /**
     * Payload data bigger than this value will be stored in separate files, instead of the database. Valid
     *
     * @return the size threshold in bytes, defaults to 4096.
     */
    virtual qint64 sizeThreshold() const;

    /**
     * This method is called to setup initial database settings after a connection is established.
     */
    virtual void setup();

    /**
     * Disables foreign key constraint checks.
     */
    virtual bool disableConstraintChecks(const QSqlDatabase &db) = 0;

    /**
     * Re-enables foreign key constraint checks.
     */
    virtual bool enableConstraintChecks(const QSqlDatabase &db) = 0;

protected:
    explicit DbConfig();
    explicit DbConfig(const QString &configFile);

    /**
     * Returns the suggested default database name, if none is specified in the configuration already.
     * This includes instance namespaces, so usually this is not necessary to use in combination
     * with internal databases (in process or using our own server instance).
     */
    static QString defaultDatabaseName();

    /*
     * Returns the Database backend we should use by default. Usually it should be the same value
     * configured as AKONADI_DATABASE_BACKEND at build time, but this method checks if that
     * backend is really available and if it's not, it falls back to returning "QSQLITE".
     */
    static QString defaultAvailableDatabaseBackend(QSettings &settings);

    /**
     * Calls QProcess::execute() and also prints the command and arguments via qCDebug()
     */
    int execute(const QString &cmd, const QStringList &args) const;

private:
    Q_DISABLE_COPY(DbConfig)

    qint64 mSizeThreshold;
};

} // namespace Server
} // namespace Akonadi
