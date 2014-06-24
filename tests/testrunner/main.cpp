/*
 *
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

#include "config.h" //krazy:exclude=includes
#include "setup.h"
#include "shellscript.h"
#include "testrunner.h"

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <QDebug>
#include <KLocalizedString>

#include <signal.h>

static SetupTest *setup = 0;
static TestRunner *runner = 0;

void sigHandler( int signal )
{
  qDebug() << "Received signal" << signal;
  static int sigCounter = 0;
  if ( sigCounter == 0 ) { // try clean shutdown
    if ( runner ) {
      runner->terminate();
    }
    if ( setup ) {
      setup->shutdown();
    }
  } else if ( sigCounter == 1 ) { // force shutdown
    if ( setup ) {
      setup->shutdownHarder();
    }
  } else { // give up and just exit
    exit( 255 );
  }
  ++sigCounter;
}

int main( int argc, char **argv )
{
  KAboutData aboutdata( "akonadi-TES", 0,
                        ki18n( "Akonadi Testing Environment Setup" ),
                        "1.0",
                        ki18n( "Setup Environmnet" ),
                        KAboutData::License_GPL,
                        ki18n( "(c) 2008 Igor Trindade Oliveira" ) );

  KCmdLineArgs::init( argc, argv, &aboutdata );

  KCmdLineOptions options;
  options.add( "c" ).add( "config <configfile>", ki18n( "Configuration file to open" ), "config.xml" );
  options.add( "!+[test]", ki18n( "Test to run automatically, interactive if none specified" ) );
  options.add( "testenv <path>", ki18n( "Path where testenvironment would be saved" ) );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;
  app.disableSessionManagement();

  const KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->isSet( "config" ) ) {
    Config::instance( args->getOption( "config" ) );
  }

#ifdef Q_OS_UNIX
  signal( SIGINT, sigHandler );
  signal( SIGQUIT, sigHandler );
#endif

  setup = new SetupTest();

  if ( !setup->startAkonadiDaemon() ) {
      delete setup;
      qCritical("Failed to start Akonadi server!");
      return 1;
  }

  ShellScript sh;
  sh.setEnvironmentVariables( setup->environmentVariables() );

  if ( args->isSet( "testenv" ) ) {
    sh.makeShellScript( args->getOption( "testenv" ) );
  } else {
    sh.makeShellScript( setup->basePath() + "testenvironment.sh" );
  }

  if ( args->count() > 0 ) {
    QStringList testArgs;
    for ( int i = 0; i < args->count(); ++i ) {
      testArgs << args->arg( i );
    }
    runner = new TestRunner( testArgs );
    QObject::connect( setup, SIGNAL(setupDone()), runner, SLOT(run()) );
    QObject::connect( setup, SIGNAL(serverExited(int)), runner, SLOT(triggerTermination(int)) );
    QObject::connect( runner, SIGNAL(finished()), setup, SLOT(shutdown()) );
  }

  int exitCode = app.exec();
  if ( runner ) {
    exitCode += runner->exitCode();
    delete runner;
  }

  delete setup;
  setup = 0;

  return exitCode;
}

