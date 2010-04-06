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

#include "dbconfigpostgresql.h"

#include "../../libs/xdgbasedirs_p.h"
#include "akdebug.h"

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;

DbConfigPostgresql::DbConfigPostgresql()
  : mDatabaseProcess( 0 )
{
}

QString DbConfigPostgresql::driverName() const
{
  return QLatin1String( "QPSQL" );
}

QString DbConfigPostgresql::databaseName() const
{
  return mDatabaseName;
}

bool DbConfigPostgresql::init( QSettings &settings )
{
  // determine default settings depending on the driver
  QString defaultDbName;
  QString defaultHostName;
  QString defaultOptions;
  QString defaultServerPath;
  QString defaultInitDbPath;
  QString defaultCleanShutdownCommand;

  defaultDbName = QLatin1String( "akonadi" );
#ifndef Q_WS_WIN // We assume that PostgreSQL is running as service on Windows
  const bool defaultInternalServer = true;
#else
  const bool defaultInternalServer = false;
#endif

  mInternalServer = settings.value( QLatin1String( "QPSQL/StartServer" ), defaultInternalServer ).toBool();
  if ( mInternalServer ) {
    const QStringList postgresSearchPath = QStringList()
      << QLatin1String( "/usr/sbin" )
      << QLatin1String( "/usr/local/sbin" )
      << QLatin1String( "/usr/lib/postgresql/8.4/bin" );

    defaultServerPath = XdgBaseDirs::findExecutableFile( QLatin1String( "pg_ctl" ), postgresSearchPath );
    defaultInitDbPath = XdgBaseDirs::findExecutableFile( QLatin1String( "initdb" ), postgresSearchPath );
    defaultHostName = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );
    defaultCleanShutdownCommand = QString::fromLatin1( "%1 stop -D%2" )
                                      .arg( defaultServerPath )
                                      .arg( XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) ) );
  }

  // read settings for current driver
  settings.beginGroup( driverName() );
  mDatabaseName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
  mHostName = settings.value( QLatin1String( "Host" ), defaultHostName ).toString();
  mUserName = settings.value( QLatin1String( "User" ) ).toString();
  mPassword = settings.value( QLatin1String( "Password" ) ).toString();
  mConnectionOptions = settings.value( QLatin1String( "Options" ), defaultOptions ).toString();
  mServerPath = settings.value( QLatin1String("ServerPath"), defaultServerPath ).toString();
  mInitDbPath = settings.value( QLatin1String("InitDbPath"), defaultInitDbPath ).toString();
  mCleanServerShutdownCommand = settings.value( QLatin1String( "CleanServerShutdownCommand" ), defaultCleanShutdownCommand ).toString();
  settings.endGroup();

  // store back the default values
  settings.beginGroup( driverName() );
  settings.setValue( QLatin1String( "Name" ), mDatabaseName );
  settings.setValue( QLatin1String( "Host" ), mHostName );
  settings.setValue( QLatin1String( "Options" ), mConnectionOptions );
  if ( !mServerPath.isEmpty() )
    settings.setValue( QLatin1String( "ServerPath" ), mServerPath );
  if ( !mInitDbPath.isEmpty() )
    settings.setValue( QLatin1String( "InitDbPath" ), mInitDbPath );
  settings.setValue( QLatin1String( "StartServer" ), mInternalServer );
  settings.endGroup();
  settings.sync();

  return true;
}

void DbConfigPostgresql::apply( QSqlDatabase &database )
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

bool DbConfigPostgresql::useInternalServer() const
{
  return mInternalServer;
}

