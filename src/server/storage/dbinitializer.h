/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include "dbintrospector.h"
#include "schematypes.h"

#include <QHash>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QStringList>

class DbInitializerTest;

namespace Akonadi
{
namespace Server
{
class Schema;
class DbUpdater;

class TestInterface
{
public:
    virtual ~TestInterface() = default;

    virtual void execStatement(const QString &statement) = 0;

protected:
    explicit TestInterface() = default;

private:
    Q_DISABLE_COPY_MOVE(TestInterface)
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
    friend class DbUpdater;

public:
    using Ptr = QSharedPointer<DbInitializer>;

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
    virtual QString sqlType(const ColumnDescription &col, int size) const;
    /** Overwrite in backend-specific sub-classes to return the SQL value for a given C++ value. */
    virtual QString sqlValue(const ColumnDescription &col, const QString &value) const;

    virtual QString buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const = 0;
    virtual QString buildAddColumnStatement(const TableDescription &tableDescription, const ColumnDescription &columnDescription) const;
    virtual QString buildCreateIndexStatement(const TableDescription &tableDescription, const IndexDescription &indexDescription) const;
    virtual QString buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const = 0;

    /**
     * Returns an SQL statements to add a foreign key constraint to an existing column @p column.
     * The default implementation returns an empty string, so any backend supporting foreign key constraints
     * must reimplement this.
     */
    virtual QStringList buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const;

    /**
     * Returns an SQL statements to remove the foreign key constraint @p fk from table @p table.
     * The default implementation returns an empty string, so any backend supporting foreign key constraints
     * must reimplement this.
     */
    virtual QStringList buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const;

    static QString buildReferentialAction(ColumnDescription::ReferentialAction onUpdate, ColumnDescription::ReferentialAction onDelete);
    /// Use for multi-column primary keys during table creation
    static QString buildPrimaryKeyStatement(const TableDescription &table);

private:
    friend class ::DbInitializerTest;
    Q_DISABLE_COPY_MOVE(DbInitializer)

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
    Schema *mSchema = nullptr;
    QString mErrorMsg;
    TestInterface *mTestInterface = nullptr;
    DbIntrospector::Ptr m_introspector;
    QStringList m_pendingIndexes;
    QStringList m_pendingForeignKeys;
    QStringList m_removedForeignKeys;
};

} // namespace Server
} // namespace Akonadi
