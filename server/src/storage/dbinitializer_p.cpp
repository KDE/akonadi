/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2010 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "storage/dbinitializer_p.h"

//BEGIN MySQL

DbInitializerMySql::DbInitializerMySql(const QSqlDatabase& database, const QString& templateFile):
  DbInitializer(database, templateFile)
{
}

QString DbInitializerMySql::sqlType(const QString & type) const
{
  if ( type == QLatin1String( "QString" ) )
    return QLatin1String( "VARCHAR(255) BINARY" );
  else
    return DbInitializer::sqlType( type );
}

QString DbInitializerMySql::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  return QString::fromLatin1( "SHOW INDEXES FROM %1 WHERE `Key_name` = '%2'" )
      .arg( tableName ).arg( indexName );
}

QString DbInitializerMySql::buildCreateTableStatement( const TableDescription &tableDescription ) const
{
  QStringList columns;
  QStringList references;

  foreach ( const ColumnDescription &columnDescription, tableDescription.columns ) {
    columns.append( buildColumnStatement( columnDescription ) );

    if ( !columnDescription.refTable.isEmpty() && !columnDescription.refColumn.isEmpty() ) {
      references << QString::fromLatin1( "FOREIGN KEY (%1) REFERENCES %2Table(%3) ON DELETE CASCADE ON UPDATE CASCADE" )
                                       .arg( columnDescription.name )
                                       .arg( columnDescription.refTable )
                                       .arg( columnDescription.refColumn );
    }
  }

  columns << references;

  const QString tableProperties = QLatin1String( " COLLATE=utf8_general_ci DEFAULT CHARSET=utf8" );

  return QString::fromLatin1( "CREATE TABLE %1 (%2) %3" ).arg( tableDescription.name, columns.join( QLatin1String( ", " ) ), tableProperties );
}

QString DbInitializerMySql::buildColumnStatement( const ColumnDescription &columnDescription ) const
{
  QString column = columnDescription.name;

  column += QLatin1Char( ' ' ) + sqlType( columnDescription.type );

  if ( !columnDescription.allowNull )
    column += QLatin1String( " NOT NULL" );

  if ( columnDescription.isAutoIncrement )
    column += QLatin1String( " AUTO_INCREMENT" );

  if ( columnDescription.isPrimaryKey )
    column += QLatin1String( " PRIMARY KEY" );

  if ( columnDescription.isUnique )
    column += QLatin1String( " UNIQUE" );

  if ( !columnDescription.defaultValue.isEmpty() ) {
    const QString defaultValue = sqlValue( columnDescription.type, columnDescription.defaultValue );

    if ( !defaultValue.isEmpty() )
      column += QString::fromLatin1( " DEFAULT %1" ).arg( defaultValue );
  }

  return column;
}

QString DbInitializerMySql::buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const
{
  QHash<QString, QString> data = dataDescription.data;
  QMutableHashIterator<QString, QString> it( data );
  while ( it.hasNext() ) {
    it.next();
    it.value().replace( QLatin1String("\\"), QLatin1String("\\\\") );
  }

  return QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
                            .arg( tableDescription.name )
                            .arg( QStringList( data.keys() ).join( QLatin1String( "," ) ) )
                            .arg( QStringList( data.values() ).join( QLatin1String( "," ) ) );
}

//END MySQL



//BEGIN Sqlite

DbInitializerSqlite::DbInitializerSqlite(const QSqlDatabase& database, const QString& templateFile):
  DbInitializer(database, templateFile)
{
}

QString DbInitializerSqlite::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  return QString::fromLatin1( "SELECT * FROM sqlite_master WHERE type='index' AND tbl_name='%1' AND name='%2';" ).arg( tableName ).arg( indexName );
}

QString DbInitializerSqlite::buildCreateTableStatement( const TableDescription &tableDescription ) const
{
  QStringList columns;

  foreach ( const ColumnDescription &columnDescription, tableDescription.columns )
    columns.append( buildColumnStatement( columnDescription ) );

  return QString::fromLatin1( "CREATE TABLE %1 (%2)" ).arg( tableDescription.name, columns.join( QLatin1String( ", " ) ) );
}

