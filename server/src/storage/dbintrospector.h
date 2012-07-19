/*
    Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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
#ifndef DBINTROSPECTOR_H
#define DBINTROSPECTOR_H

#include <QHash>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QStringList>

/**
 * Methods for introspecting the current state of a database schema.
 * I.e. this is about the structure of a database, not its content.
 */
class DbIntrospector
{
  public:
    typedef QSharedPointer<DbIntrospector> Ptr;

    /**
     * Returns an introspector instance for a given database.
     */
    static DbIntrospector::Ptr createInstance( const QSqlDatabase &database );

    virtual ~DbIntrospector();

    /**
     * Returns @c true if table @p tableName exists.
     * The default implementation relies on QSqlDatabase::tables(). Usually this
     * does not need to be reimplemented.
     */
    virtual bool hasTable( const QString &tableName );

    /**
     * Returns @c true of the given table has an index with the given name.
     * The default implementation performs the query returned by hasIndexQuery().
     * @see hasIndexQuery()
     */
    virtual bool hasIndex( const QString &tableName, const QString &indexName );

    /**
     * Check whether table @p tableName has a column named @p columnName.
     * The default implemention should work with all backends.
     */
    virtual bool hasColumn( const QString &tableName, const QString &columnName );

    // TODO: introspection for foreign key constraints on a given column

  protected:
    /**
     * Creates a new database introspector, call from subclass.
     *
     * @param database The database to introspect.
     */
    DbIntrospector( const QSqlDatabase &database );

    /**
     * Returns a query string to determine if @p tableName has an index @p indexName.
     * The query is expected to have one boolean result row/column.
     * This is used by the default implementation of hasIndex() only, thus reimplmentation
     * is not necessary if you reimplement hasIndex()
     * The default implementation asserts.
     */
    virtual QString hasIndexQuery( const QString &tableName, const QString &indexName );

  private:
    friend class DbIntrospectorTest;
    QSqlDatabase m_database;
    QHash<QString, QStringList> m_columnCache; // avoids extra db roundtrips
};

#endif // DBINTROSPECTOR_H
