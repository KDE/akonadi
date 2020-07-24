/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DBCONFIGSQLITE_H
#define DBCONFIGSQLITE_H

#include "dbconfig.h"

namespace Akonadi
{
namespace Server
{

class DbConfigSqlite : public DbConfig
{
public:
    enum Version {
        Default, /** Uses the Qt sqlite driver */
        Custom   /** Uses the custom qsqlite driver from akonadi/qsqlite */
    };

public:
    explicit DbConfigSqlite(Version driver);

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
    bool init(QSettings &settings, bool storeSettings) override;

    /**
     * This method checks if the requirements for this database connection are met
     * in the system (QSQLITE/QSQLITE3 driver is available, object can be initialized, etc.).
     */
    bool areRequirementsAvailable(QSettings &settings);

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
private:
    bool setPragma(QSqlDatabase &db, QSqlQuery &query, const QString &pragma);

    Version mDriverVersion;
    QString mDatabaseName;
    QString mHostName;
    QString mUserName;
    QString mPassword;
    QString mConnectionOptions;
};

} // namespace Server
} // namespace Akonadi

#endif
