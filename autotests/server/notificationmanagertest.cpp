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
#include "notificationmanager.h"
#include "notificationsubscriber.h"

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QDebug>

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

    void writeNotification(const Protocol::ChangeNotification &notification) Q_DECL_OVERRIDE
    {
        emittedNotifications << notification;
    }

    Protocol::ChangeNotification::List emittedNotifications;
};

class NotificationManagerTest : public QObject
{
    Q_OBJECT

    typedef QList<NotificationSubscriber *> NSList;

private Q_SLOTS:
    void testSourceFilter_data()
    {
        qRegisterMetaType<Protocol::ChangeNotification::List>();

        QTest::addColumn<bool>("allMonitored");
        QTest::addColumn<QVector<Entity::Id> >("monitoredCollections");
        QTest::addColumn<QVector<Entity::Id> >("monitoredItems");
        QTest::addColumn<QVector<QByteArray> >("monitoredResources");
        QTest::addColumn<QVector<QString> >("monitoredMimeTypes");
        QTest::addColumn<QVector<QByteArray> >("ignoredSessions");
        QTest::addColumn<Protocol::ChangeNotification>("notification");
        QTest::addColumn<bool>("accepted");

        Protocol::ItemChangeNotification itemMsg;

#define EmptyList(T) (QVector<T>())
#define List(T,x) (QVector<T>() << x)

        itemMsg = Protocol::ItemChangeNotification();
        itemMsg.setOperation(Protocol::ItemChangeNotification::Add);
        itemMsg.setParentCollection(1);
        QTest::newRow("monitorAll vs notification without items")
                << true
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(itemMsg)
                << false;

        itemMsg.addItem(1, QString(), QString(), QStringLiteral("message/rfc822"));
        QTest::newRow("monitorAll vs notification with one item")
                << true
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(itemMsg)
                << true;

        QTest::newRow("item monitored but different mimetype")
                << false
                << EmptyList(Entity::Id)
                << List(Entity::Id, 1 << 2)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("random/mimetype"))
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(itemMsg)
                << false;

        QTest::newRow("item not monitored, but mimetype matches")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(itemMsg)
                << true;

