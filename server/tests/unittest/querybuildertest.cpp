/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "querybuildertest.h"
#include "moc_querybuildertest.cpp"

#define QUERYBUILDER_UNITTEST

#include "storage/query.cpp"
#include "storage/querybuilder.cpp"

#include <QtTest/QtTest>

QTEST_MAIN(QueryBuilderTest)

using namespace Akonadi::Server;

void QueryBuilderTest::testQueryBuilder_data()
{
    mBuilders.clear();
    QTest::addColumn<int>("qbId");
    QTest::addColumn<QString>("sql");
    QTest::addColumn<QList<QVariant> >("bindValues");

    QueryBuilder qb("table", QueryBuilder::Select);
    qb.addColumn("col1");
    mBuilders << qb;
    QTest::newRow("simple select") << mBuilders.count() << QString("SELECT col1 FROM table") << QList<QVariant>();

    qb.addColumn("col2");
    mBuilders << qb;
    QTest::newRow("simple select 2") << mBuilders.count() << QString("SELECT col1, col2 FROM table") << QList<QVariant>();

    qb.addValueCondition("col1", Query::Equals, QVariant(5));
    QList<QVariant> bindVals;
    bindVals << QVariant(5);
    mBuilders << qb;
    QTest::newRow("single where") << mBuilders.count() << QString("SELECT col1, col2 FROM table WHERE ( col1 = :0 )") << bindVals;

    qb.addColumnCondition("col1", Query::LessOrEqual, "col2");
    mBuilders << qb;
    QTest::newRow("flat where") << mBuilders.count() << QString("SELECT col1, col2 FROM table WHERE ( col1 = :0 AND col1 <= col2 )") << bindVals;

    qb.setSubQueryMode(Query::Or);
    mBuilders << qb;
    QTest::newRow("flat where 2") << mBuilders.count() << QString("SELECT col1, col2 FROM table WHERE ( col1 = :0 OR col1 <= col2 )") << bindVals;

    Condition subCon;
    subCon.addColumnCondition("col1", Query::Greater, "col2");
    subCon.addValueCondition("col1", Query::NotEquals, QVariant());
    qb.addCondition(subCon);
    mBuilders << qb;
    QTest::newRow("hierarchical where") << mBuilders.count() << QString("SELECT col1, col2 FROM table WHERE ( col1 = :0 OR col1 <= col2 OR ( col1 > col2 AND col1 <> NULL ) )") << bindVals;

    qb = QueryBuilder("table");
    qb.addAggregation("col1", "count");
    mBuilders << qb;
    QTest::newRow("single aggregation") << mBuilders.count() << QString("SELECT count(col1) FROM table") << QList<QVariant>();

    qb = QueryBuilder("table");
    qb.addColumn("col1");
    qb.addSortColumn("col1");
    mBuilders << qb;
    QTest::newRow("single order by") << mBuilders.count() << QString("SELECT col1 FROM table ORDER BY col1 ASC") << QList<QVariant>();

    qb.addSortColumn("col2", Query::Descending);
    mBuilders << qb;
    QTest::newRow("multiple order by") << mBuilders.count() << QString("SELECT col1 FROM table ORDER BY col1 ASC, col2 DESC") << QList<QVariant>();

    qb = QueryBuilder("table");
    qb.addColumn("col1");
    QStringList vals;
    vals << "a" << "b" << "c";
    qb.addValueCondition("col1", Query::In, vals);
    bindVals.clear();
    bindVals << QString("a") << QString("b") << QString("c");
    mBuilders << qb;
    QTest::newRow("where in") << mBuilders.count() << QString("SELECT col1 FROM table WHERE ( col1 IN ( :0, :1, :2 ) )") << bindVals;

    qb = QueryBuilder("table", QueryBuilder::Select);
    qb.setDatabaseType(DbType::MySQL);
    qb.addColumn("col1");
    qb.setLimit(1);
    mBuilders << qb;
    QTest::newRow("SELECT with LIMIT") << mBuilders.count() << QString("SELECT col1 FROM table LIMIT 1") << QList<QVariant>();

    qb = QueryBuilder("table", QueryBuilder::Update);
    qb.setColumnValue("col1", QString("bla"));
    bindVals.clear();
    bindVals << QString("bla");
    mBuilders << qb;
    QTest::newRow("update") << mBuilders.count() << QString("UPDATE table SET col1 = :0") << bindVals;

    qb = QueryBuilder("table1", QueryBuilder::Update);
    qb.setDatabaseType(DbType::MySQL);
    qb.addJoin(QueryBuilder::InnerJoin, "table2", "table1.id", "table2.id");
    qb.addJoin(QueryBuilder::InnerJoin, "table3", "table1.id", "table3.id");
    qb.setColumnValue("col1", QString("bla"));
    bindVals.clear();
    bindVals << QString("bla");
    mBuilders << qb;
    QTest::newRow("update multi table MYSQL") << mBuilders.count() << QString("UPDATE table1, table2, table3 SET col1 = :0 WHERE ( ( table1.id = table2.id ) AND ( table1.id = table3.id ) )")
                                              << bindVals;

    qb = QueryBuilder("table1", QueryBuilder::Update);
    qb.setDatabaseType(DbType::PostgreSQL);
    qb.addJoin(QueryBuilder::InnerJoin, "table2", "table1.id", "table2.id");
    qb.addJoin(QueryBuilder::InnerJoin, "table3", "table1.id", "table3.id");
    qb.setColumnValue("col1", QString("bla"));
    mBuilders << qb;
    QTest::newRow("update multi table PSQL") << mBuilders.count() << QString("UPDATE table1 SET col1 = :0 FROM table2 JOIN table3 WHERE ( ( table1.id = table2.id ) AND ( table1.id = table3.id ) )")
                                             << bindVals;
    ///TODO: test for subquery in SQLite case

    qb = QueryBuilder("table", QueryBuilder::Insert);
    qb.setColumnValue("col1", QString("bla"));
    mBuilders << qb;
    QTest::newRow("insert single column") << mBuilders.count() << QString("INSERT INTO table (col1) VALUES (:0)") << bindVals;

    qb = QueryBuilder("table", QueryBuilder::Insert);
    qb.setColumnValue("col1", QString("bla"));
    qb.setColumnValue("col2", 5);
    bindVals << 5;
    mBuilders << qb;
    QTest::newRow("insert multi column") << mBuilders.count() << QString("INSERT INTO table (col1, col2) VALUES (:0, :1)") << bindVals;

    qb = QueryBuilder("table", QueryBuilder::Insert);
    qb.setDatabaseType(DbType::PostgreSQL);
    qb.setColumnValue("col1", QString("bla"));
    qb.setColumnValue("col2", 5);
    mBuilders << qb;
    QTest::newRow("insert multi column PSQL") << mBuilders.count() << QString("INSERT INTO table (col1, col2) VALUES (:0, :1) RETURNING id") << bindVals;

    qb.setIdentificationColumn(QString());
    mBuilders << qb;
    QTest::newRow("insert multi column PSQL without id") << mBuilders.count() << QString("INSERT INTO table (col1, col2) VALUES (:0, :1)") << bindVals;

    // test GROUP BY foo
    bindVals.clear();
    qb = QueryBuilder("table", QueryBuilder::Select);
    qb.addColumn("foo");
    qb.addGroupColumn("id1");
    mBuilders << qb;
    QTest::newRow("select group by single column") << mBuilders.count() << QString("SELECT foo FROM table GROUP BY id1") << bindVals;
    // test GROUP BY foo, bar
    qb.addGroupColumn("id2");
    mBuilders << qb;
    QTest::newRow("select group by two columns") << mBuilders.count() << QString("SELECT foo FROM table GROUP BY id1, id2") << bindVals;
    // test: HAVING .addValueCondition()
    qb.addValueCondition("bar", Equals, 1, QueryBuilder::HavingCondition);
    mBuilders << qb;
    bindVals << 1;
    QTest::newRow("select with having valueCond") << mBuilders.count() << QString("SELECT foo FROM table GROUP BY id1, id2 HAVING ( bar = :0 )") << bindVals;
    // test: HAVING .addColumnCondition()
    qb.addColumnCondition("asdf", Equals, "yxcv", QueryBuilder::HavingCondition);
    mBuilders << qb;
    QTest::newRow("select with having columnCond") << mBuilders.count() << QString("SELECT foo FROM table GROUP BY id1, id2 HAVING ( bar = :0 AND asdf = yxcv )") << bindVals;
    // test: HAVING .addCondition()
    qb.addCondition(subCon, QueryBuilder::HavingCondition);
    mBuilders << qb;
    QTest::newRow("select with having condition") << mBuilders.count() << QString("SELECT foo FROM table GROUP BY id1, id2 HAVING ( bar = :0 AND asdf = yxcv AND ( col1 > col2 AND col1 <> NULL ) )") << bindVals;
    // test: HAVING and WHERE
    qb.addValueCondition("bla", Equals, 2, QueryBuilder::WhereCondition);
    mBuilders << qb;
    bindVals.clear();
    bindVals << 2 << 1;
    QTest::newRow("select with having and where") << mBuilders.count() << QString("SELECT foo FROM table WHERE ( bla = :0 ) GROUP BY id1, id2 HAVING ( bar = :1 AND asdf = yxcv AND ( col1 > col2 AND col1 <> NULL ) )") << bindVals;

    {
        /// SELECT with JOINS
        QueryBuilder qbTpl = QueryBuilder("table1", QueryBuilder::Select);
        qbTpl.setDatabaseType(DbType::MySQL);
        qbTpl.addColumn("col");
        bindVals.clear();

        QueryBuilder qb = qbTpl;
        qb.addJoin(QueryBuilder::InnerJoin, "table2", "table2.t1_id", "table1.id");
        qb.addJoin(QueryBuilder::LeftJoin, "table3", "table1.id", "table3.t1_id");
        mBuilders << qb;
        QTest::newRow("select left join and inner join (different tables)") << mBuilders.count()
                                                                            << QString("SELECT col FROM table1 INNER JOIN table2 ON ( table2.t1_id = table1.id ) LEFT JOIN table3 ON ( table1.id = table3.t1_id )") << bindVals;

        qb = qbTpl;
        qb.addJoin(QueryBuilder::InnerJoin, "table2", "table2.t1_id", "table1.id");
        qb.addJoin(QueryBuilder::LeftJoin, "table2", "table2.t1_id", "table1.id");
        mBuilders << qb;
        // join-condition too verbose but should not have any impact on speed
        QTest::newRow("select left join and inner join (same table)") << mBuilders.count()
                                                                      << QString("SELECT col FROM table1 INNER JOIN table2 ON ( table2.t1_id = table1.id AND ( table2.t1_id = table1.id ) )") << bindVals;

        // order of joins in the query should be the same as we add the joins in code
        qb = qbTpl;
        qb.addJoin(QueryBuilder::InnerJoin, "b_table", "b_table.t1_id", "table1.id");
        qb.addJoin(QueryBuilder::InnerJoin, "a_table", "a_table.b_id", "b_table.id");
        mBuilders << qb;
        QTest::newRow("select join order") << mBuilders.count()
                                           << QString("SELECT col FROM table1 INNER JOIN b_table ON ( b_table.t1_id = table1.id ) INNER JOIN a_table ON ( a_table.b_id = b_table.id )") << bindVals;
    }

    {
        /// UPDATE with INNER JOIN
        QueryBuilder qbTpl = QueryBuilder("table1", QueryBuilder::Update);
        qbTpl.setColumnValue("col", 42);
        qbTpl.addJoin(QueryBuilder::InnerJoin, "table2", "table2.t1_id", "table1.id");
        qbTpl.addValueCondition("table2.answer", NotEquals, "foo");
        bindVals.clear();
        bindVals << QVariant(42) << QVariant("foo");

        qb = qbTpl;
        qb.setDatabaseType(DbType::MySQL);
        mBuilders << qb;
        QTest::newRow("update inner join MySQL") << mBuilders.count()
                                                 << QString("UPDATE table1, table2 SET col = :0 WHERE ( table2.answer <> :1 AND ( table2.t1_id = table1.id ) )") << bindVals;

        qb = qbTpl;
        qb.setDatabaseType(DbType::PostgreSQL);
        mBuilders << qb;
        QTest::newRow("update inner join PSQL") << mBuilders.count()
                                                << QString("UPDATE table1 SET col = :0 FROM table2 WHERE ( table2.answer <> :1 AND ( table2.t1_id = table1.id ) )") << bindVals;

        qb = qbTpl;
        qb.setDatabaseType(DbType::Sqlite);
        mBuilders << qb;
        QTest::newRow("update inner join SQLite") << mBuilders.count()
                                                  << QString("UPDATE table1 SET col = :0 WHERE ( ( SELECT table2.answer FROM table2 WHERE ( ( table2.t1_id = table1.id ) ) ) <> :1 )") << bindVals;

        qb = qbTpl;
        qb.setDatabaseType(DbType::Sqlite);
        Query::Condition condition;
        condition.addValueCondition("table2.col2", Query::Equals, 666);
        condition.addValueCondition("table1.col3", Query::Equals, "text");
        qb.addCondition(condition);
        qb.addValueCondition("table1.id", Query::Equals, 10);
        mBuilders << qb;
        bindVals << 666 << "text" << 10;
        QTest::newRow("update inner join SQLite with subcondition") << mBuilders.count()
                                                                    << QString("UPDATE table1 SET col = :0 WHERE ( ( SELECT table2.answer FROM table2 WHERE "
                                                                               "( ( table2.t1_id = table1.id ) ) ) <> :1 AND "
                                                                               "( ( SELECT table2.col2 FROM table2 WHERE ( ( table2.t1_id = table1.id ) ) ) = :2 AND table1.col3 = :3 ) AND "
                                                                               "table1.id = :4 )") << bindVals;

    }
}

void QueryBuilderTest::testQueryBuilder()
{
    QFETCH(int, qbId);
    QFETCH(QString, sql);
    QFETCH(QList<QVariant>, bindValues);

    --qbId;

    QVERIFY(mBuilders[qbId].exec());
    QCOMPARE(mBuilders[qbId].mStatement, sql);
    QCOMPARE(mBuilders[qbId].mBindValues, bindValues);
}
