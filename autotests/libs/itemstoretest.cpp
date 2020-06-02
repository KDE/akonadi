/*
  Copyright (c) 2006 Volker Krause <vkrause@kde.org>
  Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

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

#include "itemstoretest.h"

#include "control.h"
#include "testattribute.h"
#include "agentmanager.h"
#include "agentinstance.h"
#include "attributefactory.h"
#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "itemmodifyjob_p.h"
#include "resourceselectjob_p.h"
#include "qtest_akonadi.h"

using namespace Akonadi;

QTEST_AKONADIMAIN(ItemStoreTest)

static Collection res1_foo;
static Collection res2;
static Collection res3;

void ItemStoreTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
    AttributeFactory::registerAttribute<TestAttribute>();

    // get the collections we run the tests on
    res1_foo = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
    QVERIFY(res1_foo.isValid());
    res2 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res2")));
    QVERIFY(res2.isValid());
    res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    QVERIFY(res3.isValid());

    AkonadiTest::setAllResourcesOffline();
}

void ItemStoreTest::testFlagChange()
{
    ItemFetchJob *fjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    Item item = fjob->items()[0];

    // add a flag
    Item::Flags origFlags = item.flags();
    Item::Flags expectedFlags = origFlags;
    expectedFlags.insert("added_test_flag_1");
    item.setFlag("added_test_flag_1");
    ItemModifyJob *sjob = new ItemModifyJob(item, this);
    AKVERIFYEXEC(sjob);

    fjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items()[0];
    QCOMPARE(item.flags().count(), expectedFlags.count());
    Item::Flags diff = expectedFlags - item.flags();
    QVERIFY(diff.isEmpty());

    // set flags
    expectedFlags.insert("added_test_flag_2");
    item.setFlags(expectedFlags);
    sjob = new ItemModifyJob(item, this);
    AKVERIFYEXEC(sjob);

    fjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items()[0];
    QCOMPARE(item.flags().count(), expectedFlags.count());
    diff = expectedFlags - item.flags();
    QVERIFY(diff.isEmpty());

    // remove a flag
    item.clearFlag("added_test_flag_1");
    item.clearFlag("added_test_flag_2");
    sjob = new ItemModifyJob(item, this);
    AKVERIFYEXEC(sjob);

    fjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items()[0];
    QCOMPARE(item.flags().count(), origFlags.count());
    diff = origFlags - item.flags();
    QVERIFY(diff.isEmpty());
}

void ItemStoreTest::testDataChange_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("simple") << QByteArray("testbody");
    QTest::newRow("null") << QByteArray();
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("nullbyte") << QByteArray("\0", 1);
    QTest::newRow("nullbyte2") << QByteArray("\0X", 2);
    QTest::newRow("linebreaks") << QByteArray("line1\nline2\n\rline3\rline4\r\n");
    QTest::newRow("linebreaks2") << QByteArray("line1\r\nline2\r\n\r\n");
    QTest::newRow("linebreaks3") << QByteArray("line1\nline2");
    QByteArray b;
    QTest::newRow("big") << b.fill('a', 1 << 20);
    QTest::newRow("bignull") << b.fill('\0', 1 << 20);
    QTest::newRow("bigcr") << b.fill('\r', 1 << 20);
    QTest::newRow("biglf") << b.fill('\n', 1 << 20);
}

void ItemStoreTest::testDataChange()
{
    QFETCH(QByteArray, data);

    Item item;
    ItemFetchJob *prefetchjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(prefetchjob);
    item = prefetchjob->items()[0];
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload(data);
    QCOMPARE(item.payload<QByteArray>(), data);

    // modify data
    ItemModifyJob *sjob = new ItemModifyJob(item);
    AKVERIFYEXEC(sjob);

    ItemFetchJob *fjob = new ItemFetchJob(Item(1));
    fjob->fetchScope().fetchFullPayload();
    fjob->fetchScope().setCacheOnly(true);
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items()[0];
    QVERIFY(item.hasPayload<QByteArray>());
    QCOMPARE(item.payload<QByteArray>(), data);
    QEXPECT_FAIL("null", "STORE will not update item size on 0 sizes", Continue);
    QEXPECT_FAIL("empty", "STORE will not update item size on 0 sizes", Continue);
    QCOMPARE(item.size(), static_cast<qint64>(data.size()));
}

void ItemStoreTest::testRemoteId_data()
{
    QTest::addColumn<QString>("rid");
    QTest::addColumn<QString>("exprid");

    QTest::newRow("set") << QStringLiteral("A") << QStringLiteral("A");
    QTest::newRow("no-change") << QString() << QStringLiteral("A");
    QTest::newRow("clear") << QStringLiteral("") << QStringLiteral("");
    QTest::newRow("reset") << QStringLiteral("A") << QStringLiteral("A");
    QTest::newRow("utf8") << QStringLiteral("ä ö ü @") << QStringLiteral("ä ö ü @");
}

void ItemStoreTest::testRemoteId()
{
    QFETCH(QString, rid);
    QFETCH(QString, exprid);

    // pretend to be a resource, we cannot change remote identifiers otherwise
    ResourceSelectJob *rsel = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"), this);
    AKVERIFYEXEC(rsel);

    ItemFetchJob *prefetchjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(prefetchjob);
    Item item = prefetchjob->items()[0];

    item.setRemoteId(rid);
    ItemModifyJob *store = new ItemModifyJob(item, this);
    store->disableRevisionCheck();
    store->setIgnorePayload(true);   // we only want to update the remote id
    AKVERIFYEXEC(store);

    ItemFetchJob *fetch = new ItemFetchJob(item, this);
    AKVERIFYEXEC(fetch);
    QCOMPARE(fetch->items().count(), 1);
    item = fetch->items().at(0);
    QEXPECT_FAIL("clear", "Clearing RID by clients is currently forbidden to avoid conflicts.", Continue);
    QCOMPARE(item.remoteId().toUtf8(), exprid.toUtf8());

    // no longer pretend to be a resource
    rsel = new ResourceSelectJob(QString(), this);
    AKVERIFYEXEC(rsel);
}

void ItemStoreTest::testMultiPart()
{
    ItemFetchJob *prefetchjob = new ItemFetchJob(Item(1));
    AKVERIFYEXEC(prefetchjob);
    QCOMPARE(prefetchjob->items().count(), 1);
    Item item = prefetchjob->items()[0];
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload<QByteArray>("testmailbody");
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "extra";

    // store item
    ItemModifyJob *sjob = new ItemModifyJob(item);
    AKVERIFYEXEC(sjob);

    ItemFetchJob *fjob = new ItemFetchJob(Item(1));
    fjob->fetchScope().fetchAttribute<TestAttribute>();
    fjob->fetchScope().fetchFullPayload();
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items()[0];
    QVERIFY(item.hasPayload<QByteArray>());
    QCOMPARE(item.payload<QByteArray>(), QByteArray("testmailbody"));
    QVERIFY(item.hasAttribute<TestAttribute>());
    QCOMPARE(item.attribute<TestAttribute>()->data, QByteArray("extra"));

    // clean up
    item.removeAttribute("EXTRA");
    sjob = new ItemModifyJob(item);
    AKVERIFYEXEC(sjob);
}

void ItemStoreTest::testPartRemove()
{
    ItemFetchJob *prefetchjob = new ItemFetchJob(Item(2));
    AKVERIFYEXEC(prefetchjob);
    Item item = prefetchjob->items()[0];
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "extra";

    // store item
    ItemModifyJob *sjob = new ItemModifyJob(item);
    AKVERIFYEXEC(sjob);

    // fetch item and its parts (should be RFC822, HEAD and EXTRA)
    ItemFetchJob *fjob = new ItemFetchJob(Item(2));
    fjob->fetchScope().fetchFullPayload();
    fjob->fetchScope().fetchAllAttributes();
    fjob->fetchScope().setCacheOnly(true);
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items()[0];
    QCOMPARE(item.attributes().count(), 2);
    QVERIFY(item.hasAttribute<TestAttribute>());

    // remove a part
    item.removeAttribute<TestAttribute>();
    sjob = new ItemModifyJob(item);
    AKVERIFYEXEC(sjob);

    // fetch item again (should only have RFC822 and HEAD left)
    ItemFetchJob *fjob2 = new ItemFetchJob(Item(2));
    fjob2->fetchScope().fetchFullPayload();
    fjob2->fetchScope().fetchAllAttributes();
    fjob2->fetchScope().setCacheOnly(true);
    AKVERIFYEXEC(fjob2);
    QCOMPARE(fjob2->items().count(), 1);
    item = fjob2->items()[0];
    QCOMPARE(item.attributes().count(), 1);
    QVERIFY(!item.hasAttribute<TestAttribute>());
}

void ItemStoreTest::testRevisionCheck()
{
    // fetch same item twice
    Item ref(2);
    ItemFetchJob *prefetchjob = new ItemFetchJob(ref);
    AKVERIFYEXEC(prefetchjob);
    QCOMPARE(prefetchjob->items().count(), 1);
    Item item1 = prefetchjob->items()[0];
    Item item2 = prefetchjob->items()[0];

    // store first item unmodified
    ItemModifyJob *sjob = new ItemModifyJob(item1);
    AKVERIFYEXEC(sjob);

    // store the first item with modifications (should work)
    item1.attribute<TestAttribute>(Item::AddIfMissing)->data = "random stuff 1";
    sjob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(sjob);

    // try to store second item with modifications (should be detected as a conflict)
    item2.attribute<TestAttribute>(Item::AddIfMissing)->data = "random stuff 2";
    ItemModifyJob *sjob2 = new ItemModifyJob(item2);
    sjob2->disableAutomaticConflictHandling();
    QVERIFY(!sjob2->exec());

    // fetch same again
    prefetchjob = new ItemFetchJob(ref);
    AKVERIFYEXEC(prefetchjob);
    item1 = prefetchjob->items()[0];

    // delete item
    ItemDeleteJob *djob = new ItemDeleteJob(ref, this);
    AKVERIFYEXEC(djob);

    // try to store it
    sjob = new ItemModifyJob(item1);
    QVERIFY(!sjob->exec());
}

void ItemStoreTest::testModificationTime()
{
    Item item;
    item.setMimeType(QStringLiteral("text/directory"));
    QVERIFY(item.modificationTime().isNull());

    ItemCreateJob *job = new ItemCreateJob(item, res1_foo);
    AKVERIFYEXEC(job);

    // The item should have a datetime set now.
    item = job->item();
    QVERIFY(!item.modificationTime().isNull());
    QDateTime initialDateTime = item.modificationTime();

    // Fetch the same item again.
    Item item2(item.id());
    ItemFetchJob *fjob = new ItemFetchJob(item2, this);
    AKVERIFYEXEC(fjob);
    item2 = fjob->items().first();
    QCOMPARE(initialDateTime, item2.modificationTime());

    // Lets wait at least a second, which is the resolution of mtime
    QTest::qWait(1000);

    // Modify the item
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "extra";
    ItemModifyJob *mjob = new ItemModifyJob(item);
    AKVERIFYEXEC(mjob);

    // The item should still have a datetime set and that date should be somewhere
    // after the initialDateTime.
    item = mjob->item();
    QVERIFY(!item.modificationTime().isNull());
    QVERIFY(initialDateTime < item.modificationTime());

    // Fetch the item after modification.
    Item item3(item.id());
    ItemFetchJob *fjob2 = new ItemFetchJob(item3, this);
    AKVERIFYEXEC(fjob2);

    // item3 should have the same modification time as item.
    item3 = fjob2->items().first();
    QCOMPARE(item3.modificationTime(), item.modificationTime());

    // Clean up
    ItemDeleteJob *idjob = new ItemDeleteJob(item, this);
    AKVERIFYEXEC(idjob);
}

void ItemStoreTest::testRemoteIdRace()
{
    // Create an item and store it
    Item item;
    item.setMimeType(QStringLiteral("text/directory"));
    ItemCreateJob *job = new ItemCreateJob(item, res1_foo);
    AKVERIFYEXEC(job);

    // Fetch the same item again. It should not have a remote Id yet, as the resource
    // is offline.
    // The remote id should be null, not only empty, so that item modify jobs with this
    // item don't overwrite the remote id.
    Item item2(job->item().id());
    ItemFetchJob *fetchJob = new ItemFetchJob(item2);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().size(), 1);
    QVERIFY(fetchJob->items().first().remoteId().isEmpty());
}

void ItemStoreTest::itemModifyJobShouldOnlySendModifiedAttributes()
{
    // Given an item with an attribute (created on the server)
    Item item;
    item.setMimeType(QStringLiteral("text/directory"));
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "initial";
    ItemCreateJob *job = new ItemCreateJob(item, res1_foo);
    AKVERIFYEXEC(job);
    item = job->item();
    QCOMPARE(item.attributes().count(), 1);

    // When one job modifies this attribute, and another one does an unrelated change
    Item item1(item.id());
    item1.attribute<TestAttribute>(Item::AddIfMissing)->data = "modified";
    ItemModifyJob *mjob = new ItemModifyJob(item1);
    mjob->disableRevisionCheck();
    AKVERIFYEXEC(mjob);

    item.setFlag("added_test_flag_1");
    // this job shouldn't send the old attribute again
    ItemModifyJob *mjob2 = new ItemModifyJob(item);
    mjob2->disableRevisionCheck();
    AKVERIFYEXEC(mjob2);

    // Then the item has the new value for the attribute (the other one didn't send the old attribute value)
    {
        auto *fetchJob = new ItemFetchJob(Item(item.id()));
        ItemFetchScope fetchScope;
        fetchScope.fetchAllAttributes(true);
        fetchJob->setFetchScope(fetchScope);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->items().size(), 1);
        const Item fetchedItem = fetchJob->items().first();
        QCOMPARE(fetchedItem.flags().count(), 1);
        QCOMPARE(fetchedItem.attributes().count(), 1);
        QCOMPARE(fetchedItem.attribute<TestAttribute>()->data, "modified");
    }
}

class ParallelJobsRunner
{
public:
    explicit ParallelJobsRunner(int count)
        : numSessions(count)
    {
        sessions.reserve(numSessions);
        modifyJobs.reserve(numSessions);
        for (int i = 0 ; i < numSessions; ++i) {
            auto *session = new Session(QByteArray::number(i));
            sessions.push_back(session);
        }
    }

    ~ParallelJobsRunner()
    {
        qDeleteAll(sessions);
    }

    void addJob(ItemModifyJob *mjob)
    {
        modifyJobs.push_back(mjob);
        QObject::connect(mjob, &KJob::result, [mjob, this]() {
            if (mjob->error()) {
                errors.append(mjob->errorString());
            }
            doneJobs.push_back(mjob);
        });
    }

    void waitForAllJobs()
    {
        for (int i = 0 ; i < modifyJobs.count(); ++i) {
            ItemModifyJob *mjob = modifyJobs.at(i);
            if (!doneJobs.contains(mjob)) {
                QSignalSpy spy(mjob, &ItemModifyJob::result);
                QVERIFY(spy.wait());
                if (mjob->error()) {
                    qWarning() << mjob->errorString();

}
                QCOMPARE(mjob->error(), KJob::NoError);
            }
        }
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QLatin1String("; "))));
    }

    const int numSessions;
    std::vector<Session *> sessions;
    QVector<ItemModifyJob *> modifyJobs, doneJobs;
    QStringList errors;
};

void ItemStoreTest::testParallelJobsAddingAttributes()
{
    // Given an item (created on the server)
    Item::Id itemId;
    {
        Item item;
        item.setMimeType(QStringLiteral("text/directory"));
        ItemCreateJob *job = new ItemCreateJob(item, res1_foo);
        AKVERIFYEXEC(job);
        itemId = job->item().id();
        QVERIFY(itemId >= 0);
    }

    // When adding N attributes from N different sessions (e.g. threads or processes)
    ParallelJobsRunner runner(10);
    for (int i = 0 ; i < runner.numSessions; ++i) {
        Item item(itemId);
        Attribute *attr = AttributeFactory::createAttribute("type" + QByteArray::number(i));
        QVERIFY(attr);
        attr->deserialize("attr" + QByteArray::number(i));
        item.addAttribute(attr);
        ItemModifyJob *mjob = new ItemModifyJob(item, runner.sessions.at(i));
        runner.addJob(mjob);
    }
    runner.waitForAllJobs();

    // Then the item should have all attributes
    ItemFetchJob *fetchJob = new ItemFetchJob(Item(itemId));
    ItemFetchScope fetchScope;
    fetchScope.fetchAllAttributes(true);
    fetchJob->setFetchScope(fetchScope);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().size(), 1);
    const Item fetchedItem = fetchJob->items().first();
    QCOMPARE(fetchedItem.attributes().count(), runner.numSessions);
}
