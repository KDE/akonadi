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

#ifndef DBCONFIG_H
#define DBCONFIG_H

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
     * This method is called whenever the Akonadi server is started
     * and before the initial database connection is set up.
     *
     * At this point the default settings should be determined, merged
     * with the given @p settings and written back.
     */
    virtual bool init(QSettings &settings) = 0;

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

protected:
    DbConfig();

    /**
     * Returns the suggested default database name, if none is specified in the configuration already.
     * This includes instance namespaces, so usually this is not necessary to use in combination
     * with internal databases (in process or using our own server instance).
     */
    static QString defaultDatabaseName();

    /**
     * Calls QProcess::execute() and also prints the command and arguments via qCDebug()
     */
    int execute(const QString &cmd, const QStringList &args) const;
private:
    Q_DISABLE_COPY(DbConfig);

    qint64 mSizeThreshold;
};

} // namespace Server
} // namespace Akonadi

#endif
