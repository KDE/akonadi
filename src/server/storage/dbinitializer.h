/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#ifndef DBINITIALIZER_H
#define DBINITIALIZER_H

#include "dbintrospector.h"
#include "schematypes.h"

#include <QHash>
#include <QPair>
#include <QVector>
#include <QStringList>
#include <QSharedPointer>
#include <QSqlDatabase>

class DbInitializerTest;

namespace Akonadi
{
namespace Server
{

class Schema;

class TestInterface
{
public:
    virtual ~TestInterface()
    {
    }

    virtual void execStatement(const QString &statement) = 0;
};

/**
 * A helper class which takes a reference to a database object and
 * the file name of a template file and initializes the database
 * according to the rules in the template file.
 *
 * TODO: Refactor this to be easily reusable for updater too
 */
class DbInitializer
{
public:
    typedef QSharedPointer<DbInitializer> Ptr;

    /**
      Returns an initializer instance for a given backend.
    */
    static DbInitializer::Ptr createInstance(const QSqlDatabase &database, Schema *schema = nullptr);

    /**
     * Destroys the database initializer.
     */
    virtual ~DbInitializer();

    /**
     * Starts the initialization process.
     * On success true is returned, false otherwise.
     *
     * If something went wrong @see errorMsg() can be used to retrieve more
     * information.
     */
    bool run();

    /**
     * Returns the textual description of an occurred error.
     */
    QString errorMsg() const;

    /**
     * Returns whether the database has working and complete foreign keys.
     * This information can be used for query optimizations.
     * @note Result is invalid before run() has been called.
     */
    bool hasForeignKeyConstraints() const;

    /**
     * Checks and creates missing indexes.
     *
     * This method is run after DbUpdater to ensure that data in new columns
     * are populated and creation of indexes and foreign keys does not fail.
     */
    bool updateIndexesAndConstraints();

    /**
     * Returns a backend-specific CREATE TABLE SQL query describing given table
     */
    virtual QString buildCreateTableStatement(const TableDescription &tableDescription) const = 0;

protected:
    /**
     * Creates a new database initializer.
     *
     * @param database The reference to the database.
     */
    DbInitializer(const QSqlDatabase &database);

    /**
     * Overwrite in backend-specific sub-classes to return the SQL type for a given C++ type.
     * @param type Name of the C++ type.
     * @param size Optional size hint for the column, if -1 use the default SQL type for @p type.
     */
    virtual QString sqlType(const QString &type, int size) const;
    /** Overwrite in backend-specific sub-classes to return the SQL value for a given C++ value. */
    virtual QString sqlValue(const QString &type, const QString &value) const;

    virtual QString buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const = 0;
    virtual QString buildAddColumnStatement(const TableDescription &tableDescription, const ColumnDescription &columnDescription) const;
    virtual QString buildCreateIndexStatement(const TableDescription &tableDescription, const IndexDescription &indexDescription) const;
    virtual QString buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const = 0;

    /**
     * Returns an SQL statement to add a foreign key constraint to an existing column @p column.
     * The default implementation returns an empty string, so any backend supporting foreign key constraints
     * must reimplement this.
     */
    virtual QString buildAddForeignKeyConstraintStatement(const TableDescription &table, const ColumnDescription &column) const;

    /**
     * Returns an SQL statement to remove the foreign key constraint @p fk from table @p table.
     * The default implementation returns an empty string, so any backend supporting foreign key constraints
     * must reimplement this.
     */
    virtual QString buildRemoveForeignKeyConstraintStatement(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const;

    static QString buildReferentialAction(ColumnDescription::ReferentialAction onUpdate, ColumnDescription::ReferentialAction onDelete);
    /// Use for multi-column primary keys during table creation
    static QString buildPrimaryKeyStatement(const TableDescription &table);

private:
    friend class ::DbInitializerTest;

    /**
     * Sets the debug @p interface that shall be used on unit test run.
     */
    void setTestInterface(TestInterface *interface);

    /**
     * Sets a different DbIntrospector. This allows unit tests to simulate certain
     * states of the database.
     */
    void setIntrospector(const DbIntrospector::Ptr &introspector);

    /** Helper method for executing a query.
     * If a debug interface is set for testing, that gets the queries instead.
     * @throws DbException if something went wrong.
     */
    void execQuery(const QString &queryString);

    bool checkTable(const TableDescription &tableDescription);
    /**
     * Checks foreign key constraints on table @p tableDescription and fixes them if necessary.
     */
    void checkForeignKeys(const TableDescription &tableDescription);
    void checkIndexes(const TableDescription &tableDescription);
    bool checkRelation(const RelationDescription &relationDescription);

    static QString referentialActionToString(ColumnDescription::ReferentialAction action);

    void execPendingQueries(const QStringList &queries);

    QSqlDatabase mDatabase;
    Schema *mSchema;
    QString mErrorMsg;
    TestInterface *mTestInterface;
    DbIntrospector::Ptr m_introspector;
    bool m_noForeignKeyContraints;
    QStringList m_pendingIndexes;
    QStringList m_pendingForeignKeys;
    QStringList m_removedForeignKeys;
};

} // namespace Server
} // namespace Akonadi

#endif
