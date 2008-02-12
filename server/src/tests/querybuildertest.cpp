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
#include "querybuildertest.moc"

#define QUERYBUILDER_UNITTEST

#include "storage/query.cpp"
#include "storage/querybuilder.cpp"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( QueryBuilderTest )

Q_DECLARE_METATYPE( Akonadi::QueryBuilder )

using namespace Akonadi;

void QueryBuilderTest::testQueryBuilder_data()
{
  QTest::addColumn<QueryBuilder>( "qb" );
  QTest::addColumn<QString>( "sql" );
  QTest::addColumn<QList<QVariant> >( "bindValues" );

  QueryBuilder qb( QueryBuilder::Select );
  qb.addTable( "table" );
  qb.addColumn( "col1" );
  QTest::newRow( "simple select" ) << qb << QString( "SELECT col1 FROM table" ) << QList<QVariant>();

  qb.addColumn( "col2" );
  QTest::newRow( "simple select 2" ) << qb << QString( "SELECT col1, col2 FROM table" ) << QList<QVariant>();

  qb.addValueCondition( "col1", Query::Equals, QVariant( 5 ) );
  QList<QVariant> bindVals;
  bindVals << QVariant( 5 );
  QTest::newRow( "single where" ) << qb << QString( "SELECT col1, col2 FROM table WHERE ( col1 = :0 )" ) << bindVals;

  qb.addColumnCondition( "col1", Query::LessOrEqual, "col2" );
  QTest::newRow( "flat where" ) << qb << QString( "SELECT col1, col2 FROM table WHERE ( col1 = :0 AND col1 <= col2 )" ) << bindVals;

  qb.setSubQueryMode( Query::Or );
  QTest::newRow( "flat where 2" ) << qb << QString( "SELECT col1, col2 FROM table WHERE ( col1 = :0 OR col1 <= col2 )" ) << bindVals;

  Condition subCon;
  subCon.addColumnCondition( "col1", Query::Greater, "col2" );
  subCon.addValueCondition( "col1", Query::NotEquals, QVariant() );
  qb.addCondition( subCon );
  QTest::newRow( "hierarchical where" ) << qb << QString( "SELECT col1, col2 FROM table WHERE ( col1 = :0 OR col1 <= col2 OR ( col1 > col2 AND col1 <> NULL ) )" ) << bindVals;

  qb = QueryBuilder();
  qb.addTable( "table" );
  qb.addColumn( "col1" );
  qb.addSortColumn( "col1" );
  QTest::newRow( "single order by" ) << qb << QString( "SELECT col1 FROM table ORDER BY col1 ASC" ) << QList<QVariant>();

  qb.addSortColumn( "col2", Query::Descending );
  QTest::newRow( "multiple order by" ) << qb << QString( "SELECT col1 FROM table ORDER BY col1 ASC, col2 DESC" ) << QList<QVariant>();

  qb = QueryBuilder();
  qb.addTable( "table" );
  qb.addColumn( "col1" );
  QStringList vals;
  vals << "a" << "b" << "c";
  qb.addValueCondition( "col1", Query::In, vals );
  bindVals.clear();
  bindVals << QString("a") << QString("b") << QString("c");
  QTest::newRow( "where in" ) << qb << QString( "SELECT col1 FROM table WHERE ( col1 IN ( :0, :1, :2 ) )" ) << bindVals;

  qb = QueryBuilder( QueryBuilder::Update );
  qb.addTable( "table" );
  qb.updateColumnValue( "col1", QString( "bla" ) );
  bindVals.clear();
  bindVals << QString( "bla" );
  QTest::newRow( "update" ) << qb << QString( "UPDATE table SET col1 = :0" ) << bindVals;

  qb = QueryBuilder( QueryBuilder::Update );
  qb.addTable( "table" );
  qb.addTable( "table2" );
  qb.updateColumnValue( "col1", QString( "bla" ) );
  qb.addColumnCondition( "table1.id", Query::Equals, "table2.id" );
  bindVals.clear();
  bindVals << QString( "bla" );
  QTest::newRow( "update" ) << qb << QString( "UPDATE table, table2 SET col1 = :0 WHERE ( table1.id = table2.id )" )
      << bindVals;
}

void QueryBuilderTest::testQueryBuilder()
{
  QFETCH( QueryBuilder, qb );
  QFETCH( QString, sql );
  QFETCH( QList<QVariant>, bindValues );

  QVERIFY( qb.exec() );
  QCOMPARE( qb.mStatement, sql );
  QCOMPARE( qb.mBindValues, bindValues );
}
