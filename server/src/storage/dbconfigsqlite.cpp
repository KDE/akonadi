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

#include "dbconfigsqlite.h"

#include "../../libs/xdgbasedirs_p.h"
#include "akdebug.h"

#include <QtCore/QDir>

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

static QString sqliteDataFile()
{
  const QString akonadiPath = dataDir() + QLatin1String( "akonadi.db" );
  if ( !QFile::exists( akonadiPath ) ) {
    QFile file( akonadiPath );
    if ( !file.open( QIODevice::WriteOnly ) )
      akFatal() << "Unable to create file" << akonadiPath << "during database initialization.";
    file.close();
  }

  return akonadiPath;
}


DbConfigSqlite::DbConfigSqlite()
{
}

QString DbConfigSqlite::driverName() const
{
  return QLatin1String( "QSQLITE" );
}

QString DbConfigSqlite::databaseName() const
{
  return mDatabaseName;
}

bool DbConfigSqlite::init( QSettings &settings )
{
  // determine default settings depending on the driver
  QString defaultDbName;

  defaultDbName = sqliteDataFile();

  // read settings for current driver
  settings.beginGroup( driverName() );
  mDatabaseName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
  mHostName = settings.value( QLatin1String( "Host" ) ).toString();
  mUserName = settings.value( QLatin1String( "User" ) ).toString();
  mPassword = settings.value( QLatin1String( "Password" ) ).toString();
  mConnectionOptions = settings.value( QLatin1String( "Options" ) ).toString();
  settings.endGroup();

  // store back the default values
  settings.beginGroup( driverName() );
  settings.setValue( QLatin1String( "Name" ), mDatabaseName );
  settings.endGroup();
  settings.sync();

  return true;
}

void DbConfigSqlite::apply( QSqlDatabase &database )
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

bool DbConfigSqlite::useInternalServer() const
{
  return false;
}
