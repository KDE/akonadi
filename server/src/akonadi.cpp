/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "akonadi.h"
#include "akonadiconnection.h"
#include "serveradaptor.h"
#include "akdebug.h"

#include "cachecleaner.h"
#include "intervalcheck.h"
#include "storage/dbconfig.h"
#include "storage/datastore.h"
#include "notificationmanager.h"
#include "resourcemanager.h"
#include "tracer.h"
#include "xesammanager.h"
#include "nepomukmanager.h"
#include "debuginterface.h"
#include "storage/itemretrievalthread.h"
#include "preprocessormanager.h"

#include "libs/xdgbasedirs_p.h"
#include "libs/protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>

using namespace Akonadi;

static AkonadiServer *s_instance = 0;

AkonadiServer::AkonadiServer( QObject* parent )
    : QLocalServer( parent )
    , mCacheCleaner( 0 )
    , mIntervalChecker( 0 )
    , mItemRetrievalThread( 0 )
    , mDatabaseProcess( 0 )
    , mAlreadyShutdown( false )
{
    const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
    QSettings settings( serverConfigFile, QSettings::IniFormat );

    DbConfig::configuredDatabase()->init( settings );

    if ( DbConfig::configuredDatabase()->useInternalServer() )
      startDatabaseProcess();
    else
      createDatabase();

    s_instance = this;

    const QString connectionSettingsFile = XdgBaseDirs::akonadiConnectionConfigFile( XdgBaseDirs::WriteOnly );
    QSettings connectionSettings( connectionSettingsFile, QSettings::IniFormat );

#ifdef Q_OS_WIN
    QString namedPipe = settings.value( QLatin1String( "Connection/NamedPipe" ), QLatin1String( "Akonadi" ) ).toString();
    if ( !listen( namedPipe ) )
      akFatal() << "Unable to listen on Named Pipe" << namedPipe;

    connectionSettings.setValue( QLatin1String( "Data/Method" ), QLatin1String( "NamedPipe" ) );
    connectionSettings.setValue( QLatin1String( "Data/NamedPipe" ), namedPipe );
#else
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    QString socketDir = settings.value( QLatin1String( "Connection/SocketDirectory" ), defaultSocketDir ).toString();
    if ( socketDir[0] != QLatin1Char( '/' ) ) {
      QDir::home().mkdir( socketDir );
      socketDir = QDir::homePath() + QLatin1Char( '/' ) + socketDir;
    }

    const QString socketFile = socketDir + QLatin1String( "/akonadiserver.socket" );
    unlink( socketFile.toUtf8().constData() );
    if ( !listen( socketFile ) )
      akFatal() << "Unable to listen on Unix socket" << socketFile;

    connectionSettings.setValue( QLatin1String( "Data/Method" ), QLatin1String( "UnixPath" ) );
    connectionSettings.setValue( QLatin1String( "Data/UnixPath" ), socketFile );
#endif

    // initialize the database
    DataStore *db = DataStore::self();
    if ( !db->database().isOpen() )
      akFatal() << "Unable to open database" << db->database().lastError().text();
    if ( !db->init() )
      akFatal() << "Unable to initialize database.";

    NotificationManager::self();
    Tracer::self();
    new DebugInterface( this );
    ResourceManager::self();

    // Initialize the preprocessor manager
    PreprocessorManager::init();

    // Forcibly disable it if configuration says so
    if ( settings.value( QLatin1String( "General/DisablePreprocessing" ), false ).toBool() )
      PreprocessorManager::instance()->setEnabled( false );

    if ( settings.value( QLatin1String( "Cache/EnableCleaner" ), true ).toBool() ) {
      mCacheCleaner = new CacheCleaner( this );
      mCacheCleaner->start( QThread::IdlePriority );
    }

    mIntervalChecker = new IntervalCheck( this );
    mIntervalChecker->start( QThread::IdlePriority );

    mItemRetrievalThread = new ItemRetrievalThread( this );
    mItemRetrievalThread->start( QThread::HighPriority );

    const QString searchManager = settings.value( QLatin1String( "Search/Manager" ), QLatin1String( "Nepomuk" ) ).toString();
    if ( searchManager == QLatin1String( "Nepomuk" ) )
      mSearchManager = new NepomukManager( this );
    else
      mSearchManager = new DummySearchManager;

    new ServerAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Server" ), this );

    const QByteArray dbusAddress = qgetenv( "DBUS_SESSION_BUS_ADDRESS" );
    if ( !dbusAddress.isEmpty() ) {
      connectionSettings.setValue( QLatin1String( "DBUS/Address" ), QLatin1String( dbusAddress ) );
    }

    connect( QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
             SLOT(serviceOwnerChanged(QString,QString,QString)) );

    // Unhide all the items that are actually hidden.
    // The hidden flag was probably left out after an (abrupt)
    // server quit. We don't attempt to resume preprocessing
    // for the items as we don't actually know at which stage the
    // operation was interrupted...
    db->unhideAllPimItems();
}

