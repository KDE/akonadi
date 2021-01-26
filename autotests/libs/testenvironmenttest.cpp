/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qtest_akonadi.h"
#include "servermanager.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QObject>

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
