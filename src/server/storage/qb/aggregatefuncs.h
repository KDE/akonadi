/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
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

#ifndef AKONADI_SERVER_QB_AGGREGATE_FUNCS_H_
#define AKONADI_SERVER_QB_AGGREGATE_FUNCS_H_

#include "column.h"

#include <QString>

namespace Akonadi
{
namespace Server
{
namespace Qb
{

class AggregateStmt : public ColumnStmt {
protected:
    AggregateStmt(const QString &column)
        : ColumnStmt(column)
    {}

};

class Sum : public AggregateStmt
{
public:
    explicit Sum(const QString &columnName)
        : AggregateStmt(columnName)
    {}

    template<typename ColumnDefinition,
             std::enable_if_t<!std::is_same<ColumnDefinition, QString>::value>>
    explicit Sum(ColumnDefinition &&column)
        : AggregateStmt{column.serialize()}
    {}

protected:
    QString serializeColumn(const QString &column) const override
    {
        return QStringLiteral("SUM(%1)").arg(column);
    }
};

class Count : public AggregateStmt
{
public:
    explicit Count()
        : AggregateStmt(QStringLiteral("*"))
    {}
    explicit Count(const QString &columnName)
        : AggregateStmt(columnName)
    {}

protected:
    QString serializeColumn(const QString &column) const override
    {
        return QStringLiteral("COUNT(%1)").arg(column);
    }
};

} // namespace Qb
} // namespace Server
} // namespace Akonadi

template<typename T>
static constexpr auto IsAggregateFunction(typename std::is_base_of<T, Akonadi::Server::Qb::AggregateStmt>::value);

#endif
