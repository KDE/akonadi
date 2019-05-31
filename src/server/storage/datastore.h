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

#include <QObject>
#include <QList>
#include <QVector>
#include <QThreadStorage>
#include <QSqlDatabase>

class QSqlQuery;
class QTimer;

#include "entities.h"
#include "notificationcollector.h"

#include <memory>

namespace Akonadi
{
namespace Server
{

class DataStore;
class DataStoreFactory
{
public:
    virtual ~DataStoreFactory() = default;
    virtual DataStore *createStore();
};

class NotificationCollector;

/**
  This class handles all the database access.

  <h4>Database configuration</h4>

  You can select between various database backends during runtime using the
  @c $HOME/.config/akonadi/akonadiserverrc configuration file.

  Example:
@verbatim
[%General]
Driver=QMYSQL

[QMYSQL_EMBEDDED]
Name=akonadi
Options=SERVER_DATADIR=/home/foo/.local/share/akonadi/db_data

[QMYSQL]
Name=akonadi
Host=localhost
User=foo
Password=*****
#Options=UNIX_SOCKET=/home/foo/.local/share/akonadi/socket-bar/mysql.socket
StartServer=true
ServerPath=/usr/sbin/mysqld

[QSQLITE]
Name=/home/foo/.local/share/akonadi/akonadi.db
@endverbatim

  Use @c General/Driver to select the QSql driver to use for database
  access. The following drivers are currently supported, other might work
  but are untested:

  - QMYSQL
  - QMYSQL_EMBEDDED
  - QSQLITE

  The options for each driver are read from the corresponding group.
  The following options are supported, dependent on the driver not all of them
  might have an effect:

  - Name: Database name, for sqlite that's the file name of the database.
  - Host: Hostname of the database server
  - User: Username for the database server
  - Password: Password for the database server
  - Options: Additional options, format is driver-dependent
  - StartServer: Start the database locally just for Akonadi instead of using an existing one
  - ServerPath: Path to the server executable
*/
class DataStore : public QObject
{
    Q_OBJECT
public:
    const constexpr static bool Silent = true;

    /**
      Closes the database connection and destroys the DataStore object.
    */
    ~DataStore() override;

    /**
      Opens the database connection.
    */
    virtual void open();

    /**
      Closes the database connection.
    */
    void close();

    /**
      Initializes the database. Should be called during startup by the main thread.
    */
    virtual bool init();

    /**
      Per thread singleton.
    */
    static DataStore *self();

    /**
     * Returns whether per thread DataStore has been created.
     */
    static bool hasDataStore();

    /* --- ItemFlags ----------------------------------------------------- */
    virtual bool setItemsFlags(const PimItem::List &items, const QVector<Flag> &flags,
                               bool *flagsChanged = nullptr, const Collection &col = Collection(), bool silent = false);
    virtual bool appendItemsFlags(const PimItem::List &items, const QVector<Flag> &flags, bool *flagsChanged = nullptr,
                                  bool checkIfExists = true, const Collection &col = Collection(), bool silent = false);
    virtual bool removeItemsFlags(const PimItem::List &items, const QVector<Flag> &flags, bool *tagsChanged = nullptr,
                                  const Collection &collection = Collection(), bool silent = false);

    /* --- ItemTags ----------------------------------------------------- */
    virtual bool setItemsTags(const PimItem::List &items, const Tag::List &tags,
                              bool *tagsChanged = nullptr, bool silent = false);
    virtual bool appendItemsTags(const PimItem::List &items, const Tag::List &tags,
                                 bool *tagsChanged = nullptr, bool checkIfExists = true,
                                 const Collection &col = Collection(), bool silent = false);
    virtual bool removeItemsTags(const PimItem::List &items, const Tag::List &tags,
                                 bool *tagsChanged = nullptr, bool silent = false);
    virtual bool removeTags(const Tag::List &tags, bool silent = false);

    /* --- ItemParts ----------------------------------------------------- */
    virtual bool removeItemParts(const PimItem &item, const QSet<QByteArray> &parts);

    // removes all payload parts for this item.
    virtual bool invalidateItemCache(const PimItem &item);

    /* --- Collection ------------------------------------------------------ */
    virtual bool appendCollection(Collection &collection, const QStringList &mimeTypes, const QMap<QByteArray, QByteArray> &attributes);

    /// removes the given collection and all its content
    virtual bool cleanupCollection(Collection &collection);
    /// same as the above but for database backends without working referential actions on foreign keys
    virtual bool cleanupCollection_slow(Collection &collection);

    /// moves the collection @p collection to @p newParent.
    virtual bool moveCollection(Collection &collection, const Collection &newParent);

    virtual bool appendMimeTypeForCollection(qint64 collectionId, const QStringList &mimeTypes);

    static QString collectionDelimiter()
    {
        return QStringLiteral("/");
    }

    /**
      Determines the active cache policy for this Collection.
      The active cache policy is set in the corresponding Collection fields.
    */
    virtual void activeCachePolicy(Collection &col);

    /// Returns all virtual collections the @p item is linked to
    QVector<Collection> virtualCollections(const PimItem &item);

    QMap< Server::Entity::Id, QList< PimItem > > virtualCollections(const Akonadi::Server::PimItem::List &items);

