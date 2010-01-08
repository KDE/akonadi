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

#include "dbconfigmysqlembedded.h"

#include "../../libs/xdgbasedirs_p.h"
#include "akdebug.h"

#include <QtCore/QDir>
#include <QtSql/QSqlDriver>

using namespace Akonadi;

static QString dataDir()
{
  QString akonadiHomeDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
  if ( akonadiHomeDir.isEmpty() ) {
    akFatal() << "Unable to create directory 'akonadi' in " << XdgBaseDirs::homePath( "data" )
              << "during database initialization";
  }

  akonadiHomeDir += QDir::separator();

  return akonadiHomeDir;
}

static QString mysqlEmbeddedDataDir()
{
  const QString dbDataDir = dataDir() + QLatin1String( "db" ) + QDir::separator();
  if ( !QDir( dbDataDir ).exists() ) {
    QDir dir;
    if ( !dir.mkdir( dbDataDir ) )
      akFatal() << "Unable to create directory" << dbDataDir << "during database initialization.";
  }

  const QString dbDir = dbDataDir + QLatin1String( "akonadi" );
  if ( !QDir( dbDir ).exists() ) {
    QDir dir;
    if ( !dir.mkdir( dbDir ) )
      akFatal() << "Unable to create directory" << dbDir << "during database initialization.";
  }

  return dbDataDir;
}

DbConfigMysqlEmbedded::DbConfigMysqlEmbedded()
{
}

QString DbConfigMysqlEmbedded::driverName() const
{
  return QLatin1String( "QMYSQL_EMBEDDED" );
}

QString DbConfigMysqlEmbedded::databaseName() const
{
  return mDatabaseName;
}

bool DbConfigMysqlEmbedded::init( QSettings &settings )
{
  // determine default settings depending on the driver
  QString defaultDbName;
  QString defaultOptions;

  defaultDbName = QLatin1String( "akonadi" );
  defaultOptions = QString::fromLatin1( "SERVER_DATADIR=%1" ).arg( mysqlEmbeddedDataDir() );

  // read settings for current driver
  settings.beginGroup( driverName() );
  mDatabaseName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
  mHostName = settings.value( QLatin1String( "Host" ) ).toString();
  mUserName = settings.value( QLatin1String( "User" ) ).toString();
  mPassword = settings.value( QLatin1String( "Password" ) ).toString();
  mConnectionOptions = settings.value( QLatin1String( "Options" ), defaultOptions ).toString();
  settings.endGroup();

  // store back the default values
  settings.beginGroup( driverName() );
  settings.setValue( QLatin1String( "Name" ), mDatabaseName );
  settings.setValue( QLatin1String( "Options" ), mConnectionOptions );
  settings.endGroup();
  settings.sync();

  return true;
}

void DbConfigMysqlEmbedded::apply( QSqlDatabase &database )
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

bool DbConfigMysqlEmbedded::useInternalServer() const
{
  return false;
}
