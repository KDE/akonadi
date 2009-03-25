/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "setup.h"
#include "config.h"
#include "symbols.h"

#include <kapplication.h>
#include <kdebug.h>
#include <KProcess>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QSignalMapper>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <signal.h>
#include <unistd.h>

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

QMap<QString, QString> SetupTest::environment() const
{
  QMap<QString, QString> env;

  foreach ( const QString& val, QProcess::systemEnvironment() ) {
    const int p = val.indexOf( QLatin1Char( '=' ) );
    if ( p > 0 ) {
      env[ val.left( p ).toUpper() ] = val.mid( p + 1 );
    }
  }

  return env;
}


bool SetupTest::clearEnvironment()
{
  const QStringList keys = environment().keys();

  foreach ( const QString& key, keys ) {
    if ( key != QLatin1String( "HOME" ) ) {
// work around a bug in the Mac OS X 10.4.0 SDK
#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && (MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4)
      /* on OSX 10.4, unsetenv is a void, not a boolean */
      unsetenv( key.toAscii() );
#else
      if ( !unsetenv( key.toAscii() ) ) {
        return false;
      }
#endif
    }
  }

  return true;
}

int SetupTest::addDBusToEnvironment( QIODevice& io )
{
  QByteArray data = io.readLine();
  int pid = -1;
  Symbols *symbol = Symbols::instance();

  while ( data.size() ) {
    if ( data[ data.size() - 1 ] == '\n' ) {
      data.resize( data.size() - 1 );
    }

    QString val( data );
    const int p = val.indexOf( '=' );
    if ( p > 0 ) {
      const QString name = val.left( p ).toUpper();
      val = val.mid( p + 1 );
      if ( name == QLatin1String( "DBUS_SESSION_BUS_PID" ) ) {
        pid = val.toInt();
        setenv( name.toAscii(), val.toAscii(), 1 );
        symbol->insertSymbol( name, val );
      } else if ( name == QLatin1String( "DBUS_SESSION_BUS_ADDRESS" ) ) {
        setenv( name.toAscii(), val.toAscii(), 1 );
        symbol->insertSymbol( name, val );
      }
    }
    data = io.readLine();
  }

  return pid;
}

int SetupTest::startDBusDaemon()
{
  QProcess dbusprocess;
  QStringList dbusargs;

  dbusprocess.start( "/usr/bin/dbus-launch", dbusargs );
  bool ok = dbusprocess.waitForStarted() && dbusprocess.waitForFinished();
  if ( !ok ) {
    kDebug() << "error starting dbus-launch";
    dbusprocess.kill();
    return -1;
  }

  int dbuspid = addDBusToEnvironment( dbusprocess );
  return dbuspid;
}

void SetupTest::stopDBusDaemon( int dbuspid )
{
  kDebug() << dbuspid;
  if ( dbuspid )
    kill( dbuspid, 15 );
  sleep( 1 );

  if ( dbuspid )
    kill( dbuspid, 9 );
}

void SetupTest::registerWithInternalDBus( const QString &address )
{
  mInternalBus = new QDBusConnection( QDBusConnection::connectToBus( address, QLatin1String( "InternalBus" ) ) );
  mInternalBus->registerService( QLatin1String( "org.kde.Akonadi.Testrunner" ) );
  mInternalBus->registerObject( QLatin1String( "/MainApplication" ),
                                KApplication::kApplication(),
                                QDBusConnection::ExportScriptableSlots |
                                QDBusConnection::ExportScriptableProperties |
                                QDBusConnection::ExportAdaptors );
  mInternalBus->registerObject( QLatin1String( "/" ), this, QDBusConnection::ExportScriptableSlots );

  QDBusConnectionInterface *busInterface = mInternalBus->interface();
  connect( busInterface, SIGNAL( serviceOwnerChanged( QString, QString, QString ) ),
           this, SLOT( dbusNameOwnerChanged( QString, QString, QString ) ) );
}

void SetupTest::startAkonadiDaemon()
{
  mAkonadiDaemonProcess->setProgram( QLatin1String( "akonadi_control" ) );
  mAkonadiDaemonProcess->start();
  mAkonadiDaemonProcess->waitForStarted( 5000 );
  kDebug() << mAkonadiDaemonProcess->pid();
}

void SetupTest::stopAkonadiDaemon()
{
  mAkonadiDaemonProcess->terminate();
  if ( !mAkonadiDaemonProcess->waitForFinished( 5000 ) ) {
    kDebug() << "Problem finishing process.";
  }
  mAkonadiDaemonProcess->close();
}

