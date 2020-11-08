/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qtest_akonadi.h"
#include "collection.h"
#include "collectionpathresolver.h"
#include "control.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "transactionjobs.h"
#include "tagcreatejob.h"
#include "itemmodifyjob.h"
#include "resourceselectjob_p.h"
#include "monitor.h"

#include <QObject>

#include <sys/types.h>

using namespace Akonadi;

class ItemDeleteTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        Control::start();
    }

    void testIllegalDelete()
    {
        ItemDeleteJob *djob = new ItemDeleteJob(Item(INT_MAX), this);
        QVERIFY(!djob->exec());

        // make sure a failed delete doesn't leave a transaction open (the kpilot bug)
        auto *tjob = new TransactionRollbackJob(this);
        QVERIFY(!tjob->exec());
    }

    void testDelete()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy spy(monitor.get(), &Monitor::itemsRemoved);

        ItemFetchJob *fjob = new ItemFetchJob(Item(1), this);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->items().count(), 1);

        ItemDeleteJob *djob = new ItemDeleteJob(Item(1), this);
        AKVERIFYEXEC(djob);

        fjob = new ItemFetchJob(Item(1), this);
        QVERIFY(!fjob->exec());

        QTRY_COMPARE(spy.count(), 1);
        auto items = spy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(items.count(), 1);
        QCOMPARE(items.at(0).id(), 1);
        QVERIFY(items.at(0).parentCollection().isValid());
    }

    void testDeleteFromUnselectedCollection()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy spy(monitor.get(), &Monitor::itemsRemoved);

        const QString path = QLatin1String("res1") +
                             CollectionPathResolver::pathDelimiter() +
                             QLatin1String("foo");
        auto *rjob = new CollectionPathResolver(path, this);
        AKVERIFYEXEC(rjob);

        ItemFetchJob *fjob = new ItemFetchJob(Collection(rjob->collection()), this);
        AKVERIFYEXEC(fjob);

        const Item::List items = fjob->items();
        QVERIFY(!items.isEmpty());

        fjob = new ItemFetchJob(items[ 0 ], this);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->items().count(), 1);

        auto *djob = new ItemDeleteJob(items[ 0 ], this);
        AKVERIFYEXEC(djob);

        fjob = new ItemFetchJob(items[ 0 ], this);
        QVERIFY(!fjob->exec());

        QTRY_COMPARE(spy.count(), 1);
        auto ntfItems = spy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(ntfItems.count(), 1);
        QCOMPARE(ntfItems.at(0).id(), items[0].id());
        QVERIFY(ntfItems.at(0).parentCollection().isValid());
    }

    void testRidDelete()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy spy(monitor.get(), &Monitor::itemsRemoved);

        {
            ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
            AKVERIFYEXEC(select);
        }
        const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        QVERIFY(col.isValid());

        Item i;
        i.setRemoteId(QStringLiteral("C"));

        auto *fjob = new ItemFetchJob(i, this);
        fjob->setCollection(col);
        AKVERIFYEXEC(fjob);
        auto items = fjob->items();
        QCOMPARE(items.count(), 1);

        auto *djob = new ItemDeleteJob(i, this);
        AKVERIFYEXEC(djob);

        QTRY_COMPARE(spy.count(), 1);
        auto ntfItems = spy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(ntfItems.count(), 1);
        QCOMPARE(ntfItems.at(0).id(), items[0].id());
        QVERIFY(ntfItems.at(0).parentCollection().isValid());

        fjob = new ItemFetchJob(i, this);
        fjob->setCollection(col);
        QVERIFY(!fjob->exec());
        {
            ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral(""));
            AKVERIFYEXEC(select);
        }
    }

    void testTagDelete()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy spy(monitor.get(), &Monitor::itemsRemoved);

        // Create tag
        Tag tag;
        tag.setName(QStringLiteral("Tag1"));
        tag.setGid("Tag1");
        auto *tjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(tjob);
        tag = tjob->tag();

        const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        QVERIFY(col.isValid());

        Item i;
        i.setRemoteId(QStringLiteral("D"));

        auto *fjob = new ItemFetchJob(i, this);
        fjob->setCollection(col);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->items().count(), 1);

        i = fjob->items().first();
        i.setTag(tag);
        auto *mjob = new ItemModifyJob(i, this);
        AKVERIFYEXEC(mjob);

        // Delete the tagged item
        auto *djob = new ItemDeleteJob(tag, this);
        AKVERIFYEXEC(djob);

        QTRY_COMPARE(spy.count(), 1);
        auto ntfItems = spy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(ntfItems.count(), 1);
        QCOMPARE(ntfItems.at(0).id(), i.id());
        QVERIFY(ntfItems.at(0).parentCollection().isValid());

        // Try to fetch the item again, there should be none
        fjob = new ItemFetchJob(i, this);
        QVERIFY(!fjob->exec());
    }

    void testCollectionDelete()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy spy(monitor.get(), &Monitor::itemsRemoved);

        const Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        auto *fjob = new ItemFetchJob(col, this);
        AKVERIFYEXEC(fjob);
        auto items = fjob->items();
        QVERIFY(items.count() > 0);

        // delete from non-empty collection
        auto *djob = new ItemDeleteJob(col, this);
        AKVERIFYEXEC(djob);

        QTRY_COMPARE(spy.count(), 1);
        auto ntfItems = spy.at(0).at(0).value<Akonadi::Item::List>();
        QCOMPARE(ntfItems.count(), items.count());
        if (ntfItems.count() > 0) {
            QVERIFY(ntfItems.at(0).parentCollection().isValid());
        }

        fjob = new ItemFetchJob(col, this);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->items().count(), 0);

        // delete from empty collection
        djob = new ItemDeleteJob(col, this);
        QVERIFY(!djob->exec());   // error: no items found

        fjob = new ItemFetchJob(col, this);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->items().count(), 0);
    }

};

QTEST_AKONADIMAIN(ItemDeleteTest)

#include "itemdeletetest.moc"
