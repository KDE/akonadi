/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "dbinitializer.h"
#include "dbinitializer_p.h"
#include "querybuilder.h"
#include "shared/akdebug.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlField>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtSql/QSqlError>

using namespace Akonadi;

DbInitializer::ColumnDescription::ColumnDescription()
  : allowNull( true ), isAutoIncrement( false ), isPrimaryKey( false ), isUnique( false )
{
}

DbInitializer::IndexDescription::IndexDescription()
  : isUnique( false )
{
}

DbInitializer::DataDescription::DataDescription()
{
}

DbInitializer::TableDescription::TableDescription()
{
}

DbInitializer::RelationDescription::RelationDescription()
{
}

DbInitializer::Ptr DbInitializer::createInstance(const QSqlDatabase& database, const QString& templateFile)
{
  if ( database.driverName().startsWith( QLatin1String( "QMYSQL" ) ) )
    return boost::shared_ptr<DbInitializer>( new DbInitializerMySql( database, templateFile ) );
  if ( database.driverName().startsWith( QLatin1String( "QSQLITE" ) ) )
    return boost::shared_ptr<DbInitializer>( new DbInitializerSqlite( database, templateFile ) );
  if ( database.driverName().startsWith( QLatin1String( "QPSQL" ) ) )
    return boost::shared_ptr<DbInitializer>( new DbInitializerPostgreSql( database, templateFile ) );
  akFatal() << database.driverName() << "backend  not supported";
  return boost::shared_ptr<DbInitializer>();
}

DbInitializer::DbInitializer( const QSqlDatabase &database, const QString &templateFile )
  : mDatabase( database ), mTemplateFile( templateFile ), mDebugInterface( 0 )
{
}

DbInitializer::~DbInitializer()
{
}

bool DbInitializer::run()
{
  qDebug() << "DbInitializer::run()";

  QFile file( mTemplateFile );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to open template file '%1'." ).arg( mTemplateFile );
    return false;
  }

  QDomDocument document;

  QString errorMsg;
  int line, column;
  if ( !document.setContent( &file, &errorMsg, &line, &column ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to parse template file '%1': %2 (%3:%4)." )
                       .arg( mTemplateFile ).arg( errorMsg ).arg( line ).arg( column );
    return false;
  }

  const QDomElement documentElement = document.documentElement();
  if ( documentElement.tagName() != QLatin1String( "database" ) ) {
    mErrorMsg = QString::fromLatin1( "Invalid format of template file '%1'." ).arg( mTemplateFile );
    return false;
  }

  QDomElement tableElement = documentElement.firstChildElement();
  while ( !tableElement.isNull() ) {
    if ( tableElement.tagName() == QLatin1String( "table" ) ) {
      if ( !checkTable( tableElement ) )
        return false;
    } else if ( tableElement.tagName() == QLatin1String( "relation" ) ) {
      if ( !checkRelation( tableElement ) )
        return false;
    } else {
      mErrorMsg = QString::fromLatin1( "Unknown tag, expected <table> and got <%1>." ).arg( tableElement.tagName() );
      return false;
    }

    tableElement = tableElement.nextSiblingElement();
  }

  qDebug() << "DbInitializer::run() done";
  return true;
}

