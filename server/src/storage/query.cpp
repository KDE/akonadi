/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "query.h"

using namespace Akonadi::Server;
using namespace Akonadi::Server::Query;

void Condition::addValueCondition( const QString &column, CompareOperator op, const QVariant &value )
{
  Q_ASSERT( !column.isEmpty() );
  Condition c;
  c.mColumn = column;
  c.mCompareOp = op;
  c.mComparedValue = value;
  mSubConditions << c;
}

void Condition::addColumnCondition( const QString &column, CompareOperator op, const QString &column2 )
{
  Q_ASSERT( !column.isEmpty() );
  Q_ASSERT( !column2.isEmpty() );
  Condition c;
  c.mColumn = column;
  c.mComparedColumn = column2;
  c.mCompareOp = op;
  mSubConditions << c;
}

Query::Condition::Condition( LogicOperator op )
  : mCompareOp( Equals )
  , mCombineOp( op )
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

void Query::Condition::setSubQueryMode( LogicOperator op )
{
  mCombineOp = op;
}

void Query::Condition::addCondition( const Condition &condition )
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

