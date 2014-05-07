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

#include "itemappendtest.h"

#include "control.h"
#include "testattribute.h"
#include "test_utils.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/attributefactory.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>

#include <QtCore/QDebug>

using namespace Akonadi;

QTEST_AKONADIMAIN( ItemAppendTest, NoGUI )

void ItemAppendTest::initTestCase()
{
  AkonadiTest::checkTestIsIsolated();
  Control::start();
  AkonadiTest::setAllResourcesOffline();
  AttributeFactory::registerAttribute<TestAttribute>();
}

void ItemAppendTest::testItemAppend_data()
{
  QTest::addColumn<QString>( "remoteId" );

  QTest::newRow( "empty" ) << QString();
  QTest::newRow( "non empty" ) << QString( "remote-id" );
  QTest::newRow( "whitespace" ) << QString( "remote id" );
  QTest::newRow( "quotes" ) << QString ( "\"remote\" id" );
  QTest::newRow( "brackets" ) << QString( "[remote id]" );
}

void ItemAppendTest::testItemAppend()
{
  const Collection testFolder1( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( testFolder1.isValid() );

  QFETCH( QString, remoteId );
  Item ref; // for cleanup

  Item item( -1 );
  item.setRemoteId( remoteId );
  item.setMimeType( "application/octet-stream" );
  item.setFlag( "TestFlag" );
  item.setSize( 3456 );
  ItemCreateJob *job = new ItemCreateJob( item, testFolder1, this );
  AKVERIFYEXEC( job );
  ref = job->item();
  QCOMPARE( ref.parentCollection(), testFolder1 );

  ItemFetchJob *fjob = new ItemFetchJob( testFolder1, this );
  fjob->fetchScope().setAncestorRetrieval( ItemFetchScope::Parent );
  AKVERIFYEXEC( fjob );
  QCOMPARE( fjob->items().count(), 1 );
  QCOMPARE( fjob->items()[0], ref );
  QCOMPARE( fjob->items()[0].remoteId(), remoteId );
  QVERIFY( fjob->items()[0].flags().contains( "TestFlag" ) );
  QCOMPARE( fjob->items()[0].parentCollection(), ref.parentCollection() );

  qint64 size = 3456;
  QCOMPARE( fjob->items()[0].size(), size );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  AKVERIFYEXEC( djob );

  fjob = new ItemFetchJob( testFolder1, this );
  AKVERIFYEXEC( fjob );
  QVERIFY( fjob->items().isEmpty() );
}

void ItemAppendTest::testContent_data()
{
  QTest::addColumn<QByteArray>( "data" );

  QTest::newRow( "null" ) << QByteArray();
  QTest::newRow( "empty" ) << QByteArray( "" );
  QTest::newRow( "nullbyte" ) << QByteArray( "\0", 1 );
  QTest::newRow( "nullbyte2" ) << QByteArray( "\0X", 2 );
  QString utf8string = QString::fromUtf8("äöüß@€µøđ¢©®");
  QTest::newRow( "utf8" ) << utf8string.toUtf8();
  QTest::newRow( "newlines" ) << QByteArray("\nsome\n\nbreaked\ncontent\n\n");
  QByteArray b;
  QTest::newRow( "big" ) << b.fill( 'a', 1 << 20 );
  QTest::newRow( "bignull" ) << b.fill( '\0', 1 << 20 );
  QTest::newRow( "bigcr" ) << b.fill( '\r', 1 << 20 );
  QTest::newRow( "biglf" ) << b.fill( '\n', 1 << 20 );
}

void ItemAppendTest::testContent()
{
  const Collection testFolder1( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( testFolder1.isValid() );

  QFETCH( QByteArray, data );

  Item item;
  item.setMimeType( "application/octet-stream" );
  if ( !data.isNull() ) {
    item.setPayload( data );
  }

  ItemCreateJob* job = new ItemCreateJob( item, testFolder1, this );
  AKVERIFYEXEC( job );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( testFolder1, this );
  fjob->fetchScope().setCacheOnly( true );
  fjob->fetchScope().fetchFullPayload();
  AKVERIFYEXEC( fjob );
  QCOMPARE( fjob->items().count(), 1 );
  Item item2 = fjob->items().first();
  QCOMPARE( item2.hasPayload(), !data.isNull() );
  if( item2.hasPayload() ) {
    QCOMPARE( item2.payload<QByteArray>(), data );
  }

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  AKVERIFYEXEC( djob );
}

void ItemAppendTest::testNewMimetype()
{
  const Collection col( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( col.isValid() );

  Item item;
  item.setMimeType( "application/new-type" );
  ItemCreateJob *job = new ItemCreateJob( item, col, this );
  AKVERIFYEXEC( job );

  item = job->item();
  QVERIFY( item.isValid() );

  ItemFetchJob *fetch = new ItemFetchJob( item, this );
  AKVERIFYEXEC( fetch );
  QCOMPARE( fetch->items().count(), 1 );
  QCOMPARE( fetch->items().first().mimeType(), item.mimeType() );
}

void ItemAppendTest::testIllegalAppend()
{
  const Collection testFolder1( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( testFolder1.isValid() );

  Item item;
  item.setMimeType( "application/octet-stream" );

  // adding item to non-existing collection
  ItemCreateJob *job = new ItemCreateJob( item, Collection( INT_MAX ), this );
  QVERIFY( !job->exec() );

  // adding item into a collection which can't handle items of this type
  const Collection col( collectionIdFromPath( "res1/foo/bla" ) );
  QVERIFY( col.isValid() );
  job = new ItemCreateJob( item, col, this );
  QEXPECT_FAIL( "", "Test not yet implemented in the server.", Continue );
  QVERIFY( !job->exec() );
}

void ItemAppendTest::testMultipartAppend()
{
  const Collection testFolder1( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( testFolder1.isValid() );

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "body data" );
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra data";
  item.setFlag( "TestFlag" );
  ItemCreateJob *job = new ItemCreateJob( item, testFolder1, this );
  AKVERIFYEXEC( job );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( ref, this );
  fjob->fetchScope().fetchFullPayload();
  fjob->fetchScope().fetchAttribute<TestAttribute>();
  AKVERIFYEXEC( fjob );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();
  QCOMPARE( item.payload<QByteArray>(), QByteArray( "body data" ) );
  QVERIFY( item.hasAttribute<TestAttribute>() );
  QCOMPARE( item.attribute<TestAttribute>()->data, QByteArray( "extra data" ) );
  QVERIFY( item.flags().contains( "TestFlag" ) );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  AKVERIFYEXEC( djob );
}

void ItemAppendTest::testInvalidMultipartAppend()
{
  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "body data" );
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra data";
  item.setFlag( "TestFlag" );
  ItemCreateJob *job = new ItemCreateJob( item, Collection( -1 ), this );
  QVERIFY( !job->exec() );

  Item item2;
  item2.setMimeType( "application/octet-stream" );
  item2.setPayload<QByteArray>( "more body data" );
  item2.attribute<TestAttribute>( Item::AddIfMissing )->data = "even more extra data";
  item2.setFlag( "TestFlag" );
  ItemCreateJob *job2 = new ItemCreateJob( item2, Collection( -1 ), this );
  QVERIFY( !job2->exec() );
}

void ItemAppendTest::testItemSize_data()
{
  QTest::addColumn<Akonadi::Item>( "item" );
  QTest::addColumn<qint64>( "size" );

  Item i( "application/octet-stream" );
  i.setPayload( QByteArray( "ABCD" ) );

  QTest::newRow( "auto size" ) << i << 4ll;
  i.setSize( 3 );
  QTest::newRow( "too small" ) << i << 4ll;
  i.setSize( 10 );
  QTest::newRow( "too large" ) << i << 10ll;
}

void ItemAppendTest::testItemSize()
{
  QFETCH( Akonadi::Item, item );
  QFETCH( qint64, size );

  const Collection col( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( col.isValid() );

  ItemCreateJob *create = new ItemCreateJob( item, col, this );
  AKVERIFYEXEC( create );
  Item newItem = create->item();

  ItemFetchJob *fetch = new ItemFetchJob( newItem, this );
  AKVERIFYEXEC( fetch );
  QCOMPARE( fetch->items().count(), 1 );

  QCOMPARE( fetch->items().first().size(), size );
}

void ItemAppendTest::testItemMerge_data()
{
  QTest::addColumn<Akonadi::Item>( "item1" );
  QTest::addColumn<Akonadi::Item>( "item2" );
  QTest::addColumn<Akonadi::Item>( "mergedItem" );
  QTest::addColumn<bool>( "silent" );

  {
    Item i1( "application/octet-stream" );
    i1.setPayload( QByteArray( "ABCD" ) );
    i1.setSize( 4 );
    i1.setRemoteId( "XYZ" );
    i1.setGid( "XYZ" );
    i1.setFlag( "TestFlag1" );
    i1.setRemoteRevision( "5" );

    Item i2( "application/octet-stream" );
    i2.setPayload( QByteArray( "DEFGH" ) );
    i2.setSize( 5 );
    i2.setRemoteId(( "XYZ" ) );
    i2.setGid( "XYZ" );
    i2.setFlag( "TestFlag2" );
    i2.setRemoteRevision( "6" );

    Item mergedItem( i2 );
    mergedItem.setFlag( "TestFlag1" );

    QTest::newRow( "merge" ) << i1 << i2 << mergedItem << false;
    QTest::newRow( "merge (silent)" ) << i1 << i2 << mergedItem << true;
  }
  {
    Item i1( "application/octet-stream" );
    i1.setPayload( QByteArray( "ABCD" ) );
    i1.setSize( 4 );
    i1.setRemoteId( "RID2" );
    i1.setGid( "GID2" );
    i1.setFlag( "TestFlag1" );
    i1.setRemoteRevision( "5" );

    Item i2( "application/octet-stream" );
    i2.setRemoteId(( "RID2" ) );
    i2.setGid( "GID2" );
    i2.setFlags( Item::Flags() << "TestFlag2" );
    i2.setRemoteRevision( "6" );

    Item mergedItem( i1 );
    mergedItem.setFlags( i2.flags() );
    mergedItem.setRemoteRevision( i2.remoteRevision() );

    QTest::newRow( "overwrite flags, and don't remove existing payload" ) << i1 << i2 << mergedItem << false;
    QTest::newRow( "overwrite flags, and don't remove existing payload (silent)" ) << i1 << i2 << mergedItem << true;
  }
}

void ItemAppendTest::testItemMerge()
{
  QFETCH( Akonadi::Item, item1 );
  QFETCH( Akonadi::Item, item2 );
  QFETCH( Akonadi::Item, mergedItem );
  QFETCH( bool, silent );

  const Collection col( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( col.isValid() );

  ItemCreateJob *create = new ItemCreateJob( item1, col, this );
  AKVERIFYEXEC( create );
  const Item createdItem = create->item();

  ItemCreateJob *merge = new ItemCreateJob( item2, col, this );
  ItemCreateJob::MergeOptions options = ItemCreateJob::GID | ItemCreateJob::RID;
  if ( silent ) {
    options |= ItemCreateJob::Silent;
  }
  merge->setMerge( options );
  AKVERIFYEXEC( merge );

  QCOMPARE( merge->item().id(), createdItem.id() );
  if ( !silent ) {
    QCOMPARE( merge->item().gid(), mergedItem.gid() );
    QCOMPARE( merge->item().remoteId(), mergedItem.remoteId() );
    QCOMPARE( merge->item().remoteRevision(), mergedItem.remoteRevision() );
    QCOMPARE( merge->item().payloadData(), mergedItem.payloadData() );
    QCOMPARE( merge->item().size(), mergedItem.size() );
    QCOMPARE( merge->item().flags(), mergedItem.flags() );
  }

  if ( merge->item().id() != createdItem.id() ) {
    ItemDeleteJob *del = new ItemDeleteJob( merge->item(), this );
    AKVERIFYEXEC( del );
  }
  ItemDeleteJob *del = new ItemDeleteJob( createdItem, this );
  AKVERIFYEXEC( del );
}

