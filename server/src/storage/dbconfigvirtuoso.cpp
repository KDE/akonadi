/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "dbconfigvirtuoso.h"

#include <QtCore/QDir>
#include <QtSql/QSqlDriver>

DbConfigVirtuoso::DbConfigVirtuoso()
{
}

QString DbConfigVirtuoso::driverName() const
{
  return QLatin1String( "QODBC" );
}

QString DbConfigVirtuoso::databaseName() const
{
  return mDatabaseName;
}

bool DbConfigVirtuoso::init( QSettings &settings )
{
  // determine default settings depending on the driver
  QString defaultDbName;
  QString defaultConnectOptions;

  defaultDbName = QLatin1String( "host=localhost:1111;uid=dba;pwd=dba;driver=/opt/virtuoso/lib/virtodbc.so" );
  defaultConnectOptions = QLatin1String( "SQL_ATTR_ODBC_VERSION=SQL_OV_ODBC3" );

  // read settings for current driver
  settings.beginGroup( driverName() );
  mDatabaseName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
  mHostName = settings.value( QLatin1String( "Host" ) ).toString();
  mUserName = settings.value( QLatin1String( "User" ) ).toString();
  mPassword = settings.value( QLatin1String( "Password" ) ).toString();
  mConnectionOptions = settings.value( QLatin1String( "Options" ), defaultConnectOptions ).toString();
  settings.endGroup();

  // store back the default values
  settings.beginGroup( driverName() );
  settings.setValue( QLatin1String( "Name" ), mDatabaseName );
  settings.setValue( QLatin1String( "Options" ), mConnectionOptions );
  settings.endGroup();
  settings.sync();

  return true;
}

void DbConfigVirtuoso::apply( QSqlDatabase &database )
{
  if ( !mDatabaseName.isEmpty() )
    database.setDatabaseName( mDatabaseName );
  if ( !mHostName.isEmpty() )
    database.setHostName( mHostName );
  if ( !mUserName.isEmpty() )
    database.setUserName( mUserName );
  if ( !mPassword.isEmpty() )
    database.setPassword( mPassword );

  database.setConnectOptions( mConnectionOptions );

  // can we check that during init() already?
  Q_ASSERT( database.driver()->hasFeature( QSqlDriver::LastInsertId ) );
}

bool DbConfigVirtuoso::useInternalServer() const
{
  return false;
}
