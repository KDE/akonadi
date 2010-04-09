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

#include "dbconfigmysql.h"

#include "../../libs/xdgbasedirs_p.h"
#include "akdebug.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;

DbConfigMysql::DbConfigMysql()
  : mDatabaseProcess( 0 )
{
}

QString DbConfigMysql::driverName() const
{
  return QLatin1String( "QMYSQL" );
}

QString DbConfigMysql::databaseName() const
{
  return mDatabaseName;
}

bool DbConfigMysql::init( QSettings &settings )
{
  // determine default settings depending on the driver
  QString defaultDbName;
  QString defaultHostName;
  QString defaultOptions;
  QString defaultServerPath;
  QString defaultCleanShutdownCommand;

  defaultDbName = QLatin1String( "akonadi" );
  const bool defaultInternalServer = true;
#ifdef MYSQLD_EXECUTABLE
   if ( QFile::exists( QLatin1String( MYSQLD_EXECUTABLE ) ) )
     defaultServerPath = QLatin1String( MYSQLD_EXECUTABLE );
#endif
  const QStringList mysqldSearchPath = QStringList()
      << QLatin1String( "/usr/sbin" )
      << QLatin1String( "/usr/local/sbin" )
      << QLatin1String( "/usr/local/libexec" )
      << QLatin1String( "/usr/libexec" )
      << QLatin1String( "/opt/mysql/libexec" )
      << QLatin1String( "/opt/local/lib/mysql5/bin" )
      << QLatin1String( "/opt/mysql/sbin" );
  if ( defaultServerPath.isEmpty() )
    defaultServerPath = XdgBaseDirs::findExecutableFile( QLatin1String( "mysqld" ), mysqldSearchPath );

  const QString mysqladminPath = XdgBaseDirs::findExecutableFile( QLatin1String( "mysqladmin" ), mysqldSearchPath );
  if ( !mysqladminPath.isEmpty() ) {
#ifndef Q_OS_WIN
    defaultCleanShutdownCommand = QString::fromLatin1( "%1 shutdown --socket=%2/mysql.socket" )
                                      .arg( mysqladminPath )
                                      .arg( XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) ) );
#else
    defaultCleanShutdownCommand = QString::fromLatin1( "%1 shutdown --shared-memory" ).arg( mysqladminPath );
#endif
  }

  mMysqlInstallDbPath = XdgBaseDirs::findExecutableFile( QLatin1String( "mysql_install_db" ), mysqldSearchPath );
  akDebug() << "Found mysql_install_db: " << mMysqlInstallDbPath;

  mMysqlUpgradeDBPath = XdgBaseDirs::findExecutableFile( QLatin1String( "mysql_upgrade" ), mysqldSearchPath );
  akDebug() << "Found mysql_upgrade: " << mMysqlUpgradeDBPath;

  mInternalServer = settings.value( QLatin1String( "QMYSQL/StartServer" ), defaultInternalServer ).toBool();
  if ( mInternalServer ) {
    const QString miscDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );
#if !(defined Q_WS_WIN)
    defaultOptions = QString::fromLatin1( "UNIX_SOCKET=%1/mysql.socket" ).arg( miscDir );
#endif
  }

  // read settings for current driver
  settings.beginGroup( driverName() );
  mDatabaseName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
  mHostName = settings.value( QLatin1String( "Host" ), defaultHostName ).toString();
  mUserName = settings.value( QLatin1String( "User" ) ).toString();
  mPassword = settings.value( QLatin1String( "Password" ) ).toString();
  mConnectionOptions = settings.value( QLatin1String( "Options" ), defaultOptions ).toString();
  mServerPath = settings.value( QLatin1String("ServerPath"), defaultServerPath ).toString();
  mCleanServerShutdownCommand = settings.value( QLatin1String( "CleanServerShutdownCommand" ), defaultCleanShutdownCommand ).toString();
  settings.endGroup();

  // verify settings and apply permanent changes (written out below)
  if ( mInternalServer && mConnectionOptions.isEmpty() )
    mConnectionOptions = defaultOptions;
  if ( mInternalServer && (mServerPath.isEmpty() || !QFile::exists( mServerPath ) ) )
    mServerPath = defaultServerPath;

  // store back the default values
  settings.beginGroup( driverName() );
  settings.setValue( QLatin1String( "Name" ), mDatabaseName );
  settings.setValue( QLatin1String( "Host" ), mHostName );
  settings.setValue( QLatin1String( "Options" ), mConnectionOptions );
  if ( !mServerPath.isEmpty() )
    settings.setValue( QLatin1String( "ServerPath" ), mServerPath );
  settings.setValue( QLatin1String( "StartServer" ), mInternalServer );
  settings.endGroup();
  settings.sync();

  // apply temporary changes to the settings
  if ( mInternalServer ) {
    mHostName.clear();
    mUserName.clear();
    mPassword.clear();
  }

  return true;
}

