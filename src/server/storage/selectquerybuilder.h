/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SELECTQUERYBUILDER_H
#define AKONADI_SELECTQUERYBUILDER_H

#include "storage/querybuilder.h"

namespace Akonadi
{
namespace Server
{

/**
  Helper class for creating and executing database SELECT queries.
*/
template <typename T> class SelectQueryBuilder : public QueryBuilder
{
public:
    /**
      Creates a new query builder.
    */
    inline SelectQueryBuilder()
        : QueryBuilder(T::tableName(), Select)
    {
        addColumns(T::fullColumnNames());
    }

    /**
      Returns the result of this SELECT query.
    */
    QVector<T> result()
    {
        return T::extractResult(query());
    }
};

} // namespace Server
} // namespace Akonadi

#endif
