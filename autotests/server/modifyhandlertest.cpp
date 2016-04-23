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

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QList<Akonadi::Protocol::ChangeNotification>)

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
        QTest::addColumn<QList<Akonadi::Protocol::ChangeNotification> >("expectedNotifications");
        QTest::addColumn<QVariant>("newValue");

        Akonadi::Protocol::ChangeNotification notificationTemplate;
        notificationTemplate.setType(Protocol::ChangeNotification::Collections);
        notificationTemplate.setOperation(Protocol::ChangeNotification::Modify);
        notificationTemplate.addEntity(5, QStringLiteral("ColD"), QStringLiteral(""));
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

            Akonadi::Protocol::ChangeNotification notification = notificationTemplate;
            notification.setItemParts(QSet<QByteArray>() << "NAME");

            QTest::newRow("modify collection") << scenarios << (QList<Akonadi::Protocol::ChangeNotification>() << notification) << QVariant::fromValue(QString::fromLatin1("New Name"));
        }
        {
            Protocol::ModifyCollectionCommand cmd(5);
            cmd.setEnabled(false);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponse());

            Akonadi::Protocol::ChangeNotification notification = notificationTemplate;
            notification.setItemParts(QSet<QByteArray>() << "ENABLED");
            Akonadi::Protocol::ChangeNotification unsubscribeNotification = notificationTemplate;
            unsubscribeNotification.setOperation(Protocol::ChangeNotification::Unsubscribe);

            QTest::newRow("disable collection") << scenarios << (QList<Akonadi::Protocol::ChangeNotification>() << notification << unsubscribeNotification) << QVariant::fromValue(false);
        }
        {
            Protocol::ModifyCollectionCommand cmd(5);
            cmd.setEnabled(true);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponse());

            Akonadi::Protocol::ChangeNotification notification = notificationTemplate;
            notification.setItemParts(QSet<QByteArray>() << "ENABLED");
            Akonadi::Protocol::ChangeNotification subscribeNotification = notificationTemplate;
            subscribeNotification.setOperation(Protocol::ChangeNotification::Subscribe);

            QTest::newRow("enable collection") << scenarios << (QList<Akonadi::Protocol::ChangeNotification>() << notification << subscribeNotification) << QVariant::fromValue(true);
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

            Akonadi::Protocol::ChangeNotification notification = notificationTemplate;
            notification.setItemParts(QSet<QByteArray>() << "ENABLED" << "SYNC" << "DISPLAY" << "INDEX");
            Akonadi::Protocol::ChangeNotification unsubscribeNotification = notificationTemplate;
            unsubscribeNotification.setOperation(Protocol::ChangeNotification::Unsubscribe);

            QTest::newRow("local override enable") << scenarios << (QList<Akonadi::Protocol::ChangeNotification>() << notification << unsubscribeNotification) << QVariant::fromValue(true);
        }
    }

    void testModify()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(QList<Protocol::ChangeNotification>, expectedNotifications);
        QFETCH(QVariant, newValue);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        QSignalSpy *notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
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
            QCOMPARE(receivedNotifications.at(i), expectedNotifications.at(i));
            Protocol::ChangeNotification notification = receivedNotifications.at(i);
            Q_FOREACH (const Protocol::ChangeNotification::Entity &entity, notification.entities()) {
                if (notification.itemParts().contains("NAME")) {
                    Collection col = Collection::retrieveById(entity.id);
                    QCOMPARE(col.name(), newValue.toString());
                }
                if (notification.itemParts().contains("ENABLED") || notification.itemParts().contains("SYNC") || notification.itemParts().contains("DISPLAY") || notification.itemParts().contains("INDEX")) {
                    Collection col = Collection::retrieveById(entity.id);
                    const bool sync = col.syncPref() == Tristate::Undefined ? col.enabled() : col.syncPref() == Tristate::True;
                    QCOMPARE(sync, newValue.toBool());
                    const bool display = col.displayPref() == Tristate::Undefined ? col.enabled() : col.displayPref() == Tristate::True;
                    QCOMPARE(display, newValue.toBool());
                    const bool index = col.indexPref() == Tristate::Undefined ? col.enabled() : col.indexPref() == Tristate::True;
                    QCOMPARE(index, newValue.toBool());
                }
            }
        }
    }

};

AKTEST_FAKESERVER_MAIN(ModifyHandlerTest)

#include "modifyhandlertest.moc"
