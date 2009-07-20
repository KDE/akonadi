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

#include "querybuilder.h"

#ifndef QUERYBUILDER_UNITTEST
#include "storage/datastore.h"
#endif

#include <QSqlRecord>

using namespace Akonadi;

static QString compareOperatorToString( Query::CompareOperator op )
{
  switch ( op ) {
    case Query::Equals:
      return QLatin1String( " = " );
    case Query::NotEquals:
      return QLatin1String( " <> " );
    case Query::Is:
      return QLatin1String( " IS " );
    case Query::IsNot:
      return QLatin1String( " IS NOT " );
    case Query::Less:
      return QLatin1String( " < " );
    case Query::LessOrEqual:
      return QLatin1String( " <= " );
    case Query::Greater:
      return QLatin1String( " > " );
    case Query::GreaterOrEqual:
      return QLatin1String( " >= " );
    case Query::In:
      return QLatin1String( " IN " );
    case Query::NotIn:
      return QLatin1String( " NOT IN " );
  }
  Q_ASSERT_X( false, "QueryBuilder::compareOperatorToString()", "Unknown compare operator." );
  return QString();
}

static QString logicOperatorToString( Query::LogicOperator op )
{
  switch ( op ) {
    case Query::And:
      return QLatin1String( " AND " );
    case Query::Or:
      return QLatin1String( " OR " );
  }
  Q_ASSERT_X( false, "QueryBuilder::logicOperatorToString()", "Unknown logic operator." );
  return QString();
}

static QString sortOrderToString( Query::SortOrder order )
{
  switch ( order ) {
    case Query::Ascending:
      return QLatin1String( " ASC" );
    case Query::Descending:
      return QLatin1String( " DESC" );
  }
  Q_ASSERT_X( false, "QueryBuilder::sortOrderToString()", "Unknown sort order." );
  return QString();
}

QueryBuilder::QueryBuilder( QueryType type ) :
#ifndef QUERYBUILDER_UNITTEST
    mDatabaseType( qsqlDriverNameToDatabaseType( DataStore::self()->database().driverName() ) ),
    mQuery( DataStore::self()->database() ),
#else
    mDatabaseType( Unknown ),
#endif
    mType( type ),
    mDistinct( false )
{
}

void QueryBuilder::setDatabaseType( DatabaseType type )
{
  mDatabaseType = type;
}

void QueryBuilder::addTable(const QString & table)
{
  if ( !mTables.contains( table ) )
    mTables << table;
}

void QueryBuilder::addValueCondition(const QString & column, Query::CompareOperator op, const QVariant & value)
{
  mRootCondition.addValueCondition( column, op, value );
}

void QueryBuilder::addColumnCondition(const QString & column, Query::CompareOperator op, const QString & column2)
{
  mRootCondition.addColumnCondition( column, op, column2 );
}

QSqlQuery& QueryBuilder::query()
{
  return mQuery;
}

bool QueryBuilder::exec()
{
  QString statement;

  switch ( mType ) {
    case Select:
      statement += QLatin1String( "SELECT " );
      if ( mDistinct )
        statement += QLatin1String( "DISTINCT " );
      Q_ASSERT_X( mColumns.count() > 0, "QueryBuilder::exec()", "No columns specified" );
      statement += mColumns.join( QLatin1String( ", " ) );
      statement += QLatin1String(" FROM ");
      Q_ASSERT_X( mTables.count() > 0, "QueryBuilder::exec()", "No tables specified" );
      statement += mTables.join( QLatin1String( ", " ) );
     break;
    case Insert:
    {
      statement += QLatin1String( "INSERT INTO " );
      Q_ASSERT_X( mTables.count() == 1, "QueryBuilder::exec()", "Specify exactly one table" );
      statement += mTables.first();
      statement += QLatin1String(" (");
      typedef QPair<QString,QVariant> StringVariantPair;
      QStringList cols, vals;
      foreach ( const StringVariantPair &p, mColumnValues ) {
        cols.append( p.first );
        vals.append( bindValue( p.second ) );
      }
      statement += cols.join( QLatin1String( ", " ) );
      statement += QLatin1String(") VALUES (");
      statement += vals.join( QLatin1String( ", " ) );
      statement += QLatin1Char(')');
      if ( mDatabaseType == PostgreSQL )
        statement += QLatin1String( " RETURNING id" );
      break;
    }
    case Update:
    {
      statement += QLatin1String( "UPDATE " );
      Q_ASSERT_X( mTables.count() > 0, "QueryBuilder::exec()", "No tables specified" );

      if ( mDatabaseType == MySQL ) {
        statement += mTables.join( QLatin1String( ", " ) );
      } else {
         statement += mTables.at( 0 );
      }

      statement += QLatin1String( " SET " );
      Q_ASSERT_X( mColumnValues.count() >= 1, "QueryBuilder::exec()", "At least one column needs to be changed" );
      typedef QPair<QString,QVariant> StringVariantPair;
      QStringList updStmts;
      foreach ( const StringVariantPair &p, mColumnValues ) {
        QString updStmt = p.first;
        updStmt += QLatin1String( " = " );
        updStmt += bindValue( p.second );
        updStmts << updStmt;
      }
      statement += updStmts.join( QLatin1String( ", " ) );

      if ( mDatabaseType != MySQL ) {
        // FROM TABLES
        QString part = QLatin1String( " FROM" );
        for ( int i = 1; i <  mTables.size(); i++ ) {
          statement += part + QLatin1String( " " ) + mTables.at( i );
          part = QLatin1String( " JOIN " );
        }
      }

      break;
    }
    case Delete:
      statement += QLatin1String( "DELETE FROM " );
      Q_ASSERT_X( mTables.count() == 1, "QueryBuilder::exec()", "Exactly one table needed" );
      statement += mTables.first();
      break;
    default:
      Q_ASSERT_X( false, "QueryBuilder::exec()", "Unknown enum value" );
  }

  if ( !mRootCondition.isEmpty() ) {
    statement += QLatin1String(" WHERE ");
    statement += buildWhereCondition( mRootCondition );
  }

  if ( !mSortColumns.isEmpty() ) {
    Q_ASSERT_X( mType == Select, "QueryBuilder::exec()", "Order statements are only valid for SELECT queries" );
    QStringList orderStmts;
    typedef QPair<QString, Query::SortOrder> SortColumnInfo;
    foreach ( const SortColumnInfo &order, mSortColumns ) {
      QString orderStmt;
      orderStmt += order.first;
      orderStmt += sortOrderToString( order.second );
      orderStmts << orderStmt;
    }
    statement += QLatin1String( " ORDER BY " );
    statement += orderStmts.join( QLatin1String( ", " ) );
  }

#ifndef QUERYBUILDER_UNITTEST
  mQuery.prepare( statement );
  //too heavy debug info but worths to have from time to time
//   qDebug() << "Executing query" << statement;
  for ( int i = 0; i < mBindValues.count(); ++i )
  {
    mQuery.bindValue( QString::fromLatin1( ":%1" ).arg( i ), mBindValues[i] );
//     qDebug() << QString::fromLatin1( ":%1" ).arg( i ) <<  mBindValues[i];
  }
  if ( !mQuery.exec() ) {
    qDebug() << "Error during executing query" << statement << ": " << mQuery.lastError().text();
    return false;
  }
#else
  mStatement = statement;
#endif
  return true;
}

