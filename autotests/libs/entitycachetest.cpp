/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitycache_p.h"

#include <QSignalSpy>
#include <qtest_akonadi.h>

using namespace Akonadi;

class EntityCacheTest : public QObject
{
    Q_OBJECT
private:
    template<typename T, typename FetchJob, typename FetchScope> void testCache()
    {
        EntityCache<T, FetchJob, FetchScope> cache(2);
        QSignalSpy spy(&cache, SIGNAL(dataAvailable()));
        QVERIFY(spy.isValid());

        QVERIFY(!cache.isCached(1));
        QVERIFY(!cache.isRequested(1));
        QVERIFY(!cache.retrieve(1).isValid());

        FetchScope scope;
        scope.setAncestorRetrieval(FetchScope::All);

        cache.request(1, scope);
        QVERIFY(!cache.isCached(1));
        QVERIFY(cache.isRequested(1));
        QVERIFY(!cache.retrieve(1).isValid());

        QTRY_COMPARE(spy.count(), 1);
        QVERIFY(cache.isCached(1));
        QVERIFY(cache.isRequested(1));
        const T e1 = cache.retrieve(1);
        QCOMPARE(e1.id(), 1LL);
        QVERIFY(e1.parentCollection().isValid());
        QVERIFY(!e1.parentCollection().remoteId().isEmpty() || e1.parentCollection() == Collection::root());

        spy.clear();
        cache.request(2, FetchScope());
        cache.request(3, FetchScope());

        QVERIFY(!cache.isCached(1));
        QVERIFY(!cache.isRequested(1));
        QVERIFY(cache.isRequested(2));
        QVERIFY(cache.isRequested(3));

        cache.invalidate(2);

        QTRY_COMPARE(spy.count(), 2);
        QVERIFY(cache.isCached(2));
        QVERIFY(cache.isCached(3));

        const T e2 = cache.retrieve(2);
        const T e3a = cache.retrieve(3);
        QCOMPARE(e3a.id(), 3LL);
        QVERIFY(!e2.isValid());

        cache.invalidate(3);
        const T e3b = cache.retrieve(3);
        QVERIFY(!e3b.isValid());

        spy.clear();
        // updating a cached entry removes it
        cache.update(3, FetchScope());
        cache.update(3, FetchScope());
        QVERIFY(!cache.isCached(3));
        QVERIFY(!cache.isRequested(3));
        QVERIFY(!cache.retrieve(3).isValid());

        // updating a pending entry re-fetches
        cache.request(3, FetchScope());
        cache.update(3, FetchScope());
        QVERIFY(!cache.isCached(3));
        QVERIFY(cache.isRequested(3));
        cache.update(3, FetchScope());
        QVERIFY(!cache.isCached(3));
        QVERIFY(cache.isRequested(3));

        QTRY_COMPARE(spy.count(), 3);
        QVERIFY(cache.isCached(3));
        QVERIFY(cache.retrieve(3).isValid());
    }

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }
    void testCacheGeneric_data()
    {
        QTest::addColumn<bool>("collection");
        QTest::newRow("collection") << true;
        QTest::newRow("item") << false;
    }

    void testCacheGeneric()
    {
        QFETCH(bool, collection);
        if (collection) {
            testCache<Collection, CollectionFetchJob, CollectionFetchScope>();
        } else {
            testCache<Item, ItemFetchJob, ItemFetchScope>();
        }
    }

    void testListCacheGeneric_data()
    {
        QTest::addColumn<bool>("collection");
        QTest::newRow("collection") << true;
        QTest::newRow("item") << false;
    }

    void testItemCache()
    {
        ItemCache cache(1);
        QSignalSpy spy(&cache, &EntityCacheBase::dataAvailable);
        QVERIFY(spy.isValid());

        ItemFetchScope scope;
        scope.fetchFullPayload(true);
        cache.request(1, scope);

        QTRY_COMPARE(spy.count(), 1);
        QVERIFY(cache.isCached(1));
        QVERIFY(cache.isRequested(1));
        const Item item = cache.retrieve(1);
        QCOMPARE(item.id(), 1LL);
        QVERIFY(item.hasPayload<QByteArray>());
    }

    void testListCache_ensureCached()
    {
        ItemFetchScope scope;

        EntityListCache<Item, ItemFetchJob, ItemFetchScope> cache(3);
        QSignalSpy spy(&cache, &EntityCacheBase::dataAvailable);
        QVERIFY(spy.isValid());

        cache.request(QList<Item::Id>() << 1 << 2 << 3, scope);
        QTRY_COMPARE(spy.count(), 1);
        QVERIFY(cache.isCached(QList<Item::Id>() << 1 << 2 << 3));

        cache.ensureCached(QList<Item::Id>() << 1 << 2 << 3 << 4, scope);
        QTRY_COMPARE(spy.count(), 2);
        QVERIFY(cache.isCached(QList<Item::Id>() << 1 << 2 << 3 << 4));
    }
};

QTEST_AKONADIMAIN(EntityCacheTest)

#include "entitycachetest.moc"
