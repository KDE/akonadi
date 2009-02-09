/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "dbconfig.h"
#include "akdebug.h"
#include "../../libs/xdgbasedirs_p.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStringList>
#include <QSqlDriver>

using namespace Akonadi;

class DbConfigStatic
{
  public:
    DbConfigStatic() : mInternalServer( false )
    {
      const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
      QSettings settings( serverConfigFile, QSettings::IniFormat );

      // determine driver to use
      const QString defaultDriver = QLatin1String("QMYSQL");
      mDriverName = settings.value( QLatin1String("General/Driver"), defaultDriver ).toString();
      if ( mDriverName.isEmpty() )
        mDriverName = defaultDriver;

      if ( !checkDriver( mDriverName ) ) {
        akError() << "Falling back to database driver" << defaultDriver;
        mDriverName = defaultDriver;
        if ( !checkDriver( mDriverName ) )
          akFatal() << "No usable database driver found.";
      }

      mSizeThreshold = 4096;
      QVariant v = settings.value( QLatin1String("General/SizeThreshold"), mSizeThreshold );
      if ( v.canConvert<qint64>() )
      {
        mSizeThreshold = v.value<qint64>();
      } else
      {
        mSizeThreshold = 0;
      }

      if ( mSizeThreshold < 0 )
        mSizeThreshold = 0;

      mUseExternalPayloadFile = false;
      mUseExternalPayloadFile = settings.value( QLatin1String("General/ExternalPayload"), mUseExternalPayloadFile ).toBool();

      // determine default settings depending on the driver
      QString defaultDbName;
      QString defaultOptions;
      QString defaultServerPath;

      if ( mDriverName == QLatin1String( "QMYSQL" ) ) {
        defaultDbName = QLatin1String( "akonadi" );
#ifndef Q_WS_WIN // We assume that MySQL is running as service on Windows
        const bool defaultInternalServer = true;
#else
        const bool defaultInternalServer = false;
#endif
#ifdef MYSQLD_EXECUTABLE
        defaultServerPath = QLatin1String( MYSQLD_EXECUTABLE );
#endif
        if ( defaultServerPath.isEmpty() ) {
          const QStringList mysqldSearchPath = QStringList()
              << QLatin1String("/usr/sbin")
              << QLatin1String("/usr/local/sbin")
              << QLatin1String("/usr/local/libexec")
              << QLatin1String("/usr/libexec")
              << QLatin1String("/opt/mysql/libexec");
          defaultServerPath = XdgBaseDirs::findExecutableFile( QLatin1String("mysqld"), mysqldSearchPath );
        }
        mInternalServer = settings.value( QLatin1String("QMYSQL/StartServer"), defaultInternalServer ).toBool();
        if ( mInternalServer ) {
          const QString miscDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );
          defaultOptions = QString::fromLatin1( "UNIX_SOCKET=%1/mysql.socket" ).arg( miscDir );
        }
      } else if ( mDriverName == QLatin1String("QMYSQL_EMBEDDED") ) {
        defaultDbName = QLatin1String( "akonadi" );
        defaultOptions = QString::fromLatin1( "SERVER_DATADIR=%1" ).arg( mysqlEmbeddedDataDir() );
      } else if ( mDriverName == QLatin1String( "QSQLITE" ) ) {
        defaultDbName = sqliteDataFile();
      }

      // read settings for current driver
      settings.beginGroup( mDriverName );
      mName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
      mHostName = settings.value( QLatin1String( "Host" ) ).toString();
      mUserName = settings.value( QLatin1String( "User" ) ).toString();
      mPassword = settings.value( QLatin1String( "Password" ) ).toString();
      mConnectionOptions = settings.value( QLatin1String( "Options" ), defaultOptions ).toString();
      mServerPath = settings.value( QLatin1String("ServerPath"), defaultServerPath ).toString();
      settings.endGroup();

      // verify settings and apply permanent changes (written out below)
      if ( mDriverName == QLatin1String( "QMYSQL" ) ) {
        if ( mInternalServer && mConnectionOptions.isEmpty() )
          mConnectionOptions = defaultOptions;
        if ( mInternalServer && (mServerPath.isEmpty() || !QFile::exists(mServerPath) ) )
          mServerPath = defaultServerPath;
      }

