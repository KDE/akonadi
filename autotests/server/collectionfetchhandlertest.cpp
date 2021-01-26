/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include <private/scope_p.h>

#include "aktest.h"
#include "dbinitializer.h"
#include "entities.h"
#include "fakeakonadiserver.h"
#include <storage/storagedebugger.h>

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class CollectionFetchHandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;

public:
    CollectionFetchHandlerTest()
        : QObject()
    {
        mAkonadi.setPopulateDb(false);
        mAkonadi.init();
        {
            MimeType mt(QStringLiteral("mimetype1"));
            mt.insert();
        }
        {
            MimeType mt(QStringLiteral("mimetype2"));
            mt.insert();
        }
        {
            MimeType mt(QStringLiteral("mimetype3"));
            mt.insert();
        }
        {
            MimeType mt(QStringLiteral("mimetype4"));
            mt.insert();
        }
    }

    Protocol::FetchCollectionsCommandPtr
    createCommand(const Scope &scope,
                  Protocol::FetchCollectionsCommand::Depth depth = Akonadi::Protocol::FetchCollectionsCommand::BaseCollection,
                  Protocol::Ancestor::Depth ancDepth = Protocol::Ancestor::NoAncestor,
                  const QStringList &mimeTypes = QStringList(),
                  const QString &resource = QString())
    {
        auto cmd = Protocol::FetchCollectionsCommandPtr::create(scope);
        cmd->setDepth(depth);
        cmd->setAncestorsDepth(ancDepth);
        cmd->setMimeTypes(mimeTypes);
        cmd->setResource(resource);
        return cmd;
    }

    QScopedPointer<DbInitializer> initializer;
