/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include <qtest_akonadi.h>
#include <servermanager.h>

#include <QObject>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi;

/**
  This test verifies that the testrunner set everything up correctly, so all the
  other tests work as expected.
*/
class TestEnvironmentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testDBus()
    {
        QVERIFY(QDBusConnection::sessionBus().isConnected());
    }

    void testAkonadiServer()
    {
        QVERIFY(ServerManager::isRunning());
    }

    void testResources()
    {
        QVERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(
                    ServerManager::agentServiceName(ServerManager::Resource, QStringLiteral("akonadi_knut_resource_0"))));
        QVERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(
                    ServerManager::agentServiceName(ServerManager::Resource, QStringLiteral("akonadi_knut_resource_1"))));
        QVERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(
                    ServerManager::agentServiceName(ServerManager::Resource, QStringLiteral("akonadi_knut_resource_2"))));
    }
};

QTEST_AKONADIMAIN(TestEnvironmentTest)

#include "testenvironmenttest.moc"
