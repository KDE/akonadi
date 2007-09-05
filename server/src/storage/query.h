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

#ifndef AKONADI_QUERY_H
#define AKONADI_QUERY_H

#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Akonadi {

class QueryBuilder;

/**
  Building blocks for SQL queries.
  @see QueryBuilder
*/
namespace Query {

/**
  Compare operators to be used in query conditions.
*/
enum CompareOperator {
  Equals,
  NotEquals,
  Is,
  IsNot,
  Less,
  LessOrEqual,
  Greater,
  GreaterOrEqual,
  In
};

/**
  Logic operations used to combine multiple query conditions.
*/
enum LogicOperator {
  And,
  Or
};

/**
  Sort orders.
*/
enum SortOrder {
  Ascending,
  Descending
};

/**
  Represents a WHERE condition tree.
*/
class Condition
{
  friend class Akonadi::QueryBuilder;
  public:
    /** A list of conditions. */
    typedef QList<Condition> List;

    /**
      Create an empty condition.
      @param op how to combine sub queries.
    */
    Condition( LogicOperator op = And );

    /**
      Add a WHERE condition which compares a column with a given value.
      @param column The column that should be compared.
      @param op The operator used for comparison
      @param value The value @p column is compared to.
    */
    void addValueCondition( const QString &column, CompareOperator op, const QVariant &value );

    /**
      Add a WHERE condition which compares a column with another column.
      @param column The column that should be compared.
      @param op The operator used for comparison.
      @param column2 The column @p column is compared to.
    */
    void addColumnCondition( const QString &column, CompareOperator op, const QString &column2 );

    /**
      Add a WHERE condition. Use this method to build hierarchical conditions.
    */
    void addCondition( const Condition &condition );

    /**
      Set how sub-conditions should be combined, default is And.
    */
    void setSubQueryMode( LogicOperator op );

    /**
      Returns if there are sub conditions.
    */
    bool isEmpty() const;

    /**
      Returns the list of sub-conditions.
    */
    Condition::List subConditions() const;

  private:
    Condition::List mSubConditions;
    QString mColumn;
    QString mComparedColumn;
    QVariant mComparedValue;
    CompareOperator mCompareOp;
    LogicOperator mCombineOp;
};

}

}

#endif