DbInitializer::TableDescription DbInitializer::parseTableDescription( const QDomElement &tableElement ) const
{
  TableDescription tableDescription;

  tableDescription.name = tableElement.attribute( QLatin1String( "name" ) ) + QLatin1String( "Table" );

  QDomElement childElement = tableElement.firstChildElement();
  while ( !childElement.isNull() ) {
    if ( childElement.tagName() == QLatin1String( "column" ) ) {
      ColumnDescription columnDescription;

      columnDescription.name = childElement.attribute( QLatin1String( "name" ) );
      columnDescription.type = childElement.attribute( QLatin1String( "type" ) );

      if ( childElement.hasAttribute( QLatin1String( "allowNull" ) ) )
        columnDescription.allowNull = (childElement.attribute( QLatin1String( "allowNull" ) ) == QLatin1String( "true" ));

      if ( childElement.hasAttribute( QLatin1String( "isAutoIncrement" ) ) )
        columnDescription.isAutoIncrement = (childElement.attribute( QLatin1String( "isAutoIncrement" ) ) == QLatin1String( "true" ));

      if ( childElement.hasAttribute( QLatin1String( "isPrimaryKey" ) ) )
        columnDescription.isPrimaryKey = (childElement.attribute( QLatin1String( "isPrimaryKey" ) ) == QLatin1String( "true" ));

      if ( childElement.hasAttribute( QLatin1String( "isUnique" ) ) )
        columnDescription.isUnique = (childElement.attribute( QLatin1String( "isUnique" ) ) == QLatin1String( "true" ));

      columnDescription.refTable = childElement.attribute( QLatin1String( "refTable" ) );
      columnDescription.refColumn = childElement.attribute( QLatin1String( "refColumn" ) );
      columnDescription.defaultValue = childElement.attribute( QLatin1String( "default" ) );

      tableDescription.columns.append( columnDescription );
    } else if ( childElement.tagName() == QLatin1String( "index" ) ) {
      IndexDescription indexDescription;

      indexDescription.name = childElement.attribute( QLatin1String( "name" ) );
      indexDescription.columns = childElement.attribute( QLatin1String( "columns" ) ).split( QLatin1Char( ',' ), QString::SkipEmptyParts );
      indexDescription.isUnique = (childElement.attribute( QLatin1String( "unique" ) ) == QLatin1String( "true" ));

      tableDescription.indexes.append( indexDescription );
    } else if ( childElement.tagName() == QLatin1String( "data" ) ) {
      DataDescription dataDescription;

      const QStringList columns = childElement.attribute( QLatin1String( "columns" ) ).split( QLatin1Char( ',' ), QString::SkipEmptyParts );
      const QStringList values = childElement.attribute( QLatin1String( "values" ) ).split( QLatin1Char( ',' ), QString::SkipEmptyParts );
      Q_ASSERT( columns.count() == values.count() );

      for ( int i = 0; i < columns.count(); ++i )
        dataDescription.data.insert( columns.at( i ), values.at( i ) );

      tableDescription.data.append( dataDescription );
    }

    childElement = childElement.nextSiblingElement();
  }

  return tableDescription;
}

DbInitializer::RelationDescription DbInitializer::parseRelationDescription( const QDomElement &element ) const
{
  RelationDescription relationDescription;

  relationDescription.firstTable = element.attribute( QLatin1String( "table1" ) );
  relationDescription.firstTableName = element.attribute( QLatin1String( "table1" ) ) + QLatin1String( "Table" );
  relationDescription.firstColumn = element.attribute( QLatin1String( "column1" ) );

  relationDescription.secondTable = element.attribute( QLatin1String( "table2" ) );
  relationDescription.secondTableName = element.attribute( QLatin1String( "table2" ) ) + QLatin1String( "Table" );
  relationDescription.secondColumn = element.attribute( QLatin1String( "column2" ) );

  return relationDescription;
}

