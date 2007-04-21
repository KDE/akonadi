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

#ifndef AKONADI_QUERYBUILDER_H
#define AKONADI_QUERYBUILDER_H

#include "storage/datastore.h"

namespace Akonadi {

/**
  Helper class to construct arbitrary SQL queries.
*/
class QueryBuilder
{
  public:
    /**
      Creates a new query builder.
    */
    QueryBuilder();

    /**
      Add a table to the FROM part of the query.
      @param table The table name.
    */
    void addTable( const QString &table );

    /**
      Add a WHERE condition which compares a column with a given value.
      @param column The column that should be compared.
      @param op The operator used for comparison
      @param value The value @p column is compared to.
    */
    void addValueCondition( const QString &column, const char* op, const QVariant &value );

    /**
      Add a WHERE condition which compares a column with another column.
      @param column The column that should be compared.
      @param op The operator used for comparison.
      @param column2 The column @p column is compared to.
    */
    void addColumnCondition( const QString &column, const char* op, const QString column2 );

  protected:
    class Condition
    {
      public:
        QString column, column2;
        QString op;
        QVariant value;
    };
    QList<Condition> mConditions;
    QStringList mTables;
    QSqlQuery mQuery;
};

}

#endif
