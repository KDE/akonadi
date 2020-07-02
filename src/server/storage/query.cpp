/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "query.h"

using namespace Akonadi::Server;
using namespace Akonadi::Server::Query;

void Condition::addValueCondition(const QString &column, CompareOperator op, const QVariant &value)
{
    Q_ASSERT(!column.isEmpty());
    Condition c;
    c.mColumn = column;
    c.mCompareOp = op;
    c.mComparedValue = value;
    mSubConditions << c;
}

void Condition::addColumnCondition(const QString &column, CompareOperator op, const QString &column2)
{
    Q_ASSERT(!column.isEmpty());
    Q_ASSERT(!column2.isEmpty());
    Condition c;
    c.mColumn = column;
    c.mComparedColumn = column2;
    c.mCompareOp = op;
    mSubConditions << c;
}

Query::Condition::Condition(LogicOperator op)
    : mCompareOp(Equals)
    , mCombineOp(op)
{
}

bool Query::Condition::isEmpty() const
{
    return mSubConditions.isEmpty();
}

Condition::List Query::Condition::subConditions() const
{
    return mSubConditions;
}

void Query::Condition::setSubQueryMode(LogicOperator op)
{
    mCombineOp = op;
}

void Query::Condition::addCondition(const Condition &condition)
{
    mSubConditions << condition;
}

Case::Case(const Condition &when, const QString &then, const QString &elseBranch)
{
    addCondition(when, then);
    setElse(elseBranch);
}

Case::Case(const QString &column, CompareOperator op, const QVariant &value, const QString &when, const QString &elseBranch)
{
    addValueCondition(column, op, value, when);
    setElse(elseBranch);
}

void Case::addCondition(const Condition &when, const QString &then)
{
    mWhenThen.append(qMakePair(when, then));
}

void Case::addValueCondition(const QString &column, CompareOperator op, const QVariant &value, const QString &then)
{
    Condition when;
    when.addValueCondition(column, op, value);
    addCondition(when, then);
}

void Case::addColumnCondition(const QString &column, CompareOperator op, const QString &column2, const QString &then)
{
    Condition when;
    when.addColumnCondition(column, op, column2);
    addCondition(when, then);
}

void Case::setElse(const QString &elseBranch)
{
    mElse = elseBranch;
}
