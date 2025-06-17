/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemfetchtest.h"
#include "attributefactory.h"
#include "collectionpathresolver.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "resourceselectjob_p.h"
#include "testattribute.h"

#include "qtest_akonadi.h"

using namespace Akonadi;

QTEST_AKONADI_CORE_MAIN(ItemFetchTest)

void ItemFetchTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    qRegisterMetaType<Akonadi::Item::List>();
    AttributeFactory::registerAttribute<TestAttribute>();
}

void ItemFetchTest::testFetch()
{
    auto resolver = new CollectionPathResolver(QStringLiteral("res1"), this);
    AKVERIFYEXEC(resolver);
    int colId = resolver->collection();

    // listing of an empty folder
    auto job = new ItemFetchJob(Collection(colId), this);
    AKVERIFYEXEC(job);
    QVERIFY(job->items().isEmpty());

    resolver = new CollectionPathResolver(QStringLiteral("res1/foo"), this);
    AKVERIFYEXEC(resolver);
    int colId2 = resolver->collection();

    // listing of a non-empty folder
    job = new ItemFetchJob(Collection(colId2), this);
    QSignalSpy spy(job, &ItemFetchJob::itemsReceived);
    QVERIFY(spy.isValid());
    AKVERIFYEXEC(job);
    const Item::List items = job->items();
    QCOMPARE(items.count(), 15);

    int count = 0;
    for (int i = 0; i < spy.count(); ++i) {
        auto l = spy[i][0].value<Akonadi::Item::List>();
        for (int j = 0; j < l.count(); ++j) {
            QVERIFY(items.count() > count + j);
            QCOMPARE(items[count + j], l[j]);
        }
        count += l.count();
    }
    QCOMPARE(count, items.count());

    // check if the fetch response is parsed correctly (note: order is undefined)
    Item item;
    for (const Item &it : items) {
        if (it.remoteId() == QLatin1Char('A')) {
            item = it;
        }
    }
    QVERIFY(item.isValid());

    QCOMPARE(item.flags().count(), 3);
    QVERIFY(item.hasFlag("\\SEEN"));
    QVERIFY(item.hasFlag("\\FLAGGED"));
    QVERIFY(item.hasFlag("\\DRAFT"));

    item = Item();
    for (const Item &it : items) {
        if (it.remoteId() == QLatin1Char('B')) {
            item = it;
        }
    }
    QVERIFY(item.isValid());
    QCOMPARE(item.flags().count(), 1);
    QVERIFY(item.hasFlag("\\FLAGGED"));

    item = Item();
    for (const Item &it : items) {
        if (it.remoteId() == QLatin1Char('C')) {
            item = it;
        }
    }
    QVERIFY(item.isValid());
    QVERIFY(item.flags().isEmpty());
}

void ItemFetchTest::testResourceRetrieval()
{
    Item item(1);

    auto job = new ItemFetchJob(item, this);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().fetchAllAttributes(true);
    job->fetchScope().setCacheOnly(true);
    AKVERIFYEXEC(job);
    QCOMPARE(job->items().count(), 1);
    item = job->items().first();
    QCOMPARE(item.id(), 1LL);
    QVERIFY(!item.remoteId().isEmpty());
    QVERIFY(!item.hasPayload()); // not yet in cache
    QCOMPARE(item.attributes().count(), 1);

    job = new ItemFetchJob(item, this);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().fetchAllAttributes(true);
    job->fetchScope().setCacheOnly(false);
    AKVERIFYEXEC(job);
    QCOMPARE(job->items().count(), 1);
    item = job->items().first();
    QCOMPARE(item.id(), 1LL);
    QVERIFY(!item.remoteId().isEmpty());
    QVERIFY(item.hasPayload());
    QCOMPARE(item.attributes().count(), 1);
}

void ItemFetchTest::testIllegalFetch()
{
    // fetch non-existing folder
    auto job = new ItemFetchJob(Collection(INT_MAX), this);
    QVERIFY(!job->exec());

    // listing of root
    job = new ItemFetchJob(Collection::root(), this);
    QVERIFY(!job->exec());

    // fetch a non-existing message
    job = new ItemFetchJob(Item(INT_MAX), this);
    QVERIFY(!job->exec());
    QVERIFY(job->items().isEmpty());

    // fetch message with empty reference
    job = new ItemFetchJob(Item(), this);
    QVERIFY(!job->exec());
}

void ItemFetchTest::testMultipartFetch_data()
{
    QTest::addColumn<bool>("fetchFullPayload");
    QTest::addColumn<bool>("fetchAllAttrs");
    QTest::addColumn<bool>("fetchSinglePayload");
    QTest::addColumn<bool>("fetchSingleAttr");

    QTest::newRow("empty") << false << false << false << false;
    QTest::newRow("full") << true << true << false << false;
    QTest::newRow("full payload") << true << false << false << false;
    QTest::newRow("single payload") << false << false << true << false;
    QTest::newRow("single") << false << false << true << true;
    QTest::newRow("attr full") << false << true << false << false;
    QTest::newRow("attr single") << false << false << false << true;
    QTest::newRow("mixed cross 1") << true << false << false << true;
    QTest::newRow("mixed cross 2") << false << true << true << false;
    QTest::newRow("all") << true << true << true << true;
    QTest::newRow("all payload") << true << false << true << false;
    QTest::newRow("all attr") << false << true << true << false;
}

