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

#include "dbintrospector.h"
#include "dbintrospector_impl.h"
#include "dbtype.h"
#include "dbexception.h"

#include <akdebug.h>

#include <QStringList>
#include <QSqlQuery>

DbIntrospector::Ptr DbIntrospector::createInstance(const QSqlDatabase& database)
{
  switch ( DbType::type( database ) ) {
    case DbType::MySQL:
      return Ptr( new DbIntrospectorMySql( database ) );
    case DbType::Sqlite:
      return Ptr( new DbIntrospectorSqlite( database ) );
    case DbType::PostgreSQL:
      return Ptr( new DbIntrospectorPostgreSql( database ) );
    case DbType::Virtuoso:
      return Ptr( new DbIntrospectorVirtuoso( database ) );
    case DbType::Unknown:
      break;
  }
  akFatal() << database.driverName() << "backend  not supported";
  return Ptr();
}

DbIntrospector::DbIntrospector(const QSqlDatabase& database) : m_database( database )
{
}

DbIntrospector::~DbIntrospector()
{
}

bool DbIntrospector::hasTable(const QString& tableName)
{
  return m_database.tables().contains( tableName, Qt::CaseInsensitive );
}

bool DbIntrospector::hasIndex(const QString& tableName, const QString& indexName)
{
  QSqlQuery query( m_database );
  if ( !query.exec( hasIndexQuery( tableName, indexName ) ) )
    throw DbException( "Failed to query index.", query );
  return query.next();
}

QString DbIntrospector::hasIndexQuery(const QString& tableName, const QString& indexName)
{
  Q_UNUSED( tableName );
  Q_UNUSED( indexName );
  akFatal() << "Implement index support for your database!";
  return QString();
}
