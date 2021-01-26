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

Q_DECLARE_METATYPE(QVector<QVariant>)

using namespace Akonadi::Server;

void QueryBuilderTest::testQueryBuilder_data()
{
    qRegisterMetaType<QVector<QVariant>>();
    mBuilders.clear();
    QTest::addColumn<int>("qbId");
    QTest::addColumn<QString>("sql");
    QTest::addColumn<QVector<QVariant>>("bindValues");

    QueryBuilder qb(QStringLiteral("table"), QueryBuilder::Select);
    qb.addColumn(QStringLiteral("col1"));
    mBuilders << qb;
    QTest::newRow("simple select") << mBuilders.count() << QStringLiteral("SELECT col1 FROM table") << QVector<QVariant>();

    qb.addColumn(QStringLiteral("col2"));
    mBuilders << qb;
    QTest::newRow("simple select 2") << mBuilders.count() << QStringLiteral("SELECT col1, col2 FROM table") << QVector<QVariant>();

    qb.addValueCondition(QStringLiteral("col1"), Query::Equals, QVariant(5));
    QVector<QVariant> bindVals;
    bindVals << QVariant(5);
    mBuilders << qb;
    QTest::newRow("single where") << mBuilders.count() << QStringLiteral("SELECT col1, col2 FROM table WHERE ( col1 = :0 )") << bindVals;

    qb.addColumnCondition(QStringLiteral("col1"), Query::LessOrEqual, QStringLiteral("col2"));
    mBuilders << qb;
    QTest::newRow("flat where") << mBuilders.count() << QStringLiteral("SELECT col1, col2 FROM table WHERE ( col1 = :0 AND col1 <= col2 )") << bindVals;

    qb.setSubQueryMode(Query::Or);
    mBuilders << qb;
    QTest::newRow("flat where 2") << mBuilders.count() << QStringLiteral("SELECT col1, col2 FROM table WHERE ( col1 = :0 OR col1 <= col2 )") << bindVals;

    Condition subCon;
    subCon.addColumnCondition(QStringLiteral("col1"), Query::Greater, QStringLiteral("col2"));
    subCon.addValueCondition(QStringLiteral("col1"), Query::NotEquals, QVariant());
    qb.addCondition(subCon);
    mBuilders << qb;
    QTest::newRow("hierarchical where") << mBuilders.count()
                                        << QStringLiteral(
                                               "SELECT col1, col2 FROM table WHERE ( col1 = :0 OR col1 <= col2 OR ( col1 > col2 AND col1 <> NULL ) )")
                                        << bindVals;

    qb = QueryBuilder(QStringLiteral("table"));
    qb.addAggregation(QStringLiteral("col1"), QStringLiteral("count"));
    mBuilders << qb;
    QTest::newRow("single aggregation") << mBuilders.count() << QStringLiteral("SELECT count(col1) FROM table") << QVector<QVariant>();

    qb = QueryBuilder(QStringLiteral("table"));
    qb.addColumn(QStringLiteral("col1"));
    qb.addSortColumn(QStringLiteral("col1"));
    mBuilders << qb;
    QTest::newRow("single order by") << mBuilders.count() << QStringLiteral("SELECT col1 FROM table ORDER BY col1 ASC") << QVector<QVariant>();

    qb.addSortColumn(QStringLiteral("col2"), Query::Descending);
    mBuilders << qb;
    QTest::newRow("multiple order by") << mBuilders.count() << QStringLiteral("SELECT col1 FROM table ORDER BY col1 ASC, col2 DESC") << QVector<QVariant>();

    qb = QueryBuilder(QStringLiteral("table"));
    qb.addColumn(QStringLiteral("col1"));
    QStringList vals;
    vals << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("c");
    qb.addValueCondition(QStringLiteral("col1"), Query::In, vals);
    bindVals.clear();
    bindVals << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("c");
    mBuilders << qb;
    QTest::newRow("where in") << mBuilders.count() << QStringLiteral("SELECT col1 FROM table WHERE ( col1 IN ( :0, :1, :2 ) )") << bindVals;

    qb = QueryBuilder(QStringLiteral("table"), QueryBuilder::Select);
    qb.setDatabaseType(DbType::MySQL);
    qb.addColumn(QStringLiteral("col1"));
    qb.setLimit(1);
    mBuilders << qb;
    QTest::newRow("SELECT with LIMIT") << mBuilders.count() << QStringLiteral("SELECT col1 FROM table LIMIT 1") << QVector<QVariant>();

    qb = QueryBuilder(QStringLiteral("table"), QueryBuilder::Update);
    qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
    bindVals.clear();
    bindVals << QStringLiteral("bla");
    mBuilders << qb;
    QTest::newRow("update") << mBuilders.count() << QStringLiteral("UPDATE table SET col1 = :0") << bindVals;

    qb = QueryBuilder(QStringLiteral("table1"), QueryBuilder::Update);
    qb.setDatabaseType(DbType::MySQL);
    qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table1.id"), QStringLiteral("table2.id"));
    qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table3"), QStringLiteral("table1.id"), QStringLiteral("table3.id"));
    qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
    bindVals.clear();
    bindVals << QStringLiteral("bla");
    mBuilders << qb;
    QTest::newRow("update multi table MYSQL")
        << mBuilders.count() << QStringLiteral("UPDATE table1, table2, table3 SET col1 = :0 WHERE ( ( table1.id = table2.id ) AND ( table1.id = table3.id ) )")
        << bindVals;

    qb = QueryBuilder(QStringLiteral("table1"), QueryBuilder::Update);
    qb.setDatabaseType(DbType::PostgreSQL);
    qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table1.id"), QStringLiteral("table2.id"));
    qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table3"), QStringLiteral("table1.id"), QStringLiteral("table3.id"));
    qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
    mBuilders << qb;
    QTest::newRow("update multi table PSQL")
        << mBuilders.count()
        << QStringLiteral("UPDATE table1 SET col1 = :0 FROM table2 JOIN table3 WHERE ( ( table1.id = table2.id ) AND ( table1.id = table3.id ) )") << bindVals;
    /// TODO: test for subquery in SQLite case

    qb = QueryBuilder(QStringLiteral("table"), QueryBuilder::Insert);
    qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
    mBuilders << qb;
    QTest::newRow("insert single column") << mBuilders.count() << QStringLiteral("INSERT INTO table (col1) VALUES (:0)") << bindVals;

    qb = QueryBuilder(QStringLiteral("table"), QueryBuilder::Insert);
    qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
    qb.setColumnValue(QStringLiteral("col2"), 5);
    bindVals << 5;
    mBuilders << qb;
    QTest::newRow("insert multi column") << mBuilders.count() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1)") << bindVals;

    qb = QueryBuilder(QStringLiteral("table"), QueryBuilder::Insert);
    qb.setDatabaseType(DbType::PostgreSQL);
    qb.setColumnValue(QStringLiteral("col1"), QStringLiteral("bla"));
    qb.setColumnValue(QStringLiteral("col2"), 5);
    mBuilders << qb;
    QTest::newRow("insert multi column PSQL") << mBuilders.count() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1) RETURNING id") << bindVals;

    qb.setIdentificationColumn(QString());
    mBuilders << qb;
    QTest::newRow("insert multi column PSQL without id") << mBuilders.count() << QStringLiteral("INSERT INTO table (col1, col2) VALUES (:0, :1)") << bindVals;

    // test GROUP BY foo
    bindVals.clear();
    qb = QueryBuilder(QStringLiteral("table"), QueryBuilder::Select);
    qb.addColumn(QStringLiteral("foo"));
    qb.addGroupColumn(QStringLiteral("id1"));
    mBuilders << qb;
    QTest::newRow("select group by single column") << mBuilders.count() << QStringLiteral("SELECT foo FROM table GROUP BY id1") << bindVals;
    // test GROUP BY foo, bar
    qb.addGroupColumn(QStringLiteral("id2"));
    mBuilders << qb;
    QTest::newRow("select group by two columns") << mBuilders.count() << QStringLiteral("SELECT foo FROM table GROUP BY id1, id2") << bindVals;
    // test: HAVING .addValueCondition()
    qb.addValueCondition(QStringLiteral("bar"), Equals, 1, QueryBuilder::HavingCondition);
    mBuilders << qb;
    bindVals << 1;
    QTest::newRow("select with having valueCond") << mBuilders.count() << QStringLiteral("SELECT foo FROM table GROUP BY id1, id2 HAVING ( bar = :0 )")
                                                  << bindVals;
    // test: HAVING .addColumnCondition()
    qb.addColumnCondition(QStringLiteral("asdf"), Equals, QStringLiteral("yxcv"), QueryBuilder::HavingCondition);
    mBuilders << qb;
    QTest::newRow("select with having columnCond") << mBuilders.count()
                                                   << QStringLiteral("SELECT foo FROM table GROUP BY id1, id2 HAVING ( bar = :0 AND asdf = yxcv )") << bindVals;
    // test: HAVING .addCondition()
    qb.addCondition(subCon, QueryBuilder::HavingCondition);
    mBuilders << qb;
    QTest::newRow("select with having condition")
        << mBuilders.count()
        << QStringLiteral("SELECT foo FROM table GROUP BY id1, id2 HAVING ( bar = :0 AND asdf = yxcv AND ( col1 > col2 AND col1 <> NULL ) )") << bindVals;
    // test: HAVING and WHERE
    qb.addValueCondition(QStringLiteral("bla"), Equals, 2, QueryBuilder::WhereCondition);
    mBuilders << qb;
    bindVals.clear();
    bindVals << 2 << 1;
    QTest::newRow("select with having and where")
        << mBuilders.count()
        << QStringLiteral("SELECT foo FROM table WHERE ( bla = :0 ) GROUP BY id1, id2 HAVING ( bar = :1 AND asdf = yxcv AND ( col1 > col2 AND col1 <> NULL ) )")
        << bindVals;

    {
        /// SELECT with JOINS
        QueryBuilder qbTpl = QueryBuilder(QStringLiteral("table1"), QueryBuilder::Select);
        qbTpl.setDatabaseType(DbType::MySQL);
        qbTpl.addColumn(QStringLiteral("col"));
        bindVals.clear();

        QueryBuilder qb = qbTpl;
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table3"), QStringLiteral("table1.id"), QStringLiteral("table3.t1_id"));
        mBuilders << qb;
        QTest::newRow("select left join and inner join (different tables)")
            << mBuilders.count()
            << QStringLiteral("SELECT col FROM table1 INNER JOIN table2 ON ( table2.t1_id = table1.id ) LEFT JOIN table3 ON ( table1.id = table3.t1_id )")
            << bindVals;

        qb = qbTpl;
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        mBuilders << qb;
        // join-condition too verbose but should not have any impact on speed
        QTest::newRow("select left join and inner join (same table)")
            << mBuilders.count() << QStringLiteral("SELECT col FROM table1 INNER JOIN table2 ON ( table2.t1_id = table1.id AND ( table2.t1_id = table1.id ) )")
            << bindVals;

        // order of joins in the query should be the same as we add the joins in code
        qb = qbTpl;
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("b_table"), QStringLiteral("b_table.t1_id"), QStringLiteral("table1.id"));
        qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("a_table"), QStringLiteral("a_table.b_id"), QStringLiteral("b_table.id"));
        mBuilders << qb;
        QTest::newRow("select join order")
            << mBuilders.count()
            << QStringLiteral("SELECT col FROM table1 INNER JOIN b_table ON ( b_table.t1_id = table1.id ) INNER JOIN a_table ON ( a_table.b_id = b_table.id )")
            << bindVals;
    }

    {
        /// SELECT with CASE
        QueryBuilder qbTpl = QueryBuilder(QStringLiteral("table1"), QueryBuilder::Select);
        qbTpl.setDatabaseType(DbType::MySQL);

        QueryBuilder qb = qbTpl;
        qb.addColumn(QStringLiteral("col"));
        qb.addColumn(Query::Case(QStringLiteral("col1"), Query::Greater, 42, QStringLiteral("1"), QStringLiteral("0")));
        bindVals.clear();
        bindVals << 42;
        mBuilders << qb;
        QTest::newRow("select case simple") << mBuilders.count() << QStringLiteral("SELECT col, CASE WHEN ( col1 > :0 ) THEN 1 ELSE 0 END FROM table1")
                                            << bindVals;

        qb = qbTpl;
        qb.addAggregation(QStringLiteral("table1.col1"), QStringLiteral("sum"));
        qb.addAggregation(QStringLiteral("table1.col2"), QStringLiteral("count"));
        Query::Condition cond(Query::Or);
        cond.addValueCondition(QStringLiteral("table3.col2"), Query::Equals, "value1");
        cond.addValueCondition(QStringLiteral("table3.col2"), Query::Equals, "value2");
        Query::Case caseStmt(cond, QStringLiteral("1"), QStringLiteral("0"));
        qb.addAggregation(caseStmt, QStringLiteral("sum"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table2"), QStringLiteral("table1.col3"), QStringLiteral("table2.col1"));
        qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral("table3"), QStringLiteral("table2.col2"), QStringLiteral("table3.col1"));
        bindVals.clear();
        bindVals << QStringLiteral("value1") << QStringLiteral("value2");
        mBuilders << qb;
        QTest::newRow("select case, aggregation and joins")
            << mBuilders.count()
            << QString(
                   "SELECT sum(table1.col1), count(table1.col2), sum(CASE WHEN ( table3.col2 = :0 OR table3.col2 = :1 ) THEN 1 ELSE 0 END) "
                   "FROM table1 "
                   "LEFT JOIN table2 ON ( table1.col3 = table2.col1 ) "
                   "LEFT JOIN table3 ON ( table2.col2 = table3.col1 )")
            << bindVals;
    }

    {
        /// UPDATE with INNER JOIN
        QueryBuilder qbTpl = QueryBuilder(QStringLiteral("table1"), QueryBuilder::Update);
        qbTpl.setColumnValue(QStringLiteral("col"), 42);
        qbTpl.addJoin(QueryBuilder::InnerJoin, QStringLiteral("table2"), QStringLiteral("table2.t1_id"), QStringLiteral("table1.id"));
        qbTpl.addValueCondition(QStringLiteral("table2.answer"), NotEquals, "foo");
        bindVals.clear();
        bindVals << QVariant(42) << QVariant("foo");

        qb = qbTpl;
        qb.setDatabaseType(DbType::MySQL);
        mBuilders << qb;
        QTest::newRow("update inner join MySQL") << mBuilders.count()
                                                 << QStringLiteral(
                                                        "UPDATE table1, table2 SET col = :0 WHERE ( table2.answer <> :1 AND ( table2.t1_id = table1.id ) )")
                                                 << bindVals;

        qb = qbTpl;
        qb.setDatabaseType(DbType::PostgreSQL);
        mBuilders << qb;
        QTest::newRow("update inner join PSQL") << mBuilders.count()
                                                << QStringLiteral(
                                                       "UPDATE table1 SET col = :0 FROM table2 WHERE ( table2.answer <> :1 AND ( table2.t1_id = table1.id ) )")
                                                << bindVals;

        qb = qbTpl;
        qb.setDatabaseType(DbType::Sqlite);
        mBuilders << qb;
        QTest::newRow("update inner join SQLite")
            << mBuilders.count()
            << QStringLiteral("UPDATE table1 SET col = :0 WHERE ( ( SELECT table2.answer FROM table2 WHERE ( ( table2.t1_id = table1.id ) ) ) <> :1 )")
            << bindVals;

        qb = qbTpl;
        qb.setDatabaseType(DbType::Sqlite);
        Query::Condition condition;
        condition.addValueCondition(QStringLiteral("table2.col2"), Query::Equals, 666);
        condition.addValueCondition(QStringLiteral("table1.col3"), Query::Equals, "text");
        qb.addCondition(condition);
        qb.addValueCondition(QStringLiteral("table1.id"), Query::Equals, 10);
        mBuilders << qb;
        bindVals << 666 << "text" << 10;
        QTest::newRow("update inner join SQLite with subcondition")
            << mBuilders.count()
            << QString(
                   "UPDATE table1 SET col = :0 WHERE ( ( SELECT table2.answer FROM table2 WHERE "
                   "( ( table2.t1_id = table1.id ) ) ) <> :1 AND "
                   "( ( SELECT table2.col2 FROM table2 WHERE ( ( table2.t1_id = table1.id ) ) ) = :2 AND table1.col3 = :3 ) AND "
                   "table1.id = :4 )")
            << bindVals;
    }
}

void QueryBuilderTest::testQueryBuilder()
{
    QFETCH(int, qbId);
    QFETCH(QString, sql);
    QFETCH(QVector<QVariant>, bindValues);

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
