/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QHash>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QStringList>

class DbIntrospectorTest;

namespace Akonadi
{
namespace Server
{
/**
 * Methods for introspecting the current state of a database schema.
 * I.e. this is about the structure of a database, not its content.
 */
class DbIntrospector
{
public:
    using Ptr = QSharedPointer<DbIntrospector>;

    /** A structure describing an existing foreign key. */
    class ForeignKey
    {
    public:
        QString name;
        QString column;
        QString refTable;
        QString refColumn;
        QString onUpdate; // TODO use same enum as DbInitializer
        QString onDelete; // dito
    };

    /**
     * Returns an introspector instance for a given database.
     */
    static DbIntrospector::Ptr createInstance(const QSqlDatabase &database);

    virtual ~DbIntrospector();

    /**
     * Returns @c true if table @p tableName exists.
     * The default implementation relies on QSqlDatabase::tables(). Usually this
     * does not need to be reimplemented.
     */
    virtual bool hasTable(const QString &tableName);

    /**
     * Returns @c true of the given table has an index with the given name.
     * The default implementation performs the query returned by hasIndexQuery().
     * @see hasIndexQuery()
     * @throws DbException on database errors.
     */
    virtual bool hasIndex(const QString &tableName, const QString &indexName);

    /**
     * Check whether table @p tableName has a column named @p columnName.
     * The default implementation should work with all backends.
     */
    virtual bool hasColumn(const QString &tableName, const QString &columnName);

    /**
     * Check whether table @p tableName is empty, ie. does not contain any rows.
     * The default implementation should work for all backends.
     * @throws DbException on database errors.
     */
    virtual bool isTableEmpty(const QString &tableName);

    /**
     * Returns the foreign key constraints on table @p tableName.
     * The default implementation returns an empty list, so any backend supporting
     * referential integrity should reimplement this.
     */
    virtual QList<ForeignKey> foreignKeyConstraints(const QString &tableName);

    /**
     * Returns query to retrieve the next autoincrement value for @p tableName.@p columnName.
     */
    virtual QString getAutoIncrementValueQuery(const QString &tableName, const QString &columnName) = 0;

    /**
     * Returns query to update the next autoincrement value for @p tableName.@p columnName to value @p value.
     */
    virtual QString updateAutoIncrementValueQuery(const QString &tableName, const QString &columnName, qint64 value) = 0;

protected:
    /**
     * Creates a new database introspector, call from subclass.
     *
     * @param database The database to introspect.
     */
    DbIntrospector(const QSqlDatabase &database);

    /**
     * Returns a query string to determine if @p tableName has an index @p indexName.
     * The query is expected to have one boolean result row/column.
     * This is used by the default implementation of hasIndex() only, thus reimplementation
     * is not necessary if you reimplement hasIndex()
     * The default implementation asserts.
     */
    virtual QString hasIndexQuery(const QString &tableName, const QString &indexName);

    /** The database connection we are introspecting. */
    QSqlDatabase m_database;

private:
    Q_DISABLE_COPY_MOVE(DbIntrospector)

    friend class ::DbIntrospectorTest;
    QHash<QString, QStringList> m_columnCache; // avoids extra db roundtrips
};

} // namespace Server
} // namespace Akonadi
