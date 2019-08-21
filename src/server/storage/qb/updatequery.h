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

#ifndef AKONADI_SERVER_UPDATEQUERY_H
#define AKONADI_SERVER_UPDATEQUERY_H

#include "query.h"
#include "condition.h"

#include "shared/akoptional.h"

class QTextStream;

namespace Akonadi
{
namespace Server
{
namespace Qb
{

class UpdateQuery : public Query
{
public:
    explicit UpdateQuery() = default;
    explicit UpdateQuery(DataStore &db);

    UpdateQuery &table(const QString &table);

    UpdateQuery &value(const QString &column, const QVariant &value);

    UpdateQuery &where(const ConditionStmt &condition);
    UpdateQuery &where(ConditionStmt &&condition);

    QTextStream &serialize(QTextStream &stream) const;
    BoundValues bindValues() const override;

private:
    akOptional<QString> mTable;
    QVariantMap mValues;
    akOptional<ConditionStmt> mCondition;
};

UpdateQuery Update(DataStore &db)
{
    return UpdateQuery(db);
}

} // namespace Qb
} // namespace Server
} // namespace Akonadi

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::UpdateQuery &query)
{
    return query.serialize(stream);
}


#endif
