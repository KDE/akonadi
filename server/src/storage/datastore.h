/***************************************************************************
 *   Copyright (C) 2006 by Andreas Gungl <a.gungl@gmx.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef DATASTORE_H
#define DATASTORE_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtSql/QSqlDatabase>

class QSqlQuery;

#include "entities.h"
#include "fetchquery.h"
#include "notificationcollector.h"
#include "akonadiprivate_export.h"

namespace Akonadi {

class FetchQuery;
class NotificationCollector;

/**
  This class handles all the database access.

  <h4>Database configuration</h4>

  You can select between various database backends during runtime using the
  @c $HOME/.akonadi/akonadiserverrc configuration file.

  Example:
@verbatim
[%General]
Driver=QMYSQL

[QMYSQL_EMBEDDED]
Name=akonadi
Options=SERVER_DATADIR=/home/foo/.akonadi/db

[QMYSQL]
Name=akonadi
Host=localhost
User=foo
Password=*****
#Options=UNIX_SOCKET=/home/foo/.akonadi/mysql.socket

[QSQLITE]
Name=/home/foo/.akonadi/akonadi.db
@endverbatim

  Use @c General/Driver to select the QSql driver to use for databse
  access. The following drivers are currently supported, other might work
  but are untested:

  - QMYSQL
  - QMYSQL_EMBEDDED
  - QSQLITE

  The options for each driver are read from the corresponding group.
  The following options are supported, dependend on the driver not all of them
  might have an effect:

  - Name: Database name, for sqlite that's the file name of the database.
  - Host: Hostname of the database server
  - User: Username for the database server
  - Password: Password for the database server
  - Options: Additional options, format is driver-dependent
*/
class AKONADIPRIVATE_EXPORT DataStore : public QObject
{
    Q_OBJECT
  public:
    /**
      Closes the database connection and destroys the DataStore object.
    */
    virtual ~DataStore();

    /**
      Opens the database connection.
    */
    void open();

    /**
      Closes the databse connection.
    */
    void close();

    /**
      Initializes the database. Should be called during startup by the main thread.
    */
    void init();

    /**
      Per thread signleton.
    */
    static DataStore* self();

    /* --- Flag ---------------------------------------------------------- */
    bool appendFlag( const QString & name );
    bool removeFlag( const Flag & flag );
    bool removeFlag( int id );

    /* --- ItemFlags ----------------------------------------------------- */
    bool setItemFlags( const PimItem &item, const QList<Flag> &flags );
    bool appendItemFlags( const PimItem &item, const QList<Flag> &flags );    bool removeItemFlags( const PimItem &item, const QList<Flag> &flags );

    bool appendItemFlags( int pimItemId, const QList<QByteArray> &flags );

    /* --- Location ------------------------------------------------------ */
    bool appendLocation( Location &location );
    /// removes the given location without removing its content
    bool removeLocation( const Location & location );
    /// removes the location with the given @p id without removing its content
    bool removeLocation( int id );
    /// removes the given location and all its content
    bool cleanupLocation( const Location &location );
    bool updateLocationCounts( const Location & location, int existsChange, int recentChange, int unseenChange );
    bool changeLocationPolicy( Location & location, const CachePolicy & policy );
    bool resetLocationPolicy( const Location & location );
    /// rename the collection @p location to @p newName.
    bool renameLocation( const Location &location, int newParent, const QString &newName );
    /**
      Returns all collection associated with the given resource, all collections if the
      resource is invalid.
    */
    QList<Location> listLocations( const Resource & resource = Resource() ) const;

    bool appendMimeTypeForLocation( int locationId, const QString & mimeType );
    bool appendMimeTypeForLocation( int locationId, int mimeTypeId );
    bool removeMimeTypesForLocation( int locationId );

    static QString locationDelimiter() { return QLatin1String("/"); }

    /**
      Returns the active CachePolicy for this Location.
    */
    CachePolicy activeCachePolicy( const Location &loc );

    /* --- MimeType ------------------------------------------------------ */
    bool appendMimeType( const QString & mimetype, int *insertId = 0 );
    bool removeMimeType( const MimeType & mimetype );
    bool removeMimeType( int id );

    /* --- PimItem ------------------------------------------------------- */
    bool appendPimItem( const QByteArray & data,
                        const MimeType & mimetype,
                        const Location & location,
                        const QDateTime & dateTime,
                        const QByteArray & remote_id,
                        int *insertId = 0 );
    bool updatePimItem( PimItem &pimItem, const QByteArray &data );
    bool updatePimItem( PimItem &pimItem, const Location &destination );
    bool updatePimItem( PimItem &pimItem, const QString &remoteId );
    PimItem pimItemById( int id );
    PimItem pimItemById( int id, FetchQuery::Type type );
    QList<PimItem> listPimItems( const MimeType & mimetype,
                                 const Location & location );

    QList<PimItem> listPimItems( const Location & location, const Flag &flag );

    /**
     * Removes the pim item and all referenced data ( e.g. flags )
     */
    bool cleanupPimItem( const PimItem &item );

