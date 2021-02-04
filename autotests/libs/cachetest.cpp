/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "agentmanager.h"
#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "item.h"
#include "itemcopyjob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "qtest_akonadi.h"

#include <QHash>

using namespace Akonadi;

class CacheTest : public QObject
{
    Q_OBJECT
private:
    void enableAgent(const QString &id, bool enable)
    {
        auto instance = AgentManager::self()->instance(id);
        QVERIFY(instance.isValid());

        instance.setIsOnline(enable);
        QTRY_COMPARE(Akonadi::AgentManager::self()->instance(id).isOnline(), enable);
    }

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }
    void testRetrievalErrorBurst() // caused rare server crashs with old item retrieval code
    {
        Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        QVERIFY(col.isValid());

        enableAgent(QStringLiteral("akonadi_knut_resource_0"), false);

        auto fetch = new ItemFetchJob(col, this);
        fetch->fetchScope().fetchFullPayload(true);
        QVERIFY(!fetch->exec());
    }

    void testResourceRetrievalOnFetch_data()
    {
        QTest::addColumn<Item>("item");
        QTest::addColumn<bool>("resourceEnabled");

        QTest::newRow("resource online") << Item(1) << true;
        QTest::newRow("resource offline") << Item(2) << false;
    }

    void testResourceRetrievalOnFetch()
    {
        QFETCH(Item, item);
        QFETCH(bool, resourceEnabled);

        auto fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QVERIFY(!item.hasPayload());

        enableAgent(QStringLiteral("akonadi_knut_resource_0"), resourceEnabled);

        fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        QCOMPARE(fetch->exec(), resourceEnabled);
        if (resourceEnabled) {
            QCOMPARE(fetch->items().count(), 1);
            item = fetch->items().first();
            QVERIFY(item.isValid());
            QVERIFY(item.hasPayload());
            QVERIFY(item.revision() > 0); // was changed by the resource delivering the payload
        }

        fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QCOMPARE(item.hasPayload(), resourceEnabled);
    }

    void testResourceRetrievalOnCopy_data()
    {
        QTest::addColumn<Item>("item");
        QTest::addColumn<bool>("resourceEnabled");

        QTest::newRow("online") << Item(3) << true;
        QTest::newRow("offline") << Item(4) << false;
    }

    void testResourceRetrievalOnCopy()
    {
        QFETCH(Item, item);
        QFETCH(bool, resourceEnabled);

        auto fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QVERIFY(!item.hasPayload());

        enableAgent(QStringLiteral("akonadi_knut_resource_0"), resourceEnabled);

        Collection dest(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        QVERIFY(dest.isValid());

        auto copy = new ItemCopyJob(item, dest, this);
        QCOMPARE(copy->exec(), resourceEnabled);

        fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QCOMPARE(item.hasPayload(), resourceEnabled);
    }
};

QTEST_AKONADIMAIN(CacheTest)

#include "cachetest.moc"
