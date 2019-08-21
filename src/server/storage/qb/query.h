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

#ifndef AKONADI_QUERY_H
#define AKONADI_QUERY_H

#include "../dbtype.h"
#include "shared/akoptional.h"

#include <QString>
#include <QSqlQuery>
#include <QMap>
#include <QVariantList>

#ifdef QUERYBUILDER_UNITTEST
class QueryBuilderTest;
#endif

namespace Akonadi
{
namespace Server
{

class DataStore;

namespace Qb
{

/**
  Helper class to construct arbitrary SQL queries.
*/
class Query
{
public:
    using BoundValues = QVariantList;

    Query() = default;
    virtual ~Query() = default;

    /**
      Sets the database which should execute the query. Unfortunately the SQL "standard"
      is not interpreted in the same way everywhere...
    */
    void setDatabaseType(DbType::Type type);

    /**
      Returns the query, only valid after exec().
    */
    QSqlQuery &query();

    /**
      Executes the query, returns true on success.
    */
    bool exec();

    /**
     * Returns bound values for the query.
     */
    BoundValues boundValues() const;

    virtual BoundValues bindValues() const = 0;
protected:
    /**
      Creates a new query builder.
    */
    explicit Query(DataStore &db);

    DbType::Type databaseType() const;
private:
    DbType::Type mDatabaseType = DbType::Unknown;
    QSqlQuery mQuery;
    mutable akOptional<BoundValues> mBoundValues;

#ifdef QUERYBUILDER_UNITTEST
    QString mStatement;
    friend class ::QueryBuilderTest;
#endif
};

} // namespace Qb
} // namespace Server
} // namespace Akonadi

#endif
