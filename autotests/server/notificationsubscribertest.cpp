/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include <aktest.h>

#include "entities.h"
#include "notificationsubscriber.h"

#include <QObject>
#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QVector<QString>)

class TestableNotificationSubscriber : public NotificationSubscriber
{
public:
    TestableNotificationSubscriber()
        : NotificationSubscriber()
    {
        mSubscriber = "TestSubscriber";
    }

    void setAllMonitored(bool allMonitored)
    {
        mAllMonitored = allMonitored;
    }

    void setMonitoredCollection(qint64 collection, bool monitored)
    {
        if (monitored) {
            mMonitoredCollections.insert(collection);
        } else {
            mMonitoredCollections.remove(collection);
        }
    }

    void setMonitoredItem(qint64 item, bool monitored)
    {
        if (monitored) {
            mMonitoredItems.insert(item);
        } else {
            mMonitoredItems.remove(item);
        }
    }

    void setMonitoredResource(const QByteArray &resource, bool monitored)
    {
        if (monitored) {
            mMonitoredResources.insert(resource);
        } else {
            mMonitoredResources.remove(resource);
        }
    }

    void setMonitoredMimeType(const QString &mimeType, bool monitored)
    {
        if (monitored) {
            mMonitoredMimeTypes.insert(mimeType);
        } else {
            mMonitoredMimeTypes.remove(mimeType);
        }
    }

    void setIgnoredSession(const QByteArray &session, bool ignored)
    {
        if (ignored) {
            mIgnoredSessions.insert(session);
        } else {
            mIgnoredSessions.remove(session);
        }
    }

    void writeNotification(const Protocol::ChangeNotificationPtr &notification) override
    {
        emittedNotifications << notification;
    }

    Protocol::ChangeNotificationList emittedNotifications;
};

class NotificationSubscriberTest : public QObject
{
    Q_OBJECT

    typedef QList<NotificationSubscriber *> NSList;


    Protocol::FetchItemsResponse itemResponse(qint64 id, const QString &rid, const QString &rrev, const QString &mt)
    {
        Protocol::FetchItemsResponse item;
        item.setId(id);
        item.setRemoteId(rid);
        item.setRemoteRevision(rrev);
        item.setMimeType(mt);
        return item;
    }

private Q_SLOTS:
    void testSourceFilter_data()
    {
        qRegisterMetaType<Protocol::ChangeNotificationList>();

        QTest::addColumn<bool>("allMonitored");
        QTest::addColumn<QVector<Entity::Id> >("monitoredCollections");
        QTest::addColumn<QVector<Entity::Id> >("monitoredItems");
        QTest::addColumn<QVector<QByteArray> >("monitoredResources");
        QTest::addColumn<QVector<QString> >("monitoredMimeTypes");
        QTest::addColumn<QVector<QByteArray> >("ignoredSessions");
        QTest::addColumn<Protocol::ChangeNotificationPtr>("notification");
        QTest::addColumn<bool>("accepted");

#define EmptyList(T) (QVector<T>())
#define List(T,x) (QVector<T>() << x)

        auto itemMsg = Protocol::ItemChangeNotificationPtr::create();
        itemMsg->setOperation(Protocol::ItemChangeNotification::Add);
        itemMsg->setParentCollection(1);
        QTest::newRow("monitorAll vs notification without items")
                << true
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << false;

        itemMsg = Protocol::ItemChangeNotificationPtr::create(*itemMsg);
        itemMsg->setItems({ itemResponse(1, QString(), QString(), QStringLiteral("message/rfc822")) });
        QTest::newRow("monitorAll vs notification with one item")
                << true
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        QTest::newRow("item monitored but different mimetype")
                << false
                << EmptyList(Entity::Id)
                << List(Entity::Id, 1 << 2)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("random/mimetype"))
                << EmptyList(QByteArray)
                << Protocol::ItemChangeNotificationPtr::create(*itemMsg).staticCast<Protocol::ChangeNotification>()
                << false;

        QTest::newRow("item not monitored, but mimetype matches")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << Protocol::ItemChangeNotificationPtr::create(*itemMsg).staticCast<Protocol::ChangeNotification>()
                << true;

