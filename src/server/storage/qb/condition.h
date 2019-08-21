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

#ifndef AKONADI_SERVER_CONDITION_H_
#define AKONADI_SERVER_CONDITION_H_

#include "query.h"
#include <shared/akvariant.h>

#include <QString>
#include <QVariant>
#include <QVector>

#include <initializer_list>

class QTextStream;

namespace Akonadi
{
namespace Server
{
namespace Qb
{

enum class Logic {
    And,
    Or
};

enum class Compare {
    Equals,
    NotEquals,
    Is,
    IsNot,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    In,
    NotIn,
    Like
};

class ConditionStmt {
public:
    ConditionStmt() = default;

    inline ConditionStmt(const QString &col1, const QString &col2)
        : mCond(ColumnCondition{col1, col2})
    {}
    inline ConditionStmt(const QString &col, Compare compare, const QVariant &value)
        : mCond(ValueCondition{col, value, compare})
    {}
    inline ConditionStmt(Logic logic, const QVector<ConditionStmt> &subconditions)
        : mCond(SubConditions{subconditions, logic})
    {}

    QTextStream &serialize(QTextStream &stream) const;

    Query::BoundValues bindValues() const;
private:
    struct ValueCondition {
        QString column;
        QVariant value;
        Compare comp;
    };
    struct ColumnCondition {
        QString lhCol;
        QString rhCol;
    };
    struct SubConditions {
        QVector<ConditionStmt> subconditions;
        Logic logic;
    };
    AkVariant::variant<AkVariant::monostate, ValueCondition, ColumnCondition, SubConditions> mCond;
};

inline ConditionStmt On(const QString &col1, const QString &col2)
{
    return {col1, col2};
}

inline ConditionStmt On(const QString &col1, Compare compare, const QVariant &value)
{
    return {col1, compare, value};
}

inline ConditionStmt On(ConditionStmt &&stmt)
{
    return std::move(stmt);
}

inline ConditionStmt And(std::initializer_list<ConditionStmt> conditions)
{
    return ConditionStmt(Logic::And, conditions);
}

inline ConditionStmt Or(std::initializer_list<ConditionStmt> conditions)
{
    return ConditionStmt(Logic::Or, conditions);
}

} // namespace Qb
}
}

inline QTextStream &operator<<(QTextStream &stream, const Akonadi::Server::Qb::ConditionStmt &cond)
{
    return cond.serialize(stream);
}

#endif