void ItemFetchTest::testMultipartFetch()
{
    QFETCH(bool, fetchFullPayload);
    QFETCH(bool, fetchAllAttrs);
    QFETCH(bool, fetchSinglePayload);
    QFETCH(bool, fetchSingleAttr);

    auto resolver = new CollectionPathResolver(QStringLiteral("res1/foo"), this);
    AKVERIFYEXEC(resolver);
    int colId = resolver->collection();

    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload<QByteArray>("body data");
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "extra data";
    auto job = new ItemCreateJob(item, Collection(colId), this);
    AKVERIFYEXEC(job);
    Item ref = job->item();

    auto fjob = new ItemFetchJob(ref, this);
    fjob->setCollection(Collection(colId));
    if (fetchFullPayload) {
        fjob->fetchScope().fetchFullPayload();
    }
    if (fetchAllAttrs) {
        fjob->fetchScope().fetchAttribute<TestAttribute>();
    }
    if (fetchSinglePayload) {
        fjob->fetchScope().fetchPayloadPart(Item::FullPayload);
    }
    if (fetchSingleAttr) {
        fjob->fetchScope().fetchAttribute<TestAttribute>();
    }

    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items().first();

    if (fetchFullPayload || fetchSinglePayload) {
        QCOMPARE(item.loadedPayloadParts().count(), 1);
        QVERIFY(item.hasPayload());
        QCOMPARE(item.payload<QByteArray>(), QByteArray("body data"));
    } else {
        QCOMPARE(item.loadedPayloadParts().count(), 0);
        QVERIFY(!item.hasPayload());
    }

    if (fetchAllAttrs || fetchSingleAttr) {
        QCOMPARE(item.attributes().count(), 1);
        QVERIFY(item.hasAttribute<TestAttribute>());
        QCOMPARE(item.attribute<TestAttribute>()->data, QByteArray("extra data"));
    } else {
        QCOMPARE(item.attributes().count(), 0);
    }

    // cleanup
    auto djob = new ItemDeleteJob(ref, this);
    AKVERIFYEXEC(djob);
}

void ItemFetchTest::testRidFetch()
{
    Item item;
    item.setRemoteId(QStringLiteral("A"));
    Collection col;
    col.setRemoteId(QStringLiteral("10"));

    auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"), this);
    AKVERIFYEXEC(select);

    auto job = new ItemFetchJob(item, this);
    job->setCollection(col);
    AKVERIFYEXEC(job);
    QCOMPARE(job->items().count(), 1);
    item = job->items().first();
    QVERIFY(item.isValid());
    QCOMPARE(item.remoteId(), QString::fromLatin1("A"));
    QCOMPARE(item.mimeType(), QString::fromLatin1("application/octet-stream"));
}

void ItemFetchTest::testAncestorRetrieval()
{
    auto job = new ItemFetchJob(Item(1), this);
    job->fetchScope().setAncestorRetrieval(ItemFetchScope::All);
    AKVERIFYEXEC(job);
    QCOMPARE(job->items().count(), 1);
    const Item item = job->items().first();
    QVERIFY(item.isValid());
    QCOMPARE(item.remoteId(), QString::fromLatin1("A"));
    QCOMPARE(item.mimeType(), QString::fromLatin1("application/octet-stream"));
    const Collection c = item.parentCollection();
    QCOMPARE(c.remoteId(), QLatin1StringView("10"));
    const Collection c2 = c.parentCollection();
    QCOMPARE(c2.remoteId(), QLatin1StringView("6"));
    const Collection c3 = c2.parentCollection();
    QCOMPARE(c3, Collection::root());
}

void ItemFetchTest::testFetchBatching()
{
    static constexpr size_t ItemCount = 20'010; // batch size is 10000, so we should need two full and one partial batch

    auto resolver = new CollectionPathResolver(QStringLiteral("res1/foo"), this);
    AKVERIFYEXEC(resolver);
    int colId = resolver->collection();

    auto *itemCountJob = new ItemFetchJob(Collection(colId));
    AKVERIFYEXEC(itemCountJob);
    const auto origItemsCount = itemCountJob->items().size();

    QList<Item> items;
    items.reserve(ItemCount);
    qDebug() << "Creating" << ItemCount << "items";
    ItemCreateJob *job = nullptr;
    for (size_t i = 0; i < ItemCount; ++i) {
        Item item;
        item.setMimeType(QStringLiteral("application/octet-stream"));
        item.setPayload<QByteArray>("body data");
        job = new ItemCreateJob(item, Collection(colId), this);
        connect(job, &ItemCreateJob::result, this, [job, &items]() {
            QCOMPARE(job->error(), KJob::NoError);
            items.push_back(job->item());
            if (items.size() % 1000 == 0) {
                qDebug() << "Created" << items.size() << "items";
            }
        });
    }
    // Wait for the last one synchronously
    AKVERIFYEXEC(job);

    // Now try to fetch all the 20'010 items by explicitly listing them all
    qDebug() << "Fetching" << items.size() << "items";
    auto *fetchJob = new ItemFetchJob(items, this);
    AKVERIFYEXEC(fetchJob);
    qDebug() << "Done";

    QCOMPARE(fetchJob->items().size(), ItemCount);

    // And now try to delete all of them by listing them explicitly
    qDebug() << "Deleting" << items.size() << "items";
    auto *deleteJob = new ItemDeleteJob(items, this);
    AKVERIFYEXEC(deleteJob);
    qDebug() << "Done";

    // Finally, confirm that all the items are gone
    itemCountJob = new ItemFetchJob(Collection(colId));
    AKVERIFYEXEC(itemCountJob);
    QCOMPARE(origItemsCount, itemCountJob->items().size());
}

#include "moc_itemfetchtest.cpp"
