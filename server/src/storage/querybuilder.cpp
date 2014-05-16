/*
    Copyright (c) 2007 - 2012 Volker Krause <vkrause@kde.org>

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
#include <akdebug.h>

#ifndef QUERYBUILDER_UNITTEST
#include "storage/datastore.h"
#include "storage/querycache.h"
#include "storage/storagedebugger.h"
#endif

#include <QSqlRecord>
#include <QSqlError>

using namespace Akonadi::Server;

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
  case Query::Like:
    return QLatin1String( " LIKE " );
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

QueryBuilder::QueryBuilder( const QString &table, QueryBuilder::QueryType type )
   : mTable( table )
#ifndef QUERYBUILDER_UNITTEST
   , mDatabaseType( DbType::type( DataStore::self()->database() ) )
   , mQuery( DataStore::self()->database() )
#else
   , mDatabaseType( DbType::Unknown )
#endif
   , mType( type )
   , mIdentificationColumn( QLatin1String( "id" ) )
   , mLimit( -1 )
   , mDistinct( false )
{
}

void QueryBuilder::setDatabaseType( DbType::Type type )
{
  mDatabaseType = type;
}

void QueryBuilder::addJoin( JoinType joinType, const QString &table, const Query::Condition &condition )
{
  Q_ASSERT( ( joinType == InnerJoin && ( mType == Select || mType == Update ) ) ||
            ( joinType == LeftJoin && mType == Select ) );

  if ( mJoinedTables.contains( table ) ) {
    // InnerJoin is more restrictive than a LeftJoin, hence use that in doubt
    mJoins[table].first = qMin( joinType, mJoins.value( table ).first );
    mJoins[table].second.addCondition( condition );
  } else {
    mJoins[table] = qMakePair( joinType, condition );
    mJoinedTables << table;
  }
}

void QueryBuilder::addJoin( JoinType joinType, const QString &table, const QString &col1, const QString &col2 )
{
  Query::Condition condition;
  condition.addColumnCondition( col1, Query::Equals, col2 );
  addJoin( joinType, table, condition );
}

void QueryBuilder::addValueCondition( const QString &column, Query::CompareOperator op, const QVariant &value, ConditionType type )
{
  Q_ASSERT( type == WhereCondition || ( type == HavingCondition && mType == Select ) );
  mRootCondition[type].addValueCondition( column, op, value );
}

void QueryBuilder::addColumnCondition( const QString &column, Query::CompareOperator op, const QString &column2, ConditionType type )
{
  Q_ASSERT( type == WhereCondition || ( type == HavingCondition && mType == Select ) );
  mRootCondition[type].addColumnCondition( column, op, column2 );
}

QSqlQuery &QueryBuilder::query()
{
  return mQuery;
}

void QueryBuilder::sqliteAdaptUpdateJoin( Query::Condition &condition )
{
  // FIXME: This does not cover all cases by far. It however can handle most
  // (probably all) of the update-join queries we do in Akonadi and convert them
  // properly into a SQLite-compatible query. Better than nothing ;-)

  if ( !condition.mSubConditions.isEmpty() ) {
    for ( int i = condition.mSubConditions.count() - 1; i >= 0; --i ) {
      sqliteAdaptUpdateJoin( condition.mSubConditions[i] );
    }
    return;
  }

  QString table;
  if ( condition.mColumn.contains( QLatin1Char( '.' ) ) ) {
    table = condition.mColumn.left( condition.mColumn.indexOf( QLatin1Char( '.' ) ) );
  } else {
    return;
  }

  if ( !mJoinedTables.contains( table ) ) {
    return;
  }

  const QPair<JoinType, Query::Condition> joinCondition = mJoins.value( table );

  QueryBuilder qb( table, Select );
  qb.addColumn( condition.mColumn );
  qb.addCondition( joinCondition.second );

  // Convert the subquery to string
  condition.mColumn = QLatin1String( "( " ) + qb.buildQuery() + QLatin1String( " )" );
}


QString QueryBuilder::buildQuery()
{
  QString statement;

  // we add the ON conditions of Inner Joins in a Update query here
  // but don't want to change the mRootCondition on each exec().
  Query::Condition whereCondition = mRootCondition[WhereCondition];

  switch ( mType ) {
  case Select:
    statement += QLatin1String( "SELECT " );
    if ( mDistinct ) {
      statement += QLatin1String( "DISTINCT " );
    }
    Q_ASSERT_X( mColumns.count() > 0, "QueryBuilder::exec()", "No columns specified" );
    statement += mColumns.join( QLatin1String( ", " ) );
    statement += QLatin1String( " FROM " );
    statement += mTable;
    Q_FOREACH ( const QString &joinedTable, mJoinedTables ) {
      const QPair<JoinType, Query::Condition> &join = mJoins.value( joinedTable );
      switch ( join.first ) {
      case LeftJoin:
        statement += QLatin1String( " LEFT JOIN " );
        break;
      case InnerJoin:
        statement += QLatin1String( " INNER JOIN " );
        break;
      }
      statement += joinedTable;
      statement += QLatin1String( " ON " );
      statement += buildWhereCondition( join.second );
    }
    break;
  case Insert:
  {
    statement += QLatin1String( "INSERT INTO " );
    statement += mTable;
    statement += QLatin1String( " (" );
    typedef QPair<QString,QVariant> StringVariantPair;
    QStringList cols, vals;
    Q_FOREACH ( const StringVariantPair &p, mColumnValues ) {
      cols.append( p.first );
      vals.append( bindValue( p.second ) );
    }
    statement += cols.join( QLatin1String( ", " ) );
    statement += QLatin1String( ") VALUES (" );
    statement += vals.join( QLatin1String( ", " ) );
    statement += QLatin1Char( ')' );
    if ( mDatabaseType == DbType::PostgreSQL && !mIdentificationColumn.isEmpty() ) {
      statement += QLatin1String( " RETURNING " ) + mIdentificationColumn;
    }
    break;
  }
  case Update:
  {
    // put the ON condition into the WHERE part of the UPDATE query
    if ( mDatabaseType != DbType::Sqlite ) {
      Q_FOREACH ( const QString &table, mJoinedTables ) {
        const QPair< JoinType, Query::Condition > &join = mJoins.value( table );
        Q_ASSERT( join.first == InnerJoin );
        whereCondition.addCondition( join.second );
      }
    } else {
      // Note: this will modify the whereCondition
      sqliteAdaptUpdateJoin( whereCondition );
    }

    statement += QLatin1String( "UPDATE " );
    statement += mTable;

    if ( mDatabaseType == DbType::MySQL && !mJoinedTables.isEmpty() ) {
      // for mysql we list all tables directly
      statement += QLatin1String( ", " );
      statement += mJoinedTables.join( QLatin1String( ", " ) );
    }

    statement += QLatin1String( " SET " );
    Q_ASSERT_X( mColumnValues.count() >= 1, "QueryBuilder::exec()", "At least one column needs to be changed" );
    typedef QPair<QString,QVariant> StringVariantPair;
    QStringList updStmts;
    Q_FOREACH ( const StringVariantPair &p, mColumnValues ) {
      QString updStmt = p.first;
      updStmt += QLatin1String( " = " );
      updStmt += bindValue( p.second );
      updStmts << updStmt;
    }
    statement += updStmts.join( QLatin1String( ", " ) );

    if ( mDatabaseType == DbType::PostgreSQL && !mJoinedTables.isEmpty() ) {
      // PSQL have this syntax
      // FROM t1 JOIN t2 JOIN ...
      statement += QLatin1String( " FROM " );
      statement += mJoinedTables.join( QLatin1String( " JOIN " ) );
    }

    break;
  }
  case Delete:
    statement += QLatin1String( "DELETE FROM " );
    statement += mTable;
    break;
  default:
    Q_ASSERT_X( false, "QueryBuilder::exec()", "Unknown enum value" );
  }

  if ( !whereCondition.isEmpty() ) {
    statement += QLatin1String( " WHERE " );
    statement += buildWhereCondition( whereCondition );
  }

  if ( !mGroupColumns.isEmpty() ) {
    statement += QLatin1String( " GROUP BY " );
    statement += mGroupColumns.join( QLatin1String( ", " ) );
  }

  if ( !mRootCondition[HavingCondition].isEmpty() ) {
    statement += QLatin1String( " HAVING " );
    statement += buildWhereCondition( mRootCondition[HavingCondition] );
  }

  if ( !mSortColumns.isEmpty() ) {
    Q_ASSERT_X( mType == Select, "QueryBuilder::exec()", "Order statements are only valid for SELECT queries" );
    QStringList orderStmts;
    typedef QPair<QString, Query::SortOrder> SortColumnInfo;
    Q_FOREACH ( const SortColumnInfo &order, mSortColumns ) {
      QString orderStmt;
      orderStmt += order.first;
      orderStmt += sortOrderToString( order.second );
      orderStmts << orderStmt;
    }
    statement += QLatin1String( " ORDER BY " );
    statement += orderStmts.join( QLatin1String( ", " ) );
  }

  if ( mLimit > 0 ) {
    statement += QLatin1Literal( " LIMIT " ) + QString::number( mLimit );
  }

  return statement;
}

bool QueryBuilder::retryLastTransaction( bool rollback )
{
#ifndef QUERYBUILDER_UNITTEST
  mQuery = DataStore::self()->retryLastTransaction( rollback );
  return !mQuery.lastError().isValid();
#else
  Q_UNUSED(rollback);
  return true;
#endif
}


bool QueryBuilder::exec()
{
  const QString statement = buildQuery();

#ifndef QUERYBUILDER_UNITTEST
  if ( QueryCache::contains( statement ) ) {
    mQuery = QueryCache::query( statement );
  } else {
    mQuery.prepare( statement );
    QueryCache::insert( statement, mQuery );
  }

  //too heavy debug info but worths to have from time to time
  //akDebug() << "Executing query" << statement;
  bool isBatch = false;
  for ( int i = 0; i < mBindValues.count(); ++i ) {
    mQuery.bindValue( QLatin1Char( ':' ) + QString::number( i ), mBindValues[i] );
    if ( !isBatch && mBindValues[i].canConvert<QVariantList>() ) {
      isBatch = true;
    }
    //akDebug() << QString::fromLatin1( ":%1" ).arg( i ) <<  mBindValues[i];
  }

  bool ret;

  if ( StorageDebugger::instance()->isSQLDebuggingEnabled() ) {
    QTime t;
    t.start();
    if ( isBatch ) {
      ret = mQuery.execBatch();
    } else {
      ret = mQuery.exec();
    }
    StorageDebugger::instance()->queryExecuted( mQuery, t.elapsed() );
  } else {
    StorageDebugger::instance()->incSequence();
    if ( isBatch ) {
      ret = mQuery.execBatch();
    } else {
      ret = mQuery.exec();
    }
  }

  // Add the query to DataStore so that we can replay it in case transaction deadlocks.
  // The method does nothing when this query is not executed within a transaction.
  // We don't care whether the query was successful or not. In case of error, the caller
  // will rollback the transaction anyway, and all cached queries will be removed.
  DataStore::self()->addQueryToTransaction( mQuery, isBatch );

  if ( !ret ) {
    // Handle transaction deadlocks and timeouts by attempting to replay the transaction.
    if ( mDatabaseType == DbType::PostgreSQL ) {
      const QString dbError = mQuery.lastError().databaseText();
      if ( dbError.contains( QLatin1String( "40P01" /* deadlock_detected */ ) ) ) {
        akDebug() << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
        akDebug() << mQuery.lastError().text();
        return retryLastTransaction();
      }
    } else if ( mDatabaseType == DbType::MySQL ) {
      const int error = mQuery.lastError().number();
      if ( error == 1213 /* ER_LOCK_DEADLOCK */ ) {
        akDebug() << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
        akDebug() << mQuery.lastError().text();
        return retryLastTransaction();
      } else if ( error == 1205 /* ER_LOCK_WAIT_TIMEOUT */ ) {
        akDebug() << "QueryBuilder::exec(): database reported transaction timeout, retrying transaction";
        akDebug() << mQuery.lastError().text();
        return retryLastTransaction();
      }
    } else if ( mDatabaseType == DbType::Sqlite && !DbType::isSystemSQLite( DataStore::self()->database() ) ) {
      const int error = mQuery.lastError().number();
      if ( error == 6 /* SQLITE_LOCKED */ ) {
        akDebug() << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
        akDebug() << mQuery.lastError().text();
        return retryLastTransaction( true );
      } else if ( error == 5 /* SQLITE_BUSY */ ) {
        akDebug() << "QueryBuilder::exec(): database reported transaction timeout, retrying transaction";
        akDebug() << mQuery.lastError().text();
        return retryLastTransaction( true );
      }
    } else if ( mDatabaseType == DbType::Sqlite ) {
      // We can't have a transaction deadlock in SQLite when using driver shipped
      // with Qt, because it does not support concurrent transactions and DataStore
      // serializes them through a global lock.
    }

    akError() << "DATABASE ERROR:";
    akError() << "  Error code:" << mQuery.lastError().number();
    akError() << "  DB error: " << mQuery.lastError().databaseText();
    akError() << "  Error text:" << mQuery.lastError().text();
    akError() << "  Query:" << statement;
    return false;
  }
