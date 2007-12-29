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
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionselectjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( ItemStoreTest, NoGUI )

static Collection res1_foo;
static Collection res2;
static Collection res3;

void ItemStoreTest::initTestCase()
{
  Control::start();

  // get the collections we run the tests on
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive );
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
  ItemFetchJob *fjob = new ItemFetchJob( DataReference( 1, QString() ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  Item item = fjob->items()[0];

  // add a flag
  Item::Flags origFlags = item.flags();
  Item::Flags expectedFlags = origFlags;
  expectedFlags.insert( "added_test_flag_1" );
  ItemStoreJob *sjob = new ItemStoreJob( item, this );
  sjob->addFlag( "added_test_flag_1" );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( DataReference( 1, QString() ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.flags().count(), expectedFlags.count() );
  Item::Flags diff = expectedFlags - item.flags();
  QVERIFY( diff.isEmpty() );

  // set flags
  expectedFlags.insert( "added_test_flag_2" );
  sjob = new ItemStoreJob( item, this );
  sjob->setFlags( expectedFlags );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( DataReference( 1, QString() ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.flags().count(), expectedFlags.count() );
  diff = expectedFlags - item.flags();
  QVERIFY( diff.isEmpty() );

  // remove a flag
  sjob = new ItemStoreJob( item, this );
  sjob->removeFlag( "added_test_flag_1" );
  sjob->removeFlag( "added_test_flag_2" );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( DataReference( 1, QString() ) );
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

  DataReference ref( 1, QString() );
  Item item;
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  item = prefetchjob->items()[0];
  item.setMimeType( "application/octet-stream" );
  item.setPayload( data );

  // delete data
  ItemStoreJob *sjob = new ItemStoreJob( item );
  sjob->storePayload();
  QVERIFY( sjob->exec() );

  ItemFetchJob *fjob = new ItemFetchJob( ref );
  fjob->addFetchPart( Item::PartBody );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QVERIFY( item.hasPayload<QByteArray>() );
  QCOMPARE( item.payload<QByteArray>(), data );
}

void ItemStoreTest::testItemMove()
{
  DataReference ref( 1, QString() );
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];

  ItemStoreJob *store = new ItemStoreJob( item, this );
  store->setCollection( res3 );
  QVERIFY( store->exec() );

  ItemFetchJob *fetch = new ItemFetchJob( res3, this );
  QVERIFY( fetch->exec() );
  QCOMPARE( fetch->items().count(), 1 );

  store = new ItemStoreJob( item, this );
  store->setCollection( res1_foo );
  QVERIFY( store->exec() );
}

void ItemStoreTest::testIllegalItemMove()
{
  DataReference ref( 1, QString() );
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];

  // move into invalid collection
  ItemStoreJob *store = new ItemStoreJob( item, this );
  store->setCollection( Collection( INT_MAX ) );
  QVERIFY( !store->exec() );

  // move item into folder that doesn't support its mimetype
  store = new ItemStoreJob( item, this );
  store->setCollection( res2 );
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

  DataReference ref( 1, rid );

  ItemFetchJob *prefetchjob = new ItemFetchJob( DataReference( 1, QString() ) );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];

  item.setReference( ref );
  ItemStoreJob *store = new ItemStoreJob( item, this );
  QVERIFY( store->exec() );

  ItemFetchJob *fetch = new ItemFetchJob( ref, this );
  QVERIFY( fetch->exec() );
  QCOMPARE( fetch->items().count(), 1 );
  item = fetch->items().at( 0 );
  QCOMPARE( item.reference().remoteId(), exprid );
}

void ItemStoreTest::testMultiPart()
{
  DataReference ref( 1, QString() );
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];
  item.setMimeType( "application/octet-stream" );
  item.addPart( Item::PartBody, "testmailbody" );
  item.addPart( "EXTRA", "extra" );

  // store item
  ItemStoreJob *sjob = new ItemStoreJob( item );
  sjob->storePayload();
  QVERIFY( sjob->exec() );

  ItemFetchJob *fjob = new ItemFetchJob( ref );
  fjob->addFetchPart( "EXTRA" );
  fjob->addFetchPart( Item::PartBody );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.part( Item::PartBody ), QByteArray("testmailbody") );
  QCOMPARE( item.part( "EXTRA" ), QByteArray("extra") );

  // clean up
  sjob = new ItemStoreJob( item );
  sjob->removePart( "EXTRA" );
  QVERIFY( sjob->exec() );
}

void ItemStoreTest::testPartRemove()
{
  DataReference ref( 2, QString() );
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  Item item = prefetchjob->items()[0];
  item.setMimeType( "application/octet-stream" );
  item.addPart( "EXTRA", "extra" );

  // store item
  ItemStoreJob *sjob = new ItemStoreJob( item );
  sjob->storePayload();
  QVERIFY( sjob->exec() );

  // fetch item and its parts (should be RFC822, HEAD and EXTRA)
  ItemFetchJob *fjob = new ItemFetchJob( ref );
  fjob->fetchAllParts();
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item.availableParts().count(), 3 );
  QVERIFY( item.availableParts().contains( QLatin1String( "EXTRA" ) ) );

  // remove a part
  sjob = new ItemStoreJob( item );
  sjob->removePart( "EXTRA" );
  QVERIFY( sjob->exec() );

  // fetch item again (should only have RFC822 and HEAD left)
  ItemFetchJob *fjob2 = new ItemFetchJob( ref );
  fjob2->fetchAllParts();
  QVERIFY( fjob2->exec() );
  QCOMPARE( fjob2->items().count(), 1 );
  item = fjob2->items()[0];
  QCOMPARE( item.availableParts().count(), 2 );
  QVERIFY( !item.availableParts().contains( QLatin1String( "EXTRA" ) ) );
}

void ItemStoreTest::testRevisionCheck()
{
  // make sure we don't have any other collection selected
  // otherwise EXPUNGE doesn't work and will be triggered by
  // the following tests and mess up the monitor testing
  CollectionSelectJob *sel = new CollectionSelectJob( Collection::root(), this );
  QVERIFY( sel->exec() );

  // fetch same item twice
  DataReference ref( 2, QString() );
  ItemFetchJob *prefetchjob = new ItemFetchJob( ref );
  prefetchjob->exec();
  Item item1 = prefetchjob->items()[0];
  Item item2 = prefetchjob->items()[0];

  // store first item unmodified
  ItemStoreJob *sjob = new ItemStoreJob( item1 );
  QVERIFY( sjob->exec() );

  // try to store second item
  ItemStoreJob *sjob2 = new ItemStoreJob( item2 );
  item2.addPart( "EXTRA", "extra" );
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
  sjob = new ItemStoreJob( item1 );
  QVERIFY( !sjob->exec() );
}

#include "itemstoretest.moc"
