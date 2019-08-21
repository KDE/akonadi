/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#include <QObject>
#include <QTest>
#include <QMap>

#define QUERYBUILDER_UNITTEST

#include "storage/qb/query.cpp"
#include "storage/qb/selectquery.cpp"
#include "fakedatastore.h"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Qb::SelectQuery);
Q_DECLARE_METATYPE(Qb::Query::BoundValues);

class SelectQueryTest : public QObject
{
    Q_OBJECT

    using BoundValues = QMap<QString, QVariant>;
private Q_SLOTS:
    void testQuery_data()
    {
        using BoundValues = Qb::Query::BoundValues;

        QTest::addColumn<Qb::SelectQuery>("query");
        QTest::addColumn<QString>("sql");
        QTest::addColumn<BoundValues>("boundValues");

        FakeDataStore::registerFactory();
        auto &db = *DataStore::self();

        QTest::newRow("Simple select")
            << Qb::Select(db)
                .column(QStringLiteral("col1"))
                .column(Qb::ColumnStmt(QStringLiteral("col2")).as(QStringLiteral("alias")))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT col1, col2 AS alias FROM table")
            << BoundValues{};
        QTest::newRow("Table alias")
            << Qb::Select(db)
                .column(QStringLiteral("col1"))
                .from(TableStmt(QStringLiteral("table")).as(QStringLiteral("t1")))
            << QStringLiteral("SELECT col1 FROM table AS t1")
            << BoundValues{};
        QTest::newRow("Aggregate (sum)")
            << Qb::Select(db)
                .column(Qb::Sum(QStringLiteral("col1")).as(QStringLiteral("cnt")))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT SUM(col1) AS cnt FROM table")
            << BoundValues{};
        QTest::newRow("Aggregate (count)")
            << Qb::Select(db)
                .column(Qb::Count().as(QStringLiteral("cnt")))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT COUNT(*) AS cnt FROM table")
            << BoundValues{};
        QTest::newRow("Distinct")
            << Qb::Select(db)
                .distinct()
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT DISTINCT col1 FROM table")
            << BoundValues{};
        QTest::newRow("Group by")
            << Qb::Select(db)
                .columns(QStringLiteral("col1"), Qb::Count().as(QStringLiteral("cnt")))
                .from(QStringLiteral("table"))
                .groupBy(QStringLiteral("col1"))
            << QStringLiteral("SELECT col1, COUNT(*) AS cnt FROM table GROUP BY col1")
            << BoundValues{};
        QTest::newRow("Limit")
            << Qb::Select(db)
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .limit(10)
            << QStringLiteral("SELECT col1 FROM table LIMIT 10")
            << BoundValues{};
        QTest::newRow("Order by")
            << Qb::Select(db)
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .orderBy(QStringLiteral("col2"), Qb::SortDirection::DESC)
            << QStringLiteral("SELECT col1 FROM table ORDER BY col2 DESC")
            << BoundValues{};
        QTest::newRow("Order by multiple columns")
            << Qb::Select(db)
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .orderBy(QStringLiteral("col1"), Qb::SortDirection::DESC)
                .orderBy(QStringLiteral("col2"), Qb::SortDirection::ASC)
            << QStringLiteral("SELECT col1 FROM table ORDER BY col1 DESC, col2 ASC")
            << BoundValues{};
        QTest::newRow("Left join")
            << Qb::Select(db)
                .columns(QStringLiteral("t1.col1"), QStringLiteral("t2.col2"))
                .from(TableStmt(QStringLiteral("table1")).as(QStringLiteral("t1")))
                .leftJoin(TableStmt(QStringLiteral("table2")).as(QStringLiteral("t2")),
                          Qb::On(QStringLiteral("t1.col1"), QStringLiteral("t2.col1")))
            << QStringLiteral("SELECT t1.col1, t2.col2 FROM table1 AS t1 LEFT JOIN table2 AS t2 ON (t1.col1 = t2.col1)")
            << BoundValues{};
        QTest::newRow("Right join")
            << Qb::Select(db)
                .columns(QStringLiteral("t1.col1"), ColumnStmt(QStringLiteral("table2.col2")).as(QStringLiteral("name")))
                .from(TableStmt(QStringLiteral("table1")).as(QStringLiteral("t1")))
                .rightJoin(QStringLiteral("table2"), Qb::On(QStringLiteral("table2.col1"), Qb::Compare::Less, 10))
            << QStringLiteral("SELECT t1.col1, table2.col2 AS name FROM table1 AS t1 RIGHT JOIN table2 ON (table2.col1 < ?)")
            << BoundValues{10};
        QTest::newRow("Inner join")
            << Qb::Select(db)
                .columns(QStringLiteral("t2.col1"))
                .from(QStringLiteral("t1"))
                .innerJoin(QStringLiteral("t2"),
                           Qb::On(Qb::And({{QStringLiteral("t1.col1"), QStringLiteral("t2.col1")},
                                           {QStringLiteral("t1.col1"), Qb::Compare::Equals, QStringLiteral("bla")}})))
            << QStringLiteral("SELECT t2.col1 FROM t1 INNER JOIN t2 ON ((t1.col1 = t2.col1) AND (t1.col1 = ?))")
            << BoundValues{QStringLiteral("bla")};
        QTest::newRow("Simple where, compare columns")
            << Qb::Select(db)
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table1"))
                .where({QStringLiteral("col1"), QStringLiteral("col2")})
            << QStringLiteral("SELECT col1 FROM table1 WHERE (col1 = col2)")
            << BoundValues{};
        QTest::newRow("Simple where, compare value")
            << Qb::Select(db)
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table1"))
                .where({QStringLiteral("col1"), Qb::Compare::Equals, true})
            << QStringLiteral("SELECT col1 FROM table1 WHERE (col1 = ?)")
            << BoundValues{true};
        QTest::newRow("Where with AND")
            << Qb::Select(db)
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table1"))
                .where(Qb::And({{QStringLiteral("col1"), Qb::Compare::Equals, 42},
                                {QStringLiteral("col2"), Qb::Compare::Equals, QStringLiteral("someValue")}}))
            << QStringLiteral("SELECT col1 FROM table1 WHERE ((col1 = ?) AND (col2 = ?))")
            << BoundValues{42, QStringLiteral("someValue")};
        QTest::newRow("Where with OR")
            << Qb::Select(db)
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .where(Qb::Or({{QStringLiteral("col1"), Qb::Compare::Equals, 42},
                                {QStringLiteral("col1"), Qb::Compare::GreaterOrEqual, 100}}))
            << QStringLiteral("SELECT col1 FROM table WHERE ((col1 = ?) OR (col1 >= ?))")
            << BoundValues{42,100};
        QTest::newRow("Where with nested conditions")
            << Qb::Select(db)
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .where(Qb::And({{QStringLiteral("col1"), Qb::Compare::Equals, 42},
                                Qb::Or({{QStringLiteral("col2"), Qb::Compare::Equals, 100},
                                        {QStringLiteral("col2"), Qb::Compare::Equals, 101}})}))
            << QStringLiteral("SELECT col1 FROM table WHERE ((col1 = ?) AND ((col2 = ?) OR (col2 = ?)))")
            << BoundValues{42, 100, 101};
    }

    void testQuery()
    {
        QFETCH(Qb::SelectQuery, query);
        QFETCH(QString, sql);
        QFETCH(Query::BoundValues, boundValues);

        QString buffer;
        QTextStream str(&buffer);
        str << query;

        QCOMPARE(*str.string(), sql);
        QCOMPARE(query.boundValues(), boundValues);
    }
};

QTEST_GUILESS_MAIN(SelectQueryTest)

#include "selectquerytest.moc"
