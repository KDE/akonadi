/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2012 by Volker Krause <vkrause@kde.org>                 *
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
#include "dbexception.h"
#include "shared/akdebug.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlField>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtSql/QSqlError>

#include <boost/bind.hpp>
#include <algorithm>

using namespace Akonadi;

DbInitializer::ColumnDescription::ColumnDescription()
  : size( -1 ), allowNull( true ), isAutoIncrement( false ), isPrimaryKey( false ), isUnique( false ), onUpdate( Cascade ), onDelete( Cascade )
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

int DbInitializer::TableDescription::primaryKeyColumnCount() const
{
  return std::count_if( columns.constBegin(), columns.constEnd(), boost::bind<bool>( &ColumnDescription::isPrimaryKey, _1 ) );
}

DbInitializer::RelationDescription::RelationDescription()
{
}

DbInitializer::Ptr DbInitializer::createInstance(const QSqlDatabase& database, const QString& templateFile)
{
  switch ( DbType::type( database ) ) {
    case DbType::MySQL:
      return boost::shared_ptr<DbInitializer>( new DbInitializerMySql( database, templateFile ) );
    case DbType::Sqlite:
      return boost::shared_ptr<DbInitializer>( new DbInitializerSqlite( database, templateFile ) );
    case DbType::PostgreSQL:
      return boost::shared_ptr<DbInitializer>( new DbInitializerPostgreSql( database, templateFile ) );
    case DbType::Virtuoso:
      return boost::shared_ptr<DbInitializer>( new DbInitializerVirtuoso( database, templateFile ) );
    case DbType::Unknown:
      break;
  }
  akFatal() << database.driverName() << "backend  not supported";
  return boost::shared_ptr<DbInitializer>();
}

DbInitializer::DbInitializer( const QSqlDatabase &database, const QString &templateFile )
  : mDatabase( database ), mTemplateFile( templateFile ), mTestInterface( 0 )
{
  m_introspector = DbIntrospector::createInstance( mDatabase );
}

DbInitializer::~DbInitializer()
{
}

bool DbInitializer::run()
{
  try {
    akDebug() << "DbInitializer::run()";

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
        if ( !checkTable( parseTableDescription( tableElement ) ) )
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

    akDebug() << "DbInitializer::run() done";
    return true;
  } catch ( const DbException &e ) {
    mErrorMsg = QString::fromUtf8( e.what() );
  }
  return false;
}

DbInitializer::ColumnDescription::ReferentialAction DbInitializer::parseReferentialAction(const QString& refAction)
{
  if ( refAction.isEmpty() || refAction.toLower() == QLatin1String( "cascade" ) )
    return DbInitializer::ColumnDescription::Cascade;
  if ( refAction.toLower() == QLatin1String( "restrict" ) )
    return DbInitializer::ColumnDescription::Restrict;
  if ( refAction.toLower() == QLatin1String( "setnull" ) )
    return DbInitializer::ColumnDescription::SetNull;

  Q_ASSERT( !"format error" );
  return DbInitializer::ColumnDescription::Cascade;
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

      if ( childElement.hasAttribute( QLatin1String("size") ) )
        columnDescription.size = childElement.attribute( QLatin1String("size") ).toInt();

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

      columnDescription.onUpdate = parseReferentialAction( childElement.attribute( QLatin1String( "onUpdate" ) ) );
      columnDescription.onDelete = parseReferentialAction( childElement.attribute( QLatin1String( "onDelete") ) );

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
  relationDescription.firstColumn = element.attribute( QLatin1String( "column1" ) );

  relationDescription.secondTable = element.attribute( QLatin1String( "table2" ) );
  relationDescription.secondColumn = element.attribute( QLatin1String( "column2" ) );

  return relationDescription;
}

