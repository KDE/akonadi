/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_SERVER_SELECTQUERY_H
#define AKONADI_SERVER_SELECTQUERY_H

#include "query.h"
#include "aggregatefuncs.h"
#include "condition.h"
#include "column.h"

#include <shared/akvariant.h>
#include <shared/akoptional.h>

#include <QString>
#include <QVariant>
#include <QVector>
#include <QTextStream>

#include <initializer_list>
#include <type_traits>

class QTextStream;

namespace Akonadi
{
namespace Server
{

class DataStore;

namespace Qb
{

enum class SortDirection {
    ASC,
    DESC
};

class SelectQuery;
class TableStmt
{
public:
    TableStmt() = default;
    TableStmt(const QString &table)
        : mTable(table)
    {}
    TableStmt(const SelectQuery &subSelect)
        : mTable(std::cref(subSelect))
    {}

    TableStmt &as(const QString &alias);

    QTextStream &serialize(QTextStream &stream) const;
    Query::BoundValues bindValues() const;
private:
    AkVariant::variant<QString, std::reference_wrapper<const SelectQuery>> mTable;
    QString mAlias;
};

class JoinStmt
{
public:
    JoinStmt() = default;

    enum class Type
    {
        Left,
        Right,
        Inner
    };

    TableStmt table;
    ConditionStmt onStmt;
    Type type = Type::Left;

    QTextStream &serialize(QTextStream &stream) const;
    Query::BoundValues bindValues() const;
};

class OrderByStmt
{
public:
    QString column;
    SortDirection order;

    QTextStream &serialize(QTextStream &stream) const;
};


class SelectQuery : public Query
{
public:
#ifdef QUERYBUILDER_UNITTEST
    explicit SelectQuery() = default;
#endif
    explicit SelectQuery(DataStore &db);

    SelectQuery &from(const TableStmt &table);

    SelectQuery &leftJoin(const TableStmt &table, const ConditionStmt &condition);
    SelectQuery &rightJoin(const TableStmt &table, const ConditionStmt &condition);
    SelectQuery &innerJoin(const TableStmt &table, const ConditionStmt &condition);

    SelectQuery &distinct();

    /**
      Adds the given columns to a select query.
      @param cols The columns you want to select.
    */
    SelectQuery &columns(const QStringList &cols);

    template<typename FirstCol, typename ... TailCols>
    SelectQuery &columns(FirstCol &&col, TailCols && ... tail)
    {
        column(std::forward<FirstCol>(col));
        return columns(std::forward<TailCols>(tail) ...);
    }

    template<typename Col>
    SelectQuery &columns(Col &&col)
    {
        return column(std::forward<Col>(col));
    }


    /**
     * Adds the given case statement to a select query.
     * @param caseStmt The case statement to add.
     */
    template<typename Column,
             std::enable_if_t<!std::is_same<Column, QString>::value, int> = 0>
    SelectQuery &column(Column &&col)
    {
        mColumns.push_back(col.serialize());
        return *this;
    }

    template<typename Column,
             std::enable_if_t<std::is_same<Column, QString>::value, int> = 0>
    SelectQuery &column(Column &&col)
    {
        mColumns.push_back(col);
        return *this;
    }

    /**
      Add a WHERE condition
    */
    SelectQuery &where(const ConditionStmt &condition);

    /**
      Add a HAVING condition
    */
    SelectQuery &having(const ConditionStmt &condition);

    /**
      Add sort column.
      @param column Name of the column to sort.
      @param order Sort order
    */
    SelectQuery &orderBy(const QString &column, SortDirection order = SortDirection::ASC);

    /**
      Add a GROUP BY column.
      @param column Name of the column to use for grouping.
    */
    SelectQuery &groupBy(const QString &column);

    /**
     * Limits the amount of retrieved rows.
     * @param limit the maximum number of rows to retrieve.
     */
    SelectQuery &limit(int limit);

    /**
     * Indicate to the database to acquire an exclusive lock on the rows already during
     * SELECT statement.
     */
    SelectQuery &forUpdate();

    QTextStream &serialize(QTextStream &stream) const override;

    BoundValues bindValues() const override;

private:
    akOptional<TableStmt> mTable;
    QStringList mColumns;
    QVector<JoinStmt> mJoins;
    akOptional<ConditionStmt> mWhere;
    akOptional<ConditionStmt> mHaving;
    QStringList mGroupBy;
    QVector<OrderByStmt> mOrderBy;
    akOptional<int> mLimit;
    bool mDistinct = false;
    bool mForUpdate = false;
};

SelectQuery Select(DataStore &db)
{
    return SelectQuery(db);
}

} // namespace Qb
} // namespace Server
} // namespace Akonadi

#endif
