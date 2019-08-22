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
#include "storage/qb/insertquery.cpp"
#include "storage/qb/updatequery.cpp"
#include "storage/qb/deletequery.cpp"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Qb::SelectQuery);
Q_DECLARE_METATYPE(Qb::InsertQuery);
Q_DECLARE_METATYPE(Qb::UpdateQuery);
Q_DECLARE_METATYPE(Qb::DeleteQuery);
Q_DECLARE_METATYPE(Qb::Query::BoundValues);

class QueryTest : public QObject
{
    Q_OBJECT

    using BoundValues = Qb::Query::BoundValues;

private Q_SLOTS:
    void testSelectQuery_data()
    {
        QTest::addColumn<Qb::SelectQuery>("query");
        QTest::addColumn<QString>("sql");
        QTest::addColumn<BoundValues>("boundValues");

        QTest::newRow("Simple select")
            << Qb::SelectQuery()
                .column(QStringLiteral("col1"))
                .column(Qb::ColumnStmt(QStringLiteral("col2")).as(QStringLiteral("alias")))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT col1, col2 AS alias FROM table")
            << BoundValues{};
        QTest::newRow("Table alias")
            << Qb::SelectQuery()
                .column(QStringLiteral("col1"))
                .from(TableStmt(QStringLiteral("table")).as(QStringLiteral("t1")))
            << QStringLiteral("SELECT col1 FROM table AS t1")
            << BoundValues{};
        QTest::newRow("Aggregate (sum)")
            << Qb::SelectQuery()
                .column(Qb::Sum(QStringLiteral("col1")).as(QStringLiteral("cnt")))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT SUM(col1) AS cnt FROM table")
            << BoundValues{};
        QTest::newRow("Aggregate (count)")
            << Qb::SelectQuery()
                .column(Qb::Count().as(QStringLiteral("cnt")))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT COUNT(*) AS cnt FROM table")
            << BoundValues{};
        QTest::newRow("Distinct")
            << Qb::SelectQuery()
                .distinct()
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
            << QStringLiteral("SELECT DISTINCT col1 FROM table")
            << BoundValues{};
        QTest::newRow("Group by")
            << Qb::SelectQuery()
                .columns(QStringLiteral("col1"), Qb::Count().as(QStringLiteral("cnt")))
                .from(QStringLiteral("table"))
                .groupBy(QStringLiteral("col1"))
            << QStringLiteral("SELECT col1, COUNT(*) AS cnt FROM table GROUP BY col1")
            << BoundValues{};
        QTest::newRow("Limit")
            << Qb::SelectQuery()
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .limit(10)
            << QStringLiteral("SELECT col1 FROM table LIMIT 10")
            << BoundValues{};
        QTest::newRow("Order by")
            << Qb::SelectQuery()
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .orderBy(QStringLiteral("col2"), Qb::SortDirection::DESC)
            << QStringLiteral("SELECT col1 FROM table ORDER BY col2 DESC")
            << BoundValues{};
        QTest::newRow("Order by multiple columns")
            << Qb::SelectQuery()
                .column(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .orderBy(QStringLiteral("col1"), Qb::SortDirection::DESC)
                .orderBy(QStringLiteral("col2"), Qb::SortDirection::ASC)
            << QStringLiteral("SELECT col1 FROM table ORDER BY col1 DESC, col2 ASC")
            << BoundValues{};
        QTest::newRow("Left join")
            << Qb::SelectQuery()
                .columns(QStringLiteral("t1.col1"), QStringLiteral("t2.col2"))
                .from(TableStmt(QStringLiteral("table1")).as(QStringLiteral("t1")))
                .leftJoin(TableStmt(QStringLiteral("table2")).as(QStringLiteral("t2")),
                          Qb::On(QStringLiteral("t1.col1"), QStringLiteral("t2.col1")))
            << QStringLiteral("SELECT t1.col1, t2.col2 FROM table1 AS t1 LEFT JOIN table2 AS t2 ON (t1.col1 = t2.col1)")
            << BoundValues{};
        QTest::newRow("Right join")
            << Qb::SelectQuery()
                .columns(QStringLiteral("t1.col1"), ColumnStmt(QStringLiteral("table2.col2")).as(QStringLiteral("name")))
                .from(TableStmt(QStringLiteral("table1")).as(QStringLiteral("t1")))
                .rightJoin(QStringLiteral("table2"), Qb::On(QStringLiteral("table2.col1"), Qb::Compare::Less, 10))
            << QStringLiteral("SELECT t1.col1, table2.col2 AS name FROM table1 AS t1 RIGHT JOIN table2 ON (table2.col1 < ?)")
            << BoundValues{10};
        QTest::newRow("Inner join")
            << Qb::SelectQuery()
                .columns(QStringLiteral("t2.col1"))
                .from(QStringLiteral("t1"))
                .innerJoin(QStringLiteral("t2"),
                           Qb::On(Qb::And({{QStringLiteral("t1.col1"), QStringLiteral("t2.col1")},
                                           {QStringLiteral("t1.col1"), Qb::Compare::Equals, QStringLiteral("bla")}})))
            << QStringLiteral("SELECT t2.col1 FROM t1 INNER JOIN t2 ON ((t1.col1 = t2.col1) AND (t1.col1 = ?))")
            << BoundValues{QStringLiteral("bla")};
        QTest::newRow("Simple where, compare columns")
            << Qb::SelectQuery()
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table1"))
                .where({QStringLiteral("col1"), QStringLiteral("col2")})
            << QStringLiteral("SELECT col1 FROM table1 WHERE (col1 = col2)")
            << BoundValues{};
        QTest::newRow("Simple where, compare value")
            << Qb::SelectQuery()
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table1"))
                .where({QStringLiteral("col1"), Qb::Compare::Equals, true})
            << QStringLiteral("SELECT col1 FROM table1 WHERE (col1 = ?)")
            << BoundValues{true};
        QTest::newRow("Where with AND")
            << Qb::SelectQuery()
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table1"))
                .where(Qb::And({{QStringLiteral("col1"), Qb::Compare::Equals, 42},
                                {QStringLiteral("col2"), Qb::Compare::Equals, QStringLiteral("someValue")}}))
            << QStringLiteral("SELECT col1 FROM table1 WHERE ((col1 = ?) AND (col2 = ?))")
            << BoundValues{42, QStringLiteral("someValue")};
        QTest::newRow("Where with OR")
            << Qb::SelectQuery()
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .where(Qb::Or({{QStringLiteral("col1"), Qb::Compare::Equals, 42},
                                {QStringLiteral("col1"), Qb::Compare::GreaterOrEqual, 100}}))
            << QStringLiteral("SELECT col1 FROM table WHERE ((col1 = ?) OR (col1 >= ?))")
            << BoundValues{42,100};
        QTest::newRow("Where with nested conditions")
            << Qb::SelectQuery()
                .columns(QStringLiteral("col1"))
                .from(QStringLiteral("table"))
                .where(Qb::And({{QStringLiteral("col1"), Qb::Compare::Equals, 42},
                                Qb::Or({{QStringLiteral("col2"), Qb::Compare::Equals, 100},
                                        {QStringLiteral("col2"), Qb::Compare::Equals, 101}})}))
            << QStringLiteral("SELECT col1 FROM table WHERE ((col1 = ?) AND ((col2 = ?) OR (col2 = ?)))")
            << BoundValues{42, 100, 101};
    }

    void testSelectQuery()
    {
        QFETCH(Qb::SelectQuery, query);
        QFETCH(QString, sql);
        QFETCH(BoundValues, boundValues);

        QString stmt;
        QTextStream stream(&stmt);
        query.serialize(stream);

        QCOMPARE(stmt, sql);
        QCOMPARE(query.bindValues(), boundValues);
    }

    void testInsertQuery_data()
    {
        QTest::addColumn<Qb::InsertQuery>("query");
        QTest::addColumn<QString>("sql");
        QTest::addColumn<BoundValues>("boundValues");

        QTest::newRow("Simple insert")
            << Qb::InsertQuery()
                .into(QStringLiteral("table"))
                .value(QStringLiteral("col1"), 42)
                .value(QStringLiteral("col2"), QStringLiteral("boohoo"))
                .value(QStringLiteral("col3"), true)
            << QStringLiteral("INSERT INTO table (col1, col2, col3) VALUES (?, ?, ?)")
            << BoundValues{42, QStringLiteral("boohoo"), true};
        auto query = Qb::InsertQuery()
            .into(QStringLiteral("table"))
            .value(QStringLiteral("col2"), 42)
            .returning(QStringLiteral("col1"));
        query.setDatabaseType(DbType::PostgreSQL);
        QTest::newRow("Returning")
            << query
            << QStringLiteral("INSERT INTO table (col2) VALUES (?) RETURNING col1")
            << BoundValues{42};
    }

    void testInsertQuery()
    {
        QFETCH(Qb::InsertQuery, query);
        QFETCH(QString, sql);
        QFETCH(BoundValues, boundValues);

        QString stmt;
        QTextStream stream(&stmt);
        query.serialize(stream);

        QCOMPARE(stmt, sql);
        QCOMPARE(query.bindValues(), boundValues);
    }

    void testUpdateQuery_data()
    {
        QTest::addColumn<Qb::UpdateQuery>("query");
        QTest::addColumn<QString>("sql");
        QTest::addColumn<BoundValues>("boundValues");

        QTest::newRow("Simple update")
            << Qb::UpdateQuery()
                .table(QStringLiteral("table"))
                .value(QStringLiteral("col1"), 42)
                .value(QStringLiteral("col2"), QStringLiteral("boohoo"))
                .value(QStringLiteral("col3"), true)
            << QStringLiteral("UPDATE table SET col1 = ?, col2 = ?, col3 = ?")
            << BoundValues{42, QStringLiteral("boohoo"), true};
        QTest::newRow("Update with WHERE")
            << Qb::UpdateQuery()
                .table(QStringLiteral("table"))
                .value(QStringLiteral("col1"), 42)
                .where({QStringLiteral("col2"), Qb::Compare::Equals, 1})
            << QStringLiteral("UPDATE table SET col1 = ? WHERE (col2 = ?)")
            << BoundValues{42, 1};
        QTest::newRow("Update with complex WHERE")
            << Qb::UpdateQuery()
                .table(QStringLiteral("table"))
                .value(QStringLiteral("col1"), 42)
                .where(Qb::And({{QStringLiteral("col2"), Qb::Compare::Equals, 1},
                                Qb::Or({{QStringLiteral("col2"), QStringLiteral("col3")},
                                        {QStringLiteral("col3"), Qb::Compare::Equals, 2}})}))
            << QStringLiteral("UPDATE table SET col1 = ? WHERE ((col2 = ?) AND ((col2 = col3) OR (col3 = ?)))")
            << BoundValues{42, 1, 2};
    }

    void testUpdateQuery()
    {
        QFETCH(Qb::UpdateQuery, query);
        QFETCH(QString, sql);
        QFETCH(BoundValues, boundValues);

        QString stmt;
        QTextStream stream(&stmt);
        query.serialize(stream);

        QCOMPARE(stmt, sql);
        QCOMPARE(query.bindValues(), boundValues);
    }

    void testDeleteQuery_data()
    {
        QTest::addColumn<Qb::DeleteQuery>("query");
        QTest::addColumn<QString>("sql");
        QTest::addColumn<BoundValues>("boundValues");

        QTest::newRow("Simple delete")
            << Qb::DeleteQuery()
                .table(QStringLiteral("table"))
            << QStringLiteral("DELETE FROM table")
            << BoundValues{};
        QTest::newRow("Delete with WHERE")
            << Qb::DeleteQuery()
                .table(QStringLiteral("table"))
                .where({QStringLiteral("col1"), Qb::Compare::Equals, 42})
            << QStringLiteral("DELETE FROM table WHERE (col1 = ?)")
            << BoundValues{42};
    }

    void testDeleteQuery()
    {
        QFETCH(Qb::DeleteQuery, query);
        QFETCH(QString, sql);
        QFETCH(BoundValues, boundValues);

        QString stmt;
        QTextStream stream(&stmt);
        query.serialize(stream);

        QCOMPARE(stmt, sql);
        QCOMPARE(query.bindValues(), boundValues);
    }

};

QTEST_GUILESS_MAIN(QueryTest)

#include "querytest.moc"
