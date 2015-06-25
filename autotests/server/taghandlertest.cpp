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
#include "akdebug.h"
#include "entities.h"
#include "dbinitializer.h"

#include <private/scope_p.h>

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::Tag::List);
Q_DECLARE_METATYPE(Akonadi::Server::Tag);

static NotificationMessageV3::List extractNotifications(QSignalSpy *notificationSpy)
{
    NotificationMessageV3::List receivedNotifications;
    for (int q = 0; q < notificationSpy->size(); q++) {
        //Only one notify call
        if (notificationSpy->at(q).count() != 1) {
            qWarning() << "Error: We're assuming only one notify call.";
            return NotificationMessageV3::List();
        }
        const NotificationMessageV3::List n = notificationSpy->at(q).first().value<NotificationMessageV3::List>();
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
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
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

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Tag::List>("expectedTags");
        QTest::addColumn<Akonadi::NotificationMessageV3::List>("expectedNotifications");

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
            TagType type;
            type.setName(QLatin1String("PLAIN"));
            tag.setTagType(type);

            Akonadi::NotificationMessageV3 notification;
            notification.setType(NotificationMessageV2::Tags);
            notification.setOperation(NotificationMessageV2::Add);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.addEntity(1);

            QTest::newRow("uid create relation") << scenarios << (Tag::List() << tag) << (NotificationMessageV3::List() << notification);
        }
    }

    void testStoreTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(NotificationMessageV3::List, expectedNotifications);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        const NotificationMessageV3::List receivedNotifications = extractNotifications(FakeAkonadiServer::instance()->notificationSpy());

        QCOMPARE(receivedNotifications.size(), expectedNotifications.count());
        for (int i = 0; i < expectedNotifications.size(); i++) {
            QCOMPARE(receivedNotifications.at(i), expectedNotifications.at(i));
        }

        const Tag::List tags = Tag::retrieveAll();
        QCOMPARE(tags.size(), expectedTags.size());
        for (int i = 0; i < tags.size(); i++) {
            QCOMPARE(tags.at(i).id(), expectedTags.at(i).id());
            QCOMPARE(tags.at(i).tagType().name(), QString::fromLatin1("PLAIN")/* expectedTags.at(i).tagType().name() */);
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
        type.setName(QLatin1String("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QLatin1String("gid"));
        tag.insert();

        pimItem.addTag(tag);

        TagRemoteIdResourceRelation rel;
        rel.setRemoteId(QLatin1String("TAG1RES2RID"));
        rel.setResource(res2);
        rel.setTag(tag);
        rel.insert();

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Tag::List>("expectedTags");
        QTest::addColumn<Akonadi::NotificationMessageV3::List>("expectedNotifications");
        {
            Protocol::ModifyTagCommand cmd(tag.id());
            cmd.setAttributes({ { "TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } });

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(tag, QByteArray(), cmd.attributes()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponse());

            Akonadi::NotificationMessageV3 notification;
            notification.setType(NotificationMessageV2::Tags);
            notification.setOperation(NotificationMessageV2::Modify);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.addEntity(tag.id());

            QTest::newRow("uid store name") << scenarios << (Tag::List() << tag) << (NotificationMessageV3::List() << notification);
        }

        {
            Protocol::ModifyTagCommand cmd(tag.id());
            cmd.setRemoteId("remote1");

            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << FakeAkonadiServer::selectResourceScenario(QLatin1String("testresource"))
                      << TestScenario::create(5, TestScenario::ClientCmd, cmd)
                      << TestScenario::create(5, TestScenario::ServerCmd, createResponse(tag, "remote1",
                            { { "TAG", "(\\\"tag2\\\" \\\"\\\" \\\"\\\" \\\"\\\" \\\"0\\\" () () \\\"-1\\\")" } }))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyTagResponse());

            // RID-only changes don't emit notifications
            /*
            Akonadi::NotificationMessageV3 notification;
            notification.setType(NotificationMessageV2::Tags);
            notification.setOperation(NotificationMessageV2::Modify);
            notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            notification.addEntity(tag.id());
            */

            QTest::newRow("uid store rid") << scenarios << (Tag::List() << tag) << NotificationMessageV3::List();
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
            Akonadi::NotificationMessageV3 tagChangeNtf;
            tagChangeNtf.setType(NotificationMessageV2::Tags);
            tagChangeNtf.setOperation(NotificationMessageV2::Modify);
            tagChangeNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            tagChangeNtf.addEntity(tag.id());
            */

            QTest::newRow("uid store unset one rid") << scenarios << (Tag::List() << tag) << NotificationMessageV3::List();
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

            Akonadi::NotificationMessageV3 itemUntaggedNtf;
            itemUntaggedNtf.setType(NotificationMessageV2::Items);
            itemUntaggedNtf.setOperation(NotificationMessageV2::ModifyTags);
            itemUntaggedNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            itemUntaggedNtf.addEntity(pimItem.id(), pimItem.remoteId(), QString(), pimItem.mimeType().name());
            itemUntaggedNtf.setResource(res2.name().toLatin1());
            itemUntaggedNtf.setParentCollection(col.id());
            itemUntaggedNtf.setRemovedTags(QSet<qint64>() << tag.id());

            Akonadi::NotificationMessageV3 tagRemoveNtf;
            tagRemoveNtf.setType(NotificationMessageV2::Tags);
            tagRemoveNtf.setOperation(NotificationMessageV2::Remove);
            tagRemoveNtf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
            tagRemoveNtf.addEntity(tag.id());

            QTest::newRow("uid store unset last rid") << scenarios << Tag::List() << (NotificationMessageV3::List() << itemUntaggedNtf << tagRemoveNtf);
        }
    }

    void testModifyTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(NotificationMessageV3::List, expectedNotifications);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        const NotificationMessageV3::List receivedNotifications = extractNotifications(FakeAkonadiServer::instance()->notificationSpy());

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
        type.setName(QLatin1String("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QLatin1String("gid2"));
        tag.insert();

        TagRemoteIdResourceRelation rel1;
        rel1.setRemoteId(QLatin1String("TAG2RES1RID"));
        rel1.setResource(res1);
        rel1.setTag(tag);
        rel1.insert();

        TagRemoteIdResourceRelation rel2;
        rel2.setRemoteId(QLatin1String("TAG2RES2RID"));
        rel2.setResource(res2);
        rel2.setTag(tag);
        rel2.insert();

        QTest::addColumn<TestScenario::List >("scenarios");
        QTest::addColumn<Tag::List>("expectedTags");
        QTest::addColumn<Akonadi::NotificationMessageV3::List>("expectedNotifications");
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::DeleteTagCommand(tag.id()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::DeleteTagResponse());

            Akonadi::NotificationMessageV3 ntf;
            ntf.setType(NotificationMessageV2::Tags);
            ntf.setOperation(NotificationMessageV2::Remove);
            ntf.setSessionId(FakeAkonadiServer::instanceName().toLatin1());

            Akonadi::NotificationMessageV3 res1Ntf = ntf;
            res1Ntf.addEntity(tag.id(), rel1.remoteId());
            res1Ntf.setResource(res1.name().toLatin1());

            Akonadi::NotificationMessageV3 res2Ntf = ntf;
            res2Ntf.addEntity(tag.id(), rel2.remoteId());
            res2Ntf.setResource(res2.name().toLatin1());

            Akonadi::NotificationMessageV3 clientNtf = ntf;
            clientNtf.addEntity(tag.id());

            QTest::newRow("uid remove") << scenarios << Tag::List() << (NotificationMessageV3::List() << res1Ntf << res2Ntf << clientNtf);
        }
    }

    void testRemoveTag()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(Tag::List, expectedTags);
        QFETCH(Akonadi::NotificationMessageV3::List, expectedNotifications);

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        const NotificationMessageV3::List receivedNotifications = extractNotifications(FakeAkonadiServer::instance()->notificationSpy());

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
