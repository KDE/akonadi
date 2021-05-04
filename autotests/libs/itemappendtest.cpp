/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemappendtest.h"
#include "qtest_akonadi.h"

#include "control.h"
#include "testattribute.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "attributefactory.h"
#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QDebug>

using namespace Akonadi;

QTEST_AKONADIMAIN(ItemAppendTest)

void ItemAppendTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
    AkonadiTest::setAllResourcesOffline();
    AttributeFactory::registerAttribute<TestAttribute>();
}

void ItemAppendTest::testItemAppend_data()
{
    QTest::addColumn<QString>("remoteId");

    QTest::newRow("empty") << QString();
    QTest::newRow("non empty") << QStringLiteral("remote-id");
    QTest::newRow("whitespace") << QStringLiteral("remote id");
    QTest::newRow("quotes") << QStringLiteral("\"remote\" id");
    QTest::newRow("brackets") << QStringLiteral("[remote id]");
    QTest::newRow("RID length limit") << QStringLiteral("a").repeated(1024);
}

void ItemAppendTest::testItemAppend()
{
    const Collection testFolder1(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(testFolder1.isValid());

    QFETCH(QString, remoteId);
    Item ref; // for cleanup

    Item item(-1);
    item.setRemoteId(remoteId);
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setFlag("TestFlag");
    item.setSize(3456);
    auto job = new ItemCreateJob(item, testFolder1, this);
    AKVERIFYEXEC(job);
    ref = job->item();
    QCOMPARE(ref.parentCollection(), testFolder1);

    auto fjob = new ItemFetchJob(testFolder1, this);
    fjob->fetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    QCOMPARE(fjob->items()[0], ref);
    QCOMPARE(fjob->items()[0].remoteId(), remoteId);
    QVERIFY(fjob->items()[0].flags().contains("TestFlag"));
    QCOMPARE(fjob->items()[0].parentCollection(), ref.parentCollection());

    qint64 size = 3456;
    QCOMPARE(fjob->items()[0].size(), size);

    auto djob = new ItemDeleteJob(ref, this);
    AKVERIFYEXEC(djob);

    fjob = new ItemFetchJob(testFolder1, this);
    AKVERIFYEXEC(fjob);
    QVERIFY(fjob->items().isEmpty());
}

void ItemAppendTest::testContent_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("null") << QByteArray();
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("nullbyte") << QByteArray("\0", 1);
    QTest::newRow("nullbyte2") << QByteArray("\0X", 2);
    QString utf8string = QStringLiteral("äöüß@€µøđ¢©®");
    QTest::newRow("utf8") << utf8string.toUtf8();
    QTest::newRow("newlines") << QByteArray("\nsome\n\nbreaked\ncontent\n\n");
    QByteArray b;
    QTest::newRow("big") << b.fill('a', 1U << 20);
    QTest::newRow("bignull") << b.fill('\0', 1U << 20);
    QTest::newRow("bigcr") << b.fill('\r', 1U << 20);
    QTest::newRow("biglf") << b.fill('\n', 1U << 20);
}

void ItemAppendTest::testContent()
{
    const Collection testFolder1(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(testFolder1.isValid());

    QFETCH(QByteArray, data);

    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    if (!data.isNull()) {
        item.setPayload(data);
    }

    auto job = new ItemCreateJob(item, testFolder1, this);
    AKVERIFYEXEC(job);
    Item ref = job->item();

    auto fjob = new ItemFetchJob(testFolder1, this);
    fjob->fetchScope().setCacheOnly(true);
    fjob->fetchScope().fetchFullPayload();
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    Item item2 = fjob->items().first();
    QCOMPARE(item2.hasPayload(), !data.isNull());
    if (item2.hasPayload()) {
        QCOMPARE(item2.payload<QByteArray>(), data);
    }

    auto djob = new ItemDeleteJob(ref, this);
    AKVERIFYEXEC(djob);
}

void ItemAppendTest::testNewMimetype()
{
    const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(col.isValid());

    Item item;
    item.setMimeType(QStringLiteral("application/new-type"));
    auto job = new ItemCreateJob(item, col, this);
    AKVERIFYEXEC(job);

    item = job->item();
    QVERIFY(item.isValid());

    auto fetch = new ItemFetchJob(item, this);
    AKVERIFYEXEC(fetch);
    QCOMPARE(fetch->items().count(), 1);
    QCOMPARE(fetch->items().first().mimeType(), item.mimeType());
}

void ItemAppendTest::testIllegalAppend()
{
    const Collection testFolder1(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(testFolder1.isValid());

    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));

    // adding item to non-existing collection
    auto job = new ItemCreateJob(item, Collection(INT_MAX), this);
    QVERIFY(!job->exec());

    // adding item into a collection which can't handle items of this type
    const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo/bla")));
    QVERIFY(col.isValid());
    job = new ItemCreateJob(item, col, this);
    QEXPECT_FAIL("", "Test not yet implemented in the server.", Continue);
    QVERIFY(!job->exec());
}

