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

#include "collectionattributetest.h"
#include "collectionattributetest.moc"

#include <libakonadi/collection.h>
#include <libakonadi/collectionattributefactory.h>
#include <libakonadi/collectioncreatejob.h>
#include <libakonadi/collectiondeletejob.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/collectionpathresolver.h>
#include <libakonadi/control.h>

#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( CollectionAttributeTest, NoGUI )

class TestAttribute : public CollectionAttribute
{
  public:
    TestAttribute() : CollectionAttribute() {}
    TestAttribute* clone() const { return new TestAttribute(); }
    QByteArray type() const { return "TESTATTRIBUTE"; }
    QByteArray toByteArray() const { return mData; }
    void setData( const QByteArray &data ) { mData = data; }
  private:
    QByteArray mData;
};

static int parentColId = -1;

void CollectionAttributeTest::initTestCase()
{
  Control::start();
  CollectionAttributeFactory::registerAttribute<TestAttribute>();

  CollectionPathResolver *resolver = new CollectionPathResolver( "res3", this );
  QVERIFY( resolver->exec() );
  parentColId = resolver->collection();
  QVERIFY( parentColId > 0 );
}

void CollectionAttributeTest::testAttributes_data()
{
  QTest::addColumn<QByteArray>("attr1");
  QTest::addColumn<QByteArray>("attr2");

  QTest::newRow("basic") << QByteArray("foo") << QByteArray("bar");
  QTest::newRow("emtpy1") << QByteArray("") << QByteArray("non-empty");
  QTest::newRow("empty2") << QByteArray("non-empty") << QByteArray("");
  QTest::newRow("space") << QByteArray("foo bar") << QByteArray("bar foo");
  QTest::newRow("quotes") << QByteArray("\"quoted \\ test\"") << QByteArray("single \" quote \\");
  QTest::newRow("parenthesis") << QByteArray(")") << QByteArray("(");
  QTest::newRow("binary") << QByteArray("\000") << QByteArray("\001");
}

void CollectionAttributeTest::testAttributes()
{
  QFETCH( QByteArray, attr1 );
  QFETCH( QByteArray, attr2 );

  // add a custom attribute
  TestAttribute *attr = new TestAttribute();
  attr->setData( attr1 );
  CollectionCreateJob *create = new CollectionCreateJob( Collection( parentColId ), "attribute test", this );
  create->setAttribute( attr );
  delete attr;
  QVERIFY( create->exec() );
  Collection col = create->collection();
  QVERIFY( col.isValid() );

  attr = col.attribute<TestAttribute>();
#if 0
  QVERIFY( attr != 0 );
  QCOMPARE( attr->toByteArray(), QByteArray( attr1 ) );
#endif

  CollectionListJob *list = new CollectionListJob( col, CollectionListJob::Local, this );
  QVERIFY( list->exec() );
  QCOMPARE( list->collections().count(), 1 );
  col = list->collections().first();

  QVERIFY( col.isValid() );
  attr = col.attribute<TestAttribute>();
  QVERIFY( attr != 0 );
  QCOMPARE( attr->toByteArray(), QByteArray( attr1 ) );


  // modify a custom attribute
  attr = new TestAttribute();
  attr->setData( attr2 );
  CollectionModifyJob *modify = new CollectionModifyJob( col, this );
  modify->setAttribute( attr );
  delete attr;
  QVERIFY( modify->exec() );

  list = new CollectionListJob( col, CollectionListJob::Local, this );
  QVERIFY( list->exec() );
  QCOMPARE( list->collections().count(), 1 );
  col = list->collections().first();

  QVERIFY( col.isValid() );
  attr = col.attribute<TestAttribute>();
  QVERIFY( attr != 0 );
  QCOMPARE( attr->toByteArray(), QByteArray( attr2 ) );


  // delete a custom attribute
  modify = new CollectionModifyJob( col, this );
  modify->removeAttribute( attr->type() );
  QVERIFY( modify->exec() );

  list = new CollectionListJob( col, CollectionListJob::Local, this );
  QVERIFY( list->exec() );
  QCOMPARE( list->collections().count(), 1 );
  col = list->collections().first();

  QVERIFY( col.isValid() );
  attr = col.attribute<TestAttribute>();
  QVERIFY( attr == 0 );


  // cleanup
  CollectionDeleteJob *del = new CollectionDeleteJob( col, this );
  QVERIFY( del->exec() );

}

void CollectionAttributeTest::testDefaultAttributes()
{
  Collection col;
  QCOMPARE( col.attributes().count(), 0 );
  col.addRawAttribute( "TYPE", "VALUE" );
  QCOMPARE( col.attributes().count(), 1 );
  QVERIFY( col.hasAttribute( "TYPE" ) );
  QCOMPARE( col.attribute( "TYPE" )->toByteArray(), QByteArray("VALUE") );
}