        itemMsg = Protocol::ItemChangeNotificationPtr::create(*itemMsg);
        itemMsg->setSessionId("testSession");
        QTest::newRow("item monitored but session ignored")
                << false
                << EmptyList(Entity::Id)
                << List(Entity::Id, 1)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "testSession")
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << false;

        // Simulate adding a new resource
        auto colMsg = Protocol::CollectionChangeNotificationPtr::create();
        colMsg->setOperation(Protocol::CollectionChangeNotification::Add);
        Protocol::FetchCollectionsResponse col;
        col.setId(1);
        col.setRemoteId(QStringLiteral("imap://user@some.domain/"));
        colMsg->setCollection(std::move(col));
        colMsg->setParentCollection(0);
        colMsg->setSessionId("akonadi_imap_resource_0");
        colMsg->setResource("akonadi_imap_resource_0");
        QTest::newRow("new root collection in non-monitored resource")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_search_resource")
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << colMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        itemMsg = Protocol::ItemChangeNotificationPtr::create();
        itemMsg->setOperation(Protocol::ItemChangeNotification::Move);
        itemMsg->setResource("akonadi_resource_1");
        itemMsg->setDestinationResource("akonadi_resource_2");
        itemMsg->setParentCollection(1);
        itemMsg->setParentDestCollection(2);
        itemMsg->setSessionId("kmail");
        itemMsg->setItems({ itemResponse(10, QStringLiteral("123"), QStringLiteral("1"), QStringLiteral("message/rfc822")) });
        QTest::newRow("inter-resource move, source source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_resource_1")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_resource_1")
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        QTest::newRow("inter-resource move, destination source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_resource_2")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_resource_2")
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        QTest::newRow("inter-resource move, uninterested party")
                << false
                << List(Entity::Id, 12)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("inode/directory"))
                << EmptyList(QByteArray)
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << false;

        itemMsg = Protocol::ItemChangeNotificationPtr::create();
        itemMsg->setOperation(Protocol::ItemChangeNotification::Move);
        itemMsg->setResource("akonadi_resource_0");
        itemMsg->setDestinationResource("akonadi_resource_0");
        itemMsg->setParentCollection(1);
        itemMsg->setParentDestCollection(2);
        itemMsg->setSessionId("kmail");
        itemMsg->setItems({
            itemResponse(10, QStringLiteral("123"), QStringLiteral("1"), QStringLiteral("message/rfc822")),
            itemResponse(11, QStringLiteral("456"), QStringLiteral("1"), QStringLiteral("message/rfc822")) });
        QTest::newRow("intra-resource move, owning resource")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_imap_resource_0")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_imap_resource_0")
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        colMsg = Protocol::CollectionChangeNotificationPtr::create();
        colMsg->setOperation(Protocol::CollectionChangeNotification::Add);
        colMsg->setSessionId("kmail");
        colMsg->setResource("akonadi_resource_1");
        colMsg->setParentCollection(1);
        QTest::newRow("new subfolder")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << colMsg.staticCast<Protocol::ChangeNotification>()
                << false;

        itemMsg = Protocol::ItemChangeNotificationPtr::create();
        itemMsg->setOperation(Protocol::ItemChangeNotification::Add);
        itemMsg->setSessionId("randomSession");
        itemMsg->setResource("randomResource");
        itemMsg->setParentCollection(1);
        itemMsg->setItems({ itemResponse(10, QString(), QString(), QStringLiteral("message/rfc822")) });
        QTest::newRow("new mail for mailfilter or maildispatcher")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << itemMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        auto tagMsg = Protocol::TagChangeNotificationPtr::create();
        tagMsg->setOperation(Protocol::TagChangeNotification::Remove);
        tagMsg->setSessionId("randomSession");
        tagMsg->setResource("akonadi_random_resource_0");
        {
            Protocol::FetchTagsResponse tagMsgTag;
            tagMsgTag.setId(1);
            tagMsgTag.setRemoteId("TAG");
            tagMsg->setTag(std::move(tagMsgTag));
        }
        QTest::newRow("Tag removal - resource notification - matching resource source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "akonadi_random_resource_0")
                << tagMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        QTest::newRow("Tag removal - resource notification - wrong resource source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "akonadi_another_resource_1")
                << tagMsg.staticCast<Protocol::ChangeNotification>()
                << false;

        tagMsg = Protocol::TagChangeNotificationPtr::create();
        tagMsg->setOperation(Protocol::TagChangeNotification::Remove);
        tagMsg->setSessionId("randomSession");
        {
            Protocol::FetchTagsResponse tagMsgTag;
            tagMsgTag.setId(1);
            tagMsgTag.setRemoteId("TAG");
            tagMsg->setTag(std::move(tagMsgTag));
        }
        QTest::newRow("Tag removal - client notification - client source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << tagMsg.staticCast<Protocol::ChangeNotification>()
                << true;

        QTest::newRow("Tag removal - client notification - resource source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List( QByteArray, "akonadi_some_resource_0" )
                << tagMsg.staticCast<Protocol::ChangeNotification>()
                << false;
    }

    void testSourceFilter()
    {
        QFETCH(bool, allMonitored);
        QFETCH(QVector<Entity::Id>, monitoredCollections);
        QFETCH(QVector<Entity::Id>, monitoredItems);
        QFETCH(QVector<QByteArray>, monitoredResources);
        QFETCH(QVector<QString>, monitoredMimeTypes);
        QFETCH(QVector<QByteArray>, ignoredSessions);
        QFETCH(Protocol::ChangeNotificationPtr, notification);
        QFETCH(bool, accepted);

        TestableNotificationSubscriber subscriber;

        subscriber.setAllMonitored(allMonitored);
        for (Entity::Id id : monitoredCollections) {
            subscriber.setMonitoredCollection(id, true);
        }
        for (Entity::Id id : monitoredItems) {
            subscriber.setMonitoredItem(id, true);
        }
        for (const QByteArray &res : monitoredResources) {
            subscriber.setMonitoredResource(res, true);
        }
        for (const QString &mimeType : monitoredMimeTypes) {
            subscriber.setMonitoredMimeType(mimeType, true);
        }
        for (const QByteArray &session : ignoredSessions) {
            subscriber.setIgnoredSession(session, true);
        }

        subscriber.notify({ notification });

        QTRY_COMPARE(subscriber.emittedNotifications.count(), accepted ? 1 : 0);

        if (accepted) {
            const Protocol::ChangeNotificationPtr ntf = subscriber.emittedNotifications.at(0);
            QVERIFY(ntf->isValid());
        }
    }
};

AKTEST_MAIN(NotificationSubscriberTest)

#include "notificationsubscribertest.moc"
