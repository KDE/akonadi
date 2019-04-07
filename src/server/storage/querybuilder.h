/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_QUERYBUILDER_H
#define AKONADI_QUERYBUILDER_H

#include "query.h"
#include "dbtype.h"

#include <QPair>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QSqlQuery>

#ifdef QUERYBUILDER_UNITTEST
class QueryBuilderTest;
#endif

namespace Akonadi
{
namespace Server
{

/**
  Helper class to construct arbitrary SQL queries.
*/
class QueryBuilder
{
public:
    enum QueryType {
        Select,
        Insert,
        Update,
        Delete
    };

    /**
     * When the same table gets joined as both, Inner- and LeftJoin,
     * it will be merged into a single InnerJoin since it is more
     * restrictive.
     */
    enum JoinType {
        ///NOTE: only supported for UPDATE and SELECT queries.
        InnerJoin,
        ///NOTE: only supported for SELECT queries
        LeftJoin
    };

    /**
     * Defines the place at which a condition should be evaluated.
     */
    enum ConditionType {
        /// add condition to WHERE part of the query
        WhereCondition,
        /// add condition to HAVING part of the query
        /// NOTE: only supported for SELECT queries
        HavingCondition,

        NUM_CONDITIONS
    };

    /**
      Creates a new query builder.

      @param table The main table to operate on.
    */
    explicit QueryBuilder(const QString &table, QueryType type = Select);

    /**
      Sets the database which should execute the query. Unfortunately the SQL "standard"
      is not interpreted in the same way everywhere...
    */
    void setDatabaseType(DbType::Type type);

    /**
      Join a table to the query.

      NOTE: make sure the @c JoinType is supported by the current @c QueryType
      @param joinType The type of JOIN you want to add.
      @param table The table to join.
      @param condition the ON condition for this join.
    */
    void addJoin(JoinType joinType, const QString &table, const Query::Condition &condition);

    /**
      Join a table to the query.
      This is a convenience method to create simple joins like e.g. 'LEFT JOIN t ON c1 = c2'.

      NOTE: make sure the @c JoinType is supported by the current @c QueryType
      @param joinType The type of JOIN you want to add.
      @param table The table to join.
      @param col1 The first column for the ON statement.
      @param col2 The second column for the ON statement.
    */
    void addJoin(JoinType joinType, const QString &table, const QString &col1, const QString &col2);

    /**
      Adds the given columns to a select query.
      @param cols The columns you want to select.
    */
    void addColumns(const QStringList &cols);

    /**
      Adds the given column to a select query.
      @param col The column to add.
    */
    void addColumn(const QString &col);

    /**
     * Adds the given case statement to a select query.
     * @param caseStmt The case statement to add.
     */
    void addColumn(const Query::Case &caseStmt);

    /**
     * Adds an aggregation statement.
     * @param col The column to aggregate on
     * @param aggregate The aggregation function.
     */
    void addAggregation(const QString &col, const QString &aggregate);

    /**
     * Adds and aggregation statement with CASE
     * @param caseStmt The case statement to aggregate on
     * @param aggregate The aggregation function.
     */
    void addAggregation(const Query::Case &caseStmt, const QString &aggregate);

    /**
      Add a WHERE or HAVING condition which compares a column with a given value.
      @param column The column that should be compared.
      @param op The operator used for comparison
      @param value The value @p column is compared to.
      @param type Defines whether this condition should be part of the WHERE or the HAVING
                  part of the query. Defaults to WHERE.
    */
    void addValueCondition(const QString &column, Query::CompareOperator op, const QVariant &value, ConditionType type = WhereCondition);

    /**
      Add a WHERE or HAVING condition which compares a column with another column.
      @param column The column that should be compared.
      @param op The operator used for comparison.
      @param column2 The column @p column is compared to.
      @param type Defines whether this condition should be part of the WHERE or the HAVING
                  part of the query. Defaults to WHERE.
    */
    void addColumnCondition(const QString &column, Query::CompareOperator op, const QString &column2, ConditionType type = WhereCondition);

