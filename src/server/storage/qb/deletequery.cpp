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

#include "deletequery.h"
#include "shared/akhelpers.h"

#include <QTextStream>

using namespace Akonadi::Server::Qb;

DeleteQuery::DeleteQuery(DataStore &db)
    : Query(db)
{}

DeleteQuery &DeleteQuery::table(const QString &table)
{
    mTable = table;
    return *this;
}

DeleteQuery &DeleteQuery::where(const ConditionStmt &cond)
{
    mCondition = cond;
    return *this;
}

DeleteQuery &DeleteQuery::where(ConditionStmt &&cond)
{
    mCondition = std::move(cond);
    return *this;
}

QTextStream &DeleteQuery::serialize(QTextStream &stream) const
{
    Q_ASSERT_X(mTable.has_value(), __func__, "DELETE query must specify table to delete from.");

    stream << u"DELETE FROM "_sv << *mTable;

    if (mCondition.has_value()) {
        stream << u" WHERE "_sv << *mCondition;
    }

    return stream;
}

Query::BoundValues DeleteQuery::bindValues() const
{
    if (mCondition.has_value()) {
        return mCondition->bindValues();
    }
    return {};
}
