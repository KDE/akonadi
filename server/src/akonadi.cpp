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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <unistd.h>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "serveradaptor.h"

#include "cachecleaner.h"
#include "cachepolicymanager.h"
#include "storage/datastore.h"
#include "notificationmanager.h"
#include "resourcemanager.h"
#include "tracer.h"

using namespace Akonadi;

static AkonadiServer *s_instance = 0;

AkonadiServer::AkonadiServer( QObject* parent )
    : QTcpServer( parent ), mDatabaseProcess( 0 )
{
    // create the akonadi directory
    QDir homeDir = QDir::home();
    if ( !homeDir.exists( QLatin1String( ".akonadi/" ) ) )
      homeDir.mkdir( QLatin1String( ".akonadi/" ) );

    QSettings settings( QDir::homePath() + QLatin1String("/.akonadi/akonadiserverrc"), QSettings::IniFormat );
    if ( settings.value( QLatin1String("General/Driver"), QLatin1String( "QMYSQL" ) ).toString() == QLatin1String( "QMYSQL" )
         && settings.value( QLatin1String( "MYSQL/StartServer" ), true ).toBool() )
      startDatabaseProcess();

    s_instance = this;
    if ( !listen( QHostAddress::LocalHost, 4444 ) )
      qFatal("Unable to listen on port 4444");

    // initialize the database
    DataStore *db = DataStore::self();
    if ( !db->database().isOpen() )
      qFatal("Unable to open database.");
    if ( !db->init() )
      qFatal("Unable to initalize database.");

    NotificationManager::self();
    Tracer::self();
    ResourceManager::self();
    new CachePolicyManager( this );
    mCacheCleaner = new CacheCleaner( this );
    mCacheCleaner->start( QThread::IdlePriority );

    new ServerAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Server" ), this );
}


AkonadiServer::~AkonadiServer()
{
}

void AkonadiServer::quit()
{
    mCacheCleaner->quit();
    mCacheCleaner->wait();
    delete mCacheCleaner;

    for ( int i = 0; i < mConnections.count(); ++i ) {
      if ( mConnections[ i ] ) {
        mConnections[ i ]->wait();
      }
    }

    QSettings settings( QDir::homePath() + QLatin1String("/.akonadi/akonadiserverrc"), QSettings::IniFormat );
    if ( settings.value( QLatin1String("General/Driver") ).toString() == QLatin1String( "QMYSQL" )
         && settings.value( QLatin1String( "MYSQL/StartServer" ), true ).toBool() )
      stopDatabaseProcess();

    QTimer::singleShot( 0, this, SLOT( doQuit() ) );
}

void AkonadiServer::doQuit()
{
    QCoreApplication::exit();
}

void AkonadiServer::incomingConnection( int socketDescriptor )
{
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
  // create the database directories if they don't exists
  QDir akonadiDirectory( QDir::home().path() + QLatin1String( "/.akonadi/" ) );

  if ( !akonadiDirectory.exists( QLatin1String( "db_data/" ) ) )
    akonadiDirectory.mkdir( QLatin1String( "db_data/" ) );

  if ( !akonadiDirectory.exists( QLatin1String( "db_log/" ) ) )
    akonadiDirectory.mkdir( QLatin1String( "db_log/" ) );

  if ( !akonadiDirectory.exists( QLatin1String( "db_misc/" ) ) )
    akonadiDirectory.mkdir( QLatin1String( "db_misc/" ) );

  // synthesize the mysqld command
  QStringList arguments;
  arguments << QString::fromLatin1( "--datadir=%1/db_data/" ).arg( akonadiDirectory.path() );
  arguments << QString::fromLatin1( "--log-bin=%1/db_log/" ).arg( akonadiDirectory.path() );
  arguments << QString::fromLatin1( "--log-bin-index=%1/db_log/" ).arg( akonadiDirectory.path() );
  arguments << QString::fromLatin1( "--socket=%1/db_misc/mysql.socket" ).arg( akonadiDirectory.path() );
  arguments << QString::fromLatin1( "--pid-file=%1/db_misc/mysql.pid" ).arg( akonadiDirectory.path() );
  arguments << QString::fromLatin1( "--skip-grant-table" );
  arguments << QString::fromLatin1( "--skip-networking" );

  mDatabaseProcess = new QProcess( this );
  mDatabaseProcess->start( QLatin1String( "/usr/sbin/mysqld" ), arguments );
  mDatabaseProcess->waitForStarted();

  QSqlDatabase db = QSqlDatabase::addDatabase( QLatin1String( "QMYSQL" ), QLatin1String( "initConnection" ) );
  db.setConnectOptions( QString::fromLatin1( "UNIX_SOCKET=%1/db_misc/mysql.socket" ).arg( akonadiDirectory.path() ) );

  bool opened = false;
  for ( int i = 0; i < 10; ++i ) {
    opened = db.open();
    if ( opened )
      break;

    sleep( 1 );
  }

  if ( opened ) {
    QSqlQuery query( db );
    if ( !query.exec( QLatin1String( "USE DATABASE akonadi" ) ) )
      query.exec( QLatin1String( "CREATE DATABASE akonadi" ) );
  }

  QSqlDatabase::removeDatabase( QLatin1String( "initConnection" ) );
}

void AkonadiServer::stopDatabaseProcess()
{
  mDatabaseProcess->terminate();
  mDatabaseProcess->waitForFinished();
}

#include "akonadi.moc"
