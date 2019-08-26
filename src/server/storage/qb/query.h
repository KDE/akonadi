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
#include "queryiterator.h"
#include "shared/akoptional.h"

#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QMap>
#include <QVariantList>

#include <iterator>

class QTextStream;

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
     * Returns query error, only valid after exec.
     */
    QSqlError &error() const;

    QueryResultIterator begin();
    QueryResultIterator end();

    /**
      Executes the query, returns true on success.
    */
    bool exec();

    virtual QTextStream &serialize(QTextStream &in) const = 0;
    virtual BoundValues bindValues() const = 0;

protected:
    /**
      Creates a new query builder.
    */
#ifdef QUERYBUILDER_UNITTEST
    explicit Query() = default;
#endif
    explicit Query(DataStore &db);

    DbType::Type databaseType() const;
private:
#ifndef QUERYBUILDER_UNITTEST
    DataStore &mDb;
#endif
    QSqlQuery mQuery;
    DbType::Type mDatabaseType = DbType::Unknown;
};

} // namespace Qb
} // namespace Server
} // namespace Akonadi

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::Query &query)
{
    return query.serialize(stream);
}

#endif