void DbConfigMysql::apply( QSqlDatabase &database )
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

bool DbConfigMysql::useInternalServer() const
{
  return mInternalServer;
}

void DbConfigMysql::startInternalServer()
{
  const QString mysqldPath = mServerPath;

  const QString akDir   = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/" ) );
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) );
  const QString miscDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );

  // generate config file
  const QString globalConfig = XdgBaseDirs::findResourceFile( "config", QLatin1String( "akonadi/mysql-global.conf" ) );
  const QString localConfig  = XdgBaseDirs::findResourceFile( "config", QLatin1String( "akonadi/mysql-local.conf" ) );
  const QString actualConfig = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) ) + QLatin1String( "/mysql.conf" );
  if ( globalConfig.isEmpty() )
    akFatal() << "Did not find MySQL server default configuration (mysql-global.conf)";

  bool confUpdate = false;
  QFile actualFile ( actualConfig );
  // update conf only if either global (or local) is newer than actual
  if ( (QFileInfo( globalConfig ).lastModified() > QFileInfo( actualFile ).lastModified()) ||
       (QFileInfo( localConfig ).lastModified()  > QFileInfo( actualFile ).lastModified()) )
  {
    QFile globalFile( globalConfig );
    QFile localFile ( localConfig );
    if ( globalFile.open( QFile::ReadOnly ) && actualFile.open( QFile::WriteOnly ) ) {
      actualFile.write( globalFile.readAll() );
      if ( !localConfig.isEmpty() ) {
        if ( localFile.open( QFile::ReadOnly ) ) {
          actualFile.write( localFile.readAll() );
          localFile.close();
        }
      }
      globalFile.close();
      actualFile.close();
      confUpdate = true;
    } else {
      akError() << "Unable to create MySQL server configuration file.";
      akError() << "This means that either the default configuration file (mysql-global.conf) was not readable";
      akFatal() << "or the target file (mysql.conf) could not be written.";
    }
  }

  // MySQL doesn't like world writeable config files (which makes sense), but
  // our config file somehow ends up being world-writable on some systems for no
  // apparent reason nevertheless, so fix that
  const QFile::Permissions allowedPerms = actualFile.permissions()
      & ( QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther );
  if ( allowedPerms != actualFile.permissions() )
    actualFile.setPermissions( allowedPerms );

  if ( dataDir.isEmpty() )
    akFatal() << "Akonadi server was not able to create database data directory";

  if ( akDir.isEmpty() )
    akFatal() << "Akonadi server was not able to create database log directory";

  if ( miscDir.isEmpty() )
    akFatal() << "Akonadi server was not able to create database misc directory";

  // the socket path must not exceed 103 characters, so check for max dir length right away
  if ( miscDir.length() >= 90 )
      akFatal() << "MySQL cannot deal with a socket path this long. Path was: " << miscDir;

  // move mysql error log file out of the way
  const QFileInfo errorLog( dataDir + QDir::separator() + QString::fromLatin1( "mysql.err" ) );
  if ( errorLog.exists() ) {
    QFile logFile( errorLog.absoluteFilePath() );
    QFile oldLogFile( dataDir + QDir::separator() + QString::fromLatin1( "mysql.err.old" ) );
    if ( logFile.open( QFile::ReadOnly ) && oldLogFile.open( QFile::Append ) ) {
      oldLogFile.write( logFile.readAll() );
      oldLogFile.close();
      logFile.close();
      logFile.remove();
    } else {
      akError() << "Failed to open MySQL error log.";
    }
  }

  // first run, some MySQL versions need a mysql_install_db run for that
  const QString confFile = XdgBaseDirs::findResourceFile( "config", QLatin1String("akonadi/mysql-global.conf" ));
  if ( QDir( dataDir ).entryList( QDir::NoDotAndDotDot | QDir::AllEntries ).isEmpty() && !mMysqlInstallDbPath.isEmpty() ) {
    const QStringList arguments = QStringList() << QString::fromLatin1( "--force" ) << QString::fromLatin1( "--defaults-file=%1").arg(confFile) << QString::fromLatin1( "--datadir=%1/" ).arg( dataDir ); 
    QProcess::execute( mMysqlInstallDbPath, arguments );
  } 

  // clear mysql ib_logfile's in case innodb_log_file_size option changed in last confUpdate
  if ( confUpdate ) {
    QFile( dataDir + QDir::separator() + QString::fromLatin1( "ib_logfile0" ) ).remove();
    QFile( dataDir + QDir::separator() + QString::fromLatin1( "ib_logfile1" ) ).remove();
  }

  // synthesize the mysqld command
  QStringList arguments;
  arguments << QString::fromLatin1( "--defaults-file=%1/mysql.conf" ).arg( akDir );
