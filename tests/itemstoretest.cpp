  /*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

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

#include "control.h"
#include "itemstoretest.h"
#include "testattribute.h"
#include <akonadi/attributefactory.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionselectjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemmovejob.h>
#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( ItemStoreTest, NoGUI )

static Collection res1_foo;
static Collection res2;
static Collection res3;

void ItemStoreTest::initTestCase()
{
  Control::start();
  AttributeFactory::registerAttribute<TestAttribute>();

  // get the collections we run the tests on
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  Collection res1;
  foreach ( const Collection col, list ) {
    if ( col.name() == "res1" )
      res1 = col;
    if ( col.name() == "res2" )
      res2 = col;
    if ( col.name() == "res3" )
      res3 = col;
  }
  foreach ( const Collection col, list ) {
    if ( col.name() == "foo" && col.parent() == res1.id() )
      res1_foo = col;
  }
}

void ItemStoreTest::testFlagChange()
{
  ItemFetchJob *fjob = new ItemFetchJob( Item( 1 ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  Item item = fjob->items()[0];

  // add a flag
  Item::Flags origFlags = item.flags();
  Item::Flags expectedFlags = origFlags;
  expectedFlags.insert( "added_test_flag_1" );
  item.setFlag( "added_test_flag_1" );
  ItemModifyJob *sjob = new ItemModifyJob( item, this );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( Item( 1 ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.flags().count(), expectedFlags.count() );
  Item::Flags diff = expectedFlags - item.flags();
  QVERIFY( diff.isEmpty() );

  // set flags
  expectedFlags.insert( "added_test_flag_2" );
  item.setFlags( expectedFlags );
  sjob = new ItemModifyJob( item, this );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( Item( 1 ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.flags().count(), expectedFlags.count() );
  diff = expectedFlags - item.flags();
  QVERIFY( diff.isEmpty() );

  // remove a flag
  item.clearFlag( "added_test_flag_1" );
  item.clearFlag( "added_test_flag_2" );
  sjob = new ItemModifyJob( item, this );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( Item( 1 ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.flags().count(), origFlags.count() );
  diff = origFlags - item.flags();
  QVERIFY( diff.isEmpty() );
}

void ItemStoreTest::testDataChange_data()
{
  QTest::addColumn<QByteArray>( "data" );

  QTest::newRow( "empty" ) << QByteArray();
  QTest::newRow( "nullbyte" ) << QByteArray("\0" );
  QTest::newRow( "nullbyte2" ) << QByteArray( "\0X" );
  QTest::newRow( "linebreaks" ) << QByteArray( "line1\nline2\n\rline3\rline4\r\n" );
  QTest::newRow( "linebreaks2" ) << QByteArray( "line1\r\nline2\r\n\r\n" );
  QTest::newRow( "linebreaks3" ) << QByteArray( "line1\nline2" );
  QTest::newRow( "simple" ) << QByteArray( "testbody" );
}

void ItemStoreTest::testDataChange()
{
  QFETCH( QByteArray, data );

  Item item;
  ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
  prefetchjob->exec();
  item = prefetchjob->items()[0];
  item.setMimeType( "application/octet-stream" );
  item.setPayload( data );

  // delete data
  ItemModifyJob *sjob = new ItemModifyJob( item );
  sjob->storePayload();
  QVERIFY( sjob->exec() );

  ItemFetchJob *fjob = new ItemFetchJob( Item( 1 ) );
  fjob->fetchScope().addFetchPart( Item::FullPayload );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QVERIFY( item.hasPayload<QByteArray>() );
  QCOMPARE( item.payload<QByteArray>(), data );
}

void ItemStoreTest::testItemMove()
{
  ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];

  ItemMoveJob *store = new ItemMoveJob( item, res3, this );
  QVERIFY( store->exec() );

  ItemFetchJob *fetch = new ItemFetchJob( res3, this );
  QVERIFY( fetch->exec() );
  QCOMPARE( fetch->items().count(), 1 );

  store = new ItemMoveJob( item, res1_foo, this );
  QVERIFY( store->exec() );
}

void ItemStoreTest::testIllegalItemMove()
{
  ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];

  // move into invalid collection
  ItemMoveJob *store = new ItemMoveJob( item, Collection( INT_MAX ), this );
  QVERIFY( !store->exec() );

  // move item into folder that doesn't support its mimetype
  store = new ItemMoveJob( item, res2, this );
  QEXPECT_FAIL( "", "Check not yet implemented by the server.", Continue );
  QVERIFY( !store->exec() );
}

void ItemStoreTest::testRemoteId_data()
{
  QTest::addColumn<QString>( "rid" );
  QTest::addColumn<QString>( "exprid" );

  QTest::newRow( "set" ) << QString( "A" ) << QString( "A" );
  QTest::newRow( "no-change" ) << QString() << QString( "A" );
  QTest::newRow( "clear" ) << QString( "" ) << QString( "" );
  QTest::newRow( "reset" ) << QString( "A" ) << QString( "A" );
}

void ItemStoreTest::testRemoteId()
{
  QFETCH( QString, rid );
  QFETCH( QString, exprid );

  ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];

  item.setId( 1 );
  item.setRemoteId( rid );
  ItemModifyJob *store = new ItemModifyJob( item, this );
  QVERIFY( store->exec() );

  ItemFetchJob *fetch = new ItemFetchJob( item, this );
  QVERIFY( fetch->exec() );
  QCOMPARE( fetch->items().count(), 1 );
  item = fetch->items().at( 0 );
  QCOMPARE( item.remoteId(), exprid );
}

void ItemStoreTest::testMultiPart()
{
  ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "testmailbody" );
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra";

  // store item
  ItemModifyJob *sjob = new ItemModifyJob( item );
  sjob->storePayload();
  QVERIFY( sjob->exec() );

  ItemFetchJob *fjob = new ItemFetchJob( Item( 1 ) );
  fjob->fetchScope().addFetchPart( "EXTRA" );
  fjob->fetchScope().addFetchPart( Item::FullPayload );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.payload<QByteArray>(), QByteArray("testmailbody") );
  QVERIFY( item.hasAttribute<TestAttribute>() );
  QCOMPARE( item.attribute<TestAttribute>()->data, QByteArray("extra") );

  // clean up
  item.removeAttribute( "EXTRA" );
  sjob = new ItemModifyJob( item );
  QVERIFY( sjob->exec() );
}

void ItemStoreTest::testPartRemove()
{
  ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 2 ) );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];
  item.setMimeType( "application/octet-stream" );
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra";

  // store item
  ItemModifyJob *sjob = new ItemModifyJob( item );
  sjob->storePayload();
  QVERIFY( sjob->exec() );

  // fetch item and its parts (should be RFC822, HEAD and EXTRA)
  ItemFetchJob *fjob = new ItemFetchJob( Item( 2 ) );
  fjob->fetchScope().setFetchAllParts( true );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.attributes().count(), 2 );
  QVERIFY( item.hasAttribute<TestAttribute>() );

  // remove a part
  item.removeAttribute<TestAttribute>();
  sjob = new ItemModifyJob( item );
  QVERIFY( sjob->exec() );

  // fetch item again (should only have RFC822 and HEAD left)
  ItemFetchJob *fjob2 = new ItemFetchJob( Item( 2 ) );
  fjob2->fetchScope().setFetchAllParts( true );
  QVERIFY( fjob2->exec() );
  QCOMPARE( fjob2->items().count(), 1 );
  item = fjob2->items()[0];
  QCOMPARE( item.attributes().count(), 1 );
  QVERIFY( !item.hasAttribute<TestAttribute>() );
}

void ItemStoreTest::testRevisionCheck()
{
  // make sure we don't have any other collection selected
  // otherwise EXPUNGE doesn't work and will be triggered by
  // the following tests and mess up the monitor testing
  CollectionSelectJob *sel = new CollectionSelectJob( Collection::root(), this );
  QVERIFY( sel->exec() );

  // fetch same item twice
  Item ref( 2 );
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  Item item1 = prefetchjob->items()[0];
  Item item2 = prefetchjob->items()[0];

  // store first item unmodified
  ItemModifyJob *sjob = new ItemModifyJob( item1 );
  QVERIFY( sjob->exec() );

  // try to store second item
  ItemModifyJob *sjob2 = new ItemModifyJob( item2 );
  item2.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra";
  sjob2->storePayload();
  QVERIFY( !sjob2->exec() );

  // fetch same again
  prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  item1 = prefetchjob->items()[0];

  // delete item
  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  djob->exec();

  // try to store it
  sjob = new ItemModifyJob( item1 );
  QVERIFY( !sjob->exec() );
}

#include "itemstoretest.moc"