bool DbInitializer::checkTable( const QDomElement &element )
{
  const QString tableName = element.attribute( QLatin1String( "name" ) ) + QLatin1String( "Table" );

  qDebug() << "checking table " << tableName;

  // Parse the abstract table description from XML file
  const TableDescription tableDescription = parseTableDescription( element );

  // Get the CREATE TABLE statement for the specific SQL dialect
  const QString createTableStatement = buildCreateTableStatement( tableDescription );

  QSqlQuery query( mDatabase );

  if ( !hasTable( tableName ) ) {
    qDebug() << createTableStatement;

    // We have to create the entire table.
    if ( !query.exec( createTableStatement ) ) {
      mErrorMsg = QLatin1String( "Unable to create entire table.\n" );
      mErrorMsg += QString::fromLatin1( "Query error: '%1'" ).arg( query.lastError().text() );
      return false;
    }
  } else {
    // Check for every column whether it exists.
    const QSqlRecord table = mDatabase.record( tableName );

    foreach ( const ColumnDescription &columnDescription, tableDescription.columns ) {
      bool found = false;
      for ( int i = 0; i < table.count(); ++i ) {
        const QSqlField column = table.field( i );

        if ( columnDescription.name.toLower() == column.name().toLower() ) {
          found = true;
          break;
        }
      }

      if ( !found ) {
        // Add missing column to table.

        // Get the ADD COLUMN statement for the specific SQL dialect
        const QString statement = buildAddColumnStatement( tableDescription, columnDescription );
        qDebug() << statement;

        if ( !query.exec( statement ) ) {
          mErrorMsg = QString::fromLatin1( "Unable to add column '%1' to table '%2'.\n" ).arg( columnDescription.name, tableName );
          mErrorMsg += QString::fromLatin1( "Query error: '%1'" ).arg( query.lastError().text() );
          return false;
        }
      }
    }

    // TODO: remove obsolete columns (when sqlite will support it) and adapt column type modifications
  }

  // Add indices
  foreach ( const IndexDescription &indexDescription, tableDescription.indexes ) {
    // sqlite3 needs unique index identifiers per db
    const QString indexName = QString::fromLatin1( "%1_%2" ).arg( tableName ).arg( indexDescription.name );
    if ( !hasIndex( tableName, indexName ) ) {
      // Get the CREATE INDEX statement for the specific SQL dialect
      const QString statement = buildCreateIndexStatement( tableDescription, indexDescription );

      QSqlQuery query( mDatabase );
      qDebug() << "adding index" << statement;
      if ( !query.exec( statement ) ) {
        mErrorMsg = QLatin1String( "Unable to create index.\n" );
        mErrorMsg += QString::fromLatin1( "Query error: '%1'" ).arg( query.lastError().text() );
        return false;
      }
    }
  }

  // Add initial data if table is empty
  QueryBuilder queryBuilder( tableName, QueryBuilder::Select );
  queryBuilder.addColumn( QLatin1String( "*" ) );
  queryBuilder.setLimit( 1 );
  if ( !queryBuilder.exec() ) {
    mErrorMsg = QString::fromLatin1( "Unable to retrieve data from table '%1'.\n" ).arg( tableName );
    mErrorMsg += QString::fromLatin1( "Query error: '%1'" ).arg( queryBuilder.query().lastError().text() );
    return false;
  }

  query = queryBuilder.query();
  if ( query.size() == 0  || !query.first() ) { // table is empty
    foreach ( const DataDescription &dataDescription, tableDescription.data ) {
      // Get the INSERT VALUES statement for the specific SQL dialect
      const QString statement = buildInsertValuesStatement( tableDescription, dataDescription );
      qDebug() << statement;

      if ( !query.exec( statement ) ) {
        mErrorMsg = QString::fromLatin1( "Unable to add initial data to table '%1'.\n" ).arg( tableName );
        mErrorMsg += QString::fromLatin1( "Query error: '%1'\n" ).arg( query.lastError().text() );
        mErrorMsg += QString::fromLatin1( "Query was: %1" ).arg( statement );
        return false;
      }
    }
  }

  return true;
}

bool DbInitializer::checkRelation( const QDomElement &element )
{
  const RelationDescription relationDescription = parseRelationDescription( element );

  const QString relationTableName = relationDescription.firstTable +
                                    relationDescription.secondTable +
                                    QLatin1String( "Relation" );

  qDebug() << "checking relation " << relationTableName;

  if ( !hasTable( relationTableName ) ) {
    const QString statement = buildCreateRelationTableStatement( relationTableName, relationDescription );
    qDebug() << statement;

    QSqlQuery query( mDatabase );
    if ( !query.exec( statement ) ) {
      mErrorMsg = QLatin1String( "Unable to create entire table.\n" );
      mErrorMsg += QString::fromLatin1( "Query error: '%1'" ).arg( query.lastError().text() );
      return false;
    }
  }

  return true;
}

QString DbInitializer::errorMsg() const
{
  return mErrorMsg;
}

QString DbInitializer::sqlType(const QString & type) const
{
  if ( type == QLatin1String("int") )
    return QLatin1String("INTEGER");
  if ( type == QLatin1String("qint64") )
    return QLatin1String( "BIGINT" );
  if ( type == QLatin1String("QString") )
    return QLatin1String("TEXT");
  if (type == QLatin1String("QByteArray") )
    return QLatin1String("LONGBLOB");
  if ( type == QLatin1String("QDateTime") )
    return QLatin1String("TIMESTAMP");
  if ( type == QLatin1String( "bool" ) )
    return QLatin1String("BOOL");
  Q_ASSERT( false );
  return QString();
}

QString DbInitializer::sqlValue( const QString &type, const QString &value ) const
{
  if ( type == QLatin1String( "QDateTime" ) && value == QLatin1String( "QDateTime::currentDateTime()" ) )
    return QLatin1String( "CURRENT_TIMESTAMP" );

  return value;
}

