/*
    Copyright (c) 2007 - 2012 Volker Krause <vkrause@kde.org>
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

#include "selectquery.h"
#include "shared/akhelpers.h"
#include "shared/akranges.h"

using namespace Akonadi::Server::Qb;

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::TableStmt &table)
{
    return table.serialize(stream);
}

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::JoinStmt &join)
{
    return join.serialize(stream);
}

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::OrderByStmt &order)
{
    return order.serialize(stream);
}

QTextStream &operator<<(QTextStream &stream, SortDirection dir)
{
    switch (dir) {
    case SortDirection::ASC:
        return stream << u" ASC"_sv;
    case SortDirection::DESC:
        return stream << u" DESC"_sv;
    }
}

TableStmt &TableStmt::as(const QString &alias)
{
    mAlias = alias;
    return *this;
}

QTextStream &TableStmt::serialize(QTextStream &stream) const
{
    AkVariant::visit(AkVariant::make_overload(
        [&stream](const QString &table) mutable {
            stream << table;
        },
        [&stream](const SelectQuery &subselect) mutable {
            stream << subselect;
        }),
        mTable
    );
    if (!mAlias.isEmpty()) {
        stream << u" AS "_sv << mAlias;
    }
    return stream;
}

Query::BoundValues TableStmt::bindValues() const
{
    return AkVariant::visit(AkVariant::make_overload(
        [](const QString &) {
            return Query::BoundValues{};
        },
        [](const SelectQuery &subselect) {
            return subselect.bindValues();
        }),
        mTable
    );
}

QTextStream &JoinStmt::serialize(QTextStream &stream) const
{
    switch (type) {
    case Type::Left:
        stream << u" LEFT JOIN "_sv;
        break;
    case Type::Right:
        stream << u" RIGHT JOIN "_sv;
        break;
    case Type::Inner:
        stream << u" INNER JOIN "_sv;
        break;
    }

    return stream << table << u" ON "_sv << onStmt;
}

Query::BoundValues JoinStmt::bindValues() const
{
    return table.bindValues() + onStmt.bindValues();
}

QTextStream &OrderByStmt::serialize(QTextStream &stream) const
{
    return stream << column << order;
}


SelectQuery::SelectQuery(DataStore &db)
    : Query(db)
{
    // Enable forward-only mode in SELECT queries - this is a memory optimization
    query().setForwardOnly(true);
}

SelectQuery &SelectQuery::from(const TableStmt &table)
{
    mTable = table;
    return *this;
}

SelectQuery &SelectQuery::leftJoin(const TableStmt &table, const ConditionStmt &cond)
{
    mJoins.push_back(JoinStmt{table, cond, JoinStmt::Type::Left});
    return *this;
}

SelectQuery &SelectQuery::rightJoin(const TableStmt &table, const ConditionStmt &cond)
{
    mJoins.push_back(JoinStmt{table, cond, JoinStmt::Type::Right});
    return *this;
}

SelectQuery &SelectQuery::innerJoin(const TableStmt &table, const ConditionStmt &cond)
{
    mJoins.push_back(JoinStmt{table, cond, JoinStmt::Type::Inner});
    return *this;
}

SelectQuery &SelectQuery::distinct()
{
    mDistinct = true;
    return *this;
}

SelectQuery &SelectQuery::columns(const QStringList &columns)
{
    mColumns = columns;
    return *this;
}

SelectQuery &SelectQuery::where(const ConditionStmt &cond)
{
    mWhere = cond;
    return *this;
}

SelectQuery &SelectQuery::where(ConditionStmt &&cond)
{
    mWhere = std::move(cond);
    return *this;
}

SelectQuery &SelectQuery::having(const ConditionStmt &cond)
{
    mHaving = cond;
    return *this;
}

SelectQuery &SelectQuery::having(ConditionStmt &&cond)
{
    mHaving = std::move(cond);
    return *this;
}

SelectQuery &SelectQuery::orderBy(const QString &column, SortDirection order)
{
    mOrderBy.push_back(OrderByStmt{column, order});
    return *this;
}

SelectQuery &SelectQuery::groupBy(const QString &column)
{
    mGroupBy.push_back(column);
    return *this;
}

SelectQuery &SelectQuery::limit(int limit)
{
    mLimit = limit;
    return *this;
}

SelectQuery &SelectQuery::forUpdate()
{
    mForUpdate = true;
    return *this;
}

Query::BoundValues SelectQuery::bindValues() const
{
    Q_ASSERT_X(mTable.has_value(), __func__, "SELECT query must specify a table binding values");
    BoundValues values;
    values += mTable->bindValues();
    for (const auto &join : mJoins) {
        values += join.bindValues();
    }
    if (mWhere.has_value()) {
        values += mWhere->bindValues();
    }
    if (mHaving.has_value()) {
        values += mHaving->bindValues();
    }

    return values;
}

QTextStream &SelectQuery::serialize(QTextStream &stream) const
{
    Q_ASSERT_X(mTable.has_value(), __func__, "SELECT query is missing a table.");
    Q_ASSERT_X(!mColumns.empty(), __func__, "SELECT query must specify at least one column.");


    stream << u"SELECT "_sv;
    if (mDistinct) {
        stream << u"DISTINCT "_sv;
    }
    for (auto col = mColumns.cbegin(), end = mColumns.cend(); col != end; ++col) {
        if (col != mColumns.cbegin()) {
            stream << u", "_sv;
        }
        stream << *col;
    }
    stream << u" FROM "_sv << *mTable;

    for (const auto &join : mJoins) {
        join.serialize(stream);
    }

    if (mWhere.has_value()) {
        stream << u" WHERE "_sv << *mWhere;
    }

    if (!mGroupBy.isEmpty()) {
        stream << u" GROUP BY "_sv << mGroupBy.join(", "_ls);
    }

    if (mHaving.has_value()) {
        stream << u" HAVING "_sv << *mHaving;
    }

    if (!mOrderBy.isEmpty()) {
        stream << u" ORDER BY "_sv;
        for (auto order = mOrderBy.cbegin(), end = mOrderBy.cend(); order != end; ++order) {
            if (order != mOrderBy.cbegin()) {
                stream << u", "_sv;
            }
            stream << *order;
        }
    }

    if (mLimit.has_value()) {
        stream << u" LIMIT "_sv << *mLimit;
    }

    return stream;
}

