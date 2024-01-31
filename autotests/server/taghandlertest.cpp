/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include <storage/selectquerybuilder.h>

#include "aktest.h"
#include "dbinitializer.h"
#include "entities.h"
#include "fakeakonadiserver.h"

#include "private/scope_p.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

typedef QPair<Tag, TagAttribute::List> TagTagAttributeListPair;

Q_DECLARE_METATYPE(Akonadi::Server::Tag::List)
Q_DECLARE_METATYPE(Akonadi::Server::Tag)
Q_DECLARE_METATYPE(QList<TagTagAttributeListPair>)

static Protocol::ChangeNotificationList extractNotifications(const QSharedPointer<QSignalSpy> &notificationSpy)
{
    Protocol::ChangeNotificationList receivedNotifications;
    for (int q = 0; q < notificationSpy->size(); q++) {
        // Only one notify call
        if (notificationSpy->at(q).count() != 1) {
            qWarning() << "Error: We're assuming only one notify call.";
            return Protocol::ChangeNotificationList();
        }
        const auto n = notificationSpy->at(q).first().value<Protocol::ChangeNotificationList>();
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

    FakeAkonadiServer mAkonadi;

public:
    TagHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Tag::List>();

        mAkonadi.setPopulateDb(false);
        mAkonadi.init();
    }

    Protocol::FetchTagsResponsePtr
    createResponse(const Tag &tag, const QByteArray &remoteId = QByteArray(), const Protocol::Attributes &attrs = Protocol::Attributes())
    {
        auto resp = Protocol::FetchTagsResponsePtr::create(tag.id());
        resp->setGid(tag.gid().toUtf8());
        resp->setParentId(tag.parentId());
        resp->setType(tag.tagType().name().toUtf8());
        resp->setRemoteId(remoteId);
        resp->setAttributes(attrs);
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
        QTest::addColumn<QList<QPair<Tag, TagAttribute::List>>>("expectedTags");
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");

        {
            auto cmd = Protocol::CreateTagCommandPtr::create();
            cmd->setGid("tag");
            cmd->setParentId(0);
            cmd->setType("PLAIN");
            cmd->setAttributes({{"TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")"}});

            auto resp = Protocol::FetchTagsResponsePtr::create(1);
            resp->setGid(cmd->gid());
            resp->setParentId(cmd->parentId());
            resp->setType(cmd->type());
            resp->setAttributes(cmd->attributes());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateTagResponsePtr::create());

            Tag tag;
            tag.setId(1);
            tag.setTagType(type);
            tag.setParentId(0);

            TagAttribute attribute;
            attribute.setTagId(1);
            attribute.setType("TAG");
            attribute.setValue("(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")");

            auto notification = Protocol::TagChangeNotificationPtr::create();
            notification->setOperation(Protocol::TagChangeNotification::Add);
            notification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification->setTag(Protocol::FetchTagsResponse(1));

            QTest::newRow("uid create relation") << scenarios << QList<TagTagAttributeListPair>{{tag, {attribute}}}
                                                 << Protocol::ChangeNotificationList{notification};
        }

        {
            auto cmd = Protocol::CreateTagCommandPtr::create();
            cmd->setGid("tag2");
            cmd->setParentId(1);
            cmd->setType("PLAIN");
            cmd->setAttributes({{"TAG", "(\\\"tag3\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")"}});

            auto resp = Protocol::FetchTagsResponsePtr::create(2);
            resp->setGid(cmd->gid());
            resp->setParentId(cmd->parentId());
            resp->setType(cmd->type());
            resp->setAttributes(cmd->attributes());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, resp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateTagResponsePtr::create());

            Tag tag;
            tag.setId(2);
            tag.setTagType(type);
            tag.setParentId(1);

            TagAttribute attribute;
            attribute.setTagId(2);
            attribute.setType("TAG");
            attribute.setValue("(\\\"tag3\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")");

            auto notification = Protocol::TagChangeNotificationPtr::create();
            notification->setOperation(Protocol::TagChangeNotification::Add);
            notification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification->setTag(Protocol::FetchTagsResponse(2));

            QTest::newRow("create child tag") << scenarios << QList<TagTagAttributeListPair>{{tag, {attribute}}}
                                              << Protocol::ChangeNotificationList{notification};
        }
    }

    void testStoreTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(QList<TagTagAttributeListPair>, expectedTags);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        const auto receivedNotifications = extractNotifications(mAkonadi.notificationSpy());

        QVariantList ids;
        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(*receivedNotifications.at(i), *expectedNotifications.at(i));
            ids << Protocol::cmdCast<Protocol::TagChangeNotification>(receivedNotifications.at(i)).tag().id();
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

            TagAttribute::List attributes = TagAttribute::retrieveFiltered(TagAttribute::tagIdColumn(), tags.at(i).id());
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
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");
        {
            auto cmd = Protocol::ModifyTagCommandPtr::create(tag.id());
            cmd->setAttributes({{"TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")"}});

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(tag, QByteArray(), cmd->attributes()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponsePtr::create());

            auto notification = Protocol::TagChangeNotificationPtr::create();
            notification->setOperation(Protocol::TagChangeNotification::Modify);
            notification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification->setTag(Protocol::FetchTagsResponse(tag.id()));

            QTest::newRow("uid store name") << scenarios << (Tag::List() << tag) << (Protocol::ChangeNotificationList() << notification);
        }

        {
            auto cmd = Protocol::ModifyTagCommandPtr::create(tag.id());
            cmd->setRemoteId("remote1");

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << FakeAkonadiServer::selectResourceScenario(QStringLiteral("testresource"))
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5,
                                              TestScenario::ServerCmd,
                                              createResponse(tag, "remote1", {{"TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")"}}))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponsePtr::create());

            // RID-only changes don't emit notifications
            /*
            Akonadi::Protocol::ChangeNotification notification;
            notification.setType(Protocol::ChangeNotification::Tags);
            notification.setOperation(Protocol::ChangeNotification::Modify);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.addEntity(tag.id());
            */

            QTest::newRow("uid store rid") << scenarios << (Tag::List() << tag) << Protocol::ChangeNotificationList();
        }

        {
            auto cmd = Protocol::ModifyTagCommandPtr::create(tag.id());
            cmd->setRemoteId(QByteArray());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << FakeAkonadiServer::selectResourceScenario(res.name())
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(
                             5,
                             TestScenario::ServerCmd,
                             createResponse(tag, QByteArray(), {{"TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")"}}))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponsePtr::create());

            // RID-only changes don't emit notifications
            /*
            Akonadi::Protocol::ChangeNotification tagChangeNtf;
            tagChangeNtf.setType(Protocol::ChangeNotification::Tags);
            tagChangeNtf.setOperation(Protocol::ChangeNotification::Modify);
            tagChangeNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            tagChangeNtf.addEntity(tag.id());
            */

            QTest::newRow("uid store unset one rid") << scenarios << (Tag::List() << tag) << Protocol::ChangeNotificationList();
        }

        {
            auto cmd = Protocol::ModifyTagCommandPtr::create(tag.id());
            cmd->setRemoteId(QByteArray());

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << FakeAkonadiServer::selectResourceScenario(res2.name())
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::DeleteTagResponsePtr::create())
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponsePtr::create());

            auto itemUntaggedNtf = Protocol::ItemChangeNotificationPtr::create();
            itemUntaggedNtf->setOperation(Protocol::ItemChangeNotification::ModifyTags);
            itemUntaggedNtf->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            itemUntaggedNtf->setItems({*initializer->fetchResponse(pimItem)});
            itemUntaggedNtf->setResource(res2.name().toLatin1());
            itemUntaggedNtf->setParentCollection(col.id());
            itemUntaggedNtf->setRemovedTags(QSet<qint64>() << tag.id());

            auto tagRemoveNtf = Protocol::TagChangeNotificationPtr::create();
            tagRemoveNtf->setOperation(Protocol::TagChangeNotification::Remove);
            tagRemoveNtf->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            Protocol::FetchTagsResponse ntfTag;
            ntfTag.setId(tag.id());
            ntfTag.setGid("gid");
            ntfTag.setType("PLAIN");
            tagRemoveNtf->setTag(std::move(ntfTag));

            QTest::newRow("uid store unset last rid") << scenarios << Tag::List() << (Protocol::ChangeNotificationList() << itemUntaggedNtf << tagRemoveNtf);
        }
    }

    void testModifyTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        const auto receivedNotifications = extractNotifications(mAkonadi.notificationSpy());

        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < receivedNotifications.size(); i++) {
            qDebug() << Protocol::debugString(receivedNotifications.at(i));
            qDebug() << Protocol::debugString(expectedNotifications.at(i));
            QCOMPARE(*receivedNotifications.at(i), *expectedNotifications.at(i));
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

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Tag::List>("expectedTags");
        QTest::addColumn<Protocol::ChangeNotificationList>("expectedNotifications");
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, Protocol::DeleteTagCommandPtr::create(tag.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::DeleteTagResponsePtr::create());

            auto ntf = Protocol::TagChangeNotificationPtr::create();
            ntf->setOperation(Protocol::TagChangeNotification::Remove);
            ntf->setSessionId(FakeAkonadiServer::instanceName().toLatin1());

            auto res1Ntf = Protocol::TagChangeNotificationPtr::create(*ntf);
            Protocol::FetchTagsResponse res1NtfTag;
            res1NtfTag.setId(tag.id());
            res1NtfTag.setRemoteId(rel1.remoteId().toLatin1());
            res1Ntf->setTag(std::move(res1NtfTag));
            res1Ntf->setResource(res1.name().toLatin1());

            auto res2Ntf = Protocol::TagChangeNotificationPtr::create(*ntf);
            Protocol::FetchTagsResponse res2NtfTag;
            res2NtfTag.setId(tag.id());
            res2NtfTag.setRemoteId(rel2.remoteId().toLatin1());
            res2Ntf->setTag(std::move(res2NtfTag));
            res2Ntf->setResource(res2.name().toLatin1());

            auto clientNtf = Protocol::TagChangeNotificationPtr::create(*ntf);
            clientNtf->setTag(Protocol::FetchTagsResponse(tag.id()));

            QTest::newRow("uid remove") << scenarios << Tag::List() << (Protocol::ChangeNotificationList() << res1Ntf << res2Ntf << clientNtf);
        }
    }

    void testRemoveTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(Protocol::ChangeNotificationList, expectedNotifications);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        const auto receivedNotifications = extractNotifications(mAkonadi.notificationSpy());

        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < receivedNotifications.size(); i++) {
            QCOMPARE(*receivedNotifications.at(i), *expectedNotifications.at(i));
        }

        const Tag::List tags = Tag::retrieveAll();
        QCOMPARE(tags.size(), 0);
    }
};

AKTEST_FAKESERVER_MAIN(TagHandlerTest)

#include "taghandlertest.moc"