bool DbInitializer::checkTable( const TableDescription &tableDescription )
{
  akDebug() << "checking table " << tableDescription.name;

  if ( !m_introspector->hasTable( tableDescription.name ) ) {
    // Get the CREATE TABLE statement for the specific SQL dialect
    const QString createTableStatement = buildCreateTableStatement( tableDescription );
    akDebug() << createTableStatement;
    execQuery( createTableStatement );
  } else {
    // Check for every column whether it exists, and add the missing ones
    Q_FOREACH ( const ColumnDescription &columnDescription, tableDescription.columns ) {
      if ( !m_introspector->hasColumn( tableDescription.name, columnDescription.name ) ) {
        // Get the ADD COLUMN statement for the specific SQL dialect
        const QString statement = buildAddColumnStatement( tableDescription, columnDescription );
        akDebug() << statement;
        execQuery( statement );
      }
    }

    // NOTE: we do intentionally not delete any columns here, we defer that to the updater,
    // very likely previous columns contain data that needs to be moved to a new column first.

    // Make sure the foreign key constraints are all there
    checkForeignKeys( tableDescription );
  }

  // Add indices
  Q_FOREACH ( const IndexDescription &indexDescription, tableDescription.indexes ) {
    // sqlite3 needs unique index identifiers per db
    const QString indexName = QString::fromLatin1( "%1_%2" ).arg( tableDescription.name ).arg( indexDescription.name );
    if ( !m_introspector->hasIndex( tableDescription.name, indexName ) ) {
      // Get the CREATE INDEX statement for the specific SQL dialect
      const QString statement = buildCreateIndexStatement( tableDescription, indexDescription );
      akDebug() << "adding index" << statement;
      execQuery( statement );
    }
  }

  // Add initial data if table is empty
  if ( tableDescription.data.isEmpty() )
    return true;
  if ( m_introspector->isTableEmpty( tableDescription.name ) ) {
    Q_FOREACH ( const DataDescription &dataDescription, tableDescription.data ) {
      // Get the INSERT VALUES statement for the specific SQL dialect
      const QString statement = buildInsertValuesStatement( tableDescription, dataDescription );
      akDebug() << statement;
      execQuery( statement );
    }
  }

  return true;
}

void DbInitializer::checkForeignKeys(const DbInitializer::TableDescription& tableDescription)
{
  try {
    const QVector<DbIntrospector::ForeignKey> existingForeignKeys = m_introspector->foreignKeyConstraints( tableDescription.name );
    Q_FOREACH ( const ColumnDescription &column, tableDescription.columns ) {
      DbIntrospector::ForeignKey existingForeignKey;
      Q_FOREACH ( const DbIntrospector::ForeignKey &fk, existingForeignKeys ) {
        if ( QString::compare( fk.column, column.name, Qt::CaseInsensitive ) == 0 ) {
          existingForeignKey = fk;
          break;
        }
      }

      if ( !column.refTable.isEmpty() && !column.refColumn.isEmpty() ) {
        if ( !existingForeignKey.column.isEmpty() ) {
          // there's a constraint on this column, check if it's the correct one
          if ( QString::compare( existingForeignKey.refTable, column.refTable + QLatin1Literal( "table" ), Qt::CaseInsensitive ) == 0
            && QString::compare( existingForeignKey.refColumn, column.refColumn, Qt::CaseInsensitive ) == 0
            && QString::compare( existingForeignKey.onUpdate, referentialActionToString( column.onUpdate ), Qt::CaseInsensitive ) == 0
            && QString::compare( existingForeignKey.onDelete, referentialActionToString( column.onDelete ), Qt::CaseInsensitive ) == 0 )
          {
            continue; // all good
          }

          const QString statement = buildRemoveForeignKeyConstraintStatement( existingForeignKey, tableDescription );
          if ( !statement.isEmpty() ) {
            akDebug() << "Found existing foreign constraint that doesn't match the schema:" << existingForeignKey.name
                      << existingForeignKey.column << existingForeignKey.refTable << existingForeignKey.refColumn;
            akDebug() << statement;
            execQuery( statement );
          }
        }

        const QString statement = buildAddForeignKeyConstraintStatement( tableDescription, column );
        if ( statement.isEmpty() ) // no supported
          continue;
        akDebug() << "Adding missing foreign key constraint:" << statement;
        execQuery( statement );

      } else if ( !existingForeignKey.column.isEmpty() ) {
        // constraint exists but we don't want one here
        const QString statement = buildRemoveForeignKeyConstraintStatement( existingForeignKey, tableDescription );
        if ( !statement.isEmpty() ) {
          akDebug() << "Found unexpected foreign key constraint:" << existingForeignKey.name << existingForeignKey.column
                    << existingForeignKey.refTable << existingForeignKey.refColumn;
          akDebug() << statement;
          execQuery( statement );
        }
      }
    }
  } catch ( const DbException &e ) {
    akDebug() << "Fixing foreign key constraints failed:" << e.what();
    // we ignore this since foreign keys are only used for optimizations (not all backends support them anyway)
    // TODO: we need to record this though, for said (yet to be implemented) optimizations
  }
}


