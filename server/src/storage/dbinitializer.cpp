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

#include "dbinitializer.h"

DbInitializer::DbInitializer( const QSqlDatabase &database, const QString &templateFile )
  : mDatabase( database ), mTemplateFile( templateFile )
{
}

DbInitializer::~DbInitializer()
{
}

bool DbInitializer::run()
{
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
    } else {
      mErrorMsg = QString::fromLatin1( "Unknown tag, expected <table> and got <%1>." ).arg( tableElement.tagName() );
      return false;
    }

    tableElement = tableElement.nextSiblingElement();
  }

  return true;
}

bool DbInitializer::checkTable( const QDomElement &element )
{
  const QString tableName = element.attribute( QLatin1String("name") );

  typedef QPair<QString, QString> ColumnEntry;

  QList<ColumnEntry> columnsList;
  QStringList dataList;

  QDomElement columnElement = element.firstChildElement();
  while ( !columnElement.isNull() ) {
    if ( columnElement.tagName() == QLatin1String( "column" ) ) {
      ColumnEntry entry;
      entry.first = columnElement.attribute( QLatin1String("name") );
      entry.second = columnElement.attribute( QLatin1String("type") );
      columnsList.append( entry );
    } else if ( columnElement.tagName() == QLatin1String( "data" ) ) {
      QString statement = QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
          .arg( tableName )
          .arg( columnElement.attribute( QLatin1String("columns") ) )
          .arg( columnElement.attribute( QLatin1String("values") ) );
      dataList << statement;
    } else {
      mErrorMsg = QString::fromLatin1( "Unknown tag, expected <column> and got <%1>." ).arg( columnElement.tagName() );
      return false;
    }

    columnElement = columnElement.nextSiblingElement();
  }

  QSqlQuery query( mDatabase );

  if ( !query.exec( QLatin1String("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;") ) ) {
    mErrorMsg = QLatin1String( "Unable to retrieve table information from database." );
    return false;
  }

  bool found = false;
  while ( query.next() ) {
    if ( query.value( 0 ).toString() == tableName )
      found = true;
  }

  if ( !found ) {
    /**
     * We have to create the entire table.
     */

    QString columns;
    for ( int i = 0; i < columnsList.count(); ++i ) {
      if ( i != 0 )
        columns.append( QLatin1String(", ") );

      columns.append( columnsList[ i ].first + QLatin1Char(' ') + columnsList[ i ].second );
    }

    const QString statement = QString::fromLatin1( "CREATE TABLE %1 (%2);" ).arg( tableName, columns );

    if ( !query.exec( statement ) ) {
      mErrorMsg = QLatin1String( "Unable to create entire table." );
      return false;
    }
  } else {
    /**
     * Check for every column whether it exists.
     */

    const QSqlRecord table = mDatabase.record( tableName );

    for ( int i = 0; i < columnsList.count(); ++i ) {
      const ColumnEntry entry = columnsList[ i ];

      bool found = false;
      for ( int j = 0; j < table.count(); ++j ) {
        const QSqlField column = table.field( j );

        if ( columnsList[ i ].first == column.name() ) {
          found = true;
        }
      }

      if ( !found ) {
        /**
         * Add missing column to table.
         */
        const QString statement = QString::fromLatin1( "ALTER TABLE %1 ADD COLUMN %2 %3;" )
                                         .arg( tableName, entry.first, entry.second );

        if ( !query.exec( statement ) ) {
          mErrorMsg = QString::fromLatin1( "Unable to add column '%1' to table '%2'." ).arg( entry.first, tableName );
          return false;
        }
      }
    }

    // TODO: remove obsolete columns (when sqlite will support it) and adapt column type modifications
  }

  // add initial data if table is empty
  const QString statement = QString::fromLatin1( "SELECT * FROM %1 LIMIT 1" ).arg( tableName );
  if ( !query.exec( statement ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to retrieve data from table '%1'." ).arg( tableName );
    return false;
  }
  if ( query.size() == 0  || !query.first() ) {
    foreach ( QString stmt, dataList ) {
      if ( !query.exec( stmt ) ) {
        mErrorMsg = QString::fromLatin1( "Unable to add initial data to table '%1'." ).arg( tableName );
        return false;
      }
    }
  }

  return true;
}

QString DbInitializer::errorMsg() const
{
  return mErrorMsg;
}
