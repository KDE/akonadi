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


class IntrospectableCollectionStatistics : public CollectionStatistics
{
public:
    IntrospectableCollectionStatistics()
        : CollectionStatistics()
        , mCalculationsCount(0)
    {}
    ~IntrospectableCollectionStatistics()
    {}

    int calculationsCount() const
    {
        return mCalculationsCount;
    }

protected:
    Statistics calculateCollectionStatistics(const Collection &col) Q_DECL_OVERRIDE
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

public:
    CollectionStatisticsTest()
    {
        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }
    }

    ~CollectionStatisticsTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testCalculateStats()
    {
        DbInitializer initializer;
        initializer.createResource("testresource");
        auto col = initializer.createCollection("col1");
        initializer.createItem("item1", col);
        initializer.createItem("item2", col);
        initializer.createItem("item3", col);

        IntrospectableCollectionStatistics cs;
        auto stats = cs.statistics(col);
        QCOMPARE(cs.calculationsCount(), 1);
        QCOMPARE(stats.count, 3);
        QCOMPARE(stats.read, 0);
        QCOMPARE(stats.size, 0);
    }

    void testSeenChanged()
    {
        DbInitializer initializer;
        initializer.createResource("testresource");
        auto col = initializer.createCollection("col1");
        initializer.createItem("item1", col);
        initializer.createItem("item2", col);
        initializer.createItem("item3", col);

        IntrospectableCollectionStatistics cs;
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
        DbInitializer initializer;
        initializer.createResource("testresource");
        auto col = initializer.createCollection("col1");
        initializer.createItem("item1", col);

        IntrospectableCollectionStatistics cs;
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
