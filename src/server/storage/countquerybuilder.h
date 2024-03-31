/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiserver_debug.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"

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
        Distinct,
    };

    /**
      Creates a new query builder that counts all entries in @p table.
    */
    explicit inline CountQueryBuilder(const QString &table)
        : CountQueryBuilder(DataStore::self(), table)
    {
    }

    inline CountQueryBuilder(DataStore *store, const QString &table)
        : QueryBuilder(store, table, Select)
    {
        addColumn(QStringLiteral("count(*)"));
    }

    /**
     * Creates a new query builder that counts entries in @p column of @p table.
     * If @p mode is set to @c Distinct, duplicate entries in that column are ignored.
     */
    inline CountQueryBuilder(const QString &table, const QString &column, CountMode mode)
        : CountQueryBuilder(DataStore::self(), table, column, mode)
    {
    }

    inline CountQueryBuilder(DataStore *store, const QString &table, const QString &column, CountMode mode)
        : QueryBuilder(store, table, Select)
    {
        Q_ASSERT(!table.isEmpty());
        Q_ASSERT(!column.isEmpty());
        QString s = QStringLiteral("count(");
        if (mode == Distinct) {
            s += QLatin1StringView("DISTINCT ");
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
        const auto result = query().value(0).toInt();
        query().finish();
        return result;
    }
};

} // namespace Server
} // namespace Akonadi