private Q_SLOTS:

    void testList_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);
        Collection col4 = initializer->createCollection("col4");

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(Scope(), Protocol::FetchCollectionsCommand::AllCollections))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(initializer->collection("Search")))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col4))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(col1.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("base list") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(col1.id(), Protocol::FetchCollectionsCommand::ParentCollection))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("first level list") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(col1.id(), Protocol::FetchCollectionsCommand::AllCollections))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list that filters collection") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(col2.id(), Protocol::FetchCollectionsCommand::BaseCollection, Protocol::Ancestor::AllAncestors))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2, true))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("base ancestors") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(col2.id(), Protocol::FetchCollectionsCommand::BaseCollection, Protocol::Ancestor::AllAncestors))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2, true))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("first level ancestors") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(col1.id(), Protocol::FetchCollectionsCommand::AllCollections, Protocol::Ancestor::AllAncestors))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2, true))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3, true))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive ancestors") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(Scope(), Protocol::FetchCollectionsCommand::ParentCollection))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(initializer->collection("Search")))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col4))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("first level root list") << scenarios;
        }
    }

    void testList()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testListFiltered_data()
    {
        initializer.reset(new DbInitializer);

        MimeType mtCalendar(QStringLiteral("text/calendar"));
        mtCalendar.insert();

        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        col1.update();
        Collection col2 = initializer->createCollection("col2", col1);
        col2.addMimeType(mtCalendar);
        col2.update();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(Scope(),
                                                            Protocol::FetchCollectionsCommand::AllCollections,
                                                            Protocol::Ancestor::NoAncestor,
                                                            {QLatin1String("text/calendar")}))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1, false, false))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list to display including local override") << scenarios;
        }
    }

    void testListFiltered()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testListFilterByResource()
    {
        initializer.reset(new DbInitializer);

        Resource res2;
        res2.setName(QStringLiteral("testresource2"));
        QVERIFY(res2.insert());

        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");

        Collection col2;
        col2.setName(QStringLiteral("col2"));
        col2.setRemoteId(QStringLiteral("col2"));
        col2.setResource(res2);
        QVERIFY(col2.insert());

        TestScenario::List scenarios;
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5,
                                          TestScenario::ClientCmd,
                                          createCommand(Scope(),
                                                        Protocol::FetchCollectionsCommand::AllCollections,
                                                        Protocol::Ancestor::NoAncestor,
                                                        {},
                                                        QStringLiteral("testresource")))
                  << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        col2.remove();
        res2.remove();
    }

    void testListEnabled_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2", col1);
        col2.setEnabled(false);
        col2.setSyncPref(Collection::True);
        col2.setDisplayPref(Collection::True);
        col2.setIndexPref(Collection::True);
        col2.update();
        Collection col3 = initializer->createCollection("col3", col2);
        col3.setEnabled(true);
        col3.setSyncPref(Collection::False);
        col3.setDisplayPref(Collection::False);
        col3.setIndexPref(Collection::False);
        col3.update();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;

            auto cmd = createCommand(col3.id());
            cmd->setDisplayPref(true);
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            // Listing a disabled collection should still work for base listing
            QTest::newRow("list base of disabled collection") << scenarios;
        }
        {
            TestScenario::List scenarios;

            auto cmd = createCommand(Scope(), Protocol::FetchCollectionsCommand::AllCollections);
            cmd->setDisplayPref(true);
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(initializer->collection("Search")))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list to display including local override") << scenarios;
        }
        {
            TestScenario::List scenarios;

            auto cmd = createCommand(Scope(), Protocol::FetchCollectionsCommand::AllCollections);
            cmd->setSyncPref(true);
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(initializer->collection("Search")))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list to sync including local override") << scenarios;
        }
        {
            TestScenario::List scenarios;

            auto cmd = createCommand(Scope(), Protocol::FetchCollectionsCommand::AllCollections);
            cmd->setIndexPref(true);
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(initializer->collection("Search")))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list to index including local override") << scenarios;
        }
        {
            TestScenario::List scenarios;

            auto cmd = createCommand(Scope(), Protocol::FetchCollectionsCommand::AllCollections);
            cmd->setEnabled(true);
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(initializer->collection("Search")))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("recursive list of enabled") << scenarios;
        }
    }

    void testListEnabled()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testListAttribute_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2");

        CollectionAttribute attr1;
        attr1.setType("type");
        attr1.setValue("value");
        attr1.setCollection(col1);
        attr1.insert();

        CollectionAttribute attr2;
        attr2.setType("type");
        attr2.setValue(QStringLiteral("Umlautäöü").toUtf8());
        attr2.setCollection(col2);
        attr2.insert();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(col1.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1, false, true))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("list attribute") << scenarios;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(col2.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2, false, true))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("list attribute") << scenarios;
        }
    }

    void testListAttribute()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testListAncestorAttributes_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");

        CollectionAttribute attr1;
        attr1.setType("type");
        attr1.setValue("value");
        attr1.setCollection(col1);
        attr1.insert();

        Collection col2 = initializer->createCollection("col2", col1);

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;

            auto cmd = createCommand(col2.id(), Protocol::FetchCollectionsCommand::BaseCollection, Protocol::Ancestor::AllAncestors);
            cmd->setAncestorsAttributes({"type"});
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2, true, true, {QLatin1String("type")}))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("list ancestor attribute with fetch scope") << scenarios;
        }
    }

    void testListAncestorAttributes()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testIncludeAncestors_data()
    {
        // The collection we are querying contains a load of disabled collections (typical scenario with many shared folders)
        // The collection we are NOT querying contains a reasonable amount of enabled collections (to test the performance impact of the manually filtering by
        // tree)
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");

        MimeType mtDirectory = MimeType::retrieveByName(QStringLiteral("mimetype1"));

        Collection col1 = initializer->createCollection("col1");
        col1.addMimeType(mtDirectory);
        col1.update();
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);
        Collection col4 = initializer->createCollection("col4", col3);
        col4.addMimeType(mtDirectory);
        col4.update();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(Scope(),
                                                            Protocol::FetchCollectionsCommand::AllCollections,
                                                            Protocol::Ancestor::NoAncestor,
                                                            {QLatin1String("mimetype1")}))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col1))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col4))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            QTest::newRow("ensure filtered grandparent is included") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(col1.id(),
                                                            Protocol::FetchCollectionsCommand::AllCollections,
                                                            Protocol::Ancestor::NoAncestor,
                                                            {QLatin1String("mimetype1")}))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col2))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col3))
                      << TestScenario::create(5, TestScenario::ServerCmd, initializer->listResponse(col4))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponsePtr::create());
            // This also ensures col1 is excluded although it matches the mimetype filter
            QTest::newRow("ensure filtered grandparent is included with specified parent") << scenarios;
        }
    }

    void testIncludeAncestors()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