    /**
      Add a WHERE condition. Use this to build hierarchical conditions.
      @param condition The condition that the resultset should satisfy.
      @param type Defines whether this condition should be part of the WHERE or the HAVING
                  part of the query. Defaults to WHERE.
    */
    void addCondition(const Query::Condition &condition, ConditionType type = WhereCondition);

    /**
      Define how WHERE or HAVING conditions are combined.
      @todo Give this method a better name.
      @param op The logical operator that should be used to combine the conditions.
      @param type Defines whether the operator should be used for WHERE or for HAVING
                  conditions. Defaults to WHERE conditions.
    */
    void setSubQueryMode(Query::LogicOperator op, ConditionType type = WhereCondition);

    /**
      Add sort column.
      @param column Name of the column to sort.
      @param order Sort order
    */
    void addSortColumn(const QString &column, Query::SortOrder order = Query::Ascending);

    /**
      Add a GROUP BY column.
      NOTE: Only supported in SELECT queries.
      @param column Name of the column to use for grouping.
    */
    void addGroupColumn(const QString &column);

    /**
      Add list of columns to GROUP BY.
      NOTE: Only supported in SELECT queries.
      @param columns Names of columns to use for grouping.
    */
    void addGroupColumns(const QStringList &columns);

    /**
      Sets a column to the given value (only valid for INSERT and UPDATE queries).
      @param column Column to change.
      @param value The value @p column should be set to.
    */
    void setColumnValue(const QString &column, const QVariant &value);

    /**
     * Specify whether duplicates should be included in the result.
     * @param distinct @c true to remove duplicates, @c false is the default
     */
    void setDistinct(bool distinct);

    /**
     * Limits the amount of retrieved rows.
     * @param limit the maximum number of rows to retrieve.
     * @note This has no effect on anything but SELECT queries.
     */
    void setLimit(int limit);

    /**
     * Sets the column used for identification in an INSERT statement.
     * The default is "id", only change this on tables without such a column
     * (usually n:m helper tables).
     * @param column Name of the identification column, empty string to disable this.
     * @note This only affects PostgreSQL.
     * @see insertId()
     */
    void setIdentificationColumn(const QString &column);

    /**
      Returns the query, only valid after exec().
    */
    QSqlQuery &query();

    /**
      Executes the query, returns true on success.
    */
    bool exec();

    /**
      Returns the ID of the newly created record (only valid for INSERT queries)
      @note This will assert when being used with setIdentificationColumn() called
      with an empty string.
      @returns -1 if invalid
    */
    qint64 insertId();

    /**
      Indicate to the database to acquire an exclusive lock on the rows already during
      SELECT statement.

      Only makes sense in SELECT queries.
     */
    void setForUpdate(bool forUpdate = true);

private:
    void buildQuery(QString *query);
    void bindValue(QString *query, const QVariant &value);
    void buildWhereCondition(QString *query, const Query::Condition &cond);
    void buildCaseStatement(QString *query, const Query::Case &caseStmt);

    /**
     * SQLite does not support JOINs with UPDATE, so we have to convert it into
     * subqueries
     */
    void sqliteAdaptUpdateJoin(Query::Condition &cond);

private:
    QString mTable;
    DbType::Type mDatabaseType;
    Query::Condition mRootCondition[NUM_CONDITIONS];
    QSqlQuery mQuery;
    QueryType mType;
    QStringList mColumns;
    QVector<QVariant> mBindValues;
    QVector<QPair<QString, Query::SortOrder> > mSortColumns;
    QStringList mGroupColumns;
    QVector<QPair<QString, QVariant> > mColumnValues;
    QString mIdentificationColumn;

    // we must make sure that the tables are joined in the correct order
    // QMap sorts by key which might invalidate the queries
    QStringList mJoinedTables;
    QMap< QString, QPair< JoinType, Query::Condition > > mJoins;
    int mLimit;
    bool mDistinct;
    bool mForUpdate = false;
#ifdef QUERYBUILDER_UNITTEST
    QString mStatement;
    friend class ::QueryBuilderTest;
#endif
};

} // namespace Server
} // namespace Akonadi

#endif
