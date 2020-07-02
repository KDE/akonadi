/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>


#include <storage/selectquerybuilder.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "entities.h"
#include "dbinitializer.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::Relation::List)
Q_DECLARE_METATYPE(Akonadi::Server::Relation)

static Protocol::ChangeNotificationList extractNotifications(const QSharedPointer<QSignalSpy> &notificationSpy)
{
    Protocol::ChangeNotificationList receivedNotifications;
    for (int q = 0; q < notificationSpy->size(); q++) {
        //Only one notify call
        if (notificationSpy->at(q).count() != 1) {
            qWarning() << "Error: We're assuming only one notify call.";
            return Protocol::ChangeNotificationList();
        }
        const Protocol::ChangeNotificationList n = notificationSpy->at(q).first().value<Protocol::ChangeNotificationList>();
        for (int i = 0; i < n.size(); i++) {
            // qDebug() << n.at(i);
            receivedNotifications.append(n.at(i));
        }
    }
    return receivedNotifications;
}

class RelationHandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;
public:
    RelationHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Relation::List>();

        mAkonadi.setPopulateDb(false);
        mAkonadi.init();

        RelationType type;
        type.setName(QStringLiteral("type"));
        type.insert();

        RelationType type2;
        type2.setName(QStringLiteral("type2"));
        type2.insert();
    }

    QScopedPointer<DbInitializer> initializer;

    Protocol::RelationChangeNotificationPtr relationNotification(Protocol::RelationChangeNotification::Operation op,
                                                                 const PimItem &item1,
                                                                 const PimItem &item2,
                                                                 const QString &rid,
                                                                 const QString &type = QStringLiteral("type"))
    {
        auto notification = Protocol::RelationChangeNotificationPtr::create();
        notification->setOperation(op);
        notification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        Protocol::FetchRelationsResponse relation;
        relation.setLeft(item1.id());
        relation.setLeftMimeType(item1.mimeType().name().toLatin1());
        relation.setRight(item2.id());
        relation.setRightMimeType(item2.mimeType().name().toLatin1());
        relation.setRemoteId(rid.toLatin1());
        relation.setType(type.toLatin1());
        notification->setRelation(std::move(relation));
        return notification;
    }