void ItemAppendTest::testMultipartAppend()
{
    const Collection testFolder1(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(testFolder1.isValid());

    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload<QByteArray>("body data");
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "extra data";
    item.setFlag("TestFlag");
    auto job = new ItemCreateJob(item, testFolder1, this);
    AKVERIFYEXEC(job);
    Item ref = job->item();

    auto fjob = new ItemFetchJob(ref, this);
    fjob->fetchScope().fetchFullPayload();
    fjob->fetchScope().fetchAttribute<TestAttribute>();
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items().first();
    QCOMPARE(item.payload<QByteArray>(), QByteArray("body data"));
    QVERIFY(item.hasAttribute<TestAttribute>());
    QCOMPARE(item.attribute<TestAttribute>()->data, QByteArray("extra data"));
    QVERIFY(item.flags().contains("TestFlag"));

    auto djob = new ItemDeleteJob(ref, this);
    AKVERIFYEXEC(djob);
}

void ItemAppendTest::testInvalidMultipartAppend()
{
    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload<QByteArray>("body data");
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = "extra data";
    item.setFlag("TestFlag");
    auto job = new ItemCreateJob(item, Collection(-1), this);
    QVERIFY(!job->exec());

    Item item2;
    item2.setMimeType(QStringLiteral("application/octet-stream"));
    item2.setPayload<QByteArray>("more body data");
    item2.attribute<TestAttribute>(Item::AddIfMissing)->data = "even more extra data";
    item2.setFlag("TestFlag");
    auto job2 = new ItemCreateJob(item2, Collection(-1), this);
    QVERIFY(!job2->exec());
}

void ItemAppendTest::testItemSize_data()
{
    QTest::addColumn<Akonadi::Item>("item");
    QTest::addColumn<qint64>("size");

    Item i(QStringLiteral("application/octet-stream"));
    i.setPayload(QByteArray("ABCD"));

    QTest::newRow("auto size") << i << 56LL;
    i.setSize(3);
    QTest::newRow("too small") << i << 56LL;
    i.setSize(100);
    QTest::newRow("too large") << i << 100LL;
}

void ItemAppendTest::testItemSize()
{
    QFETCH(Akonadi::Item, item);
    QFETCH(qint64, size);

    const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(col.isValid());

    auto create = new ItemCreateJob(item, col, this);
    AKVERIFYEXEC(create);
    Item newItem = create->item();

    auto fetch = new ItemFetchJob(newItem, this);
    AKVERIFYEXEC(fetch);
    QCOMPARE(fetch->items().count(), 1);

    QCOMPARE(fetch->items().first().size(), size);
}

void ItemAppendTest::testItemMerge_data()
{
    QTest::addColumn<Akonadi::Item>("item1");
    QTest::addColumn<Akonadi::Item>("item2");
    QTest::addColumn<Akonadi::Item>("mergedItem");
    QTest::addColumn<bool>("silent");

    {
        Item i1(QStringLiteral("application/octet-stream"));
        i1.setPayload(QByteArray("ABCD"));
        i1.setSize(56); // take compression into account
        i1.setRemoteId(QStringLiteral("XYZ"));
        i1.setGid(QStringLiteral("XYZ"));
        i1.setFlag("TestFlag1");
        i1.setRemoteRevision(QStringLiteral("5"));

        Item i2(QStringLiteral("application/octet-stream"));
        i2.setPayload(QByteArray("DEFGH"));
        i2.setSize(60); // the compression into account
        i2.setRemoteId(QStringLiteral("XYZ"));
        i2.setGid(QStringLiteral("XYZ"));
        i2.setFlag("TestFlag2");
        i2.setRemoteRevision(QStringLiteral("6"));

        Item mergedItem(i2);
        mergedItem.setFlag("TestFlag1");

        QTest::newRow("merge") << i1 << i2 << mergedItem << false;
        QTest::newRow("merge (silent)") << i1 << i2 << mergedItem << true;
    }
    {
        Item i1(QStringLiteral("application/octet-stream"));
        i1.setPayload(QByteArray("ABCD"));
        i1.setSize(56); // take compression into account
        i1.setRemoteId(QStringLiteral("RID2"));
        i1.setGid(QStringLiteral("GID2"));
        i1.setFlag("TestFlag1");
        i1.setRemoteRevision(QStringLiteral("5"));

        Item i2(QStringLiteral("application/octet-stream"));
        i2.setRemoteId(QStringLiteral("RID2"));
        i2.setGid(QStringLiteral("GID2"));
        i2.setFlags(Item::Flags() << "TestFlag2");
        i2.setRemoteRevision(QStringLiteral("6"));

        Item mergedItem(i1);
        mergedItem.setFlags(i2.flags());
        mergedItem.setRemoteRevision(i2.remoteRevision());

        QTest::newRow("overwrite flags, and don't remove existing payload") << i1 << i2 << mergedItem << false;
        QTest::newRow("overwrite flags, and don't remove existing payload (silent)") << i1 << i2 << mergedItem << true;
    }
}

void ItemAppendTest::testItemMerge()
{
    QFETCH(Akonadi::Item, item1);
    QFETCH(Akonadi::Item, item2);
    QFETCH(Akonadi::Item, mergedItem);
    QFETCH(bool, silent);

    const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(col.isValid());

    auto create = new ItemCreateJob(item1, col, this);
    AKVERIFYEXEC(create);
    const Item createdItem = create->item();

    auto merge = new ItemCreateJob(item2, col, this);
    ItemCreateJob::MergeOptions options = ItemCreateJob::GID | ItemCreateJob::RID;
    if (silent) {
        options |= ItemCreateJob::Silent;
    }
    merge->setMerge(options);
    AKVERIFYEXEC(merge);

    QCOMPARE(merge->item().id(), createdItem.id());
    if (!silent) {
        QCOMPARE(merge->item().gid(), mergedItem.gid());
        QCOMPARE(merge->item().remoteId(), mergedItem.remoteId());
        QCOMPARE(merge->item().remoteRevision(), mergedItem.remoteRevision());
        QCOMPARE(merge->item().payloadData(), mergedItem.payloadData());
        QCOMPARE(merge->item().size(), mergedItem.size());
        qDebug() << merge->item().flags() << mergedItem.flags();
        QCOMPARE(merge->item().flags(), mergedItem.flags());
    }

    if (merge->item().id() != createdItem.id()) {
        auto del = new ItemDeleteJob(merge->item(), this);
        AKVERIFYEXEC(del);
    }
    auto del = new ItemDeleteJob(createdItem, this);
    AKVERIFYEXEC(del);
}

void ItemAppendTest::testForeignPayload()
{
    const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(col.isValid());

    const QString filePath = QString::fromUtf8(qgetenv("TMPDIR")) + QStringLiteral("/foreignPayloadFile.mbox");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("123456789");
    file.close();

    Item item(QStringLiteral("application/octet-stream"));
    item.setPayloadPath(filePath);
    item.setRemoteId(QStringLiteral("RID3"));
    item.setSize(9);

    auto create = new ItemCreateJob(item, col, this);
    AKVERIFYEXEC(create);

    auto ref = create->item();

    auto fetch = new ItemFetchJob(ref, this);
    fetch->fetchScope().fetchFullPayload(true);
    AKVERIFYEXEC(fetch);
    const auto items = fetch->items();
    QCOMPARE(items.size(), 1);
    item = items[0];

    QVERIFY(item.hasPayload<QByteArray>());
    QCOMPARE(item.payload<QByteArray>(), QByteArray("123456789"));

    auto del = new ItemDeleteJob(item, this);
    AKVERIFYEXEC(del);

    // Make sure Akonadi does not delete a foreign payload
    QVERIFY(file.exists());
    QVERIFY(file.remove());
}