void QueryBuilder::addColumns(const QStringList & cols)
{
  mColumns << cols;
}

void QueryBuilder::addColumn(const QString & col)
{
  mColumns << col;
}

QString QueryBuilder::bindValue(const QVariant & value)
{
  mBindValues << value;
  return QString::fromLatin1( ":%1" ). arg( mBindValues.count() - 1 );
}

QString QueryBuilder::buildWhereCondition(const Query::Condition & cond)
{
  if ( !cond.isEmpty() ) {
    QStringList conds;
    foreach ( const Query::Condition &c, cond.subConditions() ) {
      conds << buildWhereCondition( c );
    }
    return QLatin1String( "( " ) + conds.join( logicOperatorToString( cond.mCombineOp ) ) + QLatin1String( " )" );
  } else {
    QString stmt = cond.mColumn;
    stmt += compareOperatorToString( cond.mCompareOp );
    if ( cond.mComparedColumn.isEmpty() ) {
      if ( cond.mComparedValue.isValid() ) {
        if ( cond.mComparedValue.canConvert( QVariant::StringList ) ) {
          stmt += QLatin1String( "( " );
          QStringList entries;
          Q_ASSERT_X( !cond.mComparedValue.toStringList().isEmpty(),
                      "QueryBuilder::buildWhereCondition()", "No values given for IN condition." );
          foreach ( const QString &entry, cond.mComparedValue.toStringList() ) {
            entries << bindValue( entry );
          }
          stmt += entries.join( QLatin1String( ", " ) );
          stmt += QLatin1String( " )" );
        } else {
          stmt += bindValue( cond.mComparedValue );
        }
      } else {
        stmt += QLatin1String( "NULL" );
      }
    } else {
      stmt += cond.mComparedColumn;
    }
    return stmt;
  }
}

void QueryBuilder::setSubQueryMode(Query::LogicOperator op)
{
  mRootCondition.setSubQueryMode( op );
}

void QueryBuilder::addCondition(const Query::Condition & condition)
{
  mRootCondition.addCondition( condition );
}

void QueryBuilder::addSortColumn(const QString & column, Query::SortOrder order )
{
  mSortColumns << qMakePair( column, order );
}

void QueryBuilder::setColumnValue(const QString & column, const QVariant & value)
{
  mColumnValues << qMakePair( column, value );
}

void QueryBuilder::setDistinct(bool distinct)
{
  mDistinct = distinct;
}

qint64 QueryBuilder::insertId()
{
  if ( mDatabaseType == PostgreSQL ) {
    query().next();
    return query().record().value( QLatin1String("id") ).toLongLong();
  } else {
    const QVariant v = query().lastInsertId();
    if ( !v.isValid() ) return -1;
    bool ok;
    const qint64 insertId = v.toLongLong( &ok );
    if ( !ok ) return -1;
    return insertId;
  }
  return -1;
}

QueryBuilder::DatabaseType QueryBuilder::qsqlDriverNameToDatabaseType (const QString & driverName)
{
  if ( driverName == QLatin1String( "QMYSQL" ) )
    return MySQL;
  if ( driverName == QLatin1String( "QPSQL" ) )
    return PostgreSQL;
  if ( driverName == QLatin1String( "SQLITE" ) )
    return Sqlite;
  return Unknown;
}
