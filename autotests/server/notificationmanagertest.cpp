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
#include "notificationsource.h"

#include <QtCore/QObject>
#include <QtTest/QTest>
#include <QSignalSpy>
#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QVector<QString>)

class NotificationManagerTest : public QObject
{
    Q_OBJECT

    typedef QList<NotificationSource *> NSList;

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

        Protocol::ChangeNotification msg;

#define EmptyList(T) (QVector<T>())
#define List(T,x) (QVector<T>() << x)

        msg = Protocol::ChangeNotification();
        msg.setType(Protocol::ChangeNotification::Items);
        msg.setOperation(Protocol::ChangeNotification::Add);
        msg.setParentCollection(1);
        QTest::newRow("monitorAll vs notification without items")
                << true
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << msg
                << false;

        msg.addEntity(1, QString(), QString(), QStringLiteral("message/rfc822"));
        QTest::newRow("monitorAll vs notification with one item")
                << true
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << EmptyList(QByteArray)
                << msg
                << true;

        QTest::newRow("item monitored but different mimetype")
                << false
                << EmptyList(Entity::Id)
                << List(Entity::Id, 1 << 2)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("random/mimetype"))
                << EmptyList(QByteArray)
                << msg
                << false;

        QTest::newRow("item not monitored, but mimetype matches")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << msg
                << true;

        msg.setSessionId("testSession");
        QTest::newRow("item monitored but session ignored")
                << false
                << EmptyList(Entity::Id)
                << List(Entity::Id, 1)
                << EmptyList(QByteArray)
                << EmptyList(QString)
                << List(QByteArray, "testSession")
                << msg
                << false;

        // Simulate adding a new resource
        msg = Protocol::ChangeNotification();
        msg.setType(Protocol::ChangeNotification::Collections);
        msg.setOperation(Protocol::ChangeNotification::Add);
        msg.addEntity(1, QStringLiteral("imap://user@some.domain/"));
        msg.setParentCollection(0);
        msg.setSessionId("akonadi_imap_resource_0");
        msg.setResource("akonadi_imap_resource_0");
        QTest::newRow("new root collection in non-monitored resource")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_search_resource")
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << msg
                << true;

        msg = Protocol::ChangeNotification();
        msg.setType(Protocol::ChangeNotification::Items);
        msg.setOperation(Protocol::ChangeNotification::Move);
        msg.setResource("akonadi_resource_1");
        msg.setDestinationResource("akonadi_resource_2");
        msg.setParentCollection(1);
        msg.setParentDestCollection(2);
        msg.setSessionId("kmail");
        msg.addEntity(10, QStringLiteral("123"), QStringLiteral("1"), QStringLiteral("message/rfc822"));
        QTest::newRow("inter-resource move, source source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_resource_1")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_resource_1")
                << msg
                << true;

        QTest::newRow("inter-resource move, destination source")
                << false
                << EmptyList(Entity::Id)
                << EmptyList(Entity::Id)
                << List(QByteArray, "akonadi_resource_2")
                << List(QString, QStringLiteral("message/rfc822"))
                << List(QByteArray, "akonadi_resource_2")
                << msg
                << true;

        QTest::newRow("inter-resource move, uninterested party")
                << false
                << List(Entity::Id, 12)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("inode/directory"))
                << EmptyList(QByteArray)
                << msg
                << false;

        msg = Protocol::ChangeNotification();
        msg.setType(Protocol::ChangeNotification::Collections);
        msg.setOperation(Protocol::ChangeNotification::Add);
        msg.setSessionId("kmail");
        msg.setResource("akonadi_resource_1");
        msg.setParentCollection(1);
        QTest::newRow("new subfolder")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << msg
                << false;

        msg = Protocol::ChangeNotification();
        msg.setType(Protocol::ChangeNotification::Items);
        msg.setOperation(Protocol::ChangeNotification::Add);
        msg.setSessionId("randomSession");
        msg.setResource("randomResource");
        msg.setParentCollection(1);
        msg.addEntity(10, QString(), QString(), QStringLiteral("message/rfc822"));
        QTest::newRow("new mail for mailfilter or maildispatcher")
                << false
                << List(Entity::Id, 0)
                << EmptyList(Entity::Id)
                << EmptyList(QByteArray)
                << List(QString, QStringLiteral("message/rfc822"))
                << EmptyList(QByteArray)
                << msg
                << true;

      msg = Protocol::ChangeNotification();
      msg.setType( Protocol::ChangeNotification::Tags );
      msg.setOperation( Protocol::ChangeNotification::Remove );
      msg.setSessionId( "randomSession" );
      msg.setResource( "akonadi_random_resource_0" );
      msg.addEntity( 1, QStringLiteral("TAG") );
      QTest::newRow( "Tag removal - resource notification - matching resource source")
        << false
        << EmptyList( Entity::Id )
        << EmptyList( Entity::Id )
        << EmptyList( QByteArray )
        << EmptyList( QString )
        << List( QByteArray, "akonadi_random_resource_0" )
        << msg
        << true;

      QTest::newRow( "Tag removal - resource notification - wrong resource source" )
        << false
        << EmptyList( Entity::Id )
        << EmptyList( Entity::Id )
        << EmptyList( QByteArray )
        << EmptyList( QString )
        << List( QByteArray, "akonadi_another_resource_1" )
        << msg
        << false;

      msg = Protocol::ChangeNotification();
      msg.setType( Protocol::ChangeNotification::Tags );
      msg.setOperation( Protocol::ChangeNotification::Remove );
      msg.setSessionId( "randomSession" );
      msg.addEntity( 1, QStringLiteral("TAG") );
      QTest::newRow( "Tag removal - client notification - client source" )
        << false
        << EmptyList( Entity::Id )
        << EmptyList( Entity::Id )
        << EmptyList( QByteArray )
        << EmptyList( QString )
        << EmptyList( QByteArray )
        << msg
        << true;

      QTest::newRow( "Tag removal - client notification - resource source" )
        << false
        << EmptyList( Entity::Id )
        << EmptyList( Entity::Id )
        << EmptyList( QByteArray )
        << EmptyList( QString )
        << List( QByteArray, "akonadi_some_resource_0" )
        << msg
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

        NotificationManager mgr;
        NotificationSource source(QStringLiteral("testSource"), QString(), &mgr);
        mgr.registerSource(&source);

        source.setAllMonitored(allMonitored);
        Q_FOREACH (Entity::Id id, monitoredCollections) {
            source.setMonitoredCollection(id, true);
        }
        Q_FOREACH (Entity::Id id, monitoredItems) {
            source.setMonitoredItem(id, true);
        }
        Q_FOREACH (const QByteArray &res, monitoredResources) {
            source.setMonitoredResource(res, true);
        }
        Q_FOREACH (const QString &mimeType, monitoredMimeTypes) {
            source.setMonitoredMimeType(mimeType, true);
        }
        Q_FOREACH (const QByteArray &session, ignoredSessions) {
            source.setIgnoredSession(session, true);
        }

        QSignalSpy spy(&source, SIGNAL(notify(Akonadi::Protocol::Command)));
        Protocol::ChangeNotification::List list;
        list << notification;
        mgr.slotNotify(list);
        mgr.emitPendingNotifications();

        QCOMPARE(spy.count(), accepted ? 1 : 0);

        if (accepted) {
            Protocol::ChangeNotification ntf = spy.at(0).at(0).value<Protocol::Command>();
            QVERIFY(ntf.isValid());
        }
    }
};

AKTEST_MAIN(NotificationManagerTest)

#include "notificationmanagertest.moc"
