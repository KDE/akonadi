/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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
#include <QObject>
#include <handler/move.h>

#include <storage/entity.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "entities.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Protocol::ItemChangeNotification::List)
Q_DECLARE_METATYPE(Akonadi::Protocol::ItemChangeNotification)

class MoveHandlerTest : public QObject
{
    Q_OBJECT

public:
    MoveHandlerTest()
    {
        try {
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }
    }

    ~MoveHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testMove_data()
    {
        const Collection srcCol = Collection::retrieveByName(QStringLiteral("Collection B"));
        const Collection destCol = Collection::retrieveByName(QStringLiteral("Collection A"));

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Protocol::ItemChangeNotification::List>("expectedNotifications");
        QTest::addColumn<QVariant>("newValue");

        Protocol::ItemChangeNotification notificationTemplate;
        notificationTemplate.setOperation(Protocol::ItemChangeNotification::Move);
        notificationTemplate.setResource("akonadi_fake_resource_0");
        notificationTemplate.setDestinationResource("akonadi_fake_resource_0");
        notificationTemplate.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        notificationTemplate.setParentCollection(srcCol.id());
        notificationTemplate.setParentDestCollection(destCol.id());

        {
            Protocol::MoveItemsCommand cmd(1, destCol.id());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::MoveItemsResponse());

            Protocol::ItemChangeNotification notification = notificationTemplate;
            notification.setItem({ Protocol::ItemChangeNotification::Item(1, QStringLiteral("A"), QString(), QStringLiteral("application/octet-stream")) });

            QTest::newRow("move item") << scenarios << Protocol::ItemChangeNotification::List{ notification }
                                       << QVariant::fromValue(destCol.id());
        }

        {
            Protocol::MoveItemsCommand cmd(QVector<qint64>{ 2, 3 }, destCol.id());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::MoveItemsResponse());

            Protocol::ItemChangeNotification notification = notificationTemplate;
            notification.setItem({ Protocol::ItemChangeNotification::Item(2, QStringLiteral("B"), QString(), QStringLiteral("application/octet-stream")),
                                   Protocol::ItemChangeNotification::Item(3, QStringLiteral("C"), QString(),
            QStringLiteral("application/octet-stream")) });

            QTest::newRow("mote items") << scenarios << Protocol::ItemChangeNotification::List{ notification }
                                        << QVariant::fromValue(destCol.id());
        }

    }

    void testMove()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::ItemChangeNotification::List, expectedNotifications);
        QFETCH(QVariant, newValue);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        auto notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (expectedNotifications.isEmpty()) {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<Protocol::ChangeNotification::List>().isEmpty());
            return;
        }
        QCOMPARE(notificationSpy->count(), 1);
        //Only one notify call
        QCOMPARE(notificationSpy->first().count(), 1);
        const Protocol::ChangeNotification::List receivedNotifications = notificationSpy->first().first().value<Protocol::ChangeNotification::List>();
        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());

        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(Protocol::ItemChangeNotification(receivedNotifications.at(i)), expectedNotifications.at(i));
            Protocol::ItemChangeNotification notification = receivedNotifications.at(i);
            QCOMPARE(notification.parentDestCollection(), newValue.toInt());

            Q_FOREACH (const auto &ntfItem, notification.items()) {
                const PimItem item = PimItem::retrieveById(ntfItem.id);
                QCOMPARE(item.collectionId(), newValue.toInt());
            }
        }
    }
};

AKTEST_FAKESERVER_MAIN(MoveHandlerTest)

#include "movehandlertest.moc"