      // store back the default values
      settings.setValue( QLatin1String( "General/Driver" ), mDriverName );
      settings.setValue( QLatin1String( "General/SizeThreshold" ), mSizeThreshold );
      settings.setValue( QLatin1String( "General/ExternalPayload" ), mUseExternalPayloadFile );
      settings.beginGroup( mDriverName );
      settings.setValue( QLatin1String( "Name" ), mName );
      settings.setValue( QLatin1String( "User" ), mUserName );
      settings.setValue( QLatin1String( "Password" ), mPassword );
      settings.setValue( QLatin1String( "Options" ), mConnectionOptions );
      if ( !mServerPath.isEmpty() )
        settings.setValue( QLatin1String( "ServerPath" ), mServerPath );
      settings.setValue( QLatin1String( "StartServer" ), mInternalServer );
      settings.endGroup();
      settings.sync();

      // apply temporary changes to the settings
      if ( mDriverName == QLatin1String( "QMYSQL" ) ) {
        if ( mInternalServer ) {
          mHostName.clear();
          mUserName.clear();
          mPassword.clear();
        }
      }

    }

    bool checkDriver( const QString &driver ) const
    {
      if ( QSqlDatabase::isDriverAvailable( driver ) )
        return true;
      akError() << "Database driver" << driver << "was not found.";
      akError() << "Available drivers are:" << QSqlDatabase::drivers();
      return false;
    }

    QString dataDir() const
    {
      QString akonadiHomeDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
      if ( akonadiHomeDir.isEmpty() ) {
        akFatal() << "Unable to create directory 'akonadi' in " << XdgBaseDirs::homePath( "data" )
            << "during database initialization";
      }
      akonadiHomeDir += QDir::separator();
      return akonadiHomeDir;
    }

    QString sqliteDataFile() const
    {
      Q_ASSERT( mDriverName == QLatin1String( "QSQLITE" ) );
      const QString akonadiPath = dataDir() + QLatin1String("akonadi.db");
      if ( !QFile::exists( akonadiPath ) ) {
        QFile file( akonadiPath );
        if ( !file.open( QIODevice::WriteOnly ) )
          akFatal() << "Unable to create file" << akonadiPath << "during database initialization.";
        file.close();
      }
      return akonadiPath;
    }

    QString mysqlEmbeddedDataDir() const
    {
      Q_ASSERT( mDriverName == QLatin1String( "QMYSQL_EMBEDDED" ) );
      const QString dbDataDir = dataDir() + QLatin1String( "db" ) + QDir::separator();
      if ( !QDir( dbDataDir ).exists() ) {
        QDir dir;
        if ( !dir.mkdir( dbDataDir ) )
          akFatal() << "Unable to create directory" << dbDataDir << "during database initialization.";
      }

      const QString dbDir = dbDataDir + QLatin1String("akonadi");
      if ( !QDir( dbDir ).exists() ) {
        QDir dir;
        if ( !dir.mkdir( dbDir ) )
          akFatal() << "Unable to create directory" << dbDir << "during database initialization.";
      }
      return dbDataDir;
    }

    QString mDriverName;
    QString mName;
    QString mHostName;
    QString mUserName;
    QString mPassword;
    QString mConnectionOptions;
    QString mServerPath;
    bool mInternalServer;
    bool mUseExternalPayloadFile;
    qint64 mSizeThreshold;
};

Q_GLOBAL_STATIC( DbConfigStatic, sInstance )

void DbConfig::init()
{
  sInstance();
}

void DbConfig::configure(QSqlDatabase & db)
{
  if ( !sInstance()->mName.isEmpty() )
    db.setDatabaseName( sInstance()->mName );
  if ( !sInstance()->mHostName.isEmpty() )
    db.setHostName( sInstance()->mHostName );
  if ( !sInstance()->mUserName.isEmpty() )
    db.setUserName( sInstance()->mUserName );
  if ( !sInstance()->mPassword.isEmpty() )
    db.setPassword( sInstance()->mPassword );
  db.setConnectOptions( sInstance()->mConnectionOptions );

  // can we check that during init() already?
  Q_ASSERT( db.driver()->hasFeature( QSqlDriver::LastInsertId ) );
//   Q_ASSERT( db.driver()->hasFeature( QSqlDriver::Transactions ) );
}

QString DbConfig::driverName()
{
  return sInstance()->mDriverName;
}

bool DbConfig::useInternalServer()
{
  return sInstance()->mInternalServer;
}

QString DbConfig::serverPath()
{
  return sInstance()->mServerPath;
}

QString DbConfig::databaseName()
{
  return sInstance()->mName;
}

qint64 DbConfig::sizeThreshold()
{
  return sInstance()->mSizeThreshold;
}

bool DbConfig::useExternalPayloadFile()
{
  return sInstance()->mUseExternalPayloadFile;
}