void SetupTest::setupAgents()
{
  if ( mAgentsCreated )
    return;
  mAgentsCreated = true;
  Config *config = Config::instance();
  QDBusInterface agentDBus( QLatin1String( "org.freedesktop.Akonadi.Control" ), QLatin1String( "/AgentManager" ),
                            QLatin1String( "org.freedesktop.Akonadi.AgentManager" ), *mInternalBus );

  const QList<QPair<QString,bool> > agents = config->agents();
  typedef QPair<QString,bool> StringBoolPair;
  foreach ( const StringBoolPair &agent, agents ) {
    kDebug() << "Creating agent" << agent.first << "...";
    QDBusReply<QString> reply = agentDBus.call( QLatin1String( "createAgentInstance" ), agent.first );
    if ( reply.isValid() && !reply.value().isEmpty() ) {
      mPendingAgents << reply.value();
      mPendingResources << reply.value();
      if ( agent.second ) {
        mPendingSyncs << reply.value();
      }
    } else {
      kError() << "createAgentInstance call failed:" << reply.error();
    }
  }

  if ( mPendingAgents.isEmpty() )
    emit setupDone();
}

void SetupTest::dbusNameOwnerChanged( const QString &name, const QString &oldOwner, const QString &newOwner )
{
  kDebug() << name << oldOwner << newOwner;

  // TODO: find out why it does not work properly when reacting on
  // org.freedesktop.Akonadi.Control
  if ( name == QLatin1String( "org.freedesktop.Akonadi" ) ) {
    if ( oldOwner.isEmpty() ) // startup
      setupAgents();
    else if ( mShuttingDown ) // our own shutdown
      shutdownHarder();
    return;
  }

  if ( name.startsWith( QLatin1String( "org.freedesktop.Akonadi.Agent." ) ) && oldOwner.isEmpty() ) {
    const QString identifier = name.mid( 30 );
    if ( mPendingAgents.contains( identifier ) ) {
      kDebug() << "Agent" << identifier << "started.";
      mPendingAgents.removeAll( identifier );
      if ( mPendingAgents.isEmpty() && mPendingResources.isEmpty() )
        QTimer::singleShot( 5000, this, SLOT(synchronizeResources()) );
    }
  }

  if ( name.startsWith( QLatin1String( "org.freedesktop.Akonadi.Resource." ) ) && oldOwner.isEmpty() ) {
    const QString identifier = name.mid( 33 );
    if ( mPendingResources.contains( identifier ) ) {
      kDebug() << "Resource" << identifier << "registered.";
      mPendingResources.removeAll( identifier );
      if ( mPendingAgents.isEmpty() && mPendingResources.isEmpty() )
        QTimer::singleShot( 5000, this, SLOT(synchronizeResources()) );
    }
  }
}

void SetupTest::synchronizeResources()
{
  foreach ( const QString id, mPendingSyncs ) {
    QDBusInterface *iface = new QDBusInterface( QString::fromLatin1( "org.freedesktop.Akonadi.Resource.%1").arg( id ),
      "/", "org.freedesktop.Akonadi.Resource", *mInternalBus, this );
    mSyncMapper->setMapping( iface, id );
    connect( iface, SIGNAL(synchronized()), mSyncMapper, SLOT(map()) );
    if ( mPendingSyncs.contains( id ) ) {
      kDebug() << "Synchronizing resource" << id << "...";
      QDBusReply<void> reply = iface->call( "synchronize" );
      if ( !reply.isValid() )
        kError() << "Syncing resource" << id << "failed: " << reply.error();
    }
  }
}

void SetupTest::resourceSynchronized(const QString& agentId)
{
  if ( mPendingSyncs.contains( agentId ) ) {
    kDebug() << "Agent" << agentId << "synchronized.";
    mPendingSyncs.removeAll( agentId );
    if ( mPendingSyncs.isEmpty() )
      emit setupDone();
  }
}

void SetupTest::copyDirectory( const QString &src, const QString &dst )
{
  QDir srcDir( src );
  srcDir.setFilter( QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot );

  const QFileInfoList list = srcDir.entryInfoList();
  for ( int i = 0; i < list.size(); ++i ) {
    if ( list.at( i ).isDir() ) {
      const QDir tmpDir( dst );
      tmpDir.mkdir( list.at( i ).fileName() );
      copyDirectory( list.at( i ).absoluteFilePath(), dst + QDir::separator() + list.at( i ).fileName() );
    } else {
      QFile::copy( srcDir.absolutePath() + QDir::separator() + list.at( i ).fileName(), dst + QDir::separator() + list.at( i ).fileName() );
    }
  }
}

