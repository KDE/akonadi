/*
 * Copyright (C) 2016  Daniel Vrátil <dvratil@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QObject>

#include "storage/collectionstatistics.h"
#include "fakeakonadiserver.h"
#include "dbinitializer.h"
#include "aktest.h"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::Collection)

class IntrospectableCollectionStatistics : public CollectionStatistics
{
public:
    IntrospectableCollectionStatistics(bool prefetch)
        : CollectionStatistics(prefetch)
        , mCalculationsCount(0)
    {}
    ~IntrospectableCollectionStatistics()
    {}

    int calculationsCount() const
    {
        return mCalculationsCount;
    }

protected:
    Statistics calculateCollectionStatistics(const Collection &col) override
    {
        ++mCalculationsCount;
        return CollectionStatistics::calculateCollectionStatistics(col);
    }

private:
    int mCalculationsCount;
};


class CollectionStatisticsTest : public QObject
{
    Q_OBJECT

    std::unique_ptr<DbInitializer> dbInitializer;
    FakeAkonadiServer mAkonadi;
public:
    CollectionStatisticsTest()
    {
        qRegisterMetaType<Collection>();

        mAkonadi.setPopulateDb(false);
        mAkonadi.init();

        dbInitializer =  std::make_unique<DbInitializer>();
    }

private Q_SLOTS:
    void testPrefetch_data()
    {
        dbInitializer->createResource("testresource");
        auto col1 = dbInitializer->createCollection("col1");
        dbInitializer->createItem("item1", col1);
        dbInitializer->createItem("item2", col1);
        auto col2 = dbInitializer->createCollection("col2");
        // empty
        auto col3 = dbInitializer->createCollection("col3");
        dbInitializer->createItem("item3", col3);

        QTest::addColumn<Collection>("collection");
        QTest::addColumn<int>("calculationsCount");
        QTest::addColumn<qint64>("count");
        QTest::addColumn<qint64>("read");
        QTest::addColumn<qint64>("size");

        QTest::newRow("col1") << col1 << 0 << 2ll << 0ll << 0ll;
        QTest::newRow("col2") << col2 << 0 << 0ll << 0ll << 0ll;
        QTest::newRow("col3") << col3 << 0 << 1ll << 0ll << 0ll;
    }

    void testPrefetch()
    {
        QFETCH(Collection, collection);
        QFETCH(int, calculationsCount);
        QFETCH(qint64, count);
        QFETCH(qint64, read);
        QFETCH(qint64, size);

        IntrospectableCollectionStatistics cs(true);
        auto stats = cs.statistics(collection);
        QCOMPARE(cs.calculationsCount(), calculationsCount);
        QCOMPARE(stats.count, count);
        QCOMPARE(stats.read, read);
        QCOMPARE(stats.size, size);
    }

    void testCalculateStats()
    {
        dbInitializer->cleanup();
        dbInitializer->createResource("testresource");
        auto col = dbInitializer->createCollection("col1");
        dbInitializer->createItem("item1", col);
        dbInitializer->createItem("item2", col);
        dbInitializer->createItem("item3", col);

        IntrospectableCollectionStatistics cs(false);
        auto stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 3);
        QCOMPARE(stats.read, 0);
        QCOMPARE(stats.size, 0);
    }

    void testSeenChanged()
    {
        dbInitializer->cleanup();
        dbInitializer->createResource("testresource");
        auto col = dbInitializer->createCollection("col1");
        dbInitializer->createItem("item1", col);
        dbInitializer->createItem("item2", col);
        dbInitializer->createItem("item3", col);

        IntrospectableCollectionStatistics cs(false);
        auto stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 3);
        QCOMPARE(stats.read, 0);
        QCOMPARE(stats.size, 0);

        cs.itemsSeenChanged(col, 2);
        stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 3);
        QCOMPARE(stats.read, 2);
        QCOMPARE(stats.size, 0);

        cs.itemsSeenChanged(col, -1);
        stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 3);
        QCOMPARE(stats.read, 1);
        QCOMPARE(stats.size, 0);
    }

    void testItemAdded()
    {
        dbInitializer->cleanup();
        dbInitializer->createResource("testresource");
        auto col = dbInitializer->createCollection("col1");
        dbInitializer->createItem("item1", col);

        IntrospectableCollectionStatistics cs(false);
        auto stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 1);
        QCOMPARE(stats.read, 0);
        QCOMPARE(stats.size, 0);

        cs.itemAdded(col, 5, true);
        stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 2);
        QCOMPARE(stats.read, 1);
        QCOMPARE(stats.size, 5);

        cs.itemAdded(col, 3, false);
        stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 3);
        QCOMPARE(stats.read, 1);
        QCOMPARE(stats.size, 8);
    }
};

AKTEST_MAIN(CollectionStatisticsTest)

#include "collectionstatisticstest.moc"
