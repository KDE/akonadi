/*
    Copyright (c) 2009 Thomas McGuire <mcguire@kde.org>

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
#include "resourceschedulertest.h"

#include "../resourcescheduler_p.h"

#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( ResourceSchedulerTest, NoGUI )

void ResourceSchedulerTest::testTaskComparision()
{
  ResourceScheduler::Task t1;
  t1.type = ResourceScheduler::ChangeReplay;
  ResourceScheduler::Task t2;
  t2.type = ResourceScheduler::ChangeReplay;
  QCOMPARE( t1, t2 );
  QList<ResourceScheduler::Task> taskList;
  taskList.append( t1 );
  QVERIFY( taskList.contains( t2 ) );

  ResourceScheduler::Task t3;
  t3.type = ResourceScheduler::DeleteResourceCollection;
  QVERIFY( !( t2 == t3 ) );
  QVERIFY( !taskList.contains( t3 ) );
}

void ResourceSchedulerTest::testChangeReplaySchedule()
{
  ResourceScheduler scheduler;
  scheduler.setOnline( true );
  qRegisterMetaType<Akonadi::Collection>("Akonadi::Collection");
  QSignalSpy changeReplaySpy( &scheduler, SIGNAL( executeChangeReplay() ) );
  QSignalSpy collectionTreeSyncSpy( &scheduler, SIGNAL( executeCollectionTreeSync() ) );
  QSignalSpy syncSpy( &scheduler, SIGNAL( executeCollectionSync( const Akonadi::Collection & ) ) );
  QVERIFY( changeReplaySpy.isValid() );
  QVERIFY( collectionTreeSyncSpy.isValid() );
  QVERIFY( syncSpy.isValid() );

  // Schedule a change replay, it should be executed first thing when we enter the
  // event loop, but not before
  QVERIFY( scheduler.isEmpty() );
  scheduler.scheduleChangeReplay();
  QVERIFY( !scheduler.isEmpty() );
  QVERIFY( changeReplaySpy.isEmpty() );
  QTest::qWait( 100 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  scheduler.taskDone();
  QTest::qWait( 100 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  
  // Schedule two change replays. The duplicate one should not be executed.
  changeReplaySpy.clear();
  scheduler.scheduleChangeReplay();
  scheduler.scheduleChangeReplay();
  QVERIFY( changeReplaySpy.isEmpty() );
  QTest::qWait( 100 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  scheduler.taskDone();
  QTest::qWait( 100 );
  QCOMPARE( changeReplaySpy.count(), 1 );

  //
  // Schedule various stuff.
  //
  Collection collection( 42 );
  changeReplaySpy.clear();
  scheduler.scheduleCollectionTreeSync();
  scheduler.scheduleChangeReplay();
  scheduler.scheduleSync(collection);
  scheduler.scheduleChangeReplay();

  QTest::qWait( 100 );
  QCOMPARE( collectionTreeSyncSpy.count(), 1 );
  QCOMPARE( changeReplaySpy.count(), 0 );
  QCOMPARE( syncSpy.count(), 0 );

  scheduler.taskDone();
  QTest::qWait( 100 );
  QCOMPARE( collectionTreeSyncSpy.count(), 1 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  QCOMPARE( syncSpy.count(), 0 );

  // Omit a taskDone() here, there shouldn't be a new signal
  QTest::qWait( 100 );
  QCOMPARE( collectionTreeSyncSpy.count(), 1 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  QCOMPARE( syncSpy.count(), 0 );

  scheduler.taskDone();
  QTest::qWait( 100 );
  QCOMPARE( collectionTreeSyncSpy.count(), 1 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  QCOMPARE( syncSpy.count(), 1 );

  // At this point, we're done, check that nothing else is emitted
  scheduler.taskDone();
  QVERIFY( scheduler.isEmpty() );
  QTest::qWait( 100 );
  QCOMPARE( collectionTreeSyncSpy.count(), 1 );
  QCOMPARE( changeReplaySpy.count(), 1 );
  QCOMPARE( syncSpy.count(), 1 );
}