bool DbInitializer::checkRelation( const QDomElement &element )
{
  const RelationDescription relationDescription = parseRelationDescription( element );

  const QString relationTableName = relationDescription.firstTable +
                                    relationDescription.secondTable +
                                    QLatin1String( "Relation" );

  // translate into a regular table and let checkTable() handle it
  TableDescription table;
  table.name = relationTableName;

  ColumnDescription column;
  column.type = QLatin1String( "qint64" );
  column.allowNull = false;
  column.isPrimaryKey = true;
  column.onUpdate = ColumnDescription::Cascade;
  column.onDelete = ColumnDescription::Cascade;
  column.name = relationDescription.firstTable + QLatin1Char( '_' ) + relationDescription.firstColumn;
  column.refTable = relationDescription.firstTable;
  column.refColumn = relationDescription.firstColumn;
  table.columns.push_back( column );

  column.name = relationDescription.secondTable + QLatin1Char( '_' ) + relationDescription.secondColumn;
  column.refTable = relationDescription.secondTable;
  column.refColumn = relationDescription.secondColumn;
  table.columns.push_back( column );

  return checkTable( table );
}

QString DbInitializer::errorMsg() const
{
  return mErrorMsg;
}

QString DbInitializer::sqlType(const QString & type, int size) const
{
  Q_UNUSED(size);
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

QString DbInitializer::buildAddColumnStatement( const TableDescription &tableDescription, const ColumnDescription &columnDescription ) const
{
  return QString::fromLatin1( "ALTER TABLE %1 ADD COLUMN %2" ).arg( tableDescription.name, buildColumnStatement( columnDescription, tableDescription ) );
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

QString DbInitializer::buildAddForeignKeyConstraintStatement(const DbInitializer::TableDescription& table, const DbInitializer::ColumnDescription& column) const
{
  Q_UNUSED( table );
  Q_UNUSED( column );
  return QString();
}

QString DbInitializer::buildRemoveForeignKeyConstraintStatement(const DbIntrospector::ForeignKey& fk, const DbInitializer::TableDescription& table) const
{
  Q_UNUSED( fk );
  Q_UNUSED( table );
  return QString();
}

QString DbInitializer::buildReferentialAction(DbInitializer::ColumnDescription::ReferentialAction onUpdate, DbInitializer::ColumnDescription::ReferentialAction onDelete)
{
  return QLatin1Literal( "ON UPDATE " ) + referentialActionToString( onUpdate )
       + QLatin1Literal( " ON DELETE ") + referentialActionToString( onDelete );
}

QString DbInitializer::referentialActionToString(DbInitializer::ColumnDescription::ReferentialAction action)
{
  switch ( action ) {
    case ColumnDescription::Cascade: return QLatin1String( "CASCADE" );
    case ColumnDescription::Restrict: return QLatin1String( "RESTRICT" );
    case ColumnDescription::SetNull: return QLatin1String( "SET NULL" );
  }

  Q_ASSERT( !"invalid referential action enum!" );
  return QString();
}

QString DbInitializer::buildPrimaryKeyStatement(const DbInitializer::TableDescription& table)
{
  QStringList cols;
  Q_FOREACH( const ColumnDescription &column, table.columns ) {
    if ( column.isPrimaryKey )
      cols.push_back( column.name );
  }
  return QLatin1Literal( "PRIMARY KEY (" ) + cols.join( QLatin1String( ", " ) ) + QLatin1Char( ')' );
}

void DbInitializer::execQuery(const QString& queryString)
{
  if ( Q_UNLIKELY( mTestInterface ) ) {
    mTestInterface->execStatement( queryString );
    return;
  }

  QSqlQuery query( mDatabase );
  if ( !query.exec( queryString ) )
    throw DbException( query );
}


void DbInitializer::setTestInterface( TestInterface *interface )
{
  mTestInterface = interface;
}

void DbInitializer::setIntrospector(const DbIntrospector::Ptr& introspector)
{
  m_introspector = introspector;
}
