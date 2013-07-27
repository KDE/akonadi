/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 * Copyright (c) 2013  Volker Krause <vkrause@kde.org>
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
#include "config.h" //krazy:exclude=includes
#include "symbols.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/resourcesynchronizationjob.h>

#include <KConfig>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <KProcess>
#include <KStandardDirs>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

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
      if ( !unsetenv( key.toLatin1() ) ) {
        return false;
      }
    }
  }

  return true;
}

bool SetupTest::startAkonadiDaemon()
{
  Q_ASSERT(Akonadi::ServerManager::hasInstanceIdentifier());

  if ( !mAkonadiDaemonProcess ) {
    mAkonadiDaemonProcess = new KProcess( this );
    connect( mAkonadiDaemonProcess, SIGNAL(finished(int)),
            this, SLOT(slotAkonadiDaemonProcessFinished(int)) );
  }

  mAkonadiDaemonProcess->setProgram( QLatin1String( "akonadi_control" ), QStringList() << QLatin1String("--instance") << instanceId() );
  mAkonadiDaemonProcess->start();
  const bool started = mAkonadiDaemonProcess->waitForStarted( 5000 );
  kDebug() << "Started akonadi daemon with pid:" << mAkonadiDaemonProcess->pid();
  return started;
}

void SetupTest::stopAkonadiDaemon()
{
  if ( !mAkonadiDaemonProcess )
    return;
  disconnect( mAkonadiDaemonProcess, SIGNAL(finished(int)), this, 0 );
  mAkonadiDaemonProcess->terminate();
  const bool finished = mAkonadiDaemonProcess->waitForFinished( 5000 );
  if ( !finished ) {
    kDebug() << "Problem finishing process.";
  }
  mAkonadiDaemonProcess->close();
  mAkonadiDaemonProcess->deleteLater();
  mAkonadiDaemonProcess = 0;
}

void SetupTest::setupAgents()
{
  if ( mAgentsCreated )
    return;
  mAgentsCreated = true;
  Config *config = Config::instance();
  const QList<QPair<QString,bool> > agents = config->agents();
  typedef QPair<QString,bool> StringBoolPair;
  foreach ( const StringBoolPair &agent, agents ) {
    kDebug() << "Creating agent" << agent.first << "...";
    ++mSetupJobCount;
    Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( agent.first, this );
    job->setProperty( "sync", agent.second );
    connect( job, SIGNAL(result(KJob*)), SLOT(agentCreationResult(KJob*)) );
    job->start();
  }

  if ( isSetupDone() )
    emit setupDone();
}

void SetupTest::agentCreationResult(KJob* job)
{
  --mSetupJobCount;
  if ( job->error() ) {
    kError() << job->errorString(); // TODO exit
    return;
  }
  const bool needsSync = job->property( "sync" ).toBool();
  if (needsSync) {
    ++mSetupJobCount;
    Akonadi::ResourceSynchronizationJob *sync = new Akonadi::ResourceSynchronizationJob(
      qobject_cast<Akonadi::AgentInstanceCreateJob*>(job)->instance(), this );
    connect( job, SIGNAL(result(KJob*)), SLOT(synchronizationResult(KJob*)) );
    sync->start();
  }

  if ( isSetupDone() )
    emit setupDone();
}

void SetupTest::synchronizationResult(KJob* job)
{
  --mSetupJobCount;
  if ( job->error() ) {
    kError() << job->errorString(); // TODO exit
  }

  if ( isSetupDone() )
    emit setupDone();
}

