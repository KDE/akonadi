/*
 * SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
    explicit IntrospectableCollectionStatistics(bool prefetch)
        : CollectionStatistics(prefetch)
        , mCalculationsCount(0)
    {}
    ~IntrospectableCollectionStatistics() override
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

    FakeAkonadiServer mAkonadi;
    std::unique_ptr<DbInitializer> dbInitializer;
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

        QTest::newRow("col1") << col1 << 0 << 2LL << 0LL << 0LL;
        QTest::newRow("col2") << col2 << 0 << 0LL << 0LL << 0LL;
        QTest::newRow("col3") << col3 << 0 << 1LL << 0LL << 0LL;
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