    /**
     * Cleanups all items which have the '\\Deleted' flag set
     */
    bool cleanupPimItems( const Location &location );

    /**
     * Returns the current position ( folder index ) of this
     * item.
     */
    int pimItemPosition( const PimItem &item );

    int highestPimItemId() const;
    int highestPimItemCountByLocation( const Location &location );

    QList<PimItem> matchingPimItemsByUID( const QList<QByteArray> &sequences,
                                          const Location & l = Location() );
    QList<PimItem> matchingPimItemsByUID( const QList<QByteArray> &sequences,
                                          FetchQuery::Type type,
                                          const Location & l = Location() );
    QList<PimItem> matchingPimItemsBySequenceNumbers( const QList<QByteArray> &sequences,
                                                      const Location &location );
    QList<PimItem> matchingPimItemsBySequenceNumbers( const QList<QByteArray> &sequences,
                                                      const Location &location,
                                                      FetchQuery::Type type );

    QList<PimItem> fetchMatchingPimItemsByUID( const FetchQuery &query, const Location& l = Location() );
    QList<PimItem> fetchMatchingPimItemsBySequenceNumbers( const FetchQuery &query,
                                                           const Location &location );

    /* --- Resource ------------------------------------------------------ */
    bool appendResource( const QString & resource, const CachePolicy & policy = CachePolicy() );
    bool removeResource( const Resource & resource );
    bool removeResource( int id );
    QList<Resource> listResources( const CachePolicy & policy );

    /* --- Persistent search --------------------------------------------- */
    bool appendPersisntentSearch( const QString &name, const QByteArray &queryString );
    bool removePersistentSearch( const PersistentSearch &search );
    QList<Location> listPersistentSearches() const;

    /* --- Helper functions ---------------------------------------------- */
    /** Returns the id of the next PIM item that is added to the db.
        @return possible id of the next PIM item that is added to the database
     */
    int uidNext() const;

    static QString storagePath();

    /**
      Begins a transaction. No changes will be written to the database and
      no notification signal will be emitted unless you call commitTransaction().
      @return true if successfull.
    */
    bool beginTransaction();

    /**
      Reverts all changes within the current transaction.
    */
    bool rollbackTransaction();

    /**
      Commits all changes within the current transaction and emits all
      collected notfication signals. If committing fails, the transaction
      will be rolled back.
    */
    bool commitTransaction();

    /**
      Returns true if there is a transaction in progress.
    */
    bool inTransaction() const;

    /**
      Returns the notification collector of this DataStore object.
      Use this to listen to change notification signals.
    */
    NotificationCollector* notificationCollector() const { return mNotificationCollector; }

    /**
      Returns the QSqlDatabase object. Use this for generating queries yourself.
    */
    QSqlDatabase database() const { return m_database; }

    /**
      Sets the current session id.
    */
    void setSessionId( const QByteArray &sessionId ) { mSessionId = sessionId; }

    /**
      Returns the name of the used database driver.
    */
    static QString driverName() { return mDbDriverName; }

Q_SIGNALS:
    /**
      Emitted if a transaction has been successfully committed.
    */
    void transactionCommitted();
    /**
      Emitted if a transaction has been aborted.
    */
    void transactionRolledBack();

protected:
    /**
      Creates a new DataStore object and opens it.
    */
    DataStore();

    void debugLastDbError( const char* actionDescription ) const;
    void debugLastQueryError( const QSqlQuery &query, const char* actionDescription ) const;
    bool removeById( int id, const QString & tableName );
    QByteArray retrieveDataFromResource( int uid, const QByteArray& remote_id,
                                         int locationid, FetchQuery::Type type );

  public:
    /** Returns the id of the most recent inserted row, or -1 if there's no such
        id.
        @param query the query we want to get the last insert id for
        @return id of the most recent inserted row, or -1
     */
    static int lastInsertId( const QSqlQuery & query );

  private:
    /** Converts the given date/time to the database format, i.e.
        "YYYY-MM-DD HH:MM:SS".
        @param dateTime the date/time in UTC
        @return the date/time in database format
        @see dateTimeToQDateTime
     */
    static QString dateTimeFromQDateTime( const QDateTime & dateTime );

    /** Converts the given date/time from database format to QDateTime.
        @param dateTime the date/time in database format
        @return the date/time as QDateTime
        @see dateTimeFromQDateTime
     */
    static QDateTime dateTimeToQDateTime( const QByteArray & dateTime );

private:
    QString m_connectionName;
    QSqlDatabase m_database;
    bool m_dbOpened;
    bool m_inTransaction;
    QByteArray mSessionId;
    NotificationCollector* mNotificationCollector;

    static QList<int> mPendingItemDeliveries;
    static QMutex mPendingItemDeliveriesMutex;
    static QWaitCondition mPendingItemDeliveriesCondition;

    // database configuration
    static QString mDbDriverName;
    static QString mDbName;
    static QString mDbHostName;
    static QString mDbUserName;
    static QString mDbPassword;
    static QString mDbConnectionOptions;
};

}
#endif
