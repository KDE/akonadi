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

#include "updatequery.h"
#include "shared/akhelpers.h"

#include <QTextStream>

using namespace Akonadi::Server::Qb;

UpdateQuery::UpdateQuery(DataStore &db)
    : Query(db)
{}

UpdateQuery &UpdateQuery::table(const QString &table)
{
    mTable = table;
    return *this;
}

UpdateQuery &UpdateQuery::value(const QString &column, const QVariant &value)
{
    mValues[column] = value;
    return *this;
}

UpdateQuery &UpdateQuery::where(const ConditionStmt &cond)
{
    mCondition = cond;
    return *this;
}

UpdateQuery &UpdateQuery::where(ConditionStmt &&cond)
{
    mCondition = std::move(cond);
    return *this;
}

QTextStream &UpdateQuery::serialize(QTextStream &stream) const
{
    Q_ASSERT_X(mTable.has_value(), __func__, "UPDATE query must specify table to update");
    Q_ASSERT_X(!mValues.empty(), __func__, "UPDATE query must update at least one value");

    stream << u"UPDATE "_sv << *mTable << u" SET "_sv;
    for (auto it = mValues.cbegin(), end = mValues.cend(); it != end; ++it) {
        if (it != mValues.cbegin()) {
            stream << u", "_sv;
        }
        stream << it.key() << u" = ?"_sv;
    }

    if (mCondition.has_value()) {
        stream << u" WHERE "_sv << *mCondition;
    }

    return stream;
}

Query::BoundValues UpdateQuery::bindValues() const
{
    auto values = mValues.values();
    if (mCondition.has_value()) {
        values += mCondition->bindValues();
    }
    return values;
}

