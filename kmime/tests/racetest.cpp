/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "racetest.h"

#include <KDebug>
#include <KProcess>

#include <akonadi/control.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agenttype.h>
#include <akonadi/agentmanager.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/kmime/localfolders.h>

#define TIMEOUT_SECONDS 20
#define MAXCOUNT 10
// NOTE: REQUESTER_EXE is defined by cmake.

using namespace Akonadi;


void RaceTest::initTestCase()
{
  QVERIFY( Control::start() );
  QTest::qWait( 1000 ); // give the MDA time to start so that we can kill it in peace
}

void RaceTest::testMultipleProcesses_data()
{
  QTest::addColumn<int>( "count" ); // how many processes to create
  QTest::addColumn<int>( "delay" ); // number of ms to wait before starting next process

  QTest::newRow( "1-nodelay" ) << 1 << 0;
  QTest::newRow( "2-nodelay" ) << 2 << 0;
  QTest::newRow( "5-nodelay" ) << 5 << 0;
  QTest::newRow( "10-nodelay" ) << 10 << 0;
  QTest::newRow( "2-shortdelay" ) << 2 << 100;
  QTest::newRow( "5-shortdelay" ) << 5 << 100;
  QTest::newRow( "10-shortdelay" ) << 10 << 100;
  QTest::newRow( "2-longdelay" ) << 2 << 1000;
  QTest::newRow( "5-longdelay" ) << 5 << 1000;
  QTest::newRow( "5-verylongdelay" ) << 5 << 4000;
  Q_ASSERT( 10 <= MAXCOUNT );
}

void RaceTest::testMultipleProcesses()
{
  QFETCH( int, count );
  QFETCH( int, delay );

  killZombies();

  // Remove all maildir instances (at most 1 really) and MDAs (which use LocalFolders).
  // (This is to ensure that one of *our* instances is the main instance.)
  AgentType::List types;
  types.append( AgentManager::self()->type( QLatin1String( "akonadi_maildir_resource" ) ) );
  types.append( AgentManager::self()->type( QLatin1String( "akonadi_maildispatcher_agent" ) ) );
  AgentInstance::List instances = AgentManager::self()->instances();
  foreach( const AgentInstance &instance, instances ) {
    if( types.contains( instance.type() ) ) {
      kDebug() << "Removing instance of type" << instance.type().identifier();
      AgentManager::self()->removeInstance( instance );
      QTest::kWaitForSignal( AgentManager::self(), SIGNAL( instanceRemoved( const Akonadi::AgentInstance& ) ) );
    }
  }
  instances = AgentManager::self()->instances();
  foreach( const AgentInstance &instance, instances ) {
    QVERIFY( !types.contains( instance.type() ) );
  }

  QSignalSpy *errorSpy[ MAXCOUNT ];
  QSignalSpy *finishedSpy[ MAXCOUNT ];
  for( int i = 0; i < count; i++ ) {
    kDebug() << "Starting process" << i + 1 << "of" << count;
    KProcess *proc = new KProcess;
    procs.append( proc );
    proc->setProgram( REQUESTER_EXE );
    errorSpy[i] = new QSignalSpy( proc, SIGNAL( error( QProcess::ProcessError ) ) );
    finishedSpy[i] = new QSignalSpy( proc, SIGNAL( finished( int, QProcess::ExitStatus ) ) );
    proc->start();
    QTest::qWait( delay );
  }
  kDebug() << "Launched" << count << "processes.";

  int seconds = 0;
  int error, finished;
  while( true ) {
    seconds++;
    QTest::qWait( 1000 );

    error = 0;
    finished = 0;
    for( int i = 0; i < count; i++ ) {
      if( errorSpy[i]->count() > 0 )
        error++;
      if( finishedSpy[i]->count() > 0 )
        finished++;
    }
    kDebug() << seconds << "seconds elapsed." << error << "processes error'd,"
      << finished << "processes finished.";

    if( error + finished >= count )
      break;

#if 0
    if( seconds >= TIMEOUT_SECONDS ) {
      kDebug() << "Timeout, gdb master!";
      QTest::qWait( 1000*1000 );
    }
#endif
    QVERIFY2( seconds < TIMEOUT_SECONDS, "Timeout" );
  }

  QCOMPARE( error, 0 );
  QCOMPARE( finished, count );
  for( int i = 0; i < count; i++ ) {
    kDebug() << "Checking exit status of process" << i + 1 << "of" << count;
    QCOMPARE( finishedSpy[i]->count(), 1 );
    QList<QVariant> args = finishedSpy[i]->takeFirst();
    QCOMPARE( args[0].toInt(), 2 );
  }

  while( !procs.isEmpty() ) {
    KProcess *proc = procs.takeFirst();
    QCOMPARE( proc->exitStatus(), QProcess::NormalExit );
    QCOMPARE( proc->exitCode(), 2 );
    delete proc;
  }
  QVERIFY( procs.isEmpty() );
}

void RaceTest::killZombies()
{
  while( !procs.isEmpty() ) {
    // These processes probably hung, and will never recover, so we need to kill them.
    // (This happens if the last test failed.)
    kDebug() << "Killing zombies from the past.";
    KProcess *proc = procs.takeFirst();
    proc->kill();
    proc->deleteLater();
  }
}

QTEST_AKONADIMAIN( RaceTest, NoGUI )

#include "racetest.moc"
