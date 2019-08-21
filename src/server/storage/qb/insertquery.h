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

#ifndef AKONADI_SERVER_INSERTQUERY_H
#define AKONADI_SERVER_INSERTQUERY_H

#include "query.h"

#include <shared/akvariant.h>
#include <shared/akoptional.h>

#include <QString>
#include <QVariant>
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

/**
 * Represents an INSERT sql query
 */
class InsertQuery : public Query
{
public:
    explicit InsertQuery() = default;
    explicit InsertQuery(DataStore &db);

    /**
     * Sets the table to insert into.
     */
    InsertQuery &into(const QString &table);

    /**
     * Specifies the column and value to be inserted.
     */
    InsertQuery &value(const QString &column, const QVariant &values);

    /**
     * Sets the column used for identification in an INSERT statement.
     * The default is "id", only change this on tables without such a column
     * (usually n:m helper tables).
     * @param column Name of the identification column, empty string to disable this.
     * @note This only affects PostgreSQL.
     * @see insertId()
     */
    InsertQuery &returning(const QString &column);

    /**
      Returns the ID of the newly created record (only valid for INSERT queries)
      @note This will assert when being used with returning() called
      with an empty string.
      @returns -1 if invalid
    */
    qint64 insertId() const;

    QTextStream &serialize(QTextStream &stream) const;

    BoundValues bindValues() const override;

private:
    akOptional<QString> mTable;
    QString mReturning = QStringLiteral("id");
    QVariantMap mValues;
    qint64 mInsertId = -1;
};

InsertQuery Insert(DataStore &db)
{
    return InsertQuery(db);
}

} // namespace Qb
} // namespace Server
} // namespace Akonadi

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::InsertQuery &query)
{
    return query.serialize(stream);
}

#endif
