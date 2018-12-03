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

#include <storage/entity.h>

#include "fakeakonadiserver.h"
#include "fakedatastore.h"
#include <shared/aktest.h>
#include "entities.h"
#include "collectionreferencemanager.h"
#include "dbinitializer.h"

#include <private/scope_p.h>

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Collection::List)

class CollectionReferenceTest : public QObject
{
    Q_OBJECT

    DbInitializer initializer;

public:
    CollectionReferenceTest()
    {
        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }

        initializer.createResource("testresource");
        initializer.createCollection("col1");
        Collection col2 = initializer.createCollection("col2");
        col2.setEnabled(false);
        col2.update();
    }

    ~CollectionReferenceTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testModify_data()
    {
        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");

        auto notificationTemplate = Protocol::CollectionChangeNotificationPtr::create();
        notificationTemplate->setOperation(Protocol::CollectionChangeNotification::Modify);
        notificationTemplate->setParentCollection(0);
        notificationTemplate->setResource("testresource");
        notificationTemplate->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        Protocol::FetchCollectionsResponse collection;
        collection.setId(initializer.collection("col2").id());
        collection.setRemoteId(QStringLiteral("col2"));
        collection.setRemoteRevision(QStringLiteral(""));
        notificationTemplate->setCollection(std::move(collection));

        {
            auto cmd = Protocol::FetchCollectionsCommandPtr::create();
            cmd->setDepth(Protocol::FetchCollectionsCommand::AllCollections);
            cmd->setResource(QStringLiteral("testresource"));
            cmd->setEnabled(true);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer.listResponse(initializer.collection("col1")))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("list before referenced first level") << scenarios << Protocol::ChangeNotificationList();
        }

        {
            auto cmd = Protocol::ModifyCollectionCommandPtr::create(initializer.collection("col2").id());
            cmd->setReferenced(true);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponsePtr::create());

            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setChangedParts(QSet<QByteArray>() << "REFERENCED");

            QTest::newRow("reference") << scenarios << (Protocol::ChangeNotificationList() << notification);
        }

        {
            auto cmd = Protocol::ModifyCollectionCommandPtr::create(initializer.collection("col2").id());
            cmd->setReferenced(true);

            auto listCmd = Protocol::FetchCollectionsCommandPtr::create(initializer.collection("col2").id());
            listCmd->setDepth(Protocol::FetchCollectionsCommand::BaseCollection);
            listCmd->setEnabled(true);

            Collection col2 = initializer.collection("col2");
            col2.setReferenced(true);
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponsePtr::create())
                      << TestScenario::create(6, TestScenario::ClientCmd, listCmd)
                      << TestScenario::create(6, TestScenario::ServerCmd, initializer.listResponse(col2))
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());

            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setChangedParts(QSet<QByteArray>() << "REFERENCED");

            QTest::newRow("list referenced base") << scenarios << (Protocol::ChangeNotificationList() << notification);
        }
        {
            auto cmd = Protocol::ModifyCollectionCommandPtr::create(initializer.collection("col2").id());
            cmd->setReferenced(true);

            auto listCmd = Protocol::FetchCollectionsCommandPtr::create();
            listCmd->setResource(QStringLiteral("testresource"));
            listCmd->setEnabled(true);
            listCmd->setDepth(Protocol::FetchCollectionsCommand::ParentCollection);

            Collection col2 = initializer.collection("col2");
            col2.setReferenced(true);
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponsePtr::create())
                      << TestScenario::create(6, TestScenario::ClientCmd, listCmd)
                      << TestScenario::create(6, TestScenario::ServerCmd, initializer.listResponse(initializer.collection("col1")))
                      << TestScenario::create(6, TestScenario::ServerCmd, initializer.listResponse(col2))
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());

            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setChangedParts(QSet<QByteArray>() << "REFERENCED");

            QTest::newRow("list referenced first level") << scenarios << (Protocol::ChangeNotificationList() << notification);
        }
        {
            auto cmd1 = Protocol::ModifyCollectionCommandPtr::create(initializer.collection("col2").id());
            cmd1->setReferenced(true);

            auto cmd2 = Protocol::ModifyCollectionCommandPtr::create(initializer.collection("col2").id());
            cmd2->setReferenced(false);

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd1)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyCollectionResponsePtr::create())
                      << TestScenario::create(6, TestScenario::ClientCmd, cmd2)
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::ModifyCollectionResponsePtr::create());

            auto notification = Protocol::CollectionChangeNotificationPtr::create(*notificationTemplate);
            notification->setChangedParts(QSet<QByteArray>() << "REFERENCED");

            QTest::newRow("dereference") << scenarios << (Protocol::ChangeNotificationList() << notification << notification);
        }
    }

    void testModify()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);

        // Clean all references from previous run
        CollectionReferenceManager::cleanup();

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        auto notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (expectedNotifications.isEmpty()) {
            QTRY_VERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<Protocol::ChangeNotificationList>().isEmpty());
        } else {
            Protocol::ChangeNotificationList receivedNotifications;
            for (int q = 0; q < notificationSpy->size(); q++) {
                //Only one notify call
                QCOMPARE(notificationSpy->first().count(), 1);
                const Protocol::ChangeNotificationList n = notificationSpy->first().first().value<Protocol::ChangeNotificationList>();
                for (int i = 0; i < n.size(); i++) {
                    receivedNotifications.append(n.at(i));
                }
            }
            QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
            for (int i = 0; i < expectedNotifications.size(); i++) {
                QCOMPARE(*receivedNotifications.at(i), *expectedNotifications.at(i));
            }
        }
    }

    void testReferenceCollection()
    {
        Collection col = initializer.createCollection("testReferenceCollection");

        CollectionReferenceManager::instance()->referenceCollection("testReferenceCollectionSession", col, true);
        QVERIFY(CollectionReferenceManager::instance()->isReferenced(col.id()));
        QVERIFY(CollectionReferenceManager::instance()->isReferenced(col.id(), "testReferenceCollectionSession"));

        CollectionReferenceManager::instance()->referenceCollection("foobar", col, false);
        QVERIFY(CollectionReferenceManager::instance()->isReferenced(col.id()));
        QVERIFY(CollectionReferenceManager::instance()->isReferenced(col.id(), "testReferenceCollectionSession"));

        CollectionReferenceManager::instance()->referenceCollection("testReferenceCollectionSession", col, false);
        QVERIFY(!CollectionReferenceManager::instance()->isReferenced(col.id()));
        QVERIFY(!CollectionReferenceManager::instance()->isReferenced(col.id(), "testReferenceCollectionSession"));
        QVERIFY(col.remove());
    }

    void testSessionClosed()
    {
        Collection col = initializer.createCollection("testSessionCollection");
        col.setReferenced(true);
        QVERIFY(col.update());
        CollectionReferenceManager::instance()->referenceCollection("testSessionClosedSession", col, true);
        CollectionReferenceManager::instance()->referenceCollection("testSessionClosedSession2", col, true);

        //Remove first session
        CollectionReferenceManager::instance()->removeSession("testSessionClosedSession2");
        QVERIFY(Collection::retrieveById(col.id()).referenced());
        QVERIFY(!CollectionReferenceManager::instance()->isReferenced(col.id(), "testSessionClosedSession2"));
        QVERIFY(CollectionReferenceManager::instance()->isReferenced(col.id(), "testSessionClosedSession"));

        CollectionReferenceManager::instance()->removeSession("testSessionClosedSession");
        QVERIFY(!Collection::retrieveById(col.id()).referenced());
        QVERIFY(!CollectionReferenceManager::instance()->isReferenced(col.id(), "testSessionClosedSession"));

        QVERIFY(col.remove());
    }

    void testCleanup()
    {
        Collection col = initializer.createCollection("testCleanupCollection");
        col.setReferenced(true);
        QVERIFY(col.update());

        CollectionReferenceManager::cleanup();
        QVERIFY(!Collection::retrieveById(col.id()).referenced());

        QVERIFY(col.remove());
    }

};

AKTEST_FAKESERVER_MAIN(CollectionReferenceTest)

#include "collectionreferencetest.moc"