        itemMsg.setSessionId("testSession");
        QTest::newRow("item monitored but session ignored")
                << false
                << EmptyList(Entity::Id)
                << List(Entity::Id, 1)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "testSession")
                << Protocol::ChangeNotification(itemMsg)
                << false;

        // Simulate adding a new resource
        Protocol::CollectionChangeNotification colMsg;
        colMsg.setOperation(Protocol::CollectionChangeNotification::Add);
        colMsg.setId(1);
        colMsg.setRemoteId(QStringLiteral("imap://user@some.domain/"));
        colMsg.setParentCollection(0);
        colMsg.setSessionId("akonadi_imap_resource_0");
        colMsg.setResource("akonadi_imap_resource_0");
        QTest::newRow("new root collection in non-monitored resource")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_search_resource")
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(colMsg)
                << true;

        itemMsg = Protocol::ItemChangeNotification();
        itemMsg.setOperation(Protocol::ItemChangeNotification::Move);
        itemMsg.setResource("akonadi_resource_1");
        itemMsg.setDestinationResource("akonadi_resource_2");
        itemMsg.setParentCollection(1);
        itemMsg.setParentDestCollection(2);
        itemMsg.setSessionId("kmail");
        itemMsg.addItem(10, QStringLiteral("123"), QStringLiteral("1"), QStringLiteral("message/rfc822"));
        QTest::newRow("inter-resource move, source source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_resource_1")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_resource_1")
                << Protocol::ChangeNotification(itemMsg)
                << true;

        QTest::newRow("inter-resource move, destination source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_resource_2")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_resource_2")
                << Protocol::ChangeNotification(itemMsg)
                << true;

        QTest::newRow("inter-resource move, uninterested party")
                << false
                << List(Entity::Id, 12)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("inode/directory"))
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(itemMsg)
                << false;

        itemMsg = Protocol::ItemChangeNotification();
        itemMsg.setOperation(Protocol::ItemChangeNotification::Move);
        itemMsg.setResource("akonadi_resource_0");
        itemMsg.setDestinationResource("akonadi_resource_0");
        itemMsg.setParentCollection(1);
        itemMsg.setParentDestCollection(2);
        itemMsg.setSessionId("kmail");
        itemMsg.addItem(10, QStringLiteral("123"), QStringLiteral("1"), QStringLiteral("message/rfc822"));
        itemMsg.addItem(11, QStringLiteral("456"), QStringLiteral("1"), QStringLiteral("message/rfc822"));
        QTest::newRow("intra-resource move, owning resource")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_imap_resource_0")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_imap_resource_0")
                << Protocol::ChangeNotification(itemMsg)
                << true;

        colMsg = Protocol::CollectionChangeNotification();
        colMsg.setOperation(Protocol::CollectionChangeNotification::Add);
        colMsg.setSessionId("kmail");
        colMsg.setResource("akonadi_resource_1");
        colMsg.setParentCollection(1);
        QTest::newRow("new subfolder")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(colMsg)
                << false;

        itemMsg = Protocol::ItemChangeNotification();
        itemMsg.setOperation(Protocol::ItemChangeNotification::Add);
        itemMsg.setSessionId("randomSession");
        itemMsg.setResource("randomResource");
        itemMsg.setParentCollection(1);
        itemMsg.addItem(10, QString(), QString(), QStringLiteral("message/rfc822"));
        QTest::newRow("new mail for mailfilter or maildispatcher")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(itemMsg)
                << true;

        Protocol::TagChangeNotification tagMsg;
        tagMsg.setOperation(Protocol::TagChangeNotification::Remove);
        tagMsg.setSessionId("randomSession");
        tagMsg.setResource("akonadi_random_resource_0");
        tagMsg.setId(1);
        tagMsg.setRemoteId(QStringLiteral("TAG"));
        QTest::newRow("Tag removal - resource notification - matching resource source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "akonadi_random_resource_0")
                << Protocol::ChangeNotification(tagMsg)
                << true;

        QTest::newRow("Tag removal - resource notification - wrong resource source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "akonadi_another_resource_1")
                << Protocol::ChangeNotification(tagMsg)
                << false;

        tagMsg = Protocol::TagChangeNotification();
        tagMsg.setOperation(Protocol::TagChangeNotification::Remove);
        tagMsg.setSessionId("randomSession");
        tagMsg.setId(1);
        tagMsg.setRemoteId(QStringLiteral("TAG"));
        QTest::newRow("Tag removal - client notification - client source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << Protocol::ChangeNotification(tagMsg)
                << true;

        QTest::newRow("Tag removal - client notification - resource source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List( QByteArray, "akonadi_some_resource_0" )
                << Protocol::ChangeNotification(tagMsg)
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
        QFETCH(Protocol::ChangeNotification, notification);
        QFETCH(bool, accepted);

        TestableNotificationSubscriber subscriber;

        subscriber.setAllMonitored(allMonitored);
        Q_FOREACH (Entity::Id id, monitoredCollections) {
            subscriber.setMonitoredCollection(id, true);
        }
        Q_FOREACH (Entity::Id id, monitoredItems) {
            subscriber.setMonitoredItem(id, true);
        }
        Q_FOREACH (const QByteArray &res, monitoredResources) {
            subscriber.setMonitoredResource(res, true);
        }
        Q_FOREACH (const QString &mimeType, monitoredMimeTypes) {
            subscriber.setMonitoredMimeType(mimeType, true);
        }
        Q_FOREACH (const QByteArray &session, ignoredSessions) {
            subscriber.setIgnoredSession(session, true);
        }

        subscriber.notify({ notification });

        QTRY_COMPARE(subscriber.emittedNotifications.count(), accepted ? 1 : 0);

        if (accepted) {
            const Protocol::ChangeNotification ntf = subscriber.emittedNotifications.at(0);
            QVERIFY(ntf.isValid());
        }
    }
};

AKTEST_MAIN(NotificationManagerTest)

#include "notificationmanagertest.moc"
