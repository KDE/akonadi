/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QObject>

#include "storage/entity.h"

#include "aktest.h"
#include "dbinitializer.h"
#include "entities.h"
#include "fakeakonadiserver.h"

#include "private/scope_p.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class CollectionCreateHandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;

public:
    CollectionCreateHandlerTest()
    {
        mAkonadi.init();
    }

private Q_SLOTS:
    void testCreate_data()
    {
        DbInitializer dbInitializer;

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Protocol::CollectionChangeNotificationPtr>("notification");

        auto notificationTemplate = Protocol::CollectionChangeNotificationPtr::create();
        notificationTemplate->setOperation(Protocol::CollectionChangeNotification::Add);
        notificationTemplate->setParentCollection(3);
        notificationTemplate->setResource("akonadi_fake_resource_0");
        notificationTemplate->setSessionId(FakeAkonadiServer::instanceName().toLatin1());

        {
            auto cmd = Protocol::CreateCollectionCommandPtr::create();
            cmd->setName(QStringLiteral("New Name"));
            cmd->setParent(Scope(3));
            cmd->setAttributes({{"MYRANDOMATTRIBUTE", ""}});

            auto resp = Protocol::FetchCollectionsResponsePtr::create(8);
            resp->setName(QStringLiteral("New Name"));
            resp->setParentId(3);
            resp->setAttributes({{"MYRANDOMATTRIBUTE", ""}});
            resp->setResource(QStringLiteral("akonadi_fake_resource_0"));
            resp->cachePolicy().setLocalParts({QLatin1StringView("ALL")});
            resp->setMimeTypes({QLatin1StringView("application/octet-stream"), QLatin1StringView("inode/directory")});

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateCollectionResponsePtr::create());

            Protocol::FetchCollectionsResponse collection(*resp);
            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setCollection(std::move(collection));

            QTest::newRow("create collection") << scenarios << notification;
        }
        {
            auto cmd = Protocol::CreateCollectionCommandPtr::create();
            cmd->setName(QStringLiteral("Name 2"));
            cmd->setParent(Scope(3));
            cmd->setEnabled(false);
            cmd->setDisplayPref(Tristate::True);
            cmd->setSyncPref(Tristate::True);
            cmd->setIndexPref(Tristate::True);

            auto resp = Protocol::FetchCollectionsResponsePtr::create(9);
            resp->setName(QStringLiteral("Name 2"));
            resp->setParentId(3);
            resp->setEnabled(false);
            resp->setDisplayPref(Tristate::True);
            resp->setSyncPref(Tristate::True);
            resp->setIndexPref(Tristate::True);
            resp->setResource(QStringLiteral("akonadi_fake_resource_0"));
            resp->cachePolicy().setLocalParts({QLatin1StringView("ALL")});
            resp->setMimeTypes({QLatin1StringView("application/octet-stream"), QLatin1StringView("inode/directory")});

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateCollectionResponsePtr::create());

            Protocol::FetchCollectionsResponse collection(*resp);
            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setCollection(std::move(collection));

            QTest::newRow("create collection with local override") << scenarios << notification;
        }

        {
            auto cmd = Protocol::CreateCollectionCommandPtr::create();
            cmd->setName(QStringLiteral("TopLevel"));
            cmd->setParent(Scope(0));
            cmd->setMimeTypes({QLatin1StringView("inode/directory")});

            auto resp = Protocol::FetchCollectionsResponsePtr::create(10);
            resp->setName(QStringLiteral("TopLevel"));
            resp->setParentId(0);
            resp->setEnabled(true);
            resp->setMimeTypes({QLatin1StringView("inode/directory")});
            resp->cachePolicy().setLocalParts({QLatin1StringView("ALL")});
            resp->setResource(QStringLiteral("akonadi_fake_resource_0"));

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario("akonadi_fake_resource_0") << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateCollectionResponsePtr::create());

            Protocol::FetchCollectionsResponse collection(*resp);
            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setSessionId("akonadi_fake_resource_0");
            notification->setParentCollection(0);
            notification->setCollection(std::move(collection));

            QTest::newRow("create top-level collection") << scenarios << notification;
        }
    }

    void testCreate()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::CollectionChangeNotificationPtr, notification);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        auto notificationSpy = mAkonadi.notificationSpy();
        if (notification->operation() != Protocol::CollectionChangeNotification::InvalidOp) {
            QTRY_COMPARE(notificationSpy->count(), 1);
            const auto notifications = notificationSpy->takeFirst().first().value<Protocol::ChangeNotificationList>();
            QCOMPARE(notifications.count(), 1);
            const auto actualNtf = notifications.first().staticCast<Protocol::CollectionChangeNotification>();
            if (*actualNtf != *notification) {
                qDebug() << "Actual:  " << Protocol::debugString(actualNtf);
                qDebug() << "Expected:" << Protocol::debugString(notification);
            }
            QCOMPARE(*notifications.first().staticCast<Protocol::CollectionChangeNotification>(), *notification);
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<Protocol::ChangeNotificationList>().isEmpty());
        }
    }
};

AKTEST_FAKESERVER_MAIN(CollectionCreateHandlerTest)

#include "collectioncreatehandlertest.moc"
