/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "storage/querycache.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

using namespace Akonadi::Server;

class QueryCacheTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init()
    {
        QueryCache::clear();
    }

    void testQueryCache()
    {
        const QString queryStatement = QStringLiteral("SELECT * FROM table");

        QVERIFY(!QueryCache::query(queryStatement).has_value());
        QueryCache::insert(QSqlDatabase(), queryStatement, QSqlQuery());
        QVERIFY(QueryCache::query(queryStatement).has_value());
    }

    void testThreadIsolation()
    {
        const QString queryStatement = QStringLiteral("SELECT * FROM table1");
        const QString queryStatement2 = QStringLiteral("SELECT * FROM table2");

        QVERIFY(!QueryCache::query(queryStatement).has_value());
        QueryCache::insert(QSqlDatabase(), queryStatement, QSqlQuery());
        QVERIFY(QueryCache::query(queryStatement).has_value());

        auto thread = std::unique_ptr<QThread>(QThread::create([&]() {
            QVERIFY(!QueryCache::query(queryStatement).has_value());

            QVERIFY(!QueryCache::query(queryStatement2).has_value());
            QueryCache::insert(QSqlDatabase(), queryStatement2, QSqlQuery());
            QVERIFY(QueryCache::query(queryStatement2).has_value());
        }));
        thread->start();
        thread->wait();

        QVERIFY(!QueryCache::query(queryStatement2).has_value());
    }

    void testLRU()
    {
        // Fill the cache
        for (size_t i = 0; i < QueryCache::capacity(); ++i) {
            QueryCache::insert(QSqlDatabase(), QStringLiteral("SELECT * FROM table%1").arg(i), QSqlQuery());
        }

        // Add one more query, triggering eviction of the oldest query from the cache
        const auto queryStatement = QStringLiteral("SELECT * FROM table50");
        QueryCache::insert(QSqlDatabase(), queryStatement, QSqlQuery());

        // The new query is inserted into the cache
        QVERIFY(QueryCache::query(queryStatement).has_value());
        // The oldest query should have been evicted
        QVERIFY(!QueryCache::query(QStringLiteral("SELECT * FROM table0")).has_value());
    }
};

QTEST_GUILESS_MAIN(QueryCacheTest)

#include "querycachetest.moc"