void DbConfigPostgresql::startInternalServer()
{
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) );
  const QString socketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );

  if ( !QFile::exists( QString::fromLatin1( "%1/PG_VERSION" ).arg( dataDir ) ) ) {
    // postgres data directory not initialized yet, so call initdb on it

    // call 'initdb -D/home/user/.local/share/akonadi/data_db'
    const QString command = QString::fromLatin1( "%1 -D%2" ).arg( mInitDbPath ).arg( dataDir );
    QProcess::execute( command );

    const QString configFileName = dataDir + QDir::separator() + QLatin1String( "postgresql.conf" );
    QFile configFile( configFileName );
    configFile.open( QIODevice::ReadOnly );

    QString content = QString::fromUtf8( configFile.readAll() );
    configFile.close();

    // avoid binding to tcp port
    content.replace( QLatin1String( "#listen_addresses = 'localhost'" ),
                     QLatin1String( "listen_addresses = ''" ) );

    // set the directory for unix domain socket communication
    content.replace( QLatin1String( "#unix_socket_directory = ''" ),
                     QString::fromLatin1( "unix_socket_directory = '%1'" ).arg( socketDir ) );

    // treat backslashes in strings literally as defined in the SQL standard
    content.replace( QLatin1String( "#standard_conforming_strings = off" ),
                     QLatin1String( "standard_conforming_strings = on" ) );

    configFile.open( QIODevice::WriteOnly );
    configFile.write( content.toUtf8() );
    configFile.close();
  }

  // synthesize the postgres command
  QStringList arguments;
  arguments << QString::fromLatin1( "-w" )
            << QString::fromLatin1( "-t10" ) // default is 60 seconds.
            << QString::fromLatin1( "start" )
            << QString::fromLatin1( "-D%1" ).arg( dataDir );

  mDatabaseProcess = new QProcess;
  mDatabaseProcess->start( mServerPath, arguments );
  if ( !mDatabaseProcess->waitForStarted() ) {
    akError() << "Could not start database server!";
    akError() << "executable:" << mServerPath;
    akError() << "arguments:" << arguments;
    akFatal() << "process error:" << mDatabaseProcess->errorString();
  }

  const QLatin1String initCon( "initConnection" );
  {
    QSqlDatabase db = QSqlDatabase::addDatabase( QLatin1String( "QPSQL" ), initCon );
    apply( db );

    // use the dummy database that is always available
    db.setDatabaseName( QLatin1String( "template1" ) );

    if ( !db.isValid() )
      akFatal() << "Invalid database object during database server startup";

    bool opened = false;
    for ( int i = 0; i < 120; ++i ) {
      opened = db.open();
      if ( opened )
        break;

      if ( mDatabaseProcess->waitForFinished( 500 ) ) {
        akError() << "Database process exited unexpectedly during initial connection!";
        akError() << "executable:" << mServerPath;
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

        // check if the 'akonadi' database already exists
        query.exec( QString::fromLatin1( "SELECT * FROM pg_catalog.pg_database WHERE datname = '%1'" ).arg( mDatabaseName ) );

        // if not, create it
        if ( !query.first() ) {
          if ( !query.exec( QString::fromLatin1( "CREATE DATABASE %1" ).arg( mDatabaseName ) ) ) {
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

void DbConfigPostgresql::stopInternalServer()
{
  if ( !mDatabaseProcess )
    return;

  // first, try the nicest approach
  if ( !mCleanServerShutdownCommand.isEmpty() ) {
    QProcess::execute( mCleanServerShutdownCommand );
    if ( mDatabaseProcess->waitForFinished( 3000 ) )
      return;
  }

  // if pg_ctl couldn't terminate all the postgres processes, we have to kill the master one.
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) );
  const QString pidFileName = QString::fromLatin1( "%1/postmaster.pid" ).arg( dataDir );
  QFile pidFile( pidFileName );
  if ( pidFile.open( QIODevice::ReadOnly ) ) {
    QString postmasterPid = QString::fromUtf8( pidFile.readLine( 0 ).trimmed() );
    akError() << "The postmaster is still running. Killing it.";

    QStringList arguments;
    arguments << QString::fromLatin1( "kill")
              << QString::fromLatin1( "ABRT" )
              << QString::fromLatin1( "%1" ).arg( postmasterPid );

    const QString command = QString::fromLatin1( "%1" ).arg( mServerPath );
    QProcess::execute( command, arguments );
  }
}
