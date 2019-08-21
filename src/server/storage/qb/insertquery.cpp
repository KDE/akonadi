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

#include "insertquery.h"

#include <QTextStream>

using namespace Akonadi::Server::Qb;

InsertQuery::InsertQuery(DataStore &db)
    : Query(db)
{}

InsertQuery &InsertQuery::into(const QString &table)
{
    mTable = table;
    return *this;
}

InsertQuery &InsertQuery::value(const QString &column, const QVariant &value)
{
    mValues[column] = value;
    return *this;
}

InsertQuery &InsertQuery::returning(const QString &column)
{
    mReturning = column;
    return *this;
}

qint64 InsertQuery::insertId() const
{
    Q_ASSERT(!mReturning.isEmpty());
    return mInsertId;
}

QTextStream &InsertQuery::serialize(QTextStream &stream) const
{
    Q_ASSERT_X(mTable.has_value(), __func__, "INSERT query must specify table.");
    Q_ASSERT_X(!mValues.empty(), __func__, "INSERT query must have at least one value.");

    stream << QStringViewLiteral("INSERT INTO ") << *mTable << QStringViewLiteral(" (");
    for (auto it = mValues.cbegin(), end = mValues.cend(); it != end; ++it) {
        if (it != mValues.cbegin()) {
            stream << QStringViewLiteral(", ");
        }
        stream << it.key();
    }
    stream << QStringViewLiteral(") VALUES (");
    for (int i = 0; i < mValues.size(); ++i) {
        if (i > 0) {
            stream << QStringViewLiteral(", ");
        }
        stream << '?';
    }
    stream << ')';

    if (databaseType() == DbType::PostgreSQL && !mReturning.isEmpty()) {
        stream << QStringViewLiteral(" RETURNING ") << mReturning;
    }

    return stream;
}

Query::BoundValues InsertQuery::bindValues() const
{
    return mValues.values();
}
