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

#include <QtCore/QSettings>
#include <QtSql/QSqlDatabase>

/**
 * A base class that provides an unique access layer to configuration
 * and initialization of different database backends.
 */
class DbConfig
{
  public:

    virtual ~DbConfig();

    /**
     * Returns the DbConfig instance for the database the user has
     * configured.
     */
    static DbConfig* configuredDatabase();

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
    virtual bool init( QSettings &settings ) = 0;

    /**
     * This method applies the configured settings to the QtSql @p database
     * instance.
     */
    virtual void apply( QSqlDatabase &database ) = 0;

    /**
     * Returns whether an internal server needs to be used.
     */
    virtual bool useInternalServer() const = 0;

    /**
     * This method is called to start an external server.
     */
    virtual void startInternalServer();

    /**
     * This method is called to stop the external server.
     */
    virtual void stopInternalServer();

    /**
     * Payload data bigger than this value will be stored in separate files, instead of the database. Valid
     * only if @ref useExternalPayloadFile returns true, otherwise it is ignored.
     * @return the size threshold in bytes, defaults to 4096.
     */
    virtual qint64 sizeThreshold() const;

    /**
     * Check if big payload data (@ref sizeThreshold) is stored in external files instead of the database.
     * @return true, if the big data is stored in external files. Default is @c false.
     */
    virtual bool useExternalPayloadFile() const;

    /**
     * Returns the socket @p directory that is passed to this method or the one
     * the user has overwritten via the config file.
     */
    QString preferredSocketDirectory( const QString &directory ) const;

  protected:
    DbConfig();

  private:
    qint64 mSizeThreshold;
    bool mUseExternalPayloadFile;
};

#endif
