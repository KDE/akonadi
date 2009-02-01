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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QSignalMapper>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <signal.h>
#include <unistd.h>

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
      if ( !unsetenv( key.toAscii() ) ) {
        return false;
      }
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
  mInternalBus->registerService( QLatin1String( "org.kde.Akonaditest" ) );
  mInternalBus->registerObject( QLatin1String( "/MainApplication" ),
                                KApplication::kApplication(),
                                QDBusConnection::ExportScriptableSlots |
                                QDBusConnection::ExportScriptableProperties |
                                QDBusConnection::ExportAdaptors );

  QDBusConnectionInterface *busInterface = mInternalBus->interface();
  connect( busInterface, SIGNAL( serviceOwnerChanged( QString, QString, QString ) ),
           this, SLOT( dbusNameOwnerChanged( QString, QString, QString ) ) );
}

void SetupTest::startAkonadiDaemon()
{
  mAkonadiDaemonProcess->start( QLatin1String( "akonadi_control" ), QStringList() );
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
  Config *config = Config::instance();
  QDBusInterface agentDBus( QLatin1String( "org.freedesktop.Akonadi.Control" ), QLatin1String( "/AgentManager" ),
                            QLatin1String( "org.freedesktop.Akonadi.AgentManager" ), *mInternalBus );

  const QList<QPair<QString,bool> > agents = config->agents();
  typedef QPair<QString,bool> StringBoolPair;
  kDebug() << agents;
  foreach ( const StringBoolPair &agent, agents ) {
    kDebug() << "inserting resource:" << agent.first;
    QDBusReply<QString> reply = agentDBus.call( QLatin1String( "createAgentInstance" ), agent.first );
    if ( reply.isValid() && !reply.value().isEmpty() ) {
      mPendingAgents << reply.value();
      if ( agent.second ) {
        mPendingSyncs << reply.value();
      }
    } else {
      kError() << "createAgentInstance call failed:" << reply.error();
    }
  }

  if ( mPendingAgents.isEmpty() && mPendingSyncs.isEmpty() )
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
      mPendingAgents.removeAll( identifier );
      if ( mPendingAgents.isEmpty() && mPendingSyncs.isEmpty() )
        emit setupDone();
    }
  }

  if ( name.startsWith( QLatin1String( "org.freedesktop.Akonadi.Resource." ) ) && oldOwner.isEmpty() ) {
    const QString identifier = name.mid( 33 );
    QDBusInterface *iface = new QDBusInterface( name, "/", "org.freedesktop.Akonadi.Resource", *mInternalBus, this );
    mSyncMapper->setMapping( iface, identifier );
    connect( iface, SIGNAL(synchronized()), mSyncMapper, SLOT(map()) );
    if ( mPendingSyncs.contains( identifier ) )
      iface->call( "synchronize" );
    kDebug() << identifier << mPendingSyncs;
  }
}

void SetupTest::resourceSynchronized(const QString& agentId)
{
  kDebug() << agentId;
  if ( mPendingSyncs.contains( agentId ) ) {
    mPendingSyncs.removeAll( agentId );
    if ( mPendingAgents.isEmpty() && mPendingSyncs.isEmpty() )
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
  const QDir tmpDir = QDir::temp();
  const QString testRunnerDir = QLatin1String( "akonadi_testrunner" );
  const QString testRunnerKdeHomeDir = testRunnerDir + QLatin1String( "/kdehome" );
  const QString testRunnerDataDir = testRunnerDir + QLatin1String( "/data" );
  const QString testRunnerConfigDir = testRunnerDir + QLatin1String( "/config" );

  if ( !tmpDir.exists( testRunnerDir ) ) {
    tmpDir.mkdir( testRunnerDir );
    tmpDir.mkdir( testRunnerKdeHomeDir );
    tmpDir.mkdir( testRunnerConfigDir );
    tmpDir.mkdir( testRunnerDataDir );
  }

  const Config *config = Config::instance();
  copyDirectory( config->kdeHome(), QDir::tempPath() + QDir::separator() + testRunnerKdeHomeDir );
  copyDirectory( config->xdgConfigHome(), QDir::tempPath() + QDir::separator() + testRunnerConfigDir  );
  copyDirectory( config->xdgDataHome(), QDir::tempPath() + QDir::separator() + testRunnerDataDir );
}

void SetupTest::deleteDirectory( const QString &dirName )
{
  Q_ASSERT( dirName.startsWith( QDir::tempPath() ) ); // just to be sure we don't run amok anywhere
  QDir dir( dirName );
  dir.setFilter( QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot );

  const QFileInfoList list = dir.entryInfoList();
  for ( int i = 0; i < list.size(); ++i ) {
    if ( list.at( i ).isDir() ) {
      deleteDirectory( list.at( i ).absoluteFilePath() );
      const QDir tmpDir( list.at( i ).absoluteDir() );
      tmpDir.rmdir( list.at( i ).fileName() );
    } else {
      QFile::remove( list.at( i ).absoluteFilePath() );
    }
  }
}

void SetupTest::cleanTempEnvironment()
{
  deleteDirectory( QDir::tempPath() + QDir::separator() + QLatin1String( "akonadi_testrunner" ) );
}

SetupTest::SetupTest() :
  mSyncMapper( new QSignalMapper( this ) )
{

  clearEnvironment();
  cleanTempEnvironment();
  createTempEnvironment();

  setenv( "KDEHOME", "/tmp/akonadi_testrunner/kdehome", 1 );
  setenv( "XDG_DATA_HOME", "/tmp/akonadi_testrunner/data", 1 );
  setenv( "XDG_CONFIG_HOME", "/tmp/akonadi_testrunner/config", 1 );

  Symbols *symbol = Symbols::instance();
  symbol->insertSymbol( "KDEHOME", QLatin1String( "/tmp/akonadi_testrunner/kdehome" ) );
  symbol->insertSymbol( "XDG_DATA_HOME", QLatin1String( "/tmp/akonadi_testrunner/data" ) );
  symbol->insertSymbol( "XDG_CONFIG_HOME", QLatin1String( "/tmp/akonadi_testrunner/config" ) );
  symbol->insertSymbol( "AKONADI_TESTRUNNER_PID", QString::number( QCoreApplication::instance()->applicationPid() ) );

  mDBusDaemonPid = startDBusDaemon();

#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 2))
  const QString dbusAddress = symbol->symbols()[ QLatin1String( "DBUS_SESSION_BUS_ADDRESS" ) ];
  registerWithInternalDBus( dbusAddress );
#endif

  mAkonadiDaemonProcess = new QProcess();

  connect( mSyncMapper, SIGNAL(mapped(QString)), SLOT(resourceSynchronized(QString)) );
}

SetupTest::~SetupTest()
{
  cleanTempEnvironment();
  delete mAkonadiDaemonProcess;
}

void SetupTest::shutdown()
{
  kDebug();
  mShuttingDown = true;
  QDBusInterface controlIface( QLatin1String( "org.freedesktop.Akonadi.Control" ), QLatin1String( "/ControlManager" ),
                               QLatin1String( "org.freedesktop.Akonadi.ControlManager" ), *mInternalBus );
  QDBusReply<void> reply = controlIface.call( "shutdown" );
  if ( !reply.isValid() ) {
    kWarning() << "Failed to shutdown Akonadi server: " << reply.error().message();
    shutdownHarder();
  } else {
    // safety timeout
    QTimer::singleShot( 30 * 1000, this, SLOT( shutdownHarder() ) );
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
