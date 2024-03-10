/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "server/storage/dbconfig.h"
#include "server/storage/datastore.h"
#include "shared/aktest.h"

#include "qtest_akonadi.h"

using namespace Akonadi::Server;

class TestDataStoreFactory : public DataStoreFactory
{
    class TestDataStore : public DataStore
    {
    public:
        TestDataStore()
            : DataStore(DbConfig::configuredDatabase())
        {
        }
    };

public:
    DataStore *createStore() override
    {
        return new TestDataStore();
    }
};

class DbDateTimeTest : public QObject
{
    Q_OBJECT
private:
    std::optional<qint64> storeDateTime(const QDateTime &dt)
    {
        // Get a collection
        Collection collection = Collection::retrieveByName(QStringLiteral("foo"));
        if (!collection.isValid()) {
            return {};
        }

        // Get a mimetype
        MimeType mimeType = MimeType::retrieveByNameOrCreate(QStringLiteral("application/octet-stream"));
        if (!mimeType.isValid()) {
            return {};
        }

        // Create a fake Item, abuse the "atime" column to test our datetime handling
        PimItem item;
        item.setRev(0);
        item.setAtime(dt);
        item.setCollection(collection);
        item.setMimeType(mimeType);
        item.setSize(0);

        PimItem::Id itemId;
        if (!item.insert(&itemId)) {
            return {};
        }
        return itemId;
    }

    std::optional<QDateTime> retrieveDateTime(PimItem::Id itemId)
    {
        PimItem item = PimItem::retrieveById(itemId);
        if (!item.isValid()) {
            return {};
        }
        return item.atime();
    }

private Q_SLOTS:
    void initTestCase()
    {
        // This test is a little bit of a hack: it runs as an isolated test, so there's the full testenv with an
        // actual Akonadi Server running, and this test is treated as an Akonadi client. But we are instantiating
        // a DataStore in this test to test database interaction, so we become a little bit of an Akonadi Server.

        AkonadiTest::checkTestIsIsolated();

        DataStore::setFactory(std::make_unique<TestDataStoreFactory>());

        auto config = DbConfig::configuredDatabase();
        QVERIFY(config != nullptr);

        auto store = DataStore::self();
        QVERIFY(store != nullptr);
        QVERIFY(store->database().isValid());
        QVERIFY(store->database().isOpen());
    }

    void cleanupTestCase()
    {
        DataStore::self()->close();
    }

    void testDateTime_data()
    {
        QTest::addColumn<QDateTime>("dateTime");

        QTest::newRow("UTC") << QDateTime({2024, 3, 10}, {10, 17, 24}, QTimeZone::UTC);
        QTest::newRow("Local") << QDateTime({2024, 3, 10}, {11, 17, 24}, QTimeZone::LocalTime);
        // Note: this test is not different from "Local" if you are in America/Anchorange timezone.
        // Also the tested date is very close to a DST change in Anchorage, but it's not important for the test (just so you know).
        QTest::newRow("Non-local") << QDateTime({2024, 3, 10}, {1, 8, 10}, QTimeZone("America/Anchorage"));
        // Note: this test only works when you are in a timezone that observes/observed DST change on 2024-03-31 at 2AM (like CET/CEST timezones).
        // This test is mostly for MySQL, which interprets datetime without a timezone (which is what the QMYSQL driver sends)
        // as local time, and 2024-03-31 02:15:00 is not a valid CET/CEST time. The purpose here is to tests that we correctly
        // convert from UTC to local time before submitting to the database.
        QTest::newRow("Local DST") << QDateTime({2024, 3, 31}, {2, 15, 0}, QTimeZone::UTC);
    }

    void testDateTime()
    {
        QFETCH(QDateTime, dateTime);
        QVERIFY(dateTime.isValid());

        const auto itemId = storeDateTime(dateTime);
        QVERIFY(itemId.has_value());

        const auto storedDt = retrieveDateTime(itemId.value());
        QVERIFY(storedDt.has_value());
        QVERIFY(storedDt->isValid());

        const auto dbTime = storedDt->toMSecsSinceEpoch();
        const auto expected = dateTime.toMSecsSinceEpoch();
        qDebug() << dateTime << *storedDt;
        QCOMPARE(dbTime, expected);
    }
};

QTEST_AKONADIMAIN(DbDateTimeTest)

#include "dbdatetimetest.moc"