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

#ifndef DBCONFIGMYSQL_H
#define DBCONFIGMYSQL_H

#include "dbconfig.h"
#include <QObject>
#include <QProcess>

namespace Akonadi
{
namespace Server
{

class DbConfigMysql : public QObject, public DbConfig
{
    Q_OBJECT

public:
    DbConfigMysql();

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
     * with the given @p settings and written back.
     */
    bool init(QSettings &settings) override;

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
    bool mInternalServer;
    QProcess *mDatabaseProcess = nullptr;
};

} // namespace Server
} // namespace Akonadi

#endif
