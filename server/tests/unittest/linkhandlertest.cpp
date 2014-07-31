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
#include <imapstreamparser.h>
#include <response.h>

#include <libs/notificationmessagev3_p.h>
#include <libs/notificationmessagev2_p.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "akdebug.h"
#include "entities.h"

#include <QtTest/QTest>
#include <QSignalSpy>

using namespace Akonadi;
using namespace Akonadi::Server;

class LinkHandlerTest : public QObject
{
    Q_OBJECT

public:
    LinkHandlerTest()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

        try {
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
    }

    ~LinkHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testLink_data()
    {
        QTest::addColumn<QList<QByteArray> >("scenario");
        QTest::addColumn<Akonadi::NotificationMessageV3>("notification");
        QTest::addColumn<bool>("expectFail");

        QList<QByteArray> scenario;
        NotificationMessageV3 notification;

        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID LINK 3 1:3"
                 << "S: 2 NO Can't link items to non-virtual collections";
        QTest::newRow("non-virtual collection") << scenario << NotificationMessageV3() << true;

        notification.setType(NotificationMessageV2::Items);
        notification.setOperation(NotificationMessageV2::Link);
        notification.addEntity(1, QLatin1String("A"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(2, QLatin1String("B"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(3, QLatin1String("C"), QString(), QLatin1String("application/octet-stream"));
        notification.setParentCollection(6);
        notification.setResource("akonadi_fake_resource_with_virtual_collections_0");
        notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID LINK 6 1:3"
                 << "S: 2 OK LINK complete";
        QTest::newRow("normal") << scenario << notification << false;

        notification.clearEntities();
        notification.addEntity(4, QLatin1String("D"), QString(), QLatin1String("application/octet-stream"));
        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID LINK 6 4 123456"
                 << "S: 2 OK LINK complete";
        QTest::newRow("existent and non-existent item") << scenario << notification << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID LINK 6 123456"
                 << "S: 2 OK LINK complete";
        QTest::newRow("non-existent item only") << scenario << NotificationMessageV3() << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
                 << "C: 3 UID LINK 6 RID (\"F\" \"G\")\n"
                 << "S: 3 OK LINK complete";
        notification.clearEntities();
        notification.clearEntities();
        notification.addEntity(6, QLatin1String("F"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(7, QLatin1String("G"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("RID items") << scenario << notification << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
                 << "C: 4 HRID LINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) UID 5"
                 << "S: 4 OK LINK complete";
        notification.setParentCollection(7);
        notification.clearEntities();
        notification.addEntity(5, QLatin1String("E"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection") << scenario << notification << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
                 << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
                 << "C: 4 HRID LINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) RID \"H\""
                 << "S: 4 OK LINK complete";
        notification.clearEntities();
        notification.addEntity(8, QLatin1String("H"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection, RID items") << scenario << notification << false;
    }

    void testLink()
    {
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(NotificationMessageV3, notification);
        QFETCH(bool, expectFail);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();

        QSignalSpy *notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (notification.isValid()) {
            QCOMPARE(notificationSpy->count(), 1);
            const NotificationMessageV3::List notifications = notificationSpy->takeFirst().first().value<NotificationMessageV3::List>();
            QCOMPARE(notifications.count(), 1);
            QCOMPARE(notifications.first(), notification);
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<NotificationMessageV3::List>().isEmpty());
        }

        Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
            if (expectFail) {
                QVERIFY(!Collection::relatesToPimItem(notification.parentCollection(), entity.id));
            } else {
                QVERIFY(Collection::relatesToPimItem(notification.parentCollection(), entity.id));
            }
        }
    }

    void testUnlink_data()
    {
        QTest::addColumn<QList<QByteArray> >("scenario");
        QTest::addColumn<Akonadi::NotificationMessageV3>("notification");
        QTest::addColumn<bool>("expectFail");

        QList<QByteArray> scenario;
        NotificationMessageV3 notification;

        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 1 UID UNLINK 3 1:3"
                 << "S: 1 NO Can't link items to non-virtual collections";
        QTest::newRow("non-virtual collection") << scenario << NotificationMessageV3() << true;

        notification.setType(NotificationMessageV2::Items);
        notification.setOperation(NotificationMessageV2::Unlink);
        notification.addEntity(1, QLatin1String("A"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(2, QLatin1String("B"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(3, QLatin1String("C"), QString(), QLatin1String("application/octet-stream"));
        notification.setParentCollection(6);
        notification.setResource("akonadi_fake_resource_with_virtual_collections_0");
        notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID UNLINK 6 1:3"
                 << "S: 2 OK LINK complete";
        QTest::newRow("normal") << scenario << notification << false;

        notification.clearEntities();
        notification.addEntity(4, QLatin1String("D"), QString(), QLatin1String("application/octet-stream"));
        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID UNLINK 6 4 2048"
                 << "S: 2 OK LINK complete";
        QTest::newRow("existent and non-existent item") << scenario << notification << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 UID UNLINK 6 4096"
                 << "S: 2 OK LINK complete";
        QTest::newRow("non-existent item only") << scenario << NotificationMessageV3() << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
                 << "C: 4 UID UNLINK 6 RID (\"F\" \"G\")"
                 << "S: 4 OK LINK complete";
        notification.clearEntities();
        notification.clearEntities();
        notification.addEntity(6, QLatin1String("F"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(7, QLatin1String("G"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("RID items") << scenario << notification << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
                 << "C: 4 HRID UNLINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) UID 5"
                 << "S: 4 OK LINK complete";
        notification.setParentCollection(7);
        notification.clearEntities();
        notification.addEntity(5, QLatin1String("E"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection") << scenario << notification << false;

        scenario.clear();
        scenario << FakeAkonadiServer::defaultScenario()
                 << FakeAkonadiServer::selectCollectionScenario(QLatin1String("Collection B"))
                 << FakeAkonadiServer::selectResourceScenario(QLatin1String("akonadi_fake_resource_with_virtual_collections_0"))
                 << "C: 4 HRID UNLINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) RID \"H\""
                 << "S: 4 OK LINK complete";
        notification.clearEntities();
        notification.addEntity(8, QLatin1String("H"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection, RID items") << scenario << notification << false;
    }

    void testUnlink()
    {
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(NotificationMessageV3, notification);
        QFETCH(bool, expectFail);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();

        QSignalSpy *notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (notification.isValid()) {
            QCOMPARE(notificationSpy->count(), 1);
            const NotificationMessageV3::List notifications = notificationSpy->takeFirst().first().value<NotificationMessageV3::List>();
            QCOMPARE(notifications.count(), 1);
            QCOMPARE(notifications.first(), notification);
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<NotificationMessageV3::List>().isEmpty());
        }

        Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
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
