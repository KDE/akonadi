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

#ifndef AKONADI_SELECTQUERYBUILDER_H
#define AKONADI_SELECTQUERYBUILDER_H

#include "storage/datastore.h"
#include "storage/querybuilder.h"

namespace Akonadi {

/**
  Helper class for creating and executing database SELECT queries.
*/
template <typename T> class SelectQueryBuilder : public QueryBuilder
{
  public:
    /**
      Creates a new query builder.
    */
    inline SelectQueryBuilder() : QueryBuilder()
    {
      addTable( T::tableName() );
    }

    /**
      Executes the query, returns true on success.
    */
    inline bool exec()
    {
      QString statement = QLatin1String( "SELECT " );
      statement += T::fullColumnNames().join( QLatin1String( ", " ) );
      statement += QLatin1String(" FROM ");
      statement += mTables.join( QLatin1String( ", " ) );
      if ( !mConditions.isEmpty() ) {
        statement += QLatin1String(" WHERE ");
        int i = 0;
        QStringList conds;
        foreach ( const Condition c, mConditions ) {
          QString cstmt = c.column;
          cstmt += QLatin1Char( ' ' );
          cstmt += c.op;
          cstmt += QLatin1Char( ' ' );
          if ( c.column2.isEmpty() ) {
            if ( c.value.isValid() )
              cstmt += QString::fromLatin1( ":%1" ).arg( i++ );
            else
              cstmt += QLatin1String( "NULL" );
          } else {
            cstmt += c.column2;
          }
          conds << cstmt;
        }
        statement += conds.join( QLatin1String( " AND " ) );
      }
      mQuery.prepare( statement );
      int i = 0;
      foreach ( const Condition c, mConditions )
        if ( c.column2.isEmpty() && c.value.isValid() )
          mQuery.bindValue( QString::fromLatin1( ":%1" ).arg( i++ ), c.value );
      if ( !mQuery.exec() ) {
        qDebug() << "Error during selecting records from table"
            << T::tableName() << mQuery.lastError().text();
        qDebug() << "Query was:" << statement;
        return false;
      }
      return true;
    }

    /**
      Returns the result of this SELECT query.
    */
    QList<T> result()
    {
      return T::extractResult( mQuery );
    }
};

}

#endif
