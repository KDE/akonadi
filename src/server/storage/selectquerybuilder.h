/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "storage/datastore.h"
#include "storage/querybuilder.h"

namespace Akonadi
{
namespace Server
{
/**
  Helper class for creating and executing database SELECT queries.
*/
template<typename T>
class SelectQueryBuilder : public QueryBuilder
{
public:
    /**
      Creates a new query builder.
    */
    inline SelectQueryBuilder()
        : SelectQueryBuilder(DataStore::self())
    {
    }

    explicit inline SelectQueryBuilder(DataStore *store)
        : QueryBuilder(store, T::tableName(), Select)
    {
        addColumns(T::fullColumnNames());
    }

    /**
      Returns the result of this SELECT query.
    */
    QList<T> result()
    {
        return T::extractResult(dataStore(), query());
    }
};

} // namespace Server
} // namespace Akonadi