QString DbInitializerSqlite::buildColumnStatement( const ColumnDescription &columnDescription ) const
{
  QString column = columnDescription.name;

  column += QLatin1Char( ' ' ) + sqlType( columnDescription.type );

  if ( !columnDescription.allowNull )
    column += QLatin1String( " NOT NULL" );

  if ( columnDescription.isAutoIncrement )
    column += QLatin1String( " AUTOINCREMENT" );

  if ( columnDescription.isPrimaryKey )
    column += QLatin1String( " INTEGER PRIMARY KEY" );

  if ( columnDescription.isUnique )
    column += QLatin1String( " UNIQUE" );

  if ( !columnDescription.defaultValue.isEmpty() ) {
    const QString defaultValue = sqlValue( columnDescription.type, columnDescription.defaultValue );

    if ( !defaultValue.isEmpty() )
      column += QString::fromLatin1( " DEFAULT %1" ).arg( defaultValue );
  }

  return column;
}

QString DbInitializerSqlite::buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const
{
  QHash<QString, QString> data = dataDescription.data;
  QMutableHashIterator<QString, QString> it( data );
  while ( it.hasNext() ) {
    it.next();
    it.value().replace( QLatin1String( "true" ), QLatin1String( "1" ) );
    it.value().replace( QLatin1String( "false" ), QLatin1String( "0" ) );
  }

  return QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
                            .arg( tableDescription.name )
                            .arg( QStringList( data.keys() ).join( QLatin1String( "," ) ) )
                            .arg( QStringList( data.values() ).join( QLatin1String( "," ) ) );
}

//END Sqlite



//BEGIN PostgreSQL

DbInitializerPostgreSql::DbInitializerPostgreSql(const QSqlDatabase& database, const QString& templateFile):
  DbInitializer(database, templateFile)
{
}

QString DbInitializerPostgreSql::sqlType(const QString& type) const
{
  if ( type == QLatin1String("qint64") )
    return QLatin1String( "int8" );
  if ( type == QLatin1String("QByteArray") )
    return QLatin1String("BYTEA");
  return DbInitializer::sqlType( type );
}

QString DbInitializerPostgreSql::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  QString query = QLatin1String( "SELECT indexname FROM pg_catalog.pg_indexes" );
  query += QString::fromLatin1( " WHERE tablename ilike '%1'" ).arg( tableName );
  query += QString::fromLatin1( " AND  indexname ilike '%1';" ).arg( indexName );
  return query;
}

QString DbInitializerPostgreSql::buildCreateTableStatement( const TableDescription &tableDescription ) const
{
  QStringList columns;

  foreach ( const ColumnDescription &columnDescription, tableDescription.columns )
    columns.append( buildColumnStatement( columnDescription ) );

  return QString::fromLatin1( "CREATE TABLE %1 (%2)" ).arg( tableDescription.name, columns.join( QLatin1String( ", " ) ) );
}

QString DbInitializerPostgreSql::buildColumnStatement( const ColumnDescription &columnDescription ) const
{
  QString column = columnDescription.name + QLatin1Char( ' ' );

  if ( columnDescription.isAutoIncrement )
    column += QLatin1String( "SERIAL" );
  else
    column += sqlType( columnDescription.type );

  if ( columnDescription.isPrimaryKey )
    column += QLatin1String( " PRIMARY KEY" );
  else if ( columnDescription.isUnique )
    column += QLatin1String( " UNIQUE" );

  if ( !columnDescription.allowNull && !columnDescription.isPrimaryKey )
    column += QLatin1String( " NOT NULL" );

  if ( !columnDescription.refTable.isEmpty() && !columnDescription.refColumn.isEmpty() ) {
    column += QString::fromLatin1( " REFERENCES %1Table(%2) ON DELETE CASCADE ON UPDATE CASCADE" )
                                 .arg( columnDescription.refTable )
                                 .arg( columnDescription.refColumn );
  }

  if ( !columnDescription.defaultValue.isEmpty() ) {
    const QString defaultValue = sqlValue( columnDescription.type, columnDescription.defaultValue );

    if ( !defaultValue.isEmpty() )
      column += QString::fromLatin1( " DEFAULT %1" ).arg( defaultValue );
  }

  return column;
}

QString DbInitializerPostgreSql::buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const
{
  return QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
                            .arg( tableDescription.name )
                            .arg( QStringList( dataDescription.data.keys() ).join( QLatin1String( "," ) ) )
                            .arg( QStringList( dataDescription.data.values() ).join( QLatin1String( "," ) ) );
}