bool DbInitializer::hasTable(const QString & tableName)
{
   return mDatabase.tables().contains( tableName, Qt::CaseInsensitive );
}

bool DbInitializer::hasIndex(const QString& tableName, const QString& indexName)
{
  QSqlQuery query( mDatabase );
  if ( !query.exec( hasIndexQuery( tableName, indexName ) ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to list index information for table %1.\n" ).arg( tableName );
    mErrorMsg += QString::fromLatin1( "Query error: '%1'" ).arg( query.lastError().text() );
    return false;
  }
  return query.next();
}

QString DbInitializer::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  Q_UNUSED( tableName );
  Q_UNUSED( indexName );
  qFatal( "Implement index support for your database!" );
  return QString();
}

QString DbInitializer::buildAddColumnStatement( const TableDescription &tableDescription, const ColumnDescription &columnDescription ) const
{
  return QString::fromLatin1( "ALTER TABLE %1 ADD COLUMN %2" ).arg( tableDescription.name, buildColumnStatement( columnDescription ) );
}

QString DbInitializer::buildCreateIndexStatement( const TableDescription &tableDescription, const IndexDescription &indexDescription ) const
{
  const QString indexName = QString::fromLatin1( "%1_%2" ).arg( tableDescription.name ).arg( indexDescription.name );
  return QString::fromLatin1( "CREATE %1 INDEX %2 ON %3 (%4)" )
                            .arg( indexDescription.isUnique ? QLatin1String( "UNIQUE" ) : QString() )
                            .arg( indexName )
                            .arg( tableDescription.name )
                            .arg( indexDescription.columns.join( QLatin1String( "," ) ) );
}

QString DbInitializer::buildCreateRelationTableStatement( const QString &tableName, const RelationDescription &relationDescription ) const
{
  QString statement = QString::fromLatin1( "CREATE TABLE %1 (" ).arg( tableName );

  statement += QString::fromLatin1( "%1_%2 INTEGER REFERENCES %3(%4), " )
      .arg( relationDescription.firstTable )
      .arg( relationDescription.firstColumn )
      .arg( relationDescription.firstTableName )
      .arg( relationDescription.firstColumn );

  statement += QString::fromLatin1( "%1_%2 INTEGER REFERENCES %3(%4), " )
      .arg( relationDescription.secondTable )
      .arg( relationDescription.secondColumn )
      .arg( relationDescription.secondTableName )
      .arg( relationDescription.secondColumn );

  statement += QString::fromLatin1( "PRIMARY KEY (%1_%2, %3_%4))" )
      .arg( relationDescription.firstTable )
      .arg( relationDescription.firstColumn )
      .arg( relationDescription.secondTable )
      .arg( relationDescription.secondColumn );

  return statement;
}

void DbInitializer::setDebugInterface( DebugInterface *interface )
{
  mDebugInterface = interface;
}

void DbInitializer::unitTestRun()
{
  QFile file( mTemplateFile );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to open template file '%1'." ).arg( mTemplateFile );
    return;
  }

  QDomDocument document;

  QString errorMsg;
  int line, column;
  if ( !document.setContent( &file, &errorMsg, &line, &column ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to parse template file '%1': %2 (%3:%4)." )
                       .arg( mTemplateFile ).arg( errorMsg ).arg( line ).arg( column );
    return;
  }

  const QDomElement documentElement = document.documentElement();
  if ( documentElement.tagName() != QLatin1String( "database" ) ) {
    mErrorMsg = QString::fromLatin1( "Invalid format of template file '%1'." ).arg( mTemplateFile );
    return;
  }

  QDomElement tableElement = documentElement.firstChildElement();
  while ( !tableElement.isNull() ) {
    if ( tableElement.tagName() == QLatin1String( "table" ) ) {
      const QString tableName = tableElement.attribute( QLatin1String( "name" ) ) + QLatin1String( "Table" );
      const TableDescription tableDescription = parseTableDescription( tableElement );
      const QString createTableStatement = buildCreateTableStatement( tableDescription );
      if ( mDebugInterface )
        mDebugInterface->createTableStatement( tableName, createTableStatement );
    } else if ( tableElement.tagName() == QLatin1String( "relation" ) ) {
    } else {
      mErrorMsg = QString::fromLatin1( "Unknown tag, expected <table> and got <%1>." ).arg( tableElement.tagName() );
      return;
    }

    tableElement = tableElement.nextSiblingElement();
  }
}
