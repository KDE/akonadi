  /*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
  ItemStoreJob *sjob = new ItemStoreJob( item.reference() );
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
  sjob = new ItemStoreJob( item.reference() );
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
  sjob = new ItemStoreJob( item.reference() );
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

void ItemStoreTest::testDataChange()
{
  DataReference ref( 1, QString() );

  // delete data
  ItemStoreJob *sjob = new ItemStoreJob( ref );
  sjob->setData( QByteArray() );
  QVERIFY( sjob->exec() );

  ItemFetchJob *fjob = new ItemFetchJob( ref );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  Item item = fjob->items()[0];
  QVERIFY( item.data().isEmpty() );

  // add data
  sjob = new ItemStoreJob( ref );
  sjob->setData( "testmailbody" );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( ref );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QVERIFY( item.data() == "testmailbody" );
}

void ItemStoreTest::testItemMove()
{
  DataReference ref( 1, QString() );

  ItemStoreJob *store = new ItemStoreJob( ref, this );
  store->setCollection( res3 );
  QVERIFY( store->exec() );

  ItemFetchJob *fetch = new ItemFetchJob( res3, this );
  QVERIFY( fetch->exec() );
  QCOMPARE( fetch->items().count(), 1 );

  store = new ItemStoreJob( ref, this );
  store->setCollection( res1_foo );
  QVERIFY( store->exec() );
}

void ItemStoreTest::testIllegalItemMove()
{
  DataReference ref( 1, QString() );

  // move into invalid collection
  ItemStoreJob *store = new ItemStoreJob( ref, this );
  store->setCollection( Collection( INT_MAX ) );
  QVERIFY( !store->exec() );

  // move item into folder that doesn't support its mimetype
  store = new ItemStoreJob( ref, this );
  store->setCollection( res2 );
  QEXPECT_FAIL( "", "Check not yet implemented by the server.", Continue );
  QVERIFY( !store->exec() );
}

#include "itemstoretest.moc"