private Q_SLOTS:
    void testStoreRelation_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        PimItem item1 = initializer->createItem("item1", col1);
        PimItem item2 = initializer->createItem("item2", col1);

        QTest::addColumn<TestScenario::List >("scenarios");
        QTest::addColumn<Relation::List>("expectedRelations");
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::ModifyRelationCommandPtr::create(item1.id(), item2.id(), "type"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyRelationResponsePtr::create());

            Relation rel;
            rel.setLeftId(item1.id());
            rel.setRightId(item2.id());
            RelationType type;
            type.setName(QStringLiteral("type"));
            rel.setRelationType(type);

            auto itemNotification = Protocol::ItemChangeNotificationPtr::create();
            itemNotification->setOperation(Protocol::ItemChangeNotification::ModifyRelations);
            itemNotification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            itemNotification->setResource("testresource");
            itemNotification->setParentCollection(col1.id());
            itemNotification->setItems({ *initializer->fetchResponse(item1), *initializer->fetchResponse(item2) });
            itemNotification->setAddedRelations({ Protocol::ItemChangeNotification::Relation(item1.id(), item2.id(), QStringLiteral("type")) });

            const auto notification = relationNotification(Protocol::RelationChangeNotification::Add, item1, item2, rel.remoteId());

            QTest::newRow("uid create relation") << scenarios << (Relation::List() << rel) << (Protocol::ChangeNotificationList() << notification << itemNotification);
        }
    }

    void testStoreRelation()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Relation::List, expectedRelations);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        const auto receivedNotifications = extractNotifications(mAkonadi.notificationSpy());
        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(*receivedNotifications.at(i), *expectedNotifications.at(i));
        }

        const Relation::List relations = Relation::retrieveAll();
        // Q_FOREACH (const Relation &rel, relations) {
        //     akDebug() << rel.leftId() << rel.rightId();
        // }
        QCOMPARE(relations.size(), expectedRelations.size());
        for (int i = 0; i < relations.size(); i++) {
            QCOMPARE(relations.at(i).leftId(), expectedRelations.at(i).leftId());
            QCOMPARE(relations.at(i).rightId(), expectedRelations.at(i).rightId());
            // QCOMPARE(relations.at(i).typeId(), expectedRelations.at(i).typeId());
            QCOMPARE(relations.at(i).remoteId(), expectedRelations.at(i).remoteId());
        }
        QueryBuilder qb(Relation::tableName(), QueryBuilder::Delete);
        qb.exec();
    }

    void testRemoveRelation_data()
    {
        initializer.reset(new DbInitializer);
        QCOMPARE(Relation::retrieveAll().size(), 0);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        PimItem item1 = initializer->createItem("item1", col1);
        PimItem item2 = initializer->createItem("item2", col1);

        Relation rel;
        rel.setLeftId(item1.id());
        rel.setRightId(item2.id());
        rel.setRelationType(RelationType::retrieveByName(QStringLiteral("type")));
        QVERIFY(rel.insert());

        Relation rel2;
        rel2.setLeftId(item1.id());
        rel2.setRightId(item2.id());
        rel2.setRelationType(RelationType::retrieveByName(QStringLiteral("type2")));
        QVERIFY(rel2.insert());

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Relation::List>("expectedRelations");
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::RemoveRelationsCommandPtr::create(item1.id(), item2.id(), "type"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::RemoveRelationsResponsePtr::create());

            auto itemNotification = Protocol::ItemChangeNotificationPtr::create();
            itemNotification->setOperation(Protocol::ItemChangeNotification::ModifyRelations);
            itemNotification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            itemNotification->setResource("testresource");
            itemNotification->setParentCollection(col1.id());
            itemNotification->setItems({ *initializer->fetchResponse(item1), *initializer->fetchResponse(item2) });
            itemNotification->setRemovedRelations({ Protocol::ItemChangeNotification::Relation(item1.id(), item2.id(), QStringLiteral("type")) });

            const auto notification = relationNotification(Protocol::RelationChangeNotification::Remove, item1, item2, rel.remoteId());

            QTest::newRow("uid remove relation") << scenarios << (Relation::List() << rel2) << (Protocol::ChangeNotificationList() << notification << itemNotification);
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::RemoveRelationsCommandPtr::create(item1.id(), item2.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::RemoveRelationsResponsePtr::create());

            auto itemNotification = Protocol::ItemChangeNotificationPtr::create();
            itemNotification->setOperation(Protocol::ItemChangeNotification::ModifyRelations);
            itemNotification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            itemNotification->setResource("testresource");
            itemNotification->setParentCollection(col1.id());
            itemNotification->setItems({ *initializer->fetchResponse(item1), *initializer->fetchResponse(item2) });
            itemNotification->setRemovedRelations({ Protocol::ItemChangeNotification::Relation(item1.id(), item2.id(), QStringLiteral("type2")) });

            const auto notification = relationNotification(Protocol::RelationChangeNotification::Remove, item1, item2, rel.remoteId(), QStringLiteral("type2"));

            QTest::newRow("uid remove relation without type") << scenarios << Relation::List() << (Protocol::ChangeNotificationList() << notification << itemNotification);
        }

    }

    void testRemoveRelation()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Relation::List, expectedRelations);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        const auto receivedNotifications = extractNotifications(mAkonadi.notificationSpy());
        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(*receivedNotifications.at(i), *expectedNotifications.at(i));
        }

        const Relation::List relations = Relation::retrieveAll();
        // Q_FOREACH (const Relation &rel, relations) {
        //     akDebug() << rel.leftId() << rel.rightId() << rel.relationType().name() << rel.remoteId();
        // }
        QCOMPARE(relations.size(), expectedRelations.size());
        for (int i = 0; i < relations.size(); i++) {
            QCOMPARE(relations.at(i).leftId(), expectedRelations.at(i).leftId());
            QCOMPARE(relations.at(i).rightId(), expectedRelations.at(i).rightId());
            QCOMPARE(relations.at(i).typeId(), expectedRelations.at(i).typeId());
            QCOMPARE(relations.at(i).remoteId(), expectedRelations.at(i).remoteId());
        }
    }

    void testListRelation_data()
    {
        QueryBuilder qb(Relation::tableName(), QueryBuilder::Delete);
        qb.exec();

        initializer.reset(new DbInitializer);
        QCOMPARE(Relation::retrieveAll().size(), 0);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        PimItem item1 = initializer->createItem("item1", col1);
        PimItem item2 = initializer->createItem("item2", col1);
        PimItem item3 = initializer->createItem("item3", col1);
        PimItem item4 = initializer->createItem("item4", col1);

        Relation rel;
        rel.setLeftId(item1.id());
        rel.setRightId(item2.id());
        rel.setRelationType(RelationType::retrieveByName(QStringLiteral("type")));
        rel.setRemoteId(QStringLiteral("foobar1"));
        QVERIFY(rel.insert());

        Relation rel2;
        rel2.setLeftId(item1.id());
        rel2.setRightId(item2.id());
        rel2.setRelationType(RelationType::retrieveByName(QStringLiteral("type2")));
        rel2.setRemoteId(QStringLiteral("foobar2"));
        QVERIFY(rel2.insert());

        Relation rel3;
        rel3.setLeftId(item3.id());
        rel3.setRightId(item4.id());
        rel3.setRelationType(RelationType::retrieveByName(QStringLiteral("type")));
        rel3.setRemoteId(QStringLiteral("foobar3"));
        QVERIFY(rel3.insert());

        Relation rel4;
        rel4.setLeftId(item4.id());
        rel4.setRightId(item3.id());
        rel4.setRelationType(RelationType::retrieveByName(QStringLiteral("type")));
        rel4.setRemoteId(QStringLiteral("foobar4"));
        QVERIFY(rel4.insert());

        QTest::addColumn<TestScenario::List>("scenarios");
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create(-1, QVector<QByteArray>{ "type" }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type", "foobar1"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item3.id(), item3.mimeType().name().toUtf8(), item4.id(), item4.mimeType().name().toUtf8(), "type", "foobar3"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item4.id(), item4.mimeType().name().toUtf8(), item3.id(), item3.mimeType().name().toUtf8(), "type", "foobar4"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("filter by type") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create())
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type", "foobar1"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type2", "foobar2"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item3.id(), item3.mimeType().name().toUtf8(), item4.id(), item4.mimeType().name().toUtf8(), "type", "foobar3"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item4.id(), item4.mimeType().name().toUtf8(), item3.id(), item3.mimeType().name().toUtf8(), "type", "foobar4"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("no filter") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create(-1, QVector<QByteArray>{}, QLatin1String("testresource")))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type", "foobar1"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type2", "foobar2"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item3.id(), item3.mimeType().name().toUtf8(), item4.id(), item4.mimeType().name().toUtf8(), "type", "foobar3"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item4.id(), item4.mimeType().name().toUtf8(), item3.id(), item3.mimeType().name().toUtf8(), "type", "foobar4"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("filter by resource with matching resource") << scenarios;
        }
        {

            Resource res;
            res.setName(QStringLiteral("testresource2"));
            res.insert();

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create(-1, QVector<QByteArray>{}, QLatin1String("testresource2")))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("filter by resource with nonmatching resource") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create(item1.id(), -1, QVector<QByteArray>{ "type" }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type", "foobar1"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("filter by left and type") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create(-1, item2.id(), QVector<QByteArray>{ "type" }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item1.id(), item1.mimeType().name().toUtf8(), item2.id(), item2.mimeType().name().toUtf8(), "type", "foobar1"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("filter by right and type") << scenarios;
        }
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::FetchRelationsCommandPtr::create(item3.id(), QVector<QByteArray>{ "type" }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item3.id(), item3.mimeType().name().toUtf8(), item4.id(), item4.mimeType().name().toUtf8(), "type", "foobar3"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create(item4.id(), item4.mimeType().name().toUtf8(), item3.id(), item3.mimeType().name().toUtf8(), "type", "foobar4"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::FetchRelationsResponsePtr::create());

            QTest::newRow("fetch by side with typefilter") << scenarios;
        }
    }

    void testListRelation()
    {
        QFETCH(TestScenario::List, scenarios);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();
    }

};

AKTEST_FAKESERVER_MAIN(RelationHandlerTest)

#include "relationhandlertest.moc"
