/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "monitor_p.h"
#include "qtest_akonadi.h"
#include <QTest>

using namespace Akonadi;

Q_DECLARE_METATYPE(Akonadi::Protocol::ChangeNotification::Operation)
Q_DECLARE_METATYPE(QSet<QByteArray>)

class MonitorFilterTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        qRegisterMetaType<Akonadi::Item>();
        qRegisterMetaType<Akonadi::Collection>();
        qRegisterMetaType<QSet<QByteArray>>();
        qRegisterMetaType<Akonadi::Item::List>();
    }

    void filterConnected_data()
    {
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Operation>("op");
        QTest::addColumn<QByteArray>("signalName");

        QTest::newRow("itemAdded") << Protocol::ChangeNotification::Add << QByteArray(SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemChanged") << Protocol::ChangeNotification::Modify << QByteArray(SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
        QTest::newRow("itemsFlagsChanged") << Protocol::ChangeNotification::ModifyFlags
                                           << QByteArray(SIGNAL(itemsFlagsChanged(Akonadi::Item::List, QSet<QByteArray>, QSet<QByteArray>)));
        QTest::newRow("itemRemoved") << Protocol::ChangeNotification::Remove << QByteArray(SIGNAL(itemRemoved(Akonadi::Item)));
        QTest::newRow("itemMoved") << Protocol::ChangeNotification::Move
                                   << QByteArray(SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("itemLinked") << Protocol::ChangeNotification::Link << QByteArray(SIGNAL(itemLinked(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemUnlinked") << Protocol::ChangeNotification::Unlink << QByteArray(SIGNAL(itemUnlinked(Akonadi::Item, Akonadi::Collection)));
    }

    void filterConnected()
    {
        QFETCH(Protocol::ChangeNotification::Operation, op);
        QFETCH(QByteArray, signalName);

        Monitor dummyMonitor;
        MonitorPrivate m(0, &dummyMonitor);

        Protocol::ChangeNotification msg;
        msg.addEntity(1);
        msg.setOperation(op);
        msg.setType(Akonadi::Protocol::ChangeNotification::Items);

        QVERIFY(!m.acceptNotification(msg));
        m.monitorAll = true;
        QVERIFY(m.acceptNotification(msg));
        QSignalSpy spy(&dummyMonitor, signalName.constData());
        QVERIFY(spy.isValid());
        QVERIFY(m.acceptNotification(msg));
        m.monitorAll = false;
        QVERIFY(!m.acceptNotification(msg));
    }

    void filterSession()
    {
        Monitor dummyMonitor;
        MonitorPrivate m(0, &dummyMonitor);
        m.monitorAll = true;
        QSignalSpy spy(&dummyMonitor, &Monitor::itemAdded);

        Protocol::ChangeNotification msg;
        msg.addEntity(1);
        msg.setOperation(Protocol::ChangeNotification::Add);
        msg.setType(Akonadi::Protocol::ChangeNotification::Items);
        msg.setSessionId("foo");

        QVERIFY(m.acceptNotification(msg));
        m.sessions.append("bar");
        QVERIFY(m.acceptNotification(msg));
        m.sessions.append("foo");
        QVERIFY(!m.acceptNotification(msg));
    }

    void filterResource_data()
    {
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Operation>("op");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Type>("type");
        QTest::addColumn<QByteArray>("signalName");

        QTest::newRow("itemAdded") << Protocol::ChangeNotification::Add << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemChanged") << Protocol::ChangeNotification::Modify << Protocol::ChangeNotification::Items
                                     << QByteArray(SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
        QTest::newRow("itemsFlagsChanged") << Protocol::ChangeNotification::ModifyFlags << Protocol::ChangeNotification::Items
                                           << QByteArray(SIGNAL(itemsFlagsChanged(Akonadi::Item::List, QSet<QByteArray>, QSet<QByteArray>)));
        QTest::newRow("itemRemoved") << Protocol::ChangeNotification::Remove << Protocol::ChangeNotification::Items
                                     << QByteArray(SIGNAL(itemRemoved(Akonadi::Item)));
        QTest::newRow("itemMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("itemLinked") << Protocol::ChangeNotification::Link << Protocol::ChangeNotification::Items
                                    << QByteArray(SIGNAL(itemLinked(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemUnlinked") << Protocol::ChangeNotification::Unlink << Protocol::ChangeNotification::Items
                                      << QByteArray(SIGNAL(itemUnlinked(Akonadi::Item, Akonadi::Collection)));

        QTest::newRow("colAdded") << Protocol::ChangeNotification::Add << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionAdded(Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colChanged") << Protocol::ChangeNotification::Modify << Protocol::ChangeNotification::Collections
                                    << QByteArray(SIGNAL(collectionChanged(Akonadi::Collection, QSet<QByteArray>)));
        QTest::newRow("colRemoved") << Protocol::ChangeNotification::Remove << Protocol::ChangeNotification::Collections
                                    << QByteArray(SIGNAL(collectionRemoved(Akonadi::Collection)));
        QTest::newRow("colMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colSubscribed") << Protocol::ChangeNotification::Subscribe << Protocol::ChangeNotification::Collections
                                       << QByteArray(SIGNAL(collectionSubscribed(Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colSubscribed") << Protocol::ChangeNotification::Unsubscribe << Protocol::ChangeNotification::Collections
                                       << QByteArray(SIGNAL(collectionUnsubscribed(Akonadi::Collection)));
    }

    void filterResource()
    {
        QFETCH(Protocol::ChangeNotification::Operation, op);
        QFETCH(Protocol::ChangeNotification::Type, type);
        QFETCH(QByteArray, signalName);

        Monitor dummyMonitor;
        MonitorPrivate m(0, &dummyMonitor);
        QSignalSpy spy(&dummyMonitor, signalName.constData());
        QVERIFY(spy.isValid());

        Protocol::ChangeNotification msg;
        msg.addEntity(1);
        msg.setOperation(op);
        msg.setParentCollection(2);
        msg.setType(type);
        msg.setResource("foo");
        msg.setSessionId("mysession");

        // using the right resource makes it pass
        QVERIFY(!m.acceptNotification(msg));
        m.resources.insert("bar");
        QVERIFY(!m.acceptNotification(msg));
        m.resources.insert("foo");
        QVERIFY(m.acceptNotification(msg));

        // filtering out the session overwrites the resource
        m.sessions.append("mysession");
        QVERIFY(!m.acceptNotification(msg));
    }

    void filterDestinationResource_data()
    {
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Operation>("op");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Type>("type");
        QTest::addColumn<QByteArray>("signalName");

        QTest::newRow("itemMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)));
    }

    void filterDestinationResource()
    {
        QFETCH(Protocol::ChangeNotification::Operation, op);
        QFETCH(Protocol::ChangeNotification::Type, type);
        QFETCH(QByteArray, signalName);

        Monitor dummyMonitor;
        MonitorPrivate m(0, &dummyMonitor);
        QSignalSpy spy(&dummyMonitor, signalName.constData());
        QVERIFY(spy.isValid());

        Protocol::ChangeNotification msg;
        msg.setOperation(op);
        msg.setType(type);
        msg.setResource("foo");
        msg.setDestinationResource("bar");
        msg.setSessionId("mysession");
        msg.addEntity(1);

        // using the right resource makes it pass
        QVERIFY(!m.acceptNotification(msg));
        m.resources.insert("bla");
        QVERIFY(!m.acceptNotification(msg));
        m.resources.insert("bar");
        QVERIFY(m.acceptNotification(msg));

        // filtering out the mimetype does not overwrite resources
        msg.addEntity(1, QString(), QString(), QStringLiteral("my/type"));
        QVERIFY(m.acceptNotification(msg));

        // filtering out the session overwrites the resource
        m.sessions.append("mysession");
        QVERIFY(!m.acceptNotification(msg));
    }

    void filterMimeType_data()
    {
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Operation>("op");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Type>("type");
        QTest::addColumn<QByteArray>("signalName");

        QTest::newRow("itemAdded") << Protocol::ChangeNotification::Add << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemChanged") << Protocol::ChangeNotification::Modify << Protocol::ChangeNotification::Items
                                     << QByteArray(SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
        QTest::newRow("itemsFlagsChanged") << Protocol::ChangeNotification::ModifyFlags << Protocol::ChangeNotification::Items
                                           << QByteArray(SIGNAL(itemsFlagsChanged(Akonadi::Item::List, QSet<QByteArray>, QSet<QByteArray>)));
        QTest::newRow("itemRemoved") << Protocol::ChangeNotification::Remove << Protocol::ChangeNotification::Items
                                     << QByteArray(SIGNAL(itemRemoved(Akonadi::Item)));
        QTest::newRow("itemMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("itemLinked") << Protocol::ChangeNotification::Link << Protocol::ChangeNotification::Items
                                    << QByteArray(SIGNAL(itemLinked(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemUnlinked") << Protocol::ChangeNotification::Unlink << Protocol::ChangeNotification::Items
                                      << QByteArray(SIGNAL(itemUnlinked(Akonadi::Item, Akonadi::Collection)));

        QTest::newRow("colAdded") << Protocol::ChangeNotification::Add << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionAdded(Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colChanged") << Protocol::ChangeNotification::Modify << Protocol::ChangeNotification::Collections
                                    << QByteArray(SIGNAL(collectionChanged(Akonadi::Collection, QSet<QByteArray>)));
        QTest::newRow("colRemoved") << Protocol::ChangeNotification::Remove << Protocol::ChangeNotification::Collections
                                    << QByteArray(SIGNAL(collectionRemoved(Akonadi::Collection)));
        QTest::newRow("colMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colSubscribed") << Protocol::ChangeNotification::Subscribe << Protocol::ChangeNotification::Collections
                                       << QByteArray(SIGNAL(collectionSubscribed(Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colSubscribed") << Protocol::ChangeNotification::Unsubscribe << Protocol::ChangeNotification::Collections
                                       << QByteArray(SIGNAL(collectionUnsubscribed(Akonadi::Collection)));
    }

    void filterMimeType()
    {
        QFETCH(Protocol::ChangeNotification::Operation, op);
        QFETCH(Protocol::ChangeNotification::Type, type);
        QFETCH(QByteArray, signalName);

        Monitor dummyMonitor;
        MonitorPrivate m(0, &dummyMonitor);
        QSignalSpy spy(&dummyMonitor, signalName.constData());
        QVERIFY(spy.isValid());

        Protocol::ChangeNotification msg;
        msg.addEntity(1, QString(), QString(), QStringLiteral("my/type"));
        msg.setOperation(op);
        msg.setParentCollection(2);
        msg.setType(type);
        msg.setResource("foo");
        msg.setSessionId("mysession");

        // using the right resource makes it pass
        QVERIFY(!m.acceptNotification(msg));
        m.mimetypes.insert(QStringLiteral("your/type"));
        QVERIFY(!m.acceptNotification(msg));
        m.mimetypes.insert(QStringLiteral("my/type"));
        QCOMPARE(m.acceptNotification(msg), type == Protocol::ChangeNotification::Items);

        // filter out the resource does not overwrite mimetype
        m.resources.insert("bar");
        QCOMPARE(m.acceptNotification(msg), type == Protocol::ChangeNotification::Items);

        // filtering out the session overwrites the mimetype
        m.sessions.append("mysession");
        QVERIFY(!m.acceptNotification(msg));
    }

    void filterCollection_data()
    {
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Operation>("op");
        QTest::addColumn<Akonadi::Protocol::ChangeNotification::Type>("type");
        QTest::addColumn<QByteArray>("signalName");

        QTest::newRow("itemAdded") << Protocol::ChangeNotification::Add << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemChanged") << Protocol::ChangeNotification::Modify << Protocol::ChangeNotification::Items
                                     << QByteArray(SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
        QTest::newRow("itemsFlagsChanged") << Protocol::ChangeNotification::ModifyFlags << Protocol::ChangeNotification::Items
                                           << QByteArray(SIGNAL(itemsFlagsChanged(Akonadi::Item::List, QSet<QByteArray>, QSet<QByteArray>)));
        QTest::newRow("itemRemoved") << Protocol::ChangeNotification::Remove << Protocol::ChangeNotification::Items
                                     << QByteArray(SIGNAL(itemRemoved(Akonadi::Item)));
        QTest::newRow("itemMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Items
                                   << QByteArray(SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("itemLinked") << Protocol::ChangeNotification::Link << Protocol::ChangeNotification::Items
                                    << QByteArray(SIGNAL(itemLinked(Akonadi::Item, Akonadi::Collection)));
        QTest::newRow("itemUnlinked") << Protocol::ChangeNotification::Unlink << Protocol::ChangeNotification::Items
                                      << QByteArray(SIGNAL(itemUnlinked(Akonadi::Item, Akonadi::Collection)));

        QTest::newRow("colAdded") << Protocol::ChangeNotification::Add << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionAdded(Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colChanged") << Protocol::ChangeNotification::Modify << Protocol::ChangeNotification::Collections
                                    << QByteArray(SIGNAL(collectionChanged(Akonadi::Collection, QSet<QByteArray>)));
        QTest::newRow("colRemoved") << Protocol::ChangeNotification::Remove << Protocol::ChangeNotification::Collections
                                    << QByteArray(SIGNAL(collectionRemoved(Akonadi::Collection)));
        QTest::newRow("colMoved") << Protocol::ChangeNotification::Move << Protocol::ChangeNotification::Collections
                                  << QByteArray(SIGNAL(collectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colSubscribed") << Protocol::ChangeNotification::Subscribe << Protocol::ChangeNotification::Collections
                                       << QByteArray(SIGNAL(collectionSubscribed(Akonadi::Collection, Akonadi::Collection)));
        QTest::newRow("colSubscribed") << Protocol::ChangeNotification::Unsubscribe << Protocol::ChangeNotification::Collections
                                       << QByteArray(SIGNAL(collectionUnsubscribed(Akonadi::Collection)));
    }

    void filterCollection()
    {
        QFETCH(Protocol::ChangeNotification::Operation, op);
        QFETCH(Protocol::ChangeNotification::Type, type);
        QFETCH(QByteArray, signalName);

        Monitor dummyMonitor;
        MonitorPrivate m(0, &dummyMonitor);
        QSignalSpy spy(&dummyMonitor, signalName.constData());
        QVERIFY(spy.isValid());

        Protocol::ChangeNotification msg;
        msg.addEntity(1, QString(), QString(), QStringLiteral("my/type"));
        msg.setOperation(op);
        msg.setParentCollection(2);
        msg.setType(type);
        msg.setResource("foo");
        msg.setSessionId("mysession");

        // using the right resource makes it pass
        QVERIFY(!m.acceptNotification(msg));
        m.collections.append(Collection(3));
        QVERIFY(!m.acceptNotification(msg));

        for (int colId = 0; colId < 3; ++colId) { // 0 == root, 1 == this, 2 == parent
            if (colId == 1 && type == Protocol::ChangeNotification::Items) {
                continue;
            }

            m.collections.clear();
            m.collections.append(Collection(colId));

            QVERIFY(m.acceptNotification(msg));

            // filter out the resource does overwrite collection
            m.resources.insert("bar");
            QVERIFY(!m.acceptNotification(msg));
            m.resources.clear();

            // filter out the mimetype does overwrite collection, for item operations (mimetype filter has no effect on collections)
            m.mimetypes.insert(QStringLiteral("your/type"));
            QCOMPARE(!m.acceptNotification(msg), type == Protocol::ChangeNotification::Items);
            m.mimetypes.clear();

            // filtering out the session overwrites the mimetype
            m.sessions.append("mysession");
            QVERIFY(!m.acceptNotification(msg));
            m.sessions.clear();

            // filter non-matching resource and matching mimetype make it pass
            m.resources.insert("bar");
            m.mimetypes.insert(QStringLiteral("my/type"));
            QVERIFY(m.acceptNotification(msg));
            m.resources.clear();
            m.mimetypes.clear();
        }
    }
};

QTEST_MAIN(MonitorFilterTest)

#include "monitorfiltertest.moc"
