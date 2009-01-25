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
#include "itemappendtest.h"
#include "testattribute.h"
#include "test_utils.h"

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
  Control::start();
}

void ItemAppendTest::testItemAppend_data()
{
  QTest::addColumn<QString>( "remoteId" );

  QTest::newRow( "empty" ) << QString();
  QTest::newRow( "non empty" ) << QString( "remote-id" );
  QTest::newRow( "whitespace" ) << QString( "remote id" );
  QTest::newRow( "quotes" ) << QString ( "\"remote\" id" );
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
  ItemCreateJob *job = new ItemCreateJob( item, Collection( testFolder1 ), this );
  QVERIFY( job->exec() );
  ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( testFolder1, this );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  QCOMPARE( fjob->items()[0], ref );
  QCOMPARE( fjob->items()[0].remoteId(), remoteId );
  QVERIFY( fjob->items()[0].flags().contains( "TestFlag" ) );

  qint64 size = 3456;
  QCOMPARE( fjob->items()[0].size(), size );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );

  fjob = new ItemFetchJob( testFolder1, this );
  QVERIFY( fjob->exec() );
  QVERIFY( fjob->items().isEmpty() );
}

void ItemAppendTest::testContent_data()
{
  QTest::addColumn<QByteArray>( "data" );

  QTest::newRow( "empty" ) << QByteArray();
  QString utf8string = QString::fromUtf8("äöüß@€µøđ¢©®");
  QTest::newRow( "utf8" ) << utf8string.toUtf8();
  QTest::newRow( "newlines" ) << QByteArray("\nsome\n\nbreaked\ncontent\n\n");
}

void ItemAppendTest::testContent()
{
  const Collection testFolder1( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( testFolder1.isValid() );

  QFETCH( QByteArray, data );

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload( data );

  ItemCreateJob* job = new ItemCreateJob( item, testFolder1, this );
  QVERIFY( job->exec() );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( testFolder1, this );
  fjob->fetchScope().fetchFullPayload();
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  Item item2 = fjob->items().first();
  QCOMPARE( data, item2.payload<QByteArray>() );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );
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

  // adding item with non-existing mimetype
  Item item2;
  item2.setMimeType( "wrong/type" );
  job = new ItemCreateJob( item2, testFolder1, this );
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
  AttributeFactory::registerAttribute<TestAttribute>();

  const Collection testFolder1( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( testFolder1.isValid() );

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload<QByteArray>( "body data" );
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = "extra data";
  item.setFlag( "TestFlag" );
  ItemCreateJob *job = new ItemCreateJob( item, testFolder1, this );
  QVERIFY( job->exec() );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( ref, this );
  fjob->fetchScope().fetchFullPayload();
  fjob->fetchScope().fetchAttribute<TestAttribute>();
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();
  QCOMPARE( item.payload<QByteArray>(), QByteArray( "body data" ) );
  QVERIFY( item.hasAttribute<TestAttribute>() );
  QCOMPARE( item.attribute<TestAttribute>()->data, QByteArray( "extra data" ) );
  QVERIFY( item.flags().contains( "TestFlag" ) );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );
}

#include "itemappendtest.moc"
