/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "collection.h"
#include "control.h"
#include "itemfetchjob.h"
#include "itemcreatejob.h"
#include "itemmovejob.h"
#include "itemfetchscope.h"
#include "collectionfetchscope.h"
#include "monitor.h"
#include "session.h"
#include "qtest_akonadi.h"
#include "resourceselectjob_p.h"

#include <QObject>

using namespace Akonadi;

class ItemMoveTest: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        Control::start();
    }

    // TODO: test inter and intra resource moves
    void testMove_data()
    {
        QTest::addColumn<Item::List>("items");
        QTest::addColumn<Collection>("destination");
        QTest::addColumn<Collection>("source");

        Collection destination(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo/bar")));
        QVERIFY(destination.isValid());

        QTest::newRow("intra-res single uid") << (Item::List() << Item(5)) << destination << Collection();

        destination = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        QVERIFY(destination.isValid());

        QTest::newRow("inter-res single uid") << (Item::List() << Item(1)) << destination << Collection();
        QTest::newRow("inter-res two uid") << (Item::List() << Item(2) << Item(3)) << destination << Collection();
        Item r1; r1.setRemoteId(QStringLiteral("D"));
        Collection ridDest;
        ridDest.setRemoteId(QStringLiteral("3"));
        Collection ridSource;
        ridSource.setRemoteId(QStringLiteral("10"));
        QTest::newRow("intra-res single rid") << (Item::List() << r1) << ridDest << ridSource;
    }

    void testMove()
    {
        QFETCH(Item::List, items);
        QFETCH(Collection, destination);
        QFETCH(Collection, source);

        Session monitorSession;
        Monitor monitor(&monitorSession);
        monitor.setObjectName(QStringLiteral("itemmovetest"));
        monitor.setCollectionMonitored(Collection::root());
        monitor.fetchCollection(true);
        monitor.itemFetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
        monitor.itemFetchScope().setFetchRemoteIdentification(true);
        QSignalSpy moveSpy(&monitor, &Monitor::itemsMoved);
        AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady);

        ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);   // for rid based moves

        ItemFetchJob *prefetchjob = new ItemFetchJob(destination, this);
        AKVERIFYEXEC(prefetchjob);
        int baseline = prefetchjob->items().size();

        ItemMoveJob *move = new ItemMoveJob(items, source, destination, this);
        AKVERIFYEXEC(move);

        ItemFetchJob *fetch = new ItemFetchJob(destination, this);
        fetch->fetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
        fetch->fetchScope().fetchFullPayload();
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), items.count() + baseline);
        foreach (const Item &movedItem, fetch->items()) {
            QVERIFY(movedItem.hasPayload());
            QVERIFY(!movedItem.payload<QByteArray>().isEmpty());
            if (destination.id() >= 0) {
                QCOMPARE(movedItem.parentCollection().id(), destination.id());
            } else {
                QCOMPARE(movedItem.parentCollection().remoteId(), destination.remoteId());
            }
        }

        QTRY_COMPARE(moveSpy.count(), 1);
        const Akonadi::Item::List &ntfItems = moveSpy.takeFirst().at(0).value<Akonadi::Item::List>();
        QCOMPARE(ntfItems.size(), items.size());
        Q_FOREACH (const Item &ntfItem, ntfItems) {
            if (destination.id() >= 0) {
                QCOMPARE(ntfItem.parentCollection().id(), destination.id());
            } else {
                QCOMPARE(ntfItem.parentCollection().remoteId(), destination.remoteId());
            }
        }
    }

    void testIllegalMove()
    {
        Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res2")));
        QVERIFY(col.isValid());

        ItemFetchJob *prefetchjob = new ItemFetchJob(Item(1));
        AKVERIFYEXEC(prefetchjob);
        QCOMPARE(prefetchjob->items().count(), 1);
        Item item = prefetchjob->items()[0];

        // move into invalid collection
        ItemMoveJob *store = new ItemMoveJob(item, Collection(INT_MAX), this);
        QVERIFY(!store->exec());

        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy itemMovedSpy(monitor.get(), &Monitor::itemsMoved);

        // move item into folder that doesn't support its mimetype
        store = new ItemMoveJob(item, col, this);
        QEXPECT_FAIL("", "Check not yet implemented by the server.", Continue);
        QVERIFY(!store->exec());

        // Wait for the notification so that it does not disturb the next test
        QTRY_COMPARE(itemMovedSpy.count(), 1);
    }

    void testMoveNotifications()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy itemMovedSpy(monitor.get(), &Monitor::itemsMoved);
        QSignalSpy itemAddedSpy(monitor.get(), &Monitor::itemAdded);

        Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        Item item(QStringLiteral("application/octet-stream"));
        item.setFlags({ "\\SEEN", "$ENCRYPTED" });
        item.setPayload(QByteArray("This is a test payload"));
        item.setSize(34);
        item.setParentCollection(col);
        auto *create = new ItemCreateJob(item, col, this);
        AKVERIFYEXEC(create);
        item = create->item();

        QTRY_COMPARE(itemAddedSpy.size(), 1);
        auto ntfItem = itemAddedSpy.at(0).at(0).value<Akonadi::Item>();
        QCOMPARE(ntfItem.id(), item.id());
        QCOMPARE(ntfItem.flags(), item.flags());

        Collection dest(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo/bar")));
        auto *move = new ItemMoveJob(item, dest, this);
        AKVERIFYEXEC(move);

        QTRY_COMPARE(itemMovedSpy.size(), 1);
        const auto ntfItems = itemMovedSpy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(ntfItems.size(), 1);
        ntfItem = ntfItems.at(0);
        QCOMPARE(ntfItem.id(), item.id());
        QCOMPARE(ntfItem.flags(), item.flags());
    }
};

QTEST_AKONADIMAIN(ItemMoveTest)

#include "itemmovetest.moc"
