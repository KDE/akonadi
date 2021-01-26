/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "monitortest.h"
#include "agentinstance.h"
#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "collectionstatistics.h"
#include "control.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "monitor.h"
#include "qtest_akonadi.h"
#include "searchcreatejob.h"
#include "searchquery.h"
#include "subscriptionjob_p.h"

#include <QSignalSpy>
#include <QVariant>

using namespace Akonadi;

QTEST_AKONADIMAIN(MonitorTest)

static Collection res3;

Q_DECLARE_METATYPE(Akonadi::Collection::Id)
Q_DECLARE_METATYPE(QSet<QByteArray>)

void MonitorTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();

    res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));

    AkonadiTest::setAllResourcesOffline();
}

void MonitorTest::testMonitor_data()
{
    QTest::addColumn<bool>("fetchCol");
    QTest::newRow("with collection fetching") << true;
    QTest::newRow("without collection fetching") << false;
}

void MonitorTest::testMonitor()
{
    QFETCH(bool, fetchCol);

    Monitor monitor;
    monitor.setCollectionMonitored(Collection::root());
    monitor.fetchCollection(fetchCol);
    monitor.itemFetchScope().fetchFullPayload();
    monitor.itemFetchScope().setCacheOnly(true);
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    // monitor signals
    qRegisterMetaType<Akonadi::Collection>();
    /*
       qRegisterMetaType<Akonadi::Collection::Id>() registers the type with a
       name of "qlonglong".  Doing
       qRegisterMetaType<Akonadi::Collection::Id>( "Akonadi::Collection::Id" )
       doesn't help. (works now , see QTBUG-937 and QTBUG-6833, -- dvratil)

       The problem here is that Akonadi::Collection::Id is a typedef to qlonglong,
       and qlonglong is already a registered meta type.  So the signal spy will
       give us a QVariant of type Akonadi::Collection::Id, but calling
       .value<Akonadi::Collection::Id>() on that variant will in fact end up
       calling qvariant_cast<qlonglong>.  From the point of view of QMetaType,
       Akonadi::Collection::Id and qlonglong are different types, so QVariant
       can't convert, and returns a default-constructed qlonglong, zero.

       When connecting to a real slot (without QSignalSpy), this problem is
       avoided, because the casting is done differently (via a lot of void
       pointers).

       The docs say nothing about qRegisterMetaType -ing a typedef, so I'm not
       sure if this is a bug or not. (cberzan)
     */
    qRegisterMetaType<Akonadi::Collection::Id>("Akonadi::Collection::Id");
    qRegisterMetaType<Akonadi::Item>();
    qRegisterMetaType<Akonadi::CollectionStatistics>();
    qRegisterMetaType<QSet<QByteArray>>();
    QSignalSpy caddspy(&monitor, &Monitor::collectionAdded);
    QSignalSpy cmodspy(&monitor, SIGNAL(collectionChanged(Akonadi::Collection, QSet<QByteArray>)));
    QSignalSpy cmvspy(&monitor, &Monitor::collectionMoved);
    QSignalSpy crmspy(&monitor, &Monitor::collectionRemoved);
    QSignalSpy cstatspy(&monitor, &Monitor::collectionStatisticsChanged);
    QSignalSpy cSubscribedSpy(&monitor, &Monitor::collectionSubscribed);
    QSignalSpy cUnsubscribedSpy(&monitor, &Monitor::collectionUnsubscribed);
    QSignalSpy iaddspy(&monitor, &Monitor::itemAdded);
    QSignalSpy imodspy(&monitor, &Monitor::itemChanged);
    QSignalSpy imvspy(&monitor, &Monitor::itemMoved);
    QSignalSpy irmspy(&monitor, &Monitor::itemRemoved);

    QVERIFY(caddspy.isValid());
    QVERIFY(cmodspy.isValid());
    QVERIFY(cmvspy.isValid());
    QVERIFY(crmspy.isValid());
    QVERIFY(cstatspy.isValid());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isValid());
    QVERIFY(imodspy.isValid());
    QVERIFY(imvspy.isValid());
    QVERIFY(irmspy.isValid());

    // create a collection
    Collection monitorCol;
    monitorCol.setParentCollection(res3);
    monitorCol.setName(QStringLiteral("monitor"));
    auto *create = new CollectionCreateJob(monitorCol, this);
    AKVERIFYEXEC(create);
    monitorCol = create->collection();
    QVERIFY(monitorCol.isValid());

    QTRY_COMPARE(caddspy.count(), 1);
    QList<QVariant> arg = caddspy.takeFirst();
    auto col = arg.at(0).value<Collection>();
    QCOMPARE(col, monitorCol);
    if (fetchCol) {
        QCOMPARE(col.name(), QStringLiteral("monitor"));
    }
    auto parent = arg.at(1).value<Collection>();
    QCOMPARE(parent, res3);

    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cstatspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // add an item
    Item newItem;
    newItem.setMimeType(QStringLiteral("application/octet-stream"));
    auto *append = new ItemCreateJob(newItem, monitorCol, this);
    AKVERIFYEXEC(append);
    Item monitorRef = append->item();
    QVERIFY(monitorRef.isValid());

    QTRY_COMPARE(cstatspy.count(), 1);
    arg = cstatspy.takeFirst();
    QCOMPARE(arg.at(0).value<Akonadi::Collection::Id>(), monitorCol.id());

    QCOMPARE(iaddspy.count(), 1);
    arg = iaddspy.takeFirst();
    Item item = arg.at(0).value<Item>();
    QCOMPARE(item, monitorRef);
    QCOMPARE(item.mimeType(), QString::fromLatin1("application/octet-stream"));
    auto collection = arg.at(1).value<Collection>();
    QCOMPARE(collection.id(), monitorCol.id());

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // modify an item
    item.setPayload<QByteArray>("some new content");
    auto *store = new ItemModifyJob(item, this);
    AKVERIFYEXEC(store);

    QTRY_COMPARE(cstatspy.count(), 1);
    arg = cstatspy.takeFirst();
    QCOMPARE(arg.at(0).value<Collection::Id>(), monitorCol.id());

    QCOMPARE(imodspy.count(), 1);
    arg = imodspy.takeFirst();
    item = arg.at(0).value<Item>();
    QCOMPARE(monitorRef, item);
    QVERIFY(item.hasPayload<QByteArray>());
    QCOMPARE(item.payload<QByteArray>(), QByteArray("some new content"));
    auto parts = arg.at(1).value<QSet<QByteArray>>();
    QCOMPARE(parts, QSet<QByteArray>() << "PLD:RFC822");

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // move an item
    auto *move = new ItemMoveJob(item, res3);
    AKVERIFYEXEC(move);
    QTRY_COMPARE(cstatspy.count(), 2);
    // NOTE: We don't make any assumptions about the order of the collectionStatisticsChanged
    // signals, they seem to arrive in random order
    QList<Collection::Id> notifiedCols;
    notifiedCols << cstatspy.takeFirst().at(0).value<Collection::Id>() << cstatspy.takeFirst().at(0).value<Collection::Id>();
    QVERIFY(notifiedCols.contains(res3.id())); // destination
    QVERIFY(notifiedCols.contains(monitorCol.id())); // source

    QCOMPARE(imvspy.count(), 1);
    arg = imvspy.takeFirst();
    item = arg.at(0).value<Item>(); // the item
    QCOMPARE(monitorRef, item);
    col = arg.at(1).value<Collection>(); // the source collection
    QCOMPARE(col.id(), monitorCol.id());
    col = arg.at(2).value<Collection>(); // the destination collection
    QCOMPARE(col.id(), res3.id());

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // delete an item
    auto *del = new ItemDeleteJob(monitorRef, this);
    AKVERIFYEXEC(del);

    QTRY_COMPARE(cstatspy.count(), 1);
    arg = cstatspy.takeFirst();
    QCOMPARE(arg.at(0).value<Collection::Id>(), res3.id());
    cmodspy.clear();

    QCOMPARE(irmspy.count(), 1);
    arg = irmspy.takeFirst();
    Item ref = qvariant_cast<Item>(arg.at(0));
    QCOMPARE(monitorRef, ref);
    QCOMPARE(ref.parentCollection(), res3);

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    imvspy.clear();

    // Unsubscribe and re-subscribed a collection that existed before the monitor was created.
    Collection subCollection = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/foo2")));
    subCollection.setName(QStringLiteral("foo2"));
    QVERIFY(subCollection.isValid());

    auto *subscribeJob = new SubscriptionJob(this);
    subscribeJob->unsubscribe(Collection::List() << subCollection);
    AKVERIFYEXEC(subscribeJob);
    // Wait for unsubscribed signal, it goes after changed, so we can check for both
    QTRY_COMPARE(cmodspy.size(), 1);
    arg = cmodspy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col.id(), subCollection.id());

    QVERIFY(cSubscribedSpy.isEmpty());
    QTRY_COMPARE(cUnsubscribedSpy.size(), 1);
    arg = cUnsubscribedSpy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col.id(), subCollection.id());

    subscribeJob = new SubscriptionJob(this);
    subscribeJob->subscribe(Collection::List() << subCollection);
    AKVERIFYEXEC(subscribeJob);
    // Wait for subscribed signal, it goes after changed, so we can check for both
    QTRY_COMPARE(cmodspy.size(), 1);
    arg = cmodspy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col.id(), subCollection.id());

    QVERIFY(cUnsubscribedSpy.isEmpty());
    QTRY_COMPARE(cSubscribedSpy.size(), 1);
    arg = cSubscribedSpy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col.id(), subCollection.id());
    if (fetchCol) {
        QVERIFY(!col.name().isEmpty());
        QCOMPARE(col.name(), subCollection.name());
    }

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cstatspy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // modify a collection
    monitorCol.setName(QStringLiteral("changed name"));
    auto *mod = new CollectionModifyJob(monitorCol, this);
    AKVERIFYEXEC(mod);

    QTRY_COMPARE(cmodspy.count(), 1);
    arg = cmodspy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col, monitorCol);
    if (fetchCol) {
        QCOMPARE(col.name(), QStringLiteral("changed name"));
    }

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cstatspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // move a collection
    Collection dest = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
    auto *cmove = new CollectionMoveJob(monitorCol, dest, this);
    AKVERIFYEXEC(cmove);

    QTRY_COMPARE(cmvspy.count(), 1);
    arg = cmvspy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col, monitorCol);
    QCOMPARE(col.parentCollection(), dest);
    if (fetchCol) {
        QCOMPARE(col.name(), monitorCol.name());
    }
    col = arg.at(1).value<Collection>();
    QCOMPARE(col, res3);
    col = arg.at(2).value<Collection>();
    QCOMPARE(col, dest);

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(crmspy.isEmpty());
    QVERIFY(cstatspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());

    // delete a collection
    auto *cdel = new CollectionDeleteJob(monitorCol, this);
    AKVERIFYEXEC(cdel);

    QTRY_COMPARE(crmspy.count(), 1);
    arg = crmspy.takeFirst();
    col = arg.at(0).value<Collection>();
    QCOMPARE(col.id(), monitorCol.id());
    QCOMPARE(col.parentCollection(), dest);

    QVERIFY(caddspy.isEmpty());
    QVERIFY(cmodspy.isEmpty());
    QVERIFY(cmvspy.isEmpty());
    QVERIFY(cstatspy.isEmpty());
    QVERIFY(cSubscribedSpy.isEmpty());
    QVERIFY(cUnsubscribedSpy.isEmpty());
    QVERIFY(iaddspy.isEmpty());
    QVERIFY(imodspy.isEmpty());
    QVERIFY(imvspy.isEmpty());
    QVERIFY(irmspy.isEmpty());
}

void MonitorTest::testVirtualCollectionsMonitoring()
{
    Monitor monitor;
    monitor.setCollectionMonitored(Collection(1)); // top-level 'Search' collection
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    QSignalSpy caddspy(&monitor, &Monitor::collectionAdded);

    SearchCreateJob *job = new SearchCreateJob(QStringLiteral("Test search collection"), Akonadi::SearchQuery(), this);
    AKVERIFYEXEC(job);
    QTRY_COMPARE(caddspy.count(), 1);
}