// No point in running the benchmark every time
#if 0

    void testListEnabledBenchmark_data()
    {
        //The collection we are quering contains a load of disabled collections (typical scenario with many shared folders)
        //The collection we are NOT querying contains a reasonable amount of enabled collections (to test the performance impact of the manually filtering by tree)
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");

        Collection toplevel = initializer->createCollection("toplevel");

        Collection col1 = initializer->createCollection("col1", toplevel);
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);

        Collection col4 = initializer->createCollection("col4", toplevel);
        Collection col5 = initializer->createCollection("col5", col4);
        col5.setEnabled(false);
        col5.update();
        Collection col6 = initializer->createCollection("col6", col5);
        col5.setEnabled(false);
        col5.update();

        MimeType mt1 = MimeType::retrieveByName(QLatin1String("mimetype1"));
        MimeType mt2 = MimeType::retrieveByName(QLatin1String("mimetype2"));
        MimeType mt3 = MimeType::retrieveByName(QLatin1String("mimetype3"));
        MimeType mt4 = MimeType::retrieveByName(QLatin1String("mimetype4"));

        QTime t;
        t.start();
        for (int i = 0; i < 100000; i++) {
            QByteArray name = QString::fromLatin1("col%1").arg(i+4).toLatin1();
            Collection col = initializer->createCollection(name.data(), col3);
            col.setEnabled(false);
            col.addMimeType(mt1);
            col.addMimeType(mt2);
            col.addMimeType(mt3);
            col.addMimeType(mt4);
            col.update();
        }
        for (int i = 0; i < 1000; i++) {
            QByteArray name = QString::fromLatin1("col%1").arg(i+100004).toLatin1();
            Collection col = initializer->createCollection(name.data(), col5);
            col.addMimeType(mt1);
            col.addMimeType(mt2);
            col.update();
        }

        qDebug() << "Created 100000 collections in" << t.elapsed() << "msecs";

        QTest::addColumn<TestScenario::List>("scenarios");

        // {
        //     QList<QByteArray> scenario;
        //     scenario << FakeAkonadiServer::defaultScenario()
        //             << "C: 2 LIST " + QByteArray::number(toplevel.id()) + " INF (ENABLED  ) ()"
        //             << "S: IGNORE 1006"
        //             << "S: 2 OK List completed";
        //     QTest::newRow("recursive list of enabled") << scenario;
        // }
        // {
        //     QList<QByteArray> scenario;
        //     scenario << FakeAkonadiServer::defaultScenario()
        //             << "C: 2 LIST " + QByteArray::number(toplevel.id()) + " INF (MIMETYPE (mimetype1) RESOURCE \"testresource\") ()"
        //             // << "C: 2 LIST " + QByteArray::number(0) + " INF (RESOURCE \"testresource\") ()"
        //             << "S: IGNORE 101005"
        //             << "S: 2 OK List completed";
        //     QTest::newRow("recursive list filtered by mimetype") << scenario;
        // }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(toplevel.id(), Protocol::FetchCollectionsCommand::AllCollections,
                                Protocol::Ancestor::AllAncestors, { QLatin1String("mimetype1") }, QLatin1String("testresource")))
                      << TestScenario::ignore(101005)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchCollectionsResponse());
            QTest::newRow("recursive list filtered by mimetype with ancestors") << scenarios;
        }
    }

    void testListEnabledBenchmark()
    {
        QFETCH(TestScenario::List, scenarios);
        // StorageDebugger::instance()->enableSQLDebugging(true);
        // StorageDebugger::instance()->writeToFile(QLatin1String("sqllog.txt"));

        QBENCHMARK {
            mAkonadi.setScenarios(scenarios);
            mAkonadi.runTest();
        }
    }

#endif
};

AKTEST_FAKESERVER_MAIN(CollectionFetchHandlerTest)

#include "collectionfetchhandlertest.moc"