#ifndef Q_WS_WIN
  arguments << QString::fromLatin1( "--datadir" ) << QString::fromLatin1( "%1/" ).arg( dataDir );
  arguments << QString::fromLatin1( "--socket=%1/mysql.socket" ).arg( miscDir );
#else
  arguments << QString::fromLatin1( "--datadir=%1/" ).arg( dataDir );
  arguments << QString::fromLatin1( "--shared-memory" );
#endif

  mDatabaseProcess = new QProcess;
  mDatabaseProcess->start( mysqldPath, arguments );
  if ( !mDatabaseProcess->waitForStarted() ) {
    akError() << "Could not start database server!";
    akError() << "executable:" << mysqldPath;
    akError() << "arguments:" << arguments;
    akFatal() << "process error:" << mDatabaseProcess->errorString();
  }

  if ( !mMysqlUpgradeDBPath.isEmpty() ) {
    const QStringList arguments = QStringList() << QString::fromLatin1( "--socket=%1/mysql.socket" ).arg( miscDir );
    QProcess::execute( mMysqlUpgradeDBPath, arguments );
  }

  const QLatin1String initCon( "initConnection" );
  {
    QSqlDatabase db = QSqlDatabase::addDatabase( QLatin1String( "QMYSQL" ), initCon );
    apply( db );

    db.setDatabaseName( QString() ); // might not exist yet, then connecting to the actual db will fail
    if ( !db.isValid() )
      akFatal() << "Invalid database object during database server startup";

    bool opened = false;
    for ( int i = 0; i < 120; ++i ) {
      opened = db.open();
      if ( opened )
        break;
      if ( mDatabaseProcess->waitForFinished( 500 ) ) {
        akError() << "Database process exited unexpectedly during initial connection!";
        akError() << "executable:" << mysqldPath;
        akError() << "arguments:" << arguments;
        akError() << "stdout:" << mDatabaseProcess->readAllStandardOutput();
        akError() << "stderr:" << mDatabaseProcess->readAllStandardError();
        akError() << "exit code:" << mDatabaseProcess->exitCode();
        akFatal() << "process error:" << mDatabaseProcess->errorString();
      }
    }

    if ( opened ) {
      {
        QSqlQuery query( db );
        if ( !query.exec( QString::fromLatin1( "USE %1" ).arg( mDatabaseName ) ) ) {
          akDebug() << "Failed to use database" << mDatabaseName;
          akDebug() << "Query error:" << query.lastError().text();
          akDebug() << "Database error:" << db.lastError().text();
          akDebug() << "Trying to create database now...";
          if ( !query.exec( QLatin1String( "CREATE DATABASE akonadi" ) ) ) {
            akError() << "Failed to create database";
            akError() << "Query error:" << query.lastError().text();
            akFatal() << "Database error:" << db.lastError().text();
          }
        }
      } // make sure query is destroyed before we close the db
      db.close();
    }
  }

  QSqlDatabase::removeDatabase( initCon );
}

void DbConfigMysql::stopInternalServer()
{
  if ( !mDatabaseProcess )
    return;

  // first, try the nicest approach
  if ( !mCleanServerShutdownCommand.isEmpty() ) {
    QProcess::execute( mCleanServerShutdownCommand );
    if ( mDatabaseProcess->waitForFinished( 3000 ) )
      return;
  }

  mDatabaseProcess->terminate();
  const bool result = mDatabaseProcess->waitForFinished( 3000 );
  // We've waited nicely for 3 seconds, to no avail, let's be rude.
  if ( !result )
    mDatabaseProcess->kill();
}