QString DbInitializerPostgreSql::buildCreateRelationTableStatement( const QString &tableName, const RelationDescription &relationDescription ) const
{
  const QString columnOptions = QLatin1String( " ON DELETE CASCADE ON UPDATE CASCADE" );

  QString statement = QString::fromLatin1( "CREATE TABLE %1 (" ).arg( tableName );

  statement += QString::fromLatin1( "%1_%2 INTEGER REFERENCES %3(%4) %5, " )
      .arg( relationDescription.firstTable )
      .arg( relationDescription.firstColumn )
      .arg( relationDescription.firstTableName )
      .arg( relationDescription.firstColumn )
      .arg( columnOptions );

  statement += QString::fromLatin1( "%1_%2 INTEGER REFERENCES %3(%4) %5, " )
      .arg( relationDescription.secondTable )
      .arg( relationDescription.secondColumn )
      .arg( relationDescription.secondTableName )
      .arg( relationDescription.secondColumn )
      .arg( columnOptions );

  statement += QString::fromLatin1( "PRIMARY KEY (%1_%2, %3_%4))" )
      .arg( relationDescription.firstTable )
      .arg( relationDescription.firstColumn )
      .arg( relationDescription.secondTable )
      .arg( relationDescription.secondColumn );

  return statement;
}

//END PostgreSQL



//BEGIN Virtuoso

DbInitializerVirtuoso::DbInitializerVirtuoso(const QSqlDatabase& database, const QString& templateFile):
  DbInitializer(database, templateFile)
{
}

QString DbInitializerVirtuoso::sqlType(const QString& type) const
{
  if ( type == QLatin1String("QString") )
    return QLatin1String( "VARCHAR(255)" );
  if (type == QLatin1String("QByteArray") )
    return QLatin1String("LONG VARCHAR");
  if ( type == QLatin1String( "bool" ) )
    return QLatin1String("CHAR");
  return DbInitializer::sqlType( type );
}

QString DbInitializerVirtuoso::sqlValue(const QString& type, const QString& value) const
{
  if ( type == QLatin1String( "bool" ) ) {
    if ( value == QLatin1String( "false" ) ) return QLatin1String( "0" );
    if ( value == QLatin1String( "true" ) ) return QLatin1String( "1" );
  }
  return DbInitializer::sqlValue( type, value );
}

bool DbInitializerVirtuoso::hasIndex(const QString& tableName, const QString& indexName)
{
  // TODO: Implement index checking for Virtuoso!
  return true;
}

QString DbInitializerVirtuoso::buildCreateTableStatement( const TableDescription &tableDescription ) const
{
  QStringList columns;

  foreach ( const ColumnDescription &columnDescription, tableDescription.columns )
    columns.append( buildColumnStatement( columnDescription ) );

  return QString::fromLatin1( "CREATE TABLE %1 (%2)" ).arg( tableDescription.name, columns.join( QLatin1String( ", " ) ) );
}

QString DbInitializerVirtuoso::buildColumnStatement( const ColumnDescription &columnDescription ) const
{
  QString column = columnDescription.name;

  column += QLatin1Char( ' ' ) + sqlType( columnDescription.type );

  if ( !columnDescription.allowNull )
    column += QLatin1String( " NOT NULL" );

  if ( columnDescription.isAutoIncrement )
    column += QLatin1String( " IDENTITY" );

  if ( columnDescription.isPrimaryKey )
    column += QLatin1String( " PRIMARY KEY" );

  if ( columnDescription.isUnique )
    column += QLatin1String( " UNIQUE" );

  if ( !columnDescription.refTable.isEmpty() && !columnDescription.refColumn.isEmpty() ) {
    column += QString::fromLatin1( " REFERENCES %1Table(%2) ON DELETE CASCADE ON UPDATE CASCADE" )
                                 .arg( columnDescription.refTable )
                                 .arg( columnDescription.refColumn );
  }

  if ( !columnDescription.defaultValue.isEmpty() ) {
    const QString defaultValue = sqlValue( columnDescription.type, columnDescription.defaultValue );

    if ( !defaultValue.isEmpty() && (defaultValue != QLatin1String( "CURRENT_TIMESTAMP" )) )
      column += QString::fromLatin1( " DEFAULT %1" ).arg( defaultValue );
  }

  return column;
}

QString DbInitializerVirtuoso::buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const
{
  QHash<QString, QString> data = dataDescription.data;
  QMutableHashIterator<QString, QString> it( data );
  while ( it.hasNext() ) {
    it.next();
    it.value().replace( QLatin1String( "true" ), QLatin1String( "1" ) );
    it.value().replace( QLatin1String( "false" ), QLatin1String( "0" ) );
  }

  return QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
                            .arg( tableDescription.name )
                            .arg( QStringList( data.keys() ).join( QLatin1String( "," ) ) )
                            .arg( QStringList( data.values() ).join( QLatin1String( "," ) ) );
}

//END Virtuoso
