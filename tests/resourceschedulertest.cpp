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

qint64 ResourceScheduler::Task::latestSerial = 0;

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