/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "private/imapset_p.h"
#include "private/scope_p.h"

#include "aktest.h"
#include "dbinitializer.h"
#include "entities.h"
#include "fakeakonadiserver.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::Tag::List)
Q_DECLARE_METATYPE(Akonadi::Server::Tag)

class FetchHandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;

public:
    FetchHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Tag::List>();

        mAkonadi.setPopulateDb(false);
        mAkonadi.init();
    }

    Protocol::FetchItemsCommandPtr createCommand(const Scope &scope, const Protocol::ScopeContext &ctx = Protocol::ScopeContext())
    {
        auto cmd = Protocol::FetchItemsCommandPtr::create(scope, ctx);
        auto fetchScope = cmd->itemFetchScope();
        fetchScope.setFetch(Protocol::ItemFetchScope::IgnoreErrors);
        cmd->setItemFetchScope(fetchScope);
        return cmd;
    }

    Protocol::FetchItemsResponsePtr createResponse(const PimItem &item)
    {
        auto resp = Protocol::FetchItemsResponsePtr::create(item.id());
        resp->setMimeType(item.mimeType().name());
        resp->setParentId(item.collectionId());

        return resp;
    }

    QScopedPointer<DbInitializer> initializer;

private Q_SLOTS:
    void testFetch_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col = initializer->createCollection("root");
        PimItem item1 = initializer->createItem("item1", col);
        PimItem item2 = initializer->createItem("item2", col);

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item1.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item1))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());

            QTest::newRow("basic fetch") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Collection, col.id())))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item2))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item1))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());
            QTest::newRow("collection context") << scenarios;
        }
    }

    void testFetch()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testFetchByTag_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col = initializer->createCollection("root");
        PimItem item1 = initializer->createItem("item1", col);
        PimItem item2 = initializer->createItem("item2", col);

        Tag tag;
        TagType type;
        type.setName(QStringLiteral("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QStringLiteral("gid"));
        tag.insert();

        item1.addTag(tag);
        item1.update();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Tag, tag.id())))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item1))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());

            QTest::newRow("fetch by tag") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario() << mAkonadi.selectResourceScenario(QStringLiteral("testresource"))
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Tag, tag.id())))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item1))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());

            QTest::newRow("uid fetch by tag from resource") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario() << mAkonadi.selectResourceScenario(QStringLiteral("testresource"))
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Collection, col.id())))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item2))
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item1))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());

            QTest::newRow("fetch collection") << scenarios;
        }
    }

    void testFetchByTag()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testFetchCommandContext_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        PimItem item1 = initializer->createItem("item1", col1);
        Collection col2 = initializer->createCollection("col2");

        Tag tag;
        TagType type;
        type.setName(QStringLiteral("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QStringLiteral("gid"));
        tag.insert();

        item1.addTag(tag);
        item1.update();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario() << mAkonadi.selectResourceScenario(QStringLiteral("testresource"))
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Collection, col2.id())))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create())
                      << TestScenario::create(6,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Tag, tag.id())))
                      << TestScenario::create(6, TestScenario::ServerCmd, createResponse(item1))
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());

            // Special case that used to be broken due to persistent command context
            QTest::newRow("fetch by tag after collection") << scenarios;
        }
    }

    void testFetchCommandContext()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

    void testList_data()
    {
        QElapsedTimer timer;
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");

        timer.start();
        QList<PimItem> items;
        for (int i = 0; i < 1000; i++) {
            items.append(initializer->createItem(QString::number(i).toLatin1().constData(), col1));
        }
        qDebug() << timer.nsecsElapsed() / 1.0e6 << "ms";
        timer.start();

        QTest::addColumn<TestScenario::List>("scenarios");

        {
            TestScenario::List scenarios;
            scenarios << mAkonadi.loginScenario()
                      << TestScenario::create(5,
                                              TestScenario::ClientCmd,
                                              createCommand(ImapSet::all(), Protocol::ScopeContext(Protocol::ScopeContext::Collection, col1.id())));
            while (!items.isEmpty()) {
                const PimItem &item = items.takeLast();
                scenarios << TestScenario::create(5, TestScenario::ServerCmd, createResponse(item));
            }
            scenarios << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchItemsResponsePtr::create());
            QTest::newRow("complete list") << scenarios;
        }
        qDebug() << timer.nsecsElapsed() / 1.0e6 << "ms";
    }

    void testList()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        // StorageDebugger::instance()->enableSQLDebugging(true);
        // StorageDebugger::instance()->writeToFile(QStringLiteral("sqllog.txt"));
        mAkonadi.runTest();
    }
};

AKTEST_FAKESERVER_MAIN(FetchHandlerTest)

#include "fetchhandlertest.moc"
