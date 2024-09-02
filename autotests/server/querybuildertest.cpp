/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "querybuildertest.h"
#include "moc_querybuildertest.cpp"

#define QUERYBUILDER_UNITTEST

#include "storage/query.cpp"
#include "storage/querybuilder.cpp"

#include <QTest>

QTEST_MAIN(QueryBuilderTest)

Q_DECLARE_METATYPE(QList<QVariant>)

using namespace Akonadi::Server;

void QueryBuilderTest::testQueryBuilder_data()
{
    qRegisterMetaType<QList<QVariant>>();
    mBuilders.clear();
    QTest::addColumn<size_t>("qbId");
    QTest::addColumn<QString>("sql");
    QTest::addColumn<QList<QVariant>>("bindValues");

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col1"));
        mBuilders.emplace_back(std::move(qb));
        QTest::newRow("simple select") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table") << QList<QVariant>();
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col1"));
        qb.addColumn(QStringLiteral("col2"));
        mBuilders.emplace_back(std::move(qb));
        QTest::newRow("simple select 2") << mBuilders.size() << QStringLiteral("SELECT col1, col2 FROM table") << QList<QVariant>();
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col1"));
        qb.addValueCondition(QStringLiteral("col1"), Query::Equals, QVariant(5));
        mBuilders.emplace_back(std::move(qb));
        QTest::newRow("single where") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 = :0 )") << QList<QVariant>{5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col1"));
        qb.addValueCondition(QStringLiteral("col1"), Query::Equals, QVariant(5));
        qb.addColumnCondition(QStringLiteral("col1"), Query::LessOrEqual, QStringLiteral("col2"));
        mBuilders.emplace_back(std::move(qb));
        QTest::newRow("flat where") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 = :0 AND col1 <= col2 )") << QList<QVariant>{5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col1"));
        qb.setSubQueryMode(Query::Or);
        qb.addValueCondition(QStringLiteral("col1"), Query::Equals, QVariant(5));
        qb.addColumnCondition(QStringLiteral("col1"), Query::LessOrEqual, QStringLiteral("col2"));
        mBuilders.emplace_back(std::move(qb));
        QTest::newRow("flat where 2") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 = :0 OR col1 <= col2 )") << QList<QVariant>{5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col1"));
        qb.setSubQueryMode(Query::Or);
        qb.addValueCondition(QStringLiteral("col1"), Query::Equals, QVariant(5));
        Condition subCon;
        subCon.addColumnCondition(QStringLiteral("col1"), Query::Greater, QStringLiteral("col2"));
        subCon.addValueCondition(QStringLiteral("col1"), Query::NotEquals, QVariant());
        qb.addCondition(subCon);
        mBuilders.emplace_back(std::move(qb));
        QTest::newRow("hierarchical where") << mBuilders.size()
                                            << QStringLiteral("SELECT col1 FROM table WHERE ( col1 = :0 OR ( col1 > col2 AND col1 <> NULL ) )")
                                            << QList<QVariant>{5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"));
        qb.addAggregation(QStringLiteral("col1"), QStringLiteral("count"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("single aggregation") << mBuilders.size() << QStringLiteral("SELECT count(col1) FROM table") << QList<QVariant>();
    }

    {
        QueryBuilder qb(QStringLiteral("table"));
        qb.addColumn(QStringLiteral("col1"));
        qb.addSortColumn(QStringLiteral("col1"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("single order by") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table ORDER BY col1 ASC") << QList<QVariant>();
    }

    {
        QueryBuilder qb(QStringLiteral("table"));
        qb.addColumn(QStringLiteral("col1"));
        qb.addSortColumn(QStringLiteral("col1"));
        qb.addSortColumn(QStringLiteral("col2"), Query::Descending);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("multiple order by") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table ORDER BY col1 ASC, col2 DESC") << QList<QVariant>();
    }

    {
        QueryBuilder qb(QStringLiteral("table"));
        qb.addColumn(QStringLiteral("col1"));
        qb.addValueCondition(QStringLiteral("col1"), Query::In, QList<QVariant>{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
        mBuilders.push_back(std::move(qb));
        QTest::newRow("where in") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 IN ( :0, :1, :2 ) )")
                                  << QList<QVariant>{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
    }

    {
        QueryBuilder qb(QStringLiteral("table"));
        qb.addColumn(QStringLiteral("col1"));
        qb.addValueCondition(QStringLiteral("col1"), Query::In, QList<qint64>{1, 2, 3, 4});
        mBuilders.push_back(std::move(qb));
        QTest::newRow("where in QList") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 IN ( :0, :1, :2, :3 ) )")
                                        << QList<QVariant>{1, 2, 3, 4};
    }

    {
        QSet<qint64> values{1, 2, 3, 4};
        QueryBuilder qb(QStringLiteral("table"));
        qb.addColumn(QStringLiteral("col1"));
        qb.addValueCondition(QStringLiteral("col1"), Query::In, values);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("where in QSet") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 IN ( :0, :1, :2, :3 ) )")
                                       << QList<QVariant>{values.cbegin(), values.cend()};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.setDatabaseType(DbType::MySQL);
        qb.addColumn(QStringLiteral("col1"));
        qb.setLimit(1);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("SELECT with LIMIT") << mBuilders.size() << QStringLiteral("SELECT col1 FROM table LIMIT 1") << QList<QVariant>();
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Update);
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update") << mBuilders.size() << QStringLiteral("UPDATE table SET col1 = :0") << QList<QVariant>{QStringLiteral("bla")};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Update);
        qb.setDatabaseType(DbType::MySQL);
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table1.id"), QStringLiteral("table2.id"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table3"), QStringLiteral("table1.id"), QStringLiteral("table3.id"));
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update multi table MYSQL")
            << mBuilders.size()
            << QStringLiteral("UPDATE table1, table2, table3 SET col1 = :0 WHERE ( ( table1.id = table2.id ) AND ( table1.id = table3.id ) )")
            << QList<QVariant>{QStringLiteral("bla")};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Update);
        qb.setDatabaseType(DbType::PostgreSQL);
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table1.id"), QStringLiteral("table2.id"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table3"), QStringLiteral("table1.id"), QStringLiteral("table3.id"));
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update multi table PSQL")
            << mBuilders.size()
            << QStringLiteral("UPDATE table1 SET col1 = :0 FROM table2 JOIN table3 WHERE ( ( table1.id = table2.id ) AND ( table1.id = table3.id ) )")
            << QList<QVariant>{QStringLiteral("bla")};
    }
    /// TODO: test for subquery in SQLite case

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Insert);
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("insert single column") << mBuilders.size() << QStringLiteral("INSERT INTO table (col1) VALUES (:0)")
                                              << QList<QVariant>{QStringLiteral("bla")};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Insert);
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        qb.setColumnValue(QStringLiteral("col2"), 5);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("insert multi column") << mBuilders.size() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1)")
                                             << QList<QVariant>{QStringLiteral("bla"), 5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Insert);
        qb.setDatabaseType(DbType::PostgreSQL);
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        qb.setColumnValue(QStringLiteral("col2"), 5);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("insert multi column PSQL") << mBuilders.size() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1) RETURNING id")
                                                  << QList<QVariant>{QStringLiteral("bla"), 5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Insert);
        qb.setDatabaseType(DbType::PostgreSQL);
        qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
        qb.setColumnValue(QStringLiteral("col2"), 5);
        qb.setIdentificationColumn({});
        mBuilders.push_back(std::move(qb));
        QTest::newRow("insert multi column PSQL without id")
            << mBuilders.size() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1)") << QList<QVariant>{QStringLiteral("bla"), 5};
    }

    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Insert);
        qb.setColumnValues(QStringLiteral("col1"), QVariantList{QStringLiteral("bla"), QStringLiteral("ble"), QStringLiteral("blo")});
        qb.setColumnValues(QStringLiteral("col2"), QVariantList{1, 2, 3});
        mBuilders.push_back(std::move(qb));
        QTest::newRow("insert multiple rows") << mBuilders.size() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1), (:2, :3), (:4, :5)")
                                              << (QList<QVariant>()
                                                  << QVariantList{QStringLiteral("bla"), 1, QStringLiteral("ble"), 2, QStringLiteral("blo"), 3});
    }

    // test GROUP BY foo
    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("foo"));
        qb.addGroupColumn(QStringLiteral("id1"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select group by single column") << mBuilders.size() << QStringLiteral("SELECT foo FROM table GROUP BY id1") << QList<QVariant>{};
    }

    // test GROUP BY foo, bar
    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("foo"));
        qb.addGroupColumn(QStringLiteral("id1"));
        qb.addGroupColumn(QStringLiteral("id2"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select group by two columns") << mBuilders.size() << QStringLiteral("SELECT foo FROM table GROUP BY id1, id2") << QList<QVariant>{};
    }

    // test: HAVING .addValueCondition()
    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("foo"));
        qb.addGroupColumn(QStringLiteral("id1"));
        qb.addValueCondition(QStringLiteral("bar"), Equals, 1, QueryBuilder::HavingCondition);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select with having valueCond") << mBuilders.size() << QStringLiteral("SELECT foo FROM table GROUP BY id1 HAVING ( bar = :0 )")
                                                      << QList<QVariant>{1};
    }

    // test: HAVING .addColumnCondition()
    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("foo"));
        qb.addGroupColumn(QStringLiteral("id1"));
        qb.addValueCondition(QStringLiteral("bar"), Equals, 1, QueryBuilder::HavingCondition);
        qb.addColumnCondition(QStringLiteral("asdf"), Equals, QStringLiteral("yxcv"), QueryBuilder::HavingCondition);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select with having columnCond")
            << mBuilders.size() << QStringLiteral("SELECT foo FROM table GROUP BY id1 HAVING ( bar = :0 AND asdf = yxcv )") << QList<QVariant>{1};
    }

    // test: HAVING .addCondition()
    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("foo"));
        qb.addGroupColumn(QStringLiteral("id1"));
        qb.addValueCondition(QStringLiteral("bar"), Equals, 1, QueryBuilder::HavingCondition);
        Condition subCon;
        subCon.addColumnCondition(QStringLiteral("col1"), Query::Greater, QStringLiteral("col2"));
        subCon.addValueCondition(QStringLiteral("col1"), Query::NotEquals, QVariant());
        qb.addCondition(subCon, QueryBuilder::HavingCondition);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select with having condition") << mBuilders.size()
                                                      << QStringLiteral(
                                                             "SELECT foo FROM table GROUP BY id1 HAVING ( bar = :0 AND ( col1 > col2 AND col1 <> NULL ) )")
                                                      << QList<QVariant>{1};
    }

    // test: HAVING and WHERE
    {
        QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("foo"));
        qb.addGroupColumn(QStringLiteral("id1"));
        qb.addValueCondition(QStringLiteral("bar"), Equals, 1, QueryBuilder::HavingCondition);
        qb.addValueCondition(QStringLiteral("bla"), Equals, 2, QueryBuilder::WhereCondition);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select with having and where") << mBuilders.size()
                                                      << QStringLiteral("SELECT foo FROM table WHERE ( bla = :0 ) GROUP BY id1 HAVING ( bar = :1 )")
                                                      << QList<QVariant>{2, 1};
    }

    // SELECT with JOINs
    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table3"), QStringLiteral("table1.id"), QStringLiteral("table3.t1_id"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select left join and inner join (different tables)")
            << mBuilders.size()
            << QStringLiteral("SELECT col FROM table1 INNER JOIN table2 ON ( table2.t1_id = table1.id ) LEFT JOIN table3 ON ( table1.id = table3.t1_id )")
            << QList<QVariant>{};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        mBuilders.push_back(std::move(qb));
        // join-condition too verbose but should not have any impact on speed
        QTest::newRow("select left join and inner join (same table)")
            << mBuilders.size() << QStringLiteral("SELECT col FROM table1 INNER JOIN table2 ON ( table2.t1_id = table1.id AND ( table2.t1_id = table1.id ) )")
            << QList<QVariant>{};
    }

    // order of joins in the query should be the same as we add the joins in code
    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("b_table"), QStringLiteral("b_table.t1_id"), QStringLiteral("table1.id"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("a_table"), QStringLiteral("a_table.b_id"), QStringLiteral("b_table.id"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select join order")
            << mBuilders.size()
            << QStringLiteral("SELECT col FROM table1 INNER JOIN b_table ON ( b_table.t1_id = table1.id ) INNER JOIN a_table ON ( a_table.b_id = b_table.id )")
            << QList<QVariant>{};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col"));
        qb.addJoin(QueryBuilder::LeftOuterJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select left outer join") << mBuilders.size()
                                                << QStringLiteral("SELECT col FROM table1 LEFT OUTER JOIN table2 ON ( table2.t1_id = table1.id )")
                                                << QList<QVariant>{};
    }

    /// SELECT with CASE
    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Select);
        qb.addColumn(QStringLiteral("col"));
        qb.addColumn(Query::Case(QStringLiteral("col1"), Query::Greater, 42, QStringLiteral("1"), QStringLiteral("0")));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select case simple") << mBuilders.size() << QStringLiteral("SELECT col, CASE WHEN ( col1 > :0 ) THEN 1 ELSE 0 END FROM table1")
                                            << QList<QVariant>{42};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Select);
        qb.addAggregation(QStringLiteral("table1.col1"), QStringLiteral("sum"));
        qb.addAggregation(QStringLiteral("table1.col2"), QStringLiteral("count"));
        Query::Condition cond(Query::Or);
        cond.addValueCondition(QStringLiteral("table3.col2"), Query::Equals, "value1");
        cond.addValueCondition(QStringLiteral("table3.col2"), Query::Equals, "value2");
        Query::Case caseStmt(cond, QStringLiteral("1"), QStringLiteral("0"));
        qb.addAggregation(caseStmt, QStringLiteral("sum"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table2"), QStringLiteral("table1.col3"), QStringLiteral("table2.col1"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table3"), QStringLiteral("table2.col2"), QStringLiteral("table3.col1"));
        mBuilders.push_back(std::move(qb));
        QTest::newRow("select case, aggregation and joins")
            << mBuilders.size()
            << QString(
                   "SELECT sum(table1.col1), count(table1.col2), sum(CASE WHEN ( table3.col2 = :0 OR table3.col2 = :1 ) THEN 1 ELSE 0 END) "
                   "FROM table1 "
                   "LEFT JOIN table2 ON ( table1.col3 = table2.col1 ) "
                   "LEFT JOIN table3 ON ( table2.col2 = table3.col1 )")
            << QList<QVariant>{QStringLiteral("value1"), QStringLiteral("value2")};
    }

    /// UPDATE with INNER JOIN
    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Update);
        qb.setDatabaseType(DbType::MySQL);
        qb.setColumnValue(QStringLiteral("col"), 42);
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addValueCondition(QStringLiteral("table2.answer"), NotEquals, "foo");
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update inner join MySQL") << mBuilders.size()
                                                 << QStringLiteral(
                                                        "UPDATE table1, table2 SET col = :0 WHERE ( table2.answer <> :1 AND ( table2.t1_id = table1.id ) )")
                                                 << QList<QVariant>{42, QStringLiteral("foo")};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Update);
        qb.setDatabaseType(DbType::PostgreSQL);
        qb.setColumnValue(QStringLiteral("col"), 42);
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addValueCondition(QStringLiteral("table2.answer"), NotEquals, "foo");
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update inner join PSQL") << mBuilders.size()
                                                << QStringLiteral(
                                                       "UPDATE table1 SET col = :0 FROM table2 WHERE ( table2.answer <> :1 AND ( table2.t1_id = table1.id ) )")
                                                << QList<QVariant>{42, QStringLiteral("foo")};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Update);
        qb.setDatabaseType(DbType::Sqlite);
        qb.setColumnValue(QStringLiteral("col"), 42);
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addValueCondition(QStringLiteral("table2.answer"), NotEquals, "foo");
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update inner join SQLite")
            << mBuilders.size()
            << QStringLiteral("UPDATE table1 SET col = :0 WHERE ( ( SELECT table2.answer FROM table2 WHERE ( ( table2.t1_id = table1.id ) ) ) <> :1 )")
            << QList<QVariant>{42, QStringLiteral("foo")};
    }

    {
        QueryBuilder qb(QStringLiteral("table1"), QueryBuilder::Update);
        qb.setDatabaseType(DbType::Sqlite);
        qb.setColumnValue(QStringLiteral("col"), 42);
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addValueCondition(QStringLiteral("table2.answer"), NotEquals, "foo");
        Query::Condition condition;
        condition.addValueCondition(QStringLiteral("table2.col2"), Query::Equals, 666);
        condition.addValueCondition(QStringLiteral("table1.col3"), Query::Equals, "text");
        qb.addCondition(condition);
        qb.addValueCondition(QStringLiteral("table1.id"), Query::Equals, 10);
        mBuilders.push_back(std::move(qb));
        QTest::newRow("update inner join SQLite with subcondition")
            << mBuilders.size()
            << QString(
                   "UPDATE table1 SET col = :0 WHERE ( ( SELECT table2.answer FROM table2 WHERE "
                   "( ( table2.t1_id = table1.id ) ) ) <> :1 AND "
                   "( ( SELECT table2.col2 FROM table2 WHERE ( ( table2.t1_id = table1.id ) ) ) = :2 AND table1.col3 = :3 ) AND "
                   "table1.id = :4 )")
            << QList<QVariant>{42, QStringLiteral("foo"), 666, QStringLiteral("text"), 10};
    }
}

