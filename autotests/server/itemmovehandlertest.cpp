/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QObject>

#include <storage/entity.h>

#include "aktest.h"
#include "entities.h"
#include "fakeakonadiserver.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class ItemMoveHandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;

public:
    ItemMoveHandlerTest()
    {
        mAkonadi.init();
    }

    Protocol::FetchItemsResponse fetchResponse(quint64 id, const QString &rid, const QString &rrev, const QString &mt)
    {
        Protocol::FetchItemsResponse item;
        item.setId(id);
        item.setRemoteId(rid);
        item.setRemoteRevision(rrev);
        item.setMimeType(mt);
        return item;
    }

private Q_SLOTS:
    void testMove_data()
    {
        const Collection srcCol = Collection::retrieveByName(QStringLiteral("Collection B"));
        const Collection destCol = Collection::retrieveByName(QStringLiteral("Collection A"));

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");
        QTest::addColumn<QVariant>("newValue");

        auto notificationTemplate = Protocol::ItemChangeNotificationPtr::create();
        notificationTemplate->setOperation(Protocol::ItemChangeNotification::Move);
        notificationTemplate->setResource("akonadi_fake_resource_0");
        notificationTemplate->setDestinationResource("akonadi_fake_resource_0");
        notificationTemplate->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        notificationTemplate->setParentCollection(srcCol.id());
        notificationTemplate->setParentDestCollection(destCol.id());

        {
            auto cmd = Protocol::MoveItemsCommandPtr::create(1, destCol.id());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::MoveItemsResponsePtr::create());

            auto notification = Protocol::ItemChangeNotificationPtr::create(*notificationTemplate);
            notification->setItems({fetchResponse(1, QStringLiteral("A"), QString(), QStringLiteral("application/octet-stream"))});

            QTest::newRow("move item") << scenarios << Protocol::ChangeNotificationList{notification} << QVariant::fromValue(destCol.id());
        }

        {
            auto cmd = Protocol::MoveItemsCommandPtr::create(QVector<qint64>{2, 3}, destCol.id());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::MoveItemsResponsePtr::create());

            auto notification = Protocol::ItemChangeNotificationPtr::create(*notificationTemplate);
            notification->setItems({fetchResponse(3, QStringLiteral("C"), QString(), QStringLiteral("application/octet-stream")),
                                    fetchResponse(2, QStringLiteral("B"), QString(), QStringLiteral("application/octet-stream"))});

            QTest::newRow("move items") << scenarios << Protocol::ChangeNotificationList{notification} << QVariant::fromValue(destCol.id());
        }
    }

    void testMove()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);
        QFETCH(QVariant, newValue);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        auto notificationSpy = mAkonadi.notificationSpy();
        if (expectedNotifications.isEmpty()) {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<Protocol::ChangeNotificationList>().isEmpty());
            return;
        }
        QCOMPARE(notificationSpy->count(), 1);
        // Only one notify call
        QCOMPARE(notificationSpy->first().count(), 1);
        const auto receivedNotifications = notificationSpy->first().first().value<Protocol::ChangeNotificationList>();
        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());

        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(*receivedNotifications.at(i).staticCast<Protocol::ItemChangeNotification>(),
                     *expectedNotifications.at(i).staticCast<Protocol::ItemChangeNotification>());
            const auto notification = receivedNotifications.at(i).staticCast<Protocol::ItemChangeNotification>();
            QCOMPARE(notification->parentDestCollection(), newValue.toInt());

            Q_FOREACH (const auto &ntfItem, notification->items()) {
                const PimItem item = PimItem::retrieveById(ntfItem.id());
                QCOMPARE(item.collectionId(), newValue.toInt());
            }
        }
    }
};

AKTEST_FAKESERVER_MAIN(ItemMoveHandlerTest)

#include "itemmovehandlertest.moc"
