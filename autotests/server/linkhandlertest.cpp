/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include <handler/link.h>

#include "fakeakonadiserver.h"
#include <shared/aktest.h>
#include "entities.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

#include <QTest>
#include <QSignalSpy>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Protocol::ItemChangeNotification)

class LinkHandlerTest : public QObject
{
    Q_OBJECT

public:
    LinkHandlerTest()
    {
        try {
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }
    }

    ~LinkHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

    Protocol::LinkItemsResponse createError(const QString &error)
    {
        Protocol::LinkItemsResponse resp;
        resp.setError(1, error);
        return resp;
    }

private Q_SLOTS:
    void testLink_data()
    {
        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Akonadi::Protocol::ItemChangeNotification>("notification");
        QTest::addColumn<bool>("expectFail");

        TestScenario::List scenarios;
        Protocol::ItemChangeNotification notification;

        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Link, ImapInterval(1, 3), 3))
                  << TestScenario::create(5, TestScenario::ServerCmd, createError(QLatin1String("Can't link items to non-virtual collections")));
        QTest::newRow("non-virtual collection") << scenarios << Protocol::ItemChangeNotification() << true;

        notification.setOperation(Protocol::ItemChangeNotification::Link);
        notification.addItem(1, QLatin1String("A"), QString(), QLatin1String("application/octet-stream"));
        notification.addItem(2, QLatin1String("B"), QString(), QLatin1String("application/octet-stream"));
        notification.addItem(3, QLatin1String("C"), QString(), QLatin1String("application/octet-stream"));
        notification.setParentCollection(6);
        notification.setResource("akonadi_fake_resource_with_virtual_collections_0");
        notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());

        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Link, ImapInterval(1, 3), 6))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::LinkItemsResponse());
        QTest::newRow("normal") << scenarios << notification << false;

        notification.clearItems();
        notification.addItem(4, QLatin1String("D"), QString(), QLatin1String("application/octet-stream"));
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Link, QVector<qint64>{ 4, 123456 }, 6))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::LinkItemsResponse());
        QTest::newRow("existent and non-existent item") << scenarios << notification << false;

        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Link, 4, 6))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::LinkItemsResponse());
        QTest::newRow("non-existent item only") << scenarios << Protocol::ItemChangeNotification() << false;

        //FIXME: All RID related operations are currently broken because we reset the collection context before every command,
        //and LINK still relies on SELECT to set the collection context.

        // scenario.clear();
        // scenario << FakeAkonadiServer::defaultScenario()
        //          << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
        //          << "C: 3 UID LINK 6 RID (\"F\" \"G\")\n"
        //          << "S: 3 OK LINK complete";
        // notification.clearEntities();
        // notification.clearEntities();
        // notification.addEntity(6, QLatin1String("F"), QString(), QLatin1String("application/octet-stream"));
        // notification.addEntity(7, QLatin1String("G"), QString(), QLatin1String("application/octet-stream"));
        // QTest::newRow("RID items") << scenario << notification << false;

        // scenario.clear();
        // scenario << FakeAkonadiServer::defaultScenario()
        //          << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
        //          << "C: 4 HRID LINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) UID 5"
        //          << "S: 4 OK LINK complete";
        // notification.setParentCollection(7);
        // notification.clearEntities();
        // notification.addEntity(5, QLatin1String("E"), QString(), QLatin1String("application/octet-stream"));
        // QTest::newRow("HRID collection") << scenario << notification << false;

        // scenario.clear();
        // scenario << FakeAkonadiServer::defaultScenario()
        //          << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
        //          << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
        //          << "C: 4 HRID LINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) RID \"H\""
        //          << "S: 4 OK LINK complete";
        // notification.clearEntities();
        // notification.addEntity(8, QLatin1String("H"), QString(), QLatin1String("application/octet-stream"));
        // QTest::newRow("HRID collection, RID items") << scenario << notification << false;
    }

    void testLink()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::ItemChangeNotification, notification);
        QFETCH(bool, expectFail);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        auto notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (notification.isValid()) {
            QCOMPARE(notificationSpy->count(), 1);
            const Protocol::ChangeNotification::List notifications = notificationSpy->takeFirst().first().value<Protocol::ChangeNotification::List>();
            QCOMPARE(notifications.count(), 1);
            QCOMPARE(Protocol::ItemChangeNotification(notifications.first()), notification);
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<Protocol::ChangeNotification::List>().isEmpty());
        }

        Q_FOREACH (const auto &entity, notification.items()) {
            if (expectFail) {
                QVERIFY(!Collection::relatesToPimItem(notification.parentCollection(), entity.id));
            } else {
                QVERIFY(Collection::relatesToPimItem(notification.parentCollection(), entity.id));
            }
        }
    }

    void testUnlink_data()
    {
        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Akonadi::Protocol::ItemChangeNotification>("notification");
        QTest::addColumn<bool>("expectFail");

        TestScenario::List scenarios;
        Protocol::ItemChangeNotification notification;

        scenarios << FakeAkonadiServer::loginScenario()
                 << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Unlink, ImapInterval(1, 3), 3))
                 << TestScenario::create(5, TestScenario::ServerCmd, createError(QLatin1String("Can't link items to non-virtual collections")));
        QTest::newRow("non-virtual collection") << scenarios << Protocol::ItemChangeNotification() << true;

        notification.setOperation(Protocol::ItemChangeNotification::Unlink);
        notification.addItem(1, QLatin1String("A"), QString(), QLatin1String("application/octet-stream"));
        notification.addItem(2, QLatin1String("B"), QString(), QLatin1String("application/octet-stream"));
        notification.addItem(3, QLatin1String("C"), QString(), QLatin1String("application/octet-stream"));
        notification.setParentCollection(6);
        notification.setResource("akonadi_fake_resource_with_virtual_collections_0");
        notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Unlink, ImapInterval(1, 3), 6))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::LinkItemsResponse());
        QTest::newRow("normal") << scenarios << notification << false;

        notification.clearItems();
        notification.addItem(4, QLatin1String("D"), QString(), QLatin1String("application/octet-stream"));
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Unlink, QVector<qint64>{ 4, 2048 }, 6))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::LinkItemsResponse());
        QTest::newRow("existent and non-existent item") << scenarios << notification << false;

        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::LinkItemsCommand(Protocol::LinkItemsCommand::Unlink, 4096, 6))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::LinkItemsResponse());
        QTest::newRow("non-existent item only") << scenarios << Protocol::ItemChangeNotification() << false;

        //FIXME: All RID related operations are currently broken because we reset the collection context before every command,
        //and LINK still relies on SELECT to set the collection context.

        // scenario.clear();
        // scenario << FakeAkonadiServer::defaultScenario()
        //          << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
        //          << "C: 4 UID UNLINK 6 RID (\"F\" \"G\")"
        //          << "S: 4 OK LINK complete";
        // notification.clearEntities();
        // notification.clearEntities();
        // notification.addEntity(6, QLatin1String("F"), QString(), QLatin1String("application/octet-stream"));
        // notification.addEntity(7, QLatin1String("G"), QString(), QLatin1String("application/octet-stream"));
        // QTest::newRow("RID items") << scenario << notification << false;

        // scenario.clear();
        // scenario << FakeAkonadiServer::defaultScenario()
        //          << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
        //          << "C: 4 HRID UNLINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) UID 5"
        //          << "S: 4 OK LINK complete";
        // notification.setParentCollection(7);
        // notification.clearEntities();
        // notification.addEntity(5, QLatin1String("E"), QString(), QLatin1String("application/octet-stream"));
        // QTest::newRow("HRID collection") << scenario << notification << false;

        // scenario.clear();
        // scenario << FakeAkonadiServer::defaultScenario()
        //          << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
        //          << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
        //          << "C: 4 HRID UNLINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) RID \"H\""
        //          << "S: 4 OK LINK complete";
        // notification.clearEntities();
        // notification.addEntity(8, QLatin1String("H"), QString(), QLatin1String("application/octet-stream"));
        // QTest::newRow("HRID collection, RID items") << scenario << notification << false;
    }

    void testUnlink()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::ItemChangeNotification, notification);
        QFETCH(bool, expectFail);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        auto notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (notification.isValid()) {
            QCOMPARE(notificationSpy->count(), 1);
            const Protocol::ChangeNotification::List notifications = notificationSpy->takeFirst().first().value<Protocol::ChangeNotification::List>();
            QCOMPARE(notifications.count(), 1);
            QCOMPARE(Protocol::ItemChangeNotification(notifications.first()), notification);
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<Protocol::ChangeNotification::List>().isEmpty());
        }

        Q_FOREACH (const auto &entity, notification.items()) {
            if (expectFail) {
                QVERIFY(Collection::relatesToPimItem(notification.parentCollection(), entity.id));
            } else {
                QVERIFY(!Collection::relatesToPimItem(notification.parentCollection(), entity.id));
            }
        }
    }

};

AKTEST_FAKESERVER_MAIN(LinkHandlerTest)

#include "linkhandlertest.moc"