#else
  mStatement = statement;
#endif
  return true;
}

void QueryBuilder::addColumns( const QStringList &cols )
{
  mColumns << cols;
}

void QueryBuilder::addColumn( const QString &col )
{
  mColumns << col;
}

void QueryBuilder::addAggregation( const QString &col, const QString &aggregate )
{
  QString s( aggregate );
  s += QLatin1Char( '(' );
  s += col;
  s += QLatin1Char( ')' );
  mColumns.append( s );
}

QString QueryBuilder::bindValue( const QVariant &value )
{
  mBindValues << value;
  return QLatin1Char( ':' ) + QString::number( mBindValues.count() - 1 );
}

QString QueryBuilder::buildWhereCondition( const Query::Condition &cond )
{
  if ( !cond.isEmpty() ) {
    QStringList conds;
    Q_FOREACH ( const Query::Condition &c, cond.subConditions() ) {
      conds << buildWhereCondition( c );
    }
    return QLatin1String( "( " ) + conds.join( logicOperatorToString( cond.mCombineOp ) ) + QLatin1String( " )" );
  } else {
    QString stmt = cond.mColumn;
    stmt += compareOperatorToString( cond.mCompareOp );
    if ( cond.mComparedColumn.isEmpty() ) {
      if ( cond.mComparedValue.isValid() ) {
        if ( cond.mComparedValue.canConvert( QVariant::List ) ) {
          stmt += QLatin1String( "( " );
          QStringList entries;
          Q_ASSERT_X( !cond.mComparedValue.toList().isEmpty(),
                      "QueryBuilder::buildWhereCondition()", "No values given for IN condition." );
          Q_FOREACH ( const QVariant &entry, cond.mComparedValue.toList() ) {
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

void QueryBuilder::setSubQueryMode( Query::LogicOperator op, ConditionType type )
{
  Q_ASSERT( type == WhereCondition || ( type == HavingCondition && mType == Select ) );
  mRootCondition[type].setSubQueryMode( op );
}

void QueryBuilder::addCondition( const Query::Condition &condition, ConditionType type )
{
  Q_ASSERT( type == WhereCondition || ( type == HavingCondition && mType == Select ) );
  mRootCondition[type].addCondition( condition );
}

void QueryBuilder::addSortColumn( const QString &column, Query::SortOrder order )
{
  mSortColumns << qMakePair( column, order );
}

void QueryBuilder::addGroupColumn( const QString &column )
{
  Q_ASSERT( mType == Select );
  mGroupColumns << column;
}

void QueryBuilder::addGroupColumns( const QStringList &columns )
{
  Q_ASSERT( mType == Select );
  mGroupColumns += columns;
}

void QueryBuilder::setColumnValue( const QString &column, const QVariant &value )
{
  mColumnValues << qMakePair( column, value );
}

void QueryBuilder::setDistinct( bool distinct )
{
  mDistinct = distinct;
}

void QueryBuilder::setLimit( int limit )
{
  mLimit = limit;
}

void QueryBuilder::setIdentificationColumn( const QString &column )
{
  mIdentificationColumn = column;
}

qint64 QueryBuilder::insertId()
{
  if ( mDatabaseType == DbType::PostgreSQL ) {
    Q_ASSERT( !mIdentificationColumn.isEmpty() );
    query().next();
    return query().record().value( mIdentificationColumn ).toLongLong();
  } else {
    const QVariant v = query().lastInsertId();
    if ( !v.isValid() ) {
      return -1;
    }
    bool ok;
    const qint64 insertId = v.toLongLong( &ok );
    if ( !ok ) {
      return -1;
    }
    return insertId;
  }
  return -1;
}
