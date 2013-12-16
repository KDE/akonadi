/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include <akonadi/control.h>
#include <akonadi/servermanager.h>

#include <QtCore/QObject>

#include <qtest_akonadi.h>

#include "test_utils.h"

using namespace Akonadi;

class ServerManagerTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      QVERIFY( Control::start() );
      trackAkonadiProcess( false );
    }

    void cleanupTestCase()
    {
      trackAkonadiProcess( true );
    }

    void testStartStop()
    {
      QSignalSpy startSpy( ServerManager::self(), SIGNAL(started()) );
      QVERIFY( startSpy.isValid() );
      QSignalSpy stopSpy( ServerManager::self(), SIGNAL(stopped()) );
      QVERIFY( stopSpy.isValid() );

      QVERIFY( ServerManager::isRunning() );
      QVERIFY( Control::start() );

      QVERIFY( startSpy.isEmpty() );
      QVERIFY( stopSpy.isEmpty() );

      QVERIFY( ServerManager::stop() );
      QVERIFY( QTest::kWaitForSignal( ServerManager::self(), SIGNAL(stopped()), 5000 ) );
      QVERIFY( !ServerManager::isRunning() );
      QVERIFY( startSpy.isEmpty() );
      QCOMPARE( stopSpy.count(), 1 );

      QVERIFY( !ServerManager::stop() );
      QVERIFY( ServerManager::start() );
      QVERIFY( QTest::kWaitForSignal( ServerManager::self(), SIGNAL(started()), 5000 ) );
      QVERIFY( ServerManager::isRunning() );
      QCOMPARE( startSpy.count(), 1 );
      QCOMPARE( stopSpy.count(), 1 );
    }

    void testRestart()
    {
      QVERIFY( ServerManager::isRunning() );
      QSignalSpy startSpy( ServerManager::self(), SIGNAL(started()) );
      QVERIFY( startSpy.isValid() );

      QVERIFY( Control::restart() );

      QVERIFY( ServerManager::isRunning() );
      QCOMPARE( startSpy.count(), 1 );
    }

};

QTEST_AKONADIMAIN( ServerManagerTest, NoGUI )

#include "servermanagertest.moc"
