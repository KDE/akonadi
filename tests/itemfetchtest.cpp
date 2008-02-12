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

#include <libakonadi/collectionpathresolver.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>

using namespace Akonadi;

#include <qtest_kde.h>

QTEST_KDEMAIN( ItemFetchTest, NoGUI )

void ItemFetchTest::initTestCase()
{
  qRegisterMetaType<Akonadi::Item::List>();
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
      QCOMPARE( items[count + j].reference(), l[j].reference() );
    }
    count += l.count();
  }
  QCOMPARE( count, items.count() );

  // check if the fetch response is parsed correctly
  Item item = items[0];
  QCOMPARE( item.reference().remoteId(), QString( "A" ) );

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

void ItemFetchTest::testIllegalFetch()
{
  // fetch non-existing folder
  ItemFetchJob *job = new ItemFetchJob( Collection( INT_MAX ), this );
  QVERIFY( !job->exec() );

  // listing of root
  job = new ItemFetchJob( Collection::root(), this );
  QVERIFY( !job->exec() );

  // fetch a non-existing message
  DataReference ref( INT_MAX, QString() );
  job = new ItemFetchJob( ref, this );
  QVERIFY( job->exec() );
  QVERIFY( job->items().isEmpty() );

  // fetch message with empty reference
  ref = DataReference();
  job = new ItemFetchJob( ref, this );
  QVERIFY( !job->exec() );
}

void ItemFetchTest::testMultipartFetch()
{
  DataReference::List refs; // for cleanup
  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo", this );
  QVERIFY( resolver->exec() );
  int colId = resolver->collection();

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.addPart( Item::PartBody, "body data" );
  item.addPart( "EXTRA", "extra data" );
  ItemAppendJob *job = new ItemAppendJob( item, Collection( colId ), this );
  QVERIFY( job->exec() );
  refs << job->reference();

  // fetch all parts manually
  ItemFetchJob *fjob = new ItemFetchJob( refs.first(), this );
  fjob->addFetchPart( Item::PartBody );
  fjob->addFetchPart( "EXTRA" );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();
  QCOMPARE( item.availableParts().count(), 2 );
  QCOMPARE( item.part( Item::PartBody ), QByteArray( "body data" ) );
  QCOMPARE( item.part( "EXTRA" ), QByteArray( "extra data" ) );

  // fetch single part
  fjob = new ItemFetchJob( refs.first(), this );
  fjob->addFetchPart( Item::PartBody );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();
  QCOMPARE( item.availableParts().count(), 1 );
  QCOMPARE( item.part( Item::PartBody ), QByteArray( "body data" ) );
  QCOMPARE( item.part( "EXTRA" ), QByteArray() );

  // fetch all parts automatically
  fjob = new ItemFetchJob( refs.first(), this );
  fjob->fetchAllParts();
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();
  QCOMPARE( item.availableParts().count(), 2 );
  QCOMPARE( item.part( Item::PartBody ), QByteArray( "body data" ) );
  QCOMPARE( item.part( "EXTRA" ), QByteArray( "extra data" ) );

  // cleanup
  foreach ( DataReference ref, refs ) {
    ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
    QVERIFY( djob->exec() );
  }

}

void ItemFetchTest::testVirtualFetch()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( "Search/kde-core-devel", this );
  QVERIFY( resolver->exec() );
  Collection col = Collection( resolver->collection() );

  ItemFetchJob *job = new ItemFetchJob( col, this );
  QVERIFY( job->exec() );
  QCOMPARE( job->items().count(), 3 );
}
