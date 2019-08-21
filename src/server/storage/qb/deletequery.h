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

#ifndef AKONADI_SERVER_DELETEQUERY_H
#define AKONADI_SERVER_DELETEQUERY_H

#include "query.h"
#include "condition.h"

class QTextStream;

namespace Akonadi
{
namespace Server
{
namespace Qb
{

class DeleteQuery : public Query
{
public:
    explicit DeleteQuery() = default;
    explicit DeleteQuery(DataStore &db);

    DeleteQuery &table(const QString &table);

    DeleteQuery &where(const ConditionStmt &condition);
    DeleteQuery &where(ConditionStmt &&condition);

    QTextStream &serialize(QTextStream &stream) const;
    BoundValues bindValues() const;

private:
    akOptional<QString> mTable;
    akOptional<ConditionStmt> mCondition;
};

DeleteQuery Delete(DataStore &db)
{
    return DeleteQuery(db);
}

} // namespace Qb
} // namespace Server
} // namepsace Akonadi

QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::DeleteQuery &query)
{
    return query.serialize(stream);
}

#endif
