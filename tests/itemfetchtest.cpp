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

#include "itemfetchtest.h"
#include "itemfetchtest.moc"
#include "collectionpathresolver_p.h"
#include "testattribute.h"

#include <akonadi/attributefactory.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

#include <qtest_akonadi.h>

QTEST_AKONADIMAIN( ItemFetchTest, NoGUI )

void ItemFetchTest::initTestCase()
{
  qRegisterMetaType<Akonadi::Item::List>();
  AttributeFactory::registerAttribute<TestAttribute>();
}

void ItemFetchTest::testFetch()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( "res1", this );
  QVERIFY( resolver->exec() );
  int colId = resolver->collection();

  // listing of an empty folder
  ItemFetchJob *job = new ItemFetchJob( Collection( colId ), this );
  QVERIFY( job->exec() );
  QVERIFY( job->items().isEmpty() );

  resolver = new CollectionPathResolver( "res1/foo", this );
  QVERIFY( resolver->exec() );
  int colId2 = resolver->collection();

  // listing of a non-empty folder
  job = new ItemFetchJob( Collection( colId2 ), this );
  QSignalSpy spy( job, SIGNAL(itemsReceived(Akonadi::Item::List)) );
  QVERIFY( spy.isValid() );
  QVERIFY( job->exec() );
  Item::List items = job->items();
  QCOMPARE( items.count(), 15 );

  int count = 0;
  for ( int i = 0; i < spy.count(); ++i ) {
    Item::List l = spy[i][0].value<Akonadi::Item::List>();
    for ( int j = 0; j < l.count(); ++j ) {
      QVERIFY( items.count() > count + j );
      QCOMPARE( items[count + j], l[j] );
    }
    count += l.count();
  }
  QCOMPARE( count, items.count() );

  // check if the fetch response is parsed correctly
  Item item = items[0];
  QCOMPARE( item.remoteId(), QString( "A" ) );

  QCOMPARE( item.flags().count(), 3 );
  QVERIFY( item.hasFlag( "\\Seen" ) );
  QVERIFY( item.hasFlag( "\\Flagged" ) );
  QVERIFY( item.hasFlag( "\\Draft" ) );

  item = items[1];
  QCOMPARE( item.flags().count(), 1 );
  QVERIFY( item.hasFlag( "\\Flagged" ) );

  item = items[2];
  QVERIFY( item.flags().isEmpty() );
}

void ItemFetchTest::testResourceRetrieval()
{
  Item item( 1 );

  ItemFetchJob *job = new ItemFetchJob( item, this );
  job->fetchScope().fetchFullPayload( true );
  job->fetchScope().fetchAllAttributes( true );
  job->fetchScope().setCacheOnly( true );
  QVERIFY( job->exec() );
  QCOMPARE( job->items().count(), 1 );
  item = job->items().first();
  QCOMPARE( item.id(), 1ll );
  QVERIFY( !item.remoteId().isEmpty() );
  QVERIFY( !item.hasPayload() ); // not yet in cache
  QCOMPARE( item.attributes().count(), 1 );

  job = new ItemFetchJob( item, this );
  job->fetchScope().fetchFullPayload( true );
  job->fetchScope().fetchAllAttributes( true );
  job->fetchScope().setCacheOnly( false );
  QVERIFY( job->exec() );
  QCOMPARE( job->items().count(), 1 );
  item = job->items().first();
  QCOMPARE( item.id(), 1ll );
  QVERIFY( !item.remoteId().isEmpty() );
  QVERIFY( item.hasPayload() );
  QCOMPARE( item.attributes().count(), 1 );
}

void ItemFetchTest::testIllegalFetch()
{
  // fetch non-existing folder
  ItemFetchJob *job = new ItemFetchJob( Collection( INT_MAX ), this );
  QVERIFY( !job->exec() );

  // listing of root
  job = new ItemFetchJob( Collection::root(), this );
  QVERIFY( !job->exec() );

  // fetch a non-existing message
  job = new ItemFetchJob( Item( INT_MAX ), this );
  QVERIFY( job->exec() );
  QVERIFY( job->items().isEmpty() );

  // fetch message with empty reference
  job = new ItemFetchJob( Item(), this );
  QVERIFY( !job->exec() );
}

void ItemFetchTest::testMultipartFetch_data()
{
  QTest::addColumn<bool>( "fetchFullPayload" );
  QTest::addColumn<bool>( "fetchAllAttrs" );
  QTest::addColumn<bool>( "fetchSinglePayload" );
  QTest::addColumn<bool>( "fetchSingleAttr" );

  QTest::newRow( "empty" ) << false << false << false << false;
  QTest::newRow( "full" ) << true << true << false << false;
  QTest::newRow( "full payload" ) << true << false << false << false;
  QTest::newRow( "single payload" ) << false << false << true << false;
  QTest::newRow( "single" ) << false << false << true << true;
  QTest::newRow( "attr full" ) << false << true << false << false;
  QTest::newRow( "attr single" ) << false << false << false << true;
  QTest::newRow( "mixed cross 1" ) << true << false << false << true;
  QTest::newRow( "mixed cross 2" ) << false << true << true << false;
  QTest::newRow( "all" ) << true << true << true << true;
  QTest::newRow( "all payload" ) << true << false << true << false;
  QTest::newRow( "all attr" ) << false << true << true << false;
}

void ItemFetchTest::testMultipartFetch()
{
  QFETCH( bool, fetchFullPayload );
  QFETCH( bool, fetchAllAttrs );
  QFETCH( bool, fetchSinglePayload );
  QFETCH( bool, fetchSingleAttr );

  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo", this );
  QVERIFY( resolver->exec() );
  int colId = resolver->collection();

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "body data" );
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra data";
  ItemCreateJob *job = new ItemCreateJob( item, Collection( colId ), this );
  QVERIFY( job->exec() );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( ref, this );
  if ( fetchFullPayload )
    fjob->fetchScope().fetchFullPayload();
  if ( fetchAllAttrs )
    fjob->fetchScope().fetchAttribute<TestAttribute>();
  if ( fetchSinglePayload )
    fjob->fetchScope().fetchPayloadPart( Item::FullPayload );
  if ( fetchSingleAttr )
    fjob->fetchScope().fetchAttribute<TestAttribute>();

  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();

  if ( fetchFullPayload || fetchSinglePayload ) {
    QCOMPARE( item.loadedPayloadParts().count(), 1 );
    QVERIFY( item.hasPayload() );
    QCOMPARE( item.payload<QByteArray>(), QByteArray( "body data" ) );
  } else {
    QCOMPARE( item.loadedPayloadParts().count(), 0 );
    QVERIFY( !item.hasPayload() );
  }

  if ( fetchAllAttrs || fetchSingleAttr ) {
    QCOMPARE( item.attributes().count(), 1 );
    QVERIFY( item.hasAttribute<TestAttribute>() );
    QCOMPARE( item.attribute<TestAttribute>()->data, QByteArray( "extra data" ) );
  } else {
    QCOMPARE( item.attributes().count(), 0 );
  }

  // cleanup
  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );
}
