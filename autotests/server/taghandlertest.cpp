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


#include <storage/selectquerybuilder.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "entities.h"
#include "dbinitializer.h"

#include <private/scope_p.h>

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

typedef QPair<Tag, TagAttribute::List> TagTagAttributeListPair;

Q_DECLARE_METATYPE(Akonadi::Server::Tag::List)
Q_DECLARE_METATYPE(Akonadi::Server::Tag)
Q_DECLARE_METATYPE(QVector<TagTagAttributeListPair>)

static Protocol::ChangeNotification::List extractNotifications(QSignalSpy *notificationSpy)
{
    Protocol::ChangeNotification::List receivedNotifications;
    for (int q = 0; q < notificationSpy->size(); q++) {
        //Only one notify call
        if (notificationSpy->at(q).count() != 1) {
            qWarning() << "Error: We're assuming only one notify call.";
            return Protocol::ChangeNotification::List();
        }
        const Protocol::ChangeNotification::List n = notificationSpy->at(q).first().value<Protocol::ChangeNotification::List>();
        for (int i = 0; i < n.size(); i++) {
            // qDebug() << n.at(i);
            receivedNotifications.append(n.at(i));
        }
    }
    return receivedNotifications;
}

class TagHandlerTest : public QObject
{
    Q_OBJECT

public:
    TagHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Tag::List>();

        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }
    }

    ~TagHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

    Protocol::FetchTagsResponse createResponse(const Tag &tag, const QByteArray &remoteId = QByteArray(),
                                               const Protocol::Attributes &attrs = Protocol::Attributes())
    {
        Protocol::FetchTagsResponse resp(tag.id());
        resp.setGid(tag.gid().toUtf8());
        resp.setParentId(tag.parentId());
        resp.setType(tag.tagType().name().toUtf8());
        resp.setRemoteId(remoteId);
        resp.setAttributes(attrs);
        return resp;
    }

    QScopedPointer<DbInitializer> initializer;

