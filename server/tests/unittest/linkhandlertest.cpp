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

#include <QtTest/QTest>
#include <QSignalSpy>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Scope)
Q_DECLARE_METATYPE(CommandContext);

class LinkHandlerTest : public QObject
{
    Q_OBJECT

private:
    FakeAkonadiServer mServer;

public:
    LinkHandlerTest()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

        Q_ASSERT_X(mServer.start(), "LinkHandlerTest",
                   "Fake Akonadi Server failed to start up, aborting test");
    }

    ~LinkHandlerTest()
    {
    }


private Q_SLOTS:
    void testLink_data()
    {
        QTest::addColumn<Scope>("scope");
        QTest::addColumn<QByteArray>("command");
        QTest::addColumn<Akonadi::NotificationMessageV3>("notification");
        QTest::addColumn<CommandContext>("context");
        QTest::addColumn<bool>("expectFail");

        Scope scope(Scope::Uid);
        QByteArray command;
        CommandContext context;
        NotificationMessageV3 notification;

        command = "1 UID LINK 3 1:3\n";
        QTest::newRow("non-virtual collection") << scope << command << NotificationMessageV3() << context << true;

        notification.setType(NotificationMessageV2::Items);
        notification.setOperation(NotificationMessageV2::Link);
        notification.addEntity(1, QLatin1String("A"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(2, QLatin1String("B"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(3, QLatin1String("C"), QString(), QLatin1String("application/octet-stream"));
        notification.setParentCollection(6);
        notification.setResource("akonadi_fake_resource_with_virtual_collections_0");
        command = "1 UID LINK 6 1:3\n";
        QTest::newRow("normal") << scope << command << notification << context << false;

        notification.clearEntities();
        notification.addEntity(4, QLatin1String("D"), QString(), QLatin1String("application/octet-stream"));
        command = "1 UID LINK 6 4 2048\n";
        QTest::newRow("existent and non-existent item") << scope << command << notification << context << false;

        command = "1 UID LINK 6 4096\n";
        QTest::newRow("non-existent item only") << scope << command << NotificationMessageV3() << context << false;

        context.setCollection(Collection::retrieveByName(QLatin1String("Collection B")));
        QVERIFY(context.collection().isValid());
        command = "1 UID LINK 6 RID (\"F\" \"G\")\n";
        notification.clearEntities();
        notification.clearEntities();
        notification.addEntity(6, QLatin1String("F"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(7, QLatin1String("G"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("RID items") << scope << command << notification << context << false;

        scope = Scope(Scope::HierarchicalRid);
        context.setCollection(Collection());
        context.setResource(Resource::retrieveByName(QLatin1String("akonadi_fake_resource_with_virtual_collections_0")));
        QVERIFY(context.resource().isValid());
        command = "1 HRID LINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) UID 5\n";
        notification.setParentCollection(7);
        notification.clearEntities();
        notification.addEntity(5, QLatin1String("E"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection") << scope << command << notification << context << false;

        command= "1 HRID LINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) RID \"H\"\n";
        context.setCollection(Collection::retrieveByName(QLatin1String("Collection B")));
        QVERIFY(context.collection().isValid());
        notification.clearEntities();
        notification.addEntity(8, QLatin1String("H"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection, RID items") << scope << command << notification << context << false;
    }

    void testLink()
    {
        QFETCH(Scope, scope);
        QFETCH(QByteArray, command);
        QFETCH(NotificationMessageV3, notification);
        QFETCH(CommandContext, context);
        QFETCH(bool, expectFail);

        // Will be deleted by Connection after test is finished
        Link *handler = new Link(scope.scope(), true);

        FakeConnection *fakeConnection = mServer.connection(handler, command, context);

        NotificationCollector *collector = fakeConnection->storageBackend()->notificationCollector();
        QSignalSpy responseSpy(fakeConnection, SIGNAL(responseAvailable(Akonadi::Server::Response)));
        QSignalSpy notificationSpy(collector, SIGNAL(notify(Akonadi::NotificationMessageV3::List)));

        // Run the actual test
        fakeConnection->run();

        QCOMPARE(responseSpy.count(), 1);
        const Response response = responseSpy.takeFirst().first().value<Response>();
        if (expectFail) {
            QVERIFY(response.resultCode() != Response::OK);
            QCOMPARE(notificationSpy.count(), 0);
        } else {
            if (!response.resultCode() == Response::OK) {
                qDebug() << "Result code: " << response.resultCode();
                qDebug() << "Response: " << response.asString();
                QFAIL("Incorrect result code, expected OK");
            }
            if (notification.isValid()) {
                QCOMPARE(notificationSpy.count(), 1);
                const NotificationMessageV3::List notifications = notificationSpy.takeFirst().first().value<NotificationMessageV3::List>();
                QCOMPARE(notifications.count(), 1);
                QCOMPARE(notifications.first(), notification);
            } else {
                QVERIFY(notificationSpy.isEmpty() || notificationSpy.takeFirst().first().value<NotificationMessageV3::List>().isEmpty());
            }
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
        QTest::addColumn<Scope>("scope");
        QTest::addColumn<QByteArray>("command");
        QTest::addColumn<Akonadi::NotificationMessageV3>("notification");
        QTest::addColumn<CommandContext>("context");
        QTest::addColumn<bool>("expectFail");

        Scope scope(Scope::Uid);
        QByteArray command;
        CommandContext context;
        NotificationMessageV3 notification;

        command = "1 UID UNLINK 3 1:3\n";
        QTest::newRow("non-virtual collection") << scope << command << NotificationMessageV3() << context << true;

        notification.setType(NotificationMessageV2::Items);
        notification.setOperation(NotificationMessageV2::Unlink);
        notification.addEntity(1, QLatin1String("A"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(2, QLatin1String("B"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(3, QLatin1String("C"), QString(), QLatin1String("application/octet-stream"));
        notification.setParentCollection(6);
        notification.setResource("akonadi_fake_resource_with_virtual_collections_0");
        command = "1 UID UNLINK 6 1:3\n";
        QTest::newRow("normal") << scope << command << notification << context << false;

        notification.clearEntities();
        notification.addEntity(4, QLatin1String("D"), QString(), QLatin1String("application/octet-stream"));
        command = "1 UID UNLINK 6 4 2048\n";
        QTest::newRow("existent and non-existent item") << scope << command << notification << context << false;

        command = "1 UID UNLINK 6 4096\n";
        QTest::newRow("non-existent item only") << scope << command << NotificationMessageV3() << context << false;

        context.setCollection(Collection::retrieveByName(QLatin1String("Collection B")));
        QVERIFY(context.collection().isValid());
        command = "1 UID UNLINK 6 RID (\"F\" \"G\")\n";
        notification.clearEntities();
        notification.clearEntities();
        notification.addEntity(6, QLatin1String("F"), QString(), QLatin1String("application/octet-stream"));
        notification.addEntity(7, QLatin1String("G"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("RID items") << scope << command << notification << context << false;

        scope = Scope(Scope::HierarchicalRid);
        context.setCollection(Collection());
        context.setResource(Resource::retrieveByName(QLatin1String("akonadi_fake_resource_with_virtual_collections_0")));
        QVERIFY(context.resource().isValid());
        command = "1 HRID UNLINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) UID 5\n";
        notification.setParentCollection(7);
        notification.clearEntities();
        notification.addEntity(5, QLatin1String("E"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection") << scope << command << notification << context << false;

        command= "1 HRID UNLINK ((-1, \"virtual2\") (-1, \"virtual\") (-1, \"\")) RID \"H\"\n";
        context.setCollection(Collection::retrieveByName(QLatin1String("Collection B")));
        QVERIFY(context.collection().isValid());
        notification.clearEntities();
        notification.addEntity(8, QLatin1String("H"), QString(), QLatin1String("application/octet-stream"));
        QTest::newRow("HRID collection, RID items") << scope << command << notification << context << false;
    }


    void testUnlink()
    {
        QFETCH(Scope, scope);
        QFETCH(QByteArray, command);
        QFETCH(NotificationMessageV3, notification);
        QFETCH(CommandContext, context);
        QFETCH(bool, expectFail);

        // Will be deleted by Connection after test is finished
        Link *handler = new Link(scope.scope(), false);

        FakeConnection *fakeConnection = mServer.connection(handler, command, context);

        NotificationCollector *collector = fakeConnection->storageBackend()->notificationCollector();
        QSignalSpy responseSpy(fakeConnection, SIGNAL(responseAvailable(Akonadi::Server::Response)));
        QSignalSpy notificationSpy(collector, SIGNAL(notify(Akonadi::NotificationMessageV3::List)));

        // Run the actual test
        fakeConnection->run();

        QCOMPARE(responseSpy.count(), 1);
        const Response response = responseSpy.takeFirst().first().value<Response>();
        if (expectFail) {
            QVERIFY(response.resultCode() != Response::OK);
            QCOMPARE(notificationSpy.count(), 0);
        } else {
            if (!response.resultCode() == Response::OK) {
                qDebug() << "Result code: " << response.resultCode();
                qDebug() << "Response: " << response.asString();
                QFAIL("Incorrect result code, expected OK");
            }
            if (notification.isValid()) {
                QCOMPARE(notificationSpy.count(), 1);
                const NotificationMessageV3::List notifications = notificationSpy.takeFirst().first().value<NotificationMessageV3::List>();
                QCOMPARE(notifications.count(), 1);
                QCOMPARE(notifications.first(), notification);
            } else {
                QVERIFY(notificationSpy.isEmpty() || notificationSpy.takeFirst().first().value<NotificationMessageV3::List>().isEmpty());
            }
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