void QueryBuilderTest::testQueryBuilder()
{
    QFETCH(size_t, qbId);
    QFETCH(QString, sql);
    QFETCH(QList<QVariant>, bindValues);

    --qbId;

    QVERIFY(mBuilders[qbId].exec());
    QCOMPARE(mBuilders[qbId].mStatement, sql);
    QCOMPARE(mBuilders[qbId].mBindValues, bindValues);
}

void QueryBuilderTest::benchQueryBuilder()
{
    const QString table1 = QStringLiteral("Table1");
    const QString table2 = QStringLiteral("Table2");
    const QString table3 = QStringLiteral("Table3");
    const QString table1_id = QStringLiteral("Table1.id");
    const QString table2_id = QStringLiteral("Table2.id");
    const QString table3_id = QStringLiteral("Table3.id");
    const QString aggregate = QStringLiteral("COUNT");
    const QVariant value = QVariant::fromValue(QStringLiteral("asdf"));

    const QStringList columns = QStringList() << QStringLiteral("Table1.id") << QStringLiteral("Table1.fooAsdf") << QStringLiteral("Table2.barLala")
                                              << QStringLiteral("Table3.xyzFsd");

    bool executed = true;

    QBENCHMARK {
        QueryBuilder builder(table1, QueryBuilder::Select);
        builder.setDatabaseType(DbType::MySQL);
        builder.addColumns(columns);
        builder.addJoin(QueryBuilder::InnerJoin, table2, table2_id, table1_id);
        builder.addJoin(QueryBuilder::LeftJoin, table3, table1_id, table3_id);
        builder.addAggregation(columns.first(), aggregate);
        builder.addColumnCondition(columns.at(1), Query::LessOrEqual, columns.last());
        builder.addValueCondition(columns.at(3), Query::Equals, value);
        builder.addSortColumn(columns.at(2));
        builder.setLimit(10);
        builder.addGroupColumn(columns.at(3));
        executed = executed && builder.exec();
    }

    QVERIFY(executed);
}