void SetupTest::createTempEnvironment()
{
  const QDir tmpDir( basePath() );
  const QString testRunnerKdeHomeDir = QLatin1String( "kdehome" );
  const QString testRunnerDataDir = QLatin1String( "data" );
  const QString testRunnerConfigDir = QLatin1String( "config" );

  tmpDir.mkdir( testRunnerKdeHomeDir );
  tmpDir.mkdir( testRunnerConfigDir );
  tmpDir.mkdir( testRunnerDataDir );

  const Config *config = Config::instance();
  copyDirectory( config->kdeHome(), basePath() + testRunnerKdeHomeDir );
  copyDirectory( config->xdgConfigHome(), basePath() + testRunnerConfigDir  );
  copyDirectory( config->xdgDataHome(), basePath() + testRunnerDataDir );
}

void SetupTest::deleteDirectory( const QString &dirName )
{
  Q_ASSERT( dirName.startsWith( QDir::tempPath() ) ); // just to be sure we don't run amok anywhere
  QDir dir( dirName );
  dir.setFilter( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot );

  const QFileInfoList list = dir.entryInfoList();
  for ( int i = 0; i < list.size(); ++i ) {
    if ( list.at( i ).isDir() && !list.at( i ).isSymLink() ) {
      deleteDirectory( list.at( i ).absoluteFilePath() );
      const QDir tmpDir( list.at( i ).absoluteDir() );
      tmpDir.rmdir( list.at( i ).fileName() );
    } else {
      QFile::remove( list.at( i ).absoluteFilePath() );
    }
  }
  dir.cdUp();
  dir.rmdir( dirName );
}

void SetupTest::cleanTempEnvironment()
{
  deleteDirectory( basePath() );
}

SetupTest::SetupTest() :
  mShuttingDown( false ),
  mSyncMapper( new QSignalMapper( this ) ),
  mAgentsCreated( false )
{

  clearEnvironment();
  cleanTempEnvironment();
  createTempEnvironment();

  setenv( "KDEHOME", qPrintable( basePath() + "kdehome" ), 1 );
  setenv( "XDG_DATA_HOME", qPrintable( basePath() + "data" ), 1 );
  setenv( "XDG_CONFIG_HOME", qPrintable( basePath() + "config" ), 1 );

  Symbols *symbol = Symbols::instance();
  symbol->insertSymbol( "KDEHOME", basePath() + QLatin1String( "kdehome" ) );
  symbol->insertSymbol( "XDG_DATA_HOME", basePath() + QLatin1String( "data" ) );
  symbol->insertSymbol( "XDG_CONFIG_HOME", basePath() + QLatin1String( "config" ) );
  symbol->insertSymbol( "AKONADI_TESTRUNNER_PID", QString::number( QCoreApplication::instance()->applicationPid() ) );

  mDBusDaemonPid = startDBusDaemon();

#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 2))
  const QString dbusAddress = symbol->symbols()[ QLatin1String( "DBUS_SESSION_BUS_ADDRESS" ) ];
  registerWithInternalDBus( dbusAddress );
#endif

  mAkonadiDaemonProcess = new KProcess( this );

  connect( mSyncMapper, SIGNAL(mapped(QString)), SLOT(resourceSynchronized(QString)) );
}

SetupTest::~SetupTest()
{
  cleanTempEnvironment();
  delete mAkonadiDaemonProcess;
}

void SetupTest::shutdown()
{
  if ( mShuttingDown )
    return;
  mShuttingDown = true;
  // check first if the Akonadi server is still running, otherwise D-Bus auto-launch will actually start it here
  if ( mInternalBus->interface()->isServiceRegistered( "org.freedesktop.Akonadi.Control" ) ) {
    kDebug() << "Shutting down Akonadi control...";
    QDBusInterface controlIface( QLatin1String( "org.freedesktop.Akonadi.Control" ), QLatin1String( "/ControlManager" ),
                                QLatin1String( "org.freedesktop.Akonadi.ControlManager" ), *mInternalBus );
    QDBusReply<void> reply = controlIface.call( "shutdown" );
    if ( !reply.isValid() ) {
      kWarning() << "Failed to shutdown Akonadi control: " << reply.error().message();
      shutdownHarder();
    } else {
      // safety timeout
      QTimer::singleShot( 30 * 1000, this, SLOT( shutdownHarder() ) );
    }
  } else {
    shutdownHarder();
  }
}

void SetupTest::shutdownHarder()
{
  kDebug();
  mShuttingDown = false;
  stopAkonadiDaemon();
  stopDBusDaemon( mDBusDaemonPid );
  QCoreApplication::instance()->quit();
}

QString SetupTest::basePath() const
{
  const QString tempDir = QString::fromLatin1( "akonadi_testrunner-%1" )
    .arg( QCoreApplication::instance()->applicationPid() );
  if ( !QDir::temp().exists( tempDir ) )
    QDir::temp().mkdir( tempDir );
  return QDir::tempPath() + QDir::separator() + tempDir + QDir::separator();
}
