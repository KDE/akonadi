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

#ifndef AKONADI_COUNTQUERYBUILDER_H
#define AKONADI_COUNTQUERYBUILDER_H

#include "storage/querybuilder.h"
#include "akonadiserver_debug.h"

#include <QSqlError>

namespace Akonadi
{
namespace Server
{

/**
  Helper class for creating queries to count elements in a database.
*/
class CountQueryBuilder : public QueryBuilder
{
public:
    enum CountMode {
        All,
        Distinct
    };

    /**
      Creates a new query builder that counts all entries in @p table.
    */
    explicit inline CountQueryBuilder(const QString &table)
        : QueryBuilder(table, Select)
    {
        addColumn(QStringLiteral("count(*)"));
    }

    /**
     * Creates a new query builder that counts entries in @p column of @p table.
     * If @p mode is set to @c Distinct, duplicate entries in that column are ignored.
     */
    inline CountQueryBuilder(const QString &table, const QString &column, CountMode mode)
        : QueryBuilder(table, Select)
    {
        Q_ASSERT(!table.isEmpty());
        Q_ASSERT(!column.isEmpty());
        QString s = QStringLiteral("count(");
        if (mode == Distinct) {
            s += QLatin1String("DISTINCT ");
        }
        s += column;
        s += QLatin1Char(')');
        addColumn(s);
    }

    /**
      Returns the result of this query.
      @returns -1 on error.
    */
    inline int result()
    {
        if (!query().next()) {
            qCDebug(AKONADISERVER_LOG) << "Error during retrieving result of query:" << query().lastError().text();
            return -1;
        }
        return query().value(0).toInt();
    }
};

} // namespace Server
} // namespace Akonadi

#endif
