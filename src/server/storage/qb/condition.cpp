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

#include "condition.h"
#include "shared/akhelpers.h"

#include <QTextStream>

using namespace Akonadi::Server::Qb;

QTextStream &operator<<(QTextStream &stream, Logic logic)
{
    switch (logic) {
    case Logic::And:
        return stream << u" AND "_sv;
    case Logic::Or:
        return stream << u" OR "_sv;
    }
}

QTextStream &operator<<(QTextStream &stream, Compare comp)
{
    switch (comp) {
    case Compare::Equals:
        return stream << u" = "_sv;
    case Compare::NotEquals:
        return stream << u" <> "_sv;
    case Compare::Is:
        return stream << u" IS "_sv;
    case Compare::IsNot:
        return stream << u" IS NOT "_sv;
    case Compare::Less:
        return stream << u" < "_sv;
    case Compare::LessOrEqual:
        return stream << u" <= "_sv;
    case Compare::Greater:
        return stream << u" > "_sv;
    case Compare::GreaterOrEqual:
        return stream << u" >= "_sv;
    case Compare::In:
        return stream << u" IN "_sv;
    case Compare::NotIn:
        return stream << u" NOT IN "_sv;
    case Compare::Like:
        return stream << u" LIKE "_sv;
    }
}

Query::BoundValues ConditionStmt::bindValues() const
{
    return AkVariant::visit(AkVariant::make_overload(
        [](AkVariant::monostate) {
            return Query::BoundValues{};
        },
        [](const ValueCondition &cond) {
            return Query::BoundValues{cond.value};
        },
        [](const ColumnCondition &) {
            return Query::BoundValues{};
        },
        [](const SubConditions &subconds) {
            Query::BoundValues rv;
            for (const auto &cond : subconds.subconditions) {
                rv += cond.bindValues();
            }
            return rv;
        }),
        mCond
    );
}

QTextStream &ConditionStmt::serialize(QTextStream &stream) const
{
    return AkVariant::visit(AkVariant::make_overload(
        [&stream](AkVariant::monostate) -> QTextStream & {
            return stream;
        },
        [&stream](const ValueCondition &cond) mutable -> QTextStream & {
            return stream << '(' << cond.column << cond.comp << u"?)"_sv;
        },
        [&stream](const ColumnCondition &cond) mutable -> QTextStream & {
            return stream << '(' << cond.lhCol << u" = "_sv << cond.rhCol << ')';
        },
        [&stream](const SubConditions &cond) mutable -> QTextStream & {
            stream << '(';
            for (auto subcond = cond.subconditions.cbegin(), end = cond.subconditions.cend(); subcond != end; ++subcond) {
                if (subcond != cond.subconditions.cbegin()) {
                    stream << cond.logic;
                }
                stream << *subcond;
            }
            return stream << ')';
        }),
        mCond
    );
}