void SetupTest::serverStateChanged(Akonadi::ServerManager::State state)
{
  if ( state == Akonadi::ServerManager::Running )
    setupAgents();
  else if ( mShuttingDown && state == Akonadi::ServerManager::NotRunning )
    shutdownHarder();
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
  kDebug() << "Creating test environment in" << basePath();

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

// TODO Qt5: use QDir::removeRecursively
void SetupTest::deleteDirectory( const QString &dirName )
{
  Q_ASSERT( dirName.startsWith( QDir::tempPath() ) || dirName.startsWith(QLatin1String("/tmp") ) ); // just to be sure we don't run amok anywhere
  QDir dir( dirName );
  dir.setFilter( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden );

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
  mAkonadiDaemonProcess( 0 ),
  mShuttingDown( false ),
  mAgentsCreated( false ),
  mTrackAkonadiProcess( true ),
  mSetupJobCount( 0 )
{
  setupInstanceId();
  clearEnvironment();
  cleanTempEnvironment();
  createTempEnvironment();

  setenv( "KDEHOME", qPrintable( QString( basePath() + "kdehome" ) ), 1 );
  setenv( "XDG_DATA_HOME", qPrintable( QString( basePath() + "data" ) ), 1 );
  setenv( "XDG_CONFIG_HOME", qPrintable( QString( basePath() + "config" ) ), 1 );
  QHashIterator<QString, QString> iter( Config::instance()->envVars() );
  while( iter.hasNext() ) {
    iter.next();
    kDebug() << "Setting environment variable" << iter.key() << "=" << iter.value();
    setenv( qPrintable( iter.key() ), qPrintable( iter.value() ), 1 );
  }

  // No kres-migrator please
  KConfig migratorConfig( basePath() + "kdehome/share/config/kres-migratorrc" );
  KConfigGroup migrationCfg( &migratorConfig, "Migration" );
  migrationCfg.writeEntry( "Enabled", false );

  Symbols *symbol = Symbols::instance();
  symbol->insertSymbol( "KDEHOME", basePath() + QLatin1String( "kdehome" ) );
  symbol->insertSymbol( "XDG_DATA_HOME", basePath() + QLatin1String( "data" ) );
  symbol->insertSymbol( "XDG_CONFIG_HOME", basePath() + QLatin1String( "config" ) );
  symbol->insertSymbol( "AKONADI_TESTRUNNER_PID", QString::number( QCoreApplication::instance()->applicationPid() ) );

  connect( Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
           SLOT(serverStateChanged(Akonadi::ServerManager::State)) );
}

SetupTest::~SetupTest()
{
  cleanTempEnvironment();
}

void SetupTest::shutdown()
{
  if ( mShuttingDown )
    return;
  mShuttingDown = true;

  switch ( Akonadi::ServerManager::self()->state() ) {
    case Akonadi::ServerManager::Running:
    case Akonadi::ServerManager::Starting:
    case Akonadi::ServerManager::Upgrading:
      kDebug() << "Shutting down Akonadi control...";
      Akonadi::ServerManager::self()->stop();
      // safety timeout
      QTimer::singleShot( 30 * 1000, this, SLOT(shutdownHarder()) );
      break;
    case Akonadi::ServerManager::NotRunning:
    case Akonadi::ServerManager::Broken:
      shutdownHarder();
    case Akonadi::ServerManager::Stopping:
      // safety timeout
      QTimer::singleShot( 30 * 1000, this, SLOT(shutdownHarder()) );
      break;
  }
}

void SetupTest::shutdownHarder()
{
  kDebug();
  mShuttingDown = false;
  stopAkonadiDaemon();
  QCoreApplication::instance()->quit();
}

void SetupTest::restartAkonadiServer()
{
  kDebug();
  disconnect( mAkonadiDaemonProcess, SIGNAL(finished(int)), this, 0 );
  Akonadi::ServerManager::self()->stop();
  const bool shutdownResult = mAkonadiDaemonProcess->waitForFinished();
  if ( !shutdownResult ) {
    kWarning() << "Akonadi control did not shut down in time, killing it.";
    mAkonadiDaemonProcess->kill();
  }
  // we don't use Control::start() since we want to be able to kill
  // it forcefully, if necessary, and know the pid
  startAkonadiDaemon();
  // from here on, the server exiting is an error again
  connect( mAkonadiDaemonProcess, SIGNAL(finished(int)),
           this, SLOT(slotAkonadiDaemonProcessFinished(int)));
}

QString SetupTest::basePath() const
{
  QString sysTempDirPath = QDir::tempPath();
#ifdef Q_OS_UNIX
  // QDir::tempPath() makes sure to use the fully sym-link exploded
  // absolute path to the temp dir. That is nice, but on OSX it makes
  // that path really long. MySQL chokes on this, for it's socket path,
  // so work around that
  sysTempDirPath = QLatin1String("/tmp");
#endif

  const QDir sysTempDir(sysTempDirPath);
  const QString tempDir = QString::fromLatin1( "akonadi_testrunner-%1" )
    .arg( QCoreApplication::instance()->applicationPid() );
  if ( !sysTempDir.exists( tempDir ) )
    sysTempDir.mkdir( tempDir );
  return sysTempDirPath + QDir::separator() + tempDir + QDir::separator();
}

void SetupTest::slotAkonadiDaemonProcessFinished(int exitCode)
{
  if ( mTrackAkonadiProcess || exitCode != EXIT_SUCCESS ) {
    kWarning() << "Akonadi server process was terminated externally!";
    emit serverExited( exitCode );
  }
  mAkonadiDaemonProcess = 0;
}

void SetupTest::trackAkonadiProcess(bool track)
{
  mTrackAkonadiProcess = track;
}

QString SetupTest::instanceId() const
{
  // this needs to be customizable for running multiple instance in parallel eventually
  return QLatin1String("testrunner");
}

void SetupTest::setupInstanceId()
{
  setenv("AKONADI_INSTANCE", instanceId().toLocal8Bit(), 1);
}

bool SetupTest::isSetupDone() const
{
  return mSetupJobCount == 0;
}