private Q_SLOTS:
    void testStoreTag_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");

        // Make sure the type exists
        TagType type = type.retrieveByName(QStringLiteral("PLAIN"));
        if (!type.isValid()) {
            type.setName(QStringLiteral("PLAIN"));
            type.insert();
        }

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<QVector<QPair<Tag, TagAttribute::List>>>("expectedTags");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::List>("expectedNotifications");

        {
            Protocol::CreateTagCommand cmd;
            cmd.setGid("tag");
            cmd.setParentId(0);
            cmd.setType("PLAIN");
            cmd.setAttributes({ { "TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } });

            Protocol::FetchTagsResponse resp(1);
            resp.setGid(cmd.gid());
            resp.setParentId(cmd.parentId());
            resp.setType(cmd.type());
            resp.setAttributes(cmd.attributes());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateTagResponse());

            Tag tag;
            tag.setId(1);
            tag.setTagType(type);
            tag.setParentId(0);

            TagAttribute attribute;
            attribute.setTagId(1);
            attribute.setType("TAG");
            attribute.setValue("(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")");

            Akonadi::Protocol::TagChangeNotification notification;
            notification.setOperation(Protocol::TagChangeNotification::Add);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.setId(1);

            QTest::newRow("uid create relation") << scenarios
                                                 << QVector<TagTagAttributeListPair>{ { tag, { attribute } } }
                                                 << Protocol::ChangeNotification::List{ notification };
        }

        {
            Protocol::CreateTagCommand cmd;
            cmd.setGid("tag2");
            cmd.setParentId(1);
            cmd.setType("PLAIN");
            cmd.setAttributes({ { "TAG", "(\\\"tag3\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } });

            Protocol::FetchTagsResponse resp(2);
            resp.setGid(cmd.gid());
            resp.setParentId(cmd.parentId());
            resp.setType(cmd.type());
            resp.setAttributes(cmd.attributes());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateTagResponse());

            Tag tag;
            tag.setId(2);
            tag.setTagType(type);
            tag.setParentId(1);

            TagAttribute attribute;
            attribute.setTagId(2);
            attribute.setType("TAG");
            attribute.setValue("(\\\"tag3\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")");

            Akonadi::Protocol::TagChangeNotification notification;
            notification.setOperation(Protocol::TagChangeNotification::Add);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.setId(2);

            QTest::newRow("create child tag") << scenarios
                                              << QVector<TagTagAttributeListPair>{ { tag, { attribute } } }
                                              << Protocol::ChangeNotification::List{ notification };
        }
    }

    void testStoreTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(QVector<TagTagAttributeListPair>, expectedTags);
        QFETCH(Protocol::ChangeNotification::List, expectedNotifications);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        const Protocol::ChangeNotification::List receivedNotifications = extractNotifications(FakeAkonadiServer::instance()->notificationSpy());

        QVariantList ids;
        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(receivedNotifications.at(i), expectedNotifications.at(i));
            ids << Protocol::TagChangeNotification(receivedNotifications.at(i)).id();
        }

        SelectQueryBuilder<Tag> qb;
        qb.addValueCondition(Tag::idColumn(), Query::In, ids);
        QVERIFY(qb.exec());
        const Tag::List tags = qb.result();
        QCOMPARE(tags.size(), expectedTags.size());
        for (int i = 0; i < tags.size(); i++) {
            const Tag actual = tags.at(i);
            const Tag expected = expectedTags.at(i).first;
            const TagAttribute::List expectedAttrs = expectedTags.at(i).second;

            QCOMPARE(actual.id(), expected.id());
            QCOMPARE(actual.typeId(), expected.typeId());
            QCOMPARE(actual.parentId(), expected.parentId());

            TagAttribute::List attributes = TagAttribute::retrieveFiltered(
                TagAttribute::tagIdColumn(), tags.at(i).id());
            QCOMPARE(attributes.size(), expectedAttrs.size());
            for (int j = 0; j < attributes.size(); ++j) {
                const TagAttribute actualAttr = attributes.at(i);
                const TagAttribute expectedAttr = expectedAttrs.at(i);

                QCOMPARE(actualAttr.tagId(), expectedAttr.tagId());
                QCOMPARE(actualAttr.type(), expectedAttr.type());
                QCOMPARE(actualAttr.value(), expectedAttr.value());
            }
        }
    }

    void testModifyTag_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Resource res2 = initializer->createResource("testresource2");
        Collection col = initializer->createCollection("Col 1");
        PimItem pimItem = initializer->createItem("Item 1", col);

        Tag tag;
        TagType type;
        type.setName(QStringLiteral("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QStringLiteral("gid"));
        tag.insert();

        pimItem.addTag(tag);

        TagRemoteIdResourceRelation rel;
        rel.setRemoteId(QStringLiteral("TAG1RES2RID"));
        rel.setResource(res2);
        rel.setTag(tag);
        rel.insert();

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Tag::List>("expectedTags");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::List>("expectedNotifications");
        {
            Protocol::ModifyTagCommand cmd(tag.id());
            cmd.setAttributes({ { "TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } });

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(tag, QByteArray(), cmd.attributes()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponse());

            Akonadi::Protocol::TagChangeNotification notification;
            notification.setOperation(Protocol::TagChangeNotification::Modify);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.setId(tag.id());

            QTest::newRow("uid store name") << scenarios << (Tag::List() << tag) << (Protocol::ChangeNotification::List() << notification);
        }

        {
            Protocol::ModifyTagCommand cmd(tag.id());
            cmd.setRemoteId("remote1");

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << FakeAkonadiServer::selectResourceScenario(QStringLiteral("testresource"))
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(tag, "remote1",
                            { { "TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponse());

            // RID-only changes don't emit notifications
            /*
            Akonadi::Protocol::ChangeNotification notification;
            notification.setType(Protocol::ChangeNotification::Tags);
            notification.setOperation(Protocol::ChangeNotification::Modify);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.addEntity(tag.id());
            */

            QTest::newRow("uid store rid") << scenarios << (Tag::List() << tag) << Protocol::ChangeNotification::List();
        }

        {
            Protocol::ModifyTagCommand cmd(tag.id());
            cmd.setRemoteId(QByteArray());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << FakeAkonadiServer::selectResourceScenario(res.name())
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(tag, QByteArray(),
                            { { "TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponse());

            // RID-only changes don't emit notifications
            /*
            Akonadi::Protocol::ChangeNotification tagChangeNtf;
            tagChangeNtf.setType(Protocol::ChangeNotification::Tags);
            tagChangeNtf.setOperation(Protocol::ChangeNotification::Modify);
            tagChangeNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            tagChangeNtf.addEntity(tag.id());
            */

            QTest::newRow("uid store unset one rid") << scenarios << (Tag::List() << tag) << Protocol::ChangeNotification::List();
        }

       {
            Protocol::ModifyTagCommand cmd(tag.id());
            cmd.setRemoteId(QByteArray());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << FakeAkonadiServer::selectResourceScenario(res2.name())
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::DeleteTagResponse())
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponse());

            Akonadi::Protocol::ItemChangeNotification itemUntaggedNtf;
            itemUntaggedNtf.setOperation(Protocol::ItemChangeNotification::ModifyTags);
            itemUntaggedNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            itemUntaggedNtf.addItem(pimItem.id(), pimItem.remoteId(), QString(), pimItem.mimeType().name());
            itemUntaggedNtf.setResource(res2.name().toLatin1());
            itemUntaggedNtf.setParentCollection(col.id());
            itemUntaggedNtf.setRemovedTags(QSet<qint64>() << tag.id());

            Akonadi::Protocol::TagChangeNotification tagRemoveNtf;
            tagRemoveNtf.setOperation(Protocol::TagChangeNotification::Remove);
            tagRemoveNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            tagRemoveNtf.setId(tag.id());

            QTest::newRow("uid store unset last rid") << scenarios << Tag::List() << (Protocol::ChangeNotification::List() << itemUntaggedNtf << tagRemoveNtf);
        }
    }

    void testModifyTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(Protocol::ChangeNotification::List, expectedNotifications);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        const Protocol::ChangeNotification::List receivedNotifications = extractNotifications(FakeAkonadiServer::instance()->notificationSpy());

        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < receivedNotifications.size(); i++) {
            QCOMPARE(receivedNotifications.at(i), expectedNotifications.at(i));
        }

        const Tag::List tags = Tag::retrieveAll();
        QCOMPARE(tags.size(), expectedTags.size());
        for (int i = 0; i < tags.size(); i++) {
            QCOMPARE(tags.at(i).id(), expectedTags.at(i).id());
            QCOMPARE(tags.at(i).tagType().name(), expectedTags.at(i).tagType().name());
        }
    }

    void testRemoveTag_data()
    {
        initializer.reset(new DbInitializer);
        Resource res1 = initializer->createResource("testresource3");
        Resource res2 = initializer->createResource("testresource4");

        Tag tag;
        TagType type;
        type.setName(QStringLiteral("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QStringLiteral("gid2"));
        tag.insert();

        TagRemoteIdResourceRelation rel1;
        rel1.setRemoteId(QStringLiteral("TAG2RES1RID"));
        rel1.setResource(res1);
        rel1.setTag(tag);
        rel1.insert();

        TagRemoteIdResourceRelation rel2;
        rel2.setRemoteId(QStringLiteral("TAG2RES2RID"));
        rel2.setResource(res2);
        rel2.setTag(tag);
        rel2.insert();

        QTest::addColumn<TestScenario::List >("scenarios");
        QTest::addColumn<Tag::List>("expectedTags");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::List>("expectedNotifications");
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::DeleteTagCommand(tag.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::DeleteTagResponse());

            Akonadi::Protocol::TagChangeNotification ntf;
            ntf.setOperation(Protocol::TagChangeNotification::Remove);
            ntf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());

            Akonadi::Protocol::TagChangeNotification res1Ntf = ntf;
            res1Ntf.setId(tag.id());
            res1Ntf.setRemoteId(rel1.remoteId());
            res1Ntf.setResource(res1.name().toLatin1());

            Akonadi::Protocol::TagChangeNotification res2Ntf = ntf;
            res2Ntf.setId(tag.id());
            res2Ntf.setRemoteId(rel2.remoteId());
            res2Ntf.setResource(res2.name().toLatin1());

            Akonadi::Protocol::TagChangeNotification clientNtf = ntf;
            clientNtf.setId(tag.id());

            QTest::newRow("uid remove") << scenarios << Tag::List() << (Protocol::ChangeNotification::List() << res1Ntf << res2Ntf << clientNtf);
        }
    }

    void testRemoveTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(Akonadi::Protocol::ChangeNotification::List, expectedNotifications);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        const Protocol::ChangeNotification::List receivedNotifications = extractNotifications(FakeAkonadiServer::instance()->notificationSpy());

        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < receivedNotifications.size(); i++) {
            QCOMPARE(receivedNotifications.at(i), expectedNotifications.at(i));
        }

        const Tag::List tags = Tag::retrieveAll();
        QCOMPARE(tags.size(), 0);
    }
};

AKTEST_FAKESERVER_MAIN(TagHandlerTest)

#include "taghandlertest.moc"
