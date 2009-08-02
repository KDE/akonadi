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

#include "itemtest.h"
#include "itemtest.moc"
#include "testattribute.h"

#include <akonadi/item.h>

#include <qtest_kde.h>
#include <collection.h>

QTEST_KDEMAIN( ItemTest, NoGUI )

using namespace Akonadi;

void ItemTest::testMultipart()
{
  Item item;
  item.setMimeType( "application/octet-stream" );

  QSet<QByteArray> parts;
  QCOMPARE( item.loadedPayloadParts(), parts );

  QByteArray bodyData = "bodydata";
  item.setPayload<QByteArray>( bodyData );
  parts << Item::FullPayload;
  QCOMPARE( item.loadedPayloadParts(), parts );
  QCOMPARE( item.payload<QByteArray>(), bodyData );

  QByteArray myData = "mypartdata";
  item.attribute<TestAttribute>( Item::AddIfMissing )->data = myData;

  QCOMPARE( item.loadedPayloadParts(), parts );
  QCOMPARE( item.attributes().count(), 1 );
  QVERIFY( item.hasAttribute<TestAttribute>() );
  QCOMPARE( item.attribute<TestAttribute>()->data, myData );
}

void ItemTest::testInheritance()
{
  Item a;

  a.setRemoteId( "Hello World" );

  Item b( a );
  b.setFlag( "\\send" );
}

void ItemTest::testParentCollection ()
{
  Item a;
  QVERIFY( !a.parentCollection().isValid() );

  a.setParentCollection( Collection::root() );
  QCOMPARE( a.parentCollection(), Collection::root() );
  Item b = a;
  QCOMPARE( b.parentCollection(), Collection::root() );

  Item c;
  c.parentCollection().setRemoteId( "foo" );
  QCOMPARE( c.parentCollection().remoteId(), QString( "foo" ) );
  const Item d = c;
  QCOMPARE( d.parentCollection().remoteId(), QString( "foo" ) );

  const Item e;
  QVERIFY( !e.parentCollection().isValid() );

  Collection col( 5 );
  Item f;
  f.setParentCollection( col );
  QCOMPARE( f.parentCollection(), col );
  Item g = f;
  QCOMPARE( g.parentCollection(), col );
  b = g;
  QCOMPARE( b.parentCollection(), col );
}
