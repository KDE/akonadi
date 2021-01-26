/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "servermanager.h"
#include "control.h"
#include "qtest_akonadi.h"

#include <QObject>

using namespace Akonadi;

class ServerManagerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        QVERIFY(Control::start());
        trackAkonadiProcess(false);
    }

    void cleanupTestCase()
    {
        trackAkonadiProcess(true);
    }

    void testStartStop()
    {
        QSignalSpy startSpy(ServerManager::self(), SIGNAL(started()));
        QVERIFY(startSpy.isValid());
        QSignalSpy stopSpy(ServerManager::self(), SIGNAL(stopped()));
        QVERIFY(stopSpy.isValid());

        QVERIFY(ServerManager::isRunning());
        QVERIFY(Control::start());

        QVERIFY(startSpy.isEmpty());
        QVERIFY(stopSpy.isEmpty());

        {
            QSignalSpy spy(ServerManager::self(), SIGNAL(stopped()));
            QVERIFY(ServerManager::stop());
            QTRY_VERIFY(spy.count() >= 1);
        }
        QVERIFY(!ServerManager::isRunning());
        QVERIFY(startSpy.isEmpty());
        QCOMPARE(stopSpy.count(), 1);

        QVERIFY(!ServerManager::stop());
        {
            QSignalSpy spy(ServerManager::self(), SIGNAL(started()));
            QVERIFY(ServerManager::start());
            QTRY_VERIFY(spy.count() >= 1);
        }
        QVERIFY(ServerManager::isRunning());
        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(stopSpy.count(), 1);
    }

    void testRestart()
    {
        QVERIFY(ServerManager::isRunning());
        QSignalSpy startSpy(ServerManager::self(), SIGNAL(started()));
        QVERIFY(startSpy.isValid());

        QVERIFY(Control::restart());

        QVERIFY(ServerManager::isRunning());
        QCOMPARE(startSpy.count(), 1);
    }
};

QTEST_AKONADIMAIN(ServerManagerTest)

#include "servermanagertest.moc"