AkonadiServer::~AkonadiServer()
{
}

template <typename T> static void quitThread( T & thread )
{
  if ( !thread )
    return;
  thread->quit();
  thread->wait();
  delete thread;
  thread = 0;
}

void AkonadiServer::quit()
{
    if ( mAlreadyShutdown )
      return;
    mAlreadyShutdown = true;

    qDebug() << "terminating service threads";
    quitThread( mCacheCleaner );
    quitThread( mIntervalChecker );
    quitThread( mItemRetrievalThread );

    delete mSearchManager;
    mSearchManager = 0;

    qDebug() << "terminating connection threads";
    for ( int i = 0; i < mConnections.count(); ++i )
      quitThread( mConnections[ i ] );
    mConnections.clear();

    // Terminate the preprocessor manager before the database but after all connections are gone
    PreprocessorManager::done();

    DataStore::self()->close();
    Q_ASSERT( QSqlDatabase::connectionNames().isEmpty() );

    qDebug() << "stopping db process";
    stopDatabaseProcess();

    QSettings settings( XdgBaseDirs::akonadiServerConfigFile(), QSettings::IniFormat );
    const QString connectionSettingsFile = XdgBaseDirs::akonadiConnectionConfigFile( XdgBaseDirs::WriteOnly );

#ifndef Q_OS_WIN
    QSettings connectionSettings( connectionSettingsFile, QSettings::IniFormat );
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    const QString socketDir = settings.value( QLatin1String( "Connection/SocketDirectory" ), defaultSocketDir ).toString();

    if ( !QDir::home().remove( socketDir + QLatin1String( "/akonadiserver.socket" ) ) )
        akError() << "Failed to remove Unix socket";
#endif
    if ( !QDir::home().remove( connectionSettingsFile ) )
        akError() << "Failed to remove runtime connection config file";

    QTimer::singleShot( 0, this, SLOT( doQuit() ) );
}

void AkonadiServer::doQuit()
{
    QCoreApplication::exit();
}

void AkonadiServer::incomingConnection( quintptr socketDescriptor )
{
    if ( mAlreadyShutdown )
      return;
    QPointer<AkonadiConnection> thread = new AkonadiConnection( socketDescriptor, this );
    connect( thread, SIGNAL( finished() ), thread, SLOT( deleteLater() ) );
    mConnections.append( thread );
    thread->start();
}


AkonadiServer * AkonadiServer::instance()
{
    if ( !s_instance )
        s_instance = new AkonadiServer();
    return s_instance;
}

void AkonadiServer::startDatabaseProcess()
{
  if ( !DbConfig::configuredDatabase()->useInternalServer() )
    return;

  // create the database directories if they don't exists
  XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/" ) );
  XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/file_db_data" ) );

  DbConfig::configuredDatabase()->startInternalServer();
}

void AkonadiServer::createDatabase()
{
  const QLatin1String initCon( "initConnection" );
  QSqlDatabase db = QSqlDatabase::addDatabase( DbConfig::configuredDatabase()->driverName(), initCon );
  DbConfig::configuredDatabase()->apply( db );
  db.setDatabaseName( DbConfig::configuredDatabase()->databaseName() );
  if ( !db.isValid() )
    akFatal() << "Invalid database object during initial database connection";

  if ( db.open() )
    db.close();
  else {
    akDebug() << "Failed to use database" << DbConfig::configuredDatabase()->databaseName();
    akDebug() << "Database error:" << db.lastError().text();
    akDebug() << "Trying to create database now...";

    db.close();
    db.setDatabaseName( QString() );
    if ( db.open() ) {
      {
        QSqlQuery query( db );
        if ( !query.exec( QLatin1String( "CREATE DATABASE akonadi" ) ) ) {
          akError() << "Failed to create database";
          akError() << "Query error:" << query.lastError().text();
          akFatal() << "Database error:" << db.lastError().text();
        }
      } // make sure query is destroyed before we close the db
      db.close();
    }
  }
  QSqlDatabase::removeDatabase( initCon );
}

void AkonadiServer::stopDatabaseProcess()
{
  if ( !DbConfig::configuredDatabase()->useInternalServer() )
    return;

  DbConfig::configuredDatabase()->stopInternalServer();
}

void AkonadiServer::serviceOwnerChanged(const QString & name, const QString & oldOwner, const QString & newOwner)
{
  Q_UNUSED( oldOwner );
  if ( name == QLatin1String( AKONADI_DBUS_CONTROL_SERVICE ) && newOwner.isEmpty() ) {
    akError() << "Control process died, committing suicide!";
    quit();
  }
}

#include "akonadi.moc"