    /* --- PimItem ------------------------------------------------------- */
    virtual bool appendPimItem(QVector<Part> &parts,
                               const QVector<Flag> &flags,
                               const MimeType &mimetype,
                               const Collection &collection,
                               const QDateTime &dateTime,
                               const QString &remote_id,
                               const QString &remoteRevision,
                               const QString &gid,
                               PimItem &pimItem);
    /**
     * Removes the pim item and all referenced data ( e.g. flags )
     */
    virtual bool cleanupPimItems(const PimItem::List &items, bool silent = false);

    /**
     * Unhides the specified PimItem. Emits the itemAdded() notification as
     * the hidden flag is assumed to have been set by appendPimItem() before
     * pushing the item to the preprocessor chain. The hidden item had his
     * notifications disabled until now (so for the clients the "unhide" operation
     * is actually a new item arrival).
     *
     * This function does NOT verify if the item was *really* hidden: this is
     * responsibility of the caller.
     */
    virtual bool unhidePimItem(PimItem &pimItem);

    /**
     * Unhides all the items which have the "hidden" flag set.
     * This function doesn't emit any notification about the items
     * being unhidden so it's meant to be called only in rare circumstances.
     * The most notable call to this function is at server startup
     * when we attempt to restore a clean state of the database.
     */
    virtual bool unhideAllPimItems();

    /* --- Collection attributes ------------------------------------------ */
    virtual bool addCollectionAttribute(const Collection &col, const QByteArray &key,
                                        const QByteArray &value, bool silent = false);
    /**
     * Removes the given collection attribute for @p col.
     * @throws HandlerException on database errors
     * @returns @c true if the attribute existed, @c false otherwise
     */
    virtual bool removeCollectionAttribute(const Collection &col, const QByteArray &key);

    /* --- Helper functions ---------------------------------------------- */

    /**
      Begins a transaction. No changes will be written to the database and
      no notification signal will be emitted unless you call commitTransaction().
      @return @c true if successful.
    */
    virtual bool beginTransaction(const QString &name);

    /**
      Reverts all changes within the current transaction.
    */
    virtual bool rollbackTransaction();

    /**
      Commits all changes within the current transaction and emits all
      collected notfication signals. If committing fails, the transaction
      will be rolled back.
    */
    virtual bool commitTransaction();

    /**
      Returns true if there is a transaction in progress.
    */
    bool inTransaction() const;

    /**
      Returns the notification collector of this DataStore object.
      Use this to listen to change notification signals.
    */
    NotificationCollector *notificationCollector();

    /**
      Returns the QSqlDatabase object. Use this for generating queries yourself.

      Will [re-]open the database, if it is closed.
    */
    QSqlDatabase database();

    /**
      Sets the current session id.
    */
    void setSessionId(const QByteArray &sessionId)
    {
        mSessionId = sessionId;
    }

    /**
      Returns if the database is currently open
      */
    bool isOpened() const
    {
        return m_dbOpened;
    }

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

    void debugLastDbError(const char *actionDescription) const;
    void debugLastQueryError(const QSqlQuery &query, const char *actionDescription) const;

private:
    bool doAppendItemsFlag(const PimItem::List &items, const Flag &flag,
                           const QSet<PimItem::Id> &existing, const Collection &col,
                           bool silent);

    bool doAppendItemsTag(const PimItem::List &items, const Tag &tag,
                          const QSet<Entity::Id> &existing, const Collection &col,
                          bool silent);

    /** Converts the given date/time to the database format, i.e.
        "YYYY-MM-DD HH:MM:SS".
        @param dateTime the date/time in UTC
        @return the date/time in database format
        @see dateTimeToQDateTime
     */
    static QString dateTimeFromQDateTime(const QDateTime &dateTime);

    /** Converts the given date/time from database format to QDateTime.
        @param dateTime the date/time in database format
        @return the date/time as QDateTime
        @see dateTimeFromQDateTime
     */
    static QDateTime dateTimeToQDateTime(const QByteArray &dateTime);

    /**
     * Adds the @p query to current transaction, so that it can be replayed in
     * case the transaction deadlocks or timeouts.
     *
     * When DataStore is not in transaction or SQLite is configured, this method
     * does nothing.
     *
     * All queries will automatically be removed when transaction is committed.
     *
     * This method should only be used by QueryBuilder.
     */
    void addQueryToTransaction(const QString &statement, const QVector<QVariant> &bindValues, bool isBatch);

    /**
     * Tries to execute all queries from last transaction again. If any of the
     * queries fails, the entire transaction is rolled back and fails.
     *
     * This method can only be used by QueryBuilder when database rolls back
     * transaction due to deadlock or timeout.
     *
     * @return Returns an invalid query when error occurs, or the last replayed
     *         query on success.
     */
    QSqlQuery retryLastTransaction(bool rollbackFirst);

private Q_SLOTS:
    void sendKeepAliveQuery();

protected:
    static QThreadStorage<DataStore *> sInstances;
    static std::unique_ptr<DataStoreFactory> sFactory;

    QString m_connectionName;
    QSqlDatabase m_database;
    bool m_dbOpened;
    uint m_transactionLevel;
    struct TransactionQuery {
        QString query;
        QVector<QVariant> boundValues;
        bool isBatch;
    };
    QVector<TransactionQuery> m_transactionQueries;
    QByteArray mSessionId;
    std::unique_ptr<NotificationCollector> mNotificationCollector;
    QTimer *m_keepAliveTimer = nullptr;
    static bool s_hasForeignKeyConstraints;

    // Gives QueryBuilder access to addQueryToTransaction() and retryLastTransaction()
    friend class QueryBuilder;
    friend class DataStoreFactory;

};

} // namespace Server
} // namespace Akonadi

#endif
