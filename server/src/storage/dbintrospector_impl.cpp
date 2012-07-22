/*
    Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#include "dbintrospector_impl.h"
#include "dbexception.h"
#include "querybuilder.h"

#include <QDebug>
#include <QSqlQuery>

using namespace Akonadi;

//BEGIN MySql

DbIntrospectorMySql::DbIntrospectorMySql(const QSqlDatabase& database): DbIntrospector(database)
{
}

QString DbIntrospectorMySql::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  return QString::fromLatin1( "SHOW INDEXES FROM %1 WHERE `Key_name` = '%2'" )
      .arg( tableName ).arg( indexName );
}

QVector< DbIntrospector::ForeignKey > DbIntrospectorMySql::foreignKeyConstraints(const QString& tableName)
{
  QueryBuilder qb( QLatin1String( "information_schema.REFERENTIAL_CONSTRAINTS" ), QueryBuilder::Select );
  qb.addJoin( QueryBuilder::InnerJoin, QLatin1String( "information_schema.KEY_COLUMN_USAGE" ),
              QLatin1String( "information_schema.REFERENTIAL_CONSTRAINTS.CONSTRAINT_NAME" ),
              QLatin1String( "information_schema.KEY_COLUMN_USAGE.CONSTRAINT_NAME" ) );
  qb.addColumn( QLatin1String( "information_schema.REFERENTIAL_CONSTRAINTS.CONSTRAINT_NAME" ) );
  qb.addColumn( QLatin1String( "information_schema.KEY_COLUMN_USAGE.COLUMN_NAME" ) );
  qb.addColumn( QLatin1String( "information_schema.KEY_COLUMN_USAGE.REFERENCED_TABLE_NAME" ) );
  qb.addColumn( QLatin1String( "information_schema.KEY_COLUMN_USAGE.REFERENCED_COLUMN_NAME" ) );
  qb.addColumn( QLatin1String( "information_schema.REFERENTIAL_CONSTRAINTS.UPDATE_RULE" ) );
  qb.addColumn( QLatin1String( "information_schema.REFERENTIAL_CONSTRAINTS.DELETE_RULE" ) );

  qb.addValueCondition( QLatin1String( "information_schema.KEY_COLUMN_USAGE.TABLE_SCHEMA" ), Query::Equals, m_database.databaseName() );
  qb.addValueCondition( QLatin1String( "information_schema.KEY_COLUMN_USAGE.TABLE_NAME" ), Query::Equals, tableName );

  if ( !qb.exec() )
    throw DbException( qb.query() );

  QVector<ForeignKey> result;
  while ( qb.query().next() ) {
    ForeignKey fk;
    fk.name = qb.query().value( 0 ).toString();
    fk.column = qb.query().value( 1 ).toString();
    fk.refTable = qb.query().value( 2 ).toString();
    fk.refColumn = qb.query().value( 3 ).toString();
    fk.onUpdate = qb.query().value( 4 ).toString();
    fk.onDelete = qb.query().value( 5 ).toString();
    result.push_back( fk );
  }

  return result;
}

//END MySql


//BEGIN Sqlite

DbIntrospectorSqlite::DbIntrospectorSqlite(const QSqlDatabase& database): DbIntrospector(database)
{
}

QString DbIntrospectorSqlite::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  return QString::fromLatin1( "SELECT * FROM sqlite_master WHERE type='index' AND tbl_name='%1' AND name='%2';" )
      .arg( tableName ).arg( indexName );
}

//END Sqlite


//BEGIN PostgreSql

DbIntrospectorPostgreSql::DbIntrospectorPostgreSql(const QSqlDatabase& database): DbIntrospector(database)
{
}

QString DbIntrospectorPostgreSql::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  QString query = QLatin1String( "SELECT indexname FROM pg_catalog.pg_indexes" );
  query += QString::fromLatin1( " WHERE tablename ilike '%1'" ).arg( tableName );
  query += QString::fromLatin1( " AND  indexname ilike '%1';" ).arg( indexName );
  return query;
}

//END PostgreSql


//BEGIN Virtuoso

DbIntrospectorVirtuoso::DbIntrospectorVirtuoso(const QSqlDatabase& database): DbIntrospector(database)
{
}

bool DbIntrospectorVirtuoso::hasIndex(const QString& tableName, const QString& indexName)
{
  // TODO: Implement index checking for Virtuoso!
  return true;
}

//END Virtuoso
