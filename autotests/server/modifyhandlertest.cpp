/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include <handler/modify.h>

#include <storage/entity.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "entities.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Protocol::CollectionChangeNotification::List)

class ModifyHandlerTest : public QObject
{
    Q_OBJECT

public:
    ModifyHandlerTest()
    {
        try {
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }
    }

    ~ModifyHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testModify_data()
    {
        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Protocol::CollectionChangeNotification::List>("expectedNotifications");
        QTest::addColumn<QVariant>("newValue");

        Protocol::CollectionChangeNotification notificationTemplate;
        notificationTemplate.setOperation(Protocol::CollectionChangeNotification::Modify);
        notificationTemplate.setId(5);
        notificationTemplate.setRemoteId(QStringLiteral("ColD"));
        notificationTemplate.setRemoteRevision(QStringLiteral(""));
        notificationTemplate.setParentCollection(4);
        notificationTemplate.setResource("akonadi_fake_resource_0");
        notificationTemplate.setSessionId(FakeAkonadiServer::instanceName().toLatin1());

        {
            Protocol::ModifyCollectionCommand cmd(5);
            cmd.setName(QStringLiteral("New Name"));

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponse());

            Protocol::CollectionChangeNotification notification = notificationTemplate;
            notification.setChangedParts(QSet<QByteArray>() << "NAME");

            QTest::newRow("modify collection") << scenarios << Protocol::CollectionChangeNotification::List{ notification } << QVariant::fromValue(QString::fromLatin1("New Name"));
        }
        {
            Protocol::ModifyCollectionCommand cmd(5);
            cmd.setEnabled(false);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponse());

            Protocol::CollectionChangeNotification notification = notificationTemplate;
            notification.setChangedParts(QSet<QByteArray>() << "ENABLED");
            Protocol::CollectionChangeNotification unsubscribeNotification = notificationTemplate;
            unsubscribeNotification.setOperation(Protocol::CollectionChangeNotification::Unsubscribe);

            QTest::newRow("disable collection") << scenarios << Protocol::CollectionChangeNotification::List{ notification, unsubscribeNotification} << QVariant::fromValue(false);
        }
        {
            Protocol::ModifyCollectionCommand cmd(5);
            cmd.setEnabled(true);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponse());

            Protocol::CollectionChangeNotification notification = notificationTemplate;
            notification.setChangedParts(QSet<QByteArray>() << "ENABLED");
            Protocol::CollectionChangeNotification subscribeNotification = notificationTemplate;
            subscribeNotification.setOperation(Protocol::CollectionChangeNotification::Subscribe);

            QTest::newRow("enable collection") << scenarios << Protocol::CollectionChangeNotification::List{ notification, subscribeNotification } << QVariant::fromValue(true);
        }
        {
            Protocol::ModifyCollectionCommand cmd(5);
            cmd.setEnabled(false);
            cmd.setSyncPref(Tristate::True);
            cmd.setDisplayPref(Tristate::True);
            cmd.setIndexPref(Tristate::True);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponse());

            Protocol::CollectionChangeNotification notification = notificationTemplate;
            notification.setChangedParts(QSet<QByteArray>() << "ENABLED" << "SYNC" << "DISPLAY" << "INDEX");
            Protocol::CollectionChangeNotification unsubscribeNotification = notificationTemplate;
            unsubscribeNotification.setOperation(Protocol::CollectionChangeNotification::Unsubscribe);

            QTest::newRow("local override enable") << scenarios << Protocol::CollectionChangeNotification::List{ notification, unsubscribeNotification } << QVariant::fromValue(true);
        }
    }

    void testModify()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::CollectionChangeNotification::List, expectedNotifications);
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
            QCOMPARE(Protocol::CollectionChangeNotification(receivedNotifications.at(i)), expectedNotifications.at(i));
            Protocol::CollectionChangeNotification notification = receivedNotifications.at(i);
            if (notification.changedParts().contains("NAME")) {
                Collection col = Collection::retrieveById(notification.id());
                QCOMPARE(col.name(), newValue.toString());
            }
            if (!notification.changedParts().intersect({ "ENABLED", "SYNC", "DISPLAY", "INDEX" }).isEmpty()) {
                Collection col = Collection::retrieveById(notification.id());
                const bool sync = col.syncPref() == Collection::Undefined ? col.enabled() : col.syncPref() == Collection::True;
                QCOMPARE(sync, newValue.toBool());
                const bool display = col.displayPref() == Collection::Undefined ? col.enabled() : col.displayPref() == Collection::True;
                QCOMPARE(display, newValue.toBool());
                const bool index = col.indexPref() == Collection::Undefined ? col.enabled() : col.indexPref() == Collection::True;
                QCOMPARE(index, newValue.toBool());
            }
        }
    }

};

AKTEST_FAKESERVER_MAIN(ModifyHandlerTest)

#include "modifyhandlertest.moc"
