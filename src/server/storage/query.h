/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_QUERY_H
#define AKONADI_QUERY_H

#include <QString>
#include <QVariant>
#include <QVector>

namespace Akonadi
{
namespace Server
{

class QueryBuilder;

/**
  Building blocks for SQL queries.
  @see QueryBuilder
*/
namespace Query
{

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
    In,
    NotIn,
    Like
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
    friend class Akonadi::Server::QueryBuilder;
public:
    /** A list of conditions. */
    typedef QVector<Condition> List;

    /**
      Create an empty condition.
      @param op how to combine sub queries.
    */
    explicit Condition(LogicOperator op = And);

    /**
      Add a WHERE condition which compares a column with a given value.
      @param column The column that should be compared.
      @param op The operator used for comparison
      @param value The value @p column is compared to.
    */
    void addValueCondition(const QString &column, CompareOperator op, const QVariant &value);

    /**
      Add a WHERE condition which compares a column with another column.
      @param column The column that should be compared.
      @param op The operator used for comparison.
      @param column2 The column @p column is compared to.
    */
    void addColumnCondition(const QString &column, CompareOperator op, const QString &column2);

    /**
      Add a WHERE condition. Use this method to build hierarchical conditions.
    */
    void addCondition(const Condition &condition);

    /**
      Set how sub-conditions should be combined, default is And.
    */
    void setSubQueryMode(LogicOperator op);

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

}; // class Condition

class Case
{
    friend class Akonadi::Server::QueryBuilder;
public:
    Case(const Condition &when, const QString &then, const QString &elseBranch = QString());
    Case(const QString &column, Query::CompareOperator op, const QVariant &value, const QString &when, const QString &elseBranch = QString());

    void addCondition(const Condition &when, const QString &then);
    void addValueCondition(const QString &column, Query::CompareOperator op, const QVariant &value, const QString &then);
    void addColumnCondition(const QString &column, Query::CompareOperator op, const QString &column2, const QString &then);

    void setElse(const QString &elseBranch);

private:
    QVector<QPair<Condition, QString> > mWhenThen;
    QString mElse;
};

} // namespace Query
} // namespace Server
} // namespace Akonadi

Q_DECLARE_TYPEINFO(Akonadi::Server::Query::Condition, Q_MOVABLE_TYPE);

#endif
