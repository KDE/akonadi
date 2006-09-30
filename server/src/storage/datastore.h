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
#include <QtSql/QSqlDatabase>

class QSqlQuery;

#include "entity.h"
#include "fetchquery.h"
#include "collection.h"
#include "notificationcollector.h"

namespace Akonadi {

class FetchQuery;
class NotificationCollector;

/**
  This class handles all the database access.
*/
class DataStore : public QObject
{
    Q_OBJECT
public:
    /**
      Creates a new DataStore object and opens it.
    */
    DataStore();

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
      Returns the database object. Can be used for executing queries.
    */
    QSqlDatabase database() const;

    /* -- higher level API -- */
    virtual CollectionList listCollections( const QString& prefix,
                                            const QString& mailboxPattern ) const;

    /* --- CachePolicy --------------------------------------------------- */
    bool appendCachePolicy( const QString & policy );
    bool removeCachePolicy( const CachePolicy & policy );
    bool removeCachePolicy( int id );
    CachePolicy cachePolicyById( int id );
    QList<CachePolicy> listCachePolicies();

    /* --- Flag ---------------------------------------------------------- */
    bool appendFlag( const QString & name );
    bool removeFlag( const Flag & flag );
    bool removeFlag( int id );
    Flag flagById( int id );
    Flag flagByName( const QString &name );
    QList<Flag> listFlags();

    /* --- ItemFlags ----------------------------------------------------- */
    bool setItemFlags( const PimItem &item, const QList<Flag> &flags );
    bool appendItemFlags( const PimItem &item, const QList<Flag> &flags );
    bool removeItemFlags( const PimItem &item, const QList<Flag> &flags );
    QList<Flag> itemFlags( const PimItem &item );

    bool appendItemFlags( int pimItemId, const QList<QByteArray> &flags );

    /* --- ItemMetaData--------------------------------------------------- */
    bool appendItemMetaData( const QString & metadata, const MetaType & metatype );
    bool removeItemMetaData( const ItemMetaData & metadata );
    bool removeItemMetaData( int id );
    ItemMetaData itemMetaDataById( int id );
    QList<ItemMetaData> listItemMetaData();
    QList<ItemMetaData> listItemMetaData( const MetaType & metatype );

    /* --- Location ------------------------------------------------------ */
    bool appendLocation( const QString & location, const Resource & resource,
                         int *insertId = 0 );
    bool appendLocation( const QString & location,
                 const Resource & resource,
                 const CachePolicy & policy );
    /// removes the given location without removing its content
    bool removeLocation( const Location & location );
    /// removes the location with the given @p id without removing its content
    bool removeLocation( int id );
    /// removes the given location and all its content
    bool cleanupLocation( const Location &location );
    bool updateLocationCounts( const Location & location, int existsChange, int recentChange, int unseenChange );
    bool changeLocationPolicy( const Location & location, const CachePolicy & policy );
    bool resetLocationPolicy( const Location & location );
    /// rename the collection @p location to @p newName.
    bool renameLocation( const Location &location, const QString &newName );
    Location locationById( int id ) const;
    Location locationByName( const QString &name ) const;
    /**
      Returns all collection associated with the given resource, all collections if the
      resource is invalid.
    */
    QList<Location> listLocations( const Resource & resource = Resource() ) const;
    QList<MimeType> mimeTypesForLocation( int id ) const;

    bool appendMimeTypeForLocation( int locationId, const QString & mimeType );
    bool appendMimeTypeForLocation( int locationId, int mimeTypeId );
    bool removeMimeTypesForLocation( int locationId );

    static QString locationDelimiter() { return QLatin1String("/"); }

    /* --- MimeType ------------------------------------------------------ */
    bool appendMimeType( const QString & mimetype, int *insertId = 0 );
    bool removeMimeType( const MimeType & mimetype );
    bool removeMimeType( int id );
    MimeType mimeTypeById( int id ) const;
    MimeType mimeTypeByName( const QString & mimetype ) const;
    QList<MimeType> listMimeTypes();

    /* --- MetaType ------------------------------------------------------ */
    bool appendMetaType( const QString & metatype, const MimeType & mimetype );
    bool removeMetaType( const MetaType & metatype );
    bool removeMetaType( int id );
    MetaType metaTypeById( int id );
    QList<MetaType> listMetaTypes();
    QList<MetaType> listMetaTypes( const MimeType & mimetype );

    /* --- PimItem ------------------------------------------------------- */
    bool appendPimItem( const QByteArray & data,
                        const MimeType & mimetype,
                        const Location & location,
                        const QDateTime & dateTime,
                        const QByteArray & remote_id,
                        int *insertId = 0 );
    bool updatePimItem( const PimItem &pimItem, const QByteArray &data );
    bool updatePimItem( const PimItem &pimItem, const Location &destination );
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
     * Cleanups all items which have the '\Deleted' flag set
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
    Resource resourceById( int id );
    const Resource resourceByName( const QByteArray& name ) const;
    QList<Resource> listResources() const;
    QList<Resource> listResources( const CachePolicy & policy );

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
    void debugLastDbError( const char* actionDescription ) const;
    void debugLastQueryError( const QSqlQuery &query, const char* actionDescription ) const;
    bool removeById( int id, const QString & tableName );
    QByteArray retrieveDataFromResource( const QByteArray &uid, const QByteArray& remote_id,
                                         int locationid, FetchQuery::Type type );

private:
    /** Returns the id of the most recent inserted row, or -1 if there's no such
        id.
        @param query the query we want to get the last insert id for
        @return id of the most recent inserted row, or -1
     */
    static int lastInsertId( const QSqlQuery & query );

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
    NotificationCollector* mNotificationCollector;
};

}
#endif
