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

#include "itemserializertest.h"
#include "testattribute.h"

#include <akonadi/attributefactory.h>
#include <akonadi/item.h>
#include <akonadi/itemserializer.h>

#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( ItemSerializerTest, NoGUI )

void ItemSerializerTest::testEmptyPayload()
{
  // should not crash
  QByteArray data;
  Item item;
  ItemSerializer::deserialize( item, Item::FullPayload, data, 0 );
  QVERIFY( data.isEmpty() );
}

void ItemSerializerTest::testDefaultSerializer()
{
  QByteArray serialized = "\0\r\n\0bla";
  Item item;
  item.setMimeType( "application/octet-stream" );
  ItemSerializer::deserialize( item, Item::FullPayload, serialized, 0 );

  QVERIFY( item.hasPayload<QByteArray>() );
  QCOMPARE( item.payload<QByteArray>(), serialized );

  QByteArray data;
  int version = 0;
  ItemSerializer::serialize( item, Item::FullPayload, data, version );
  QCOMPARE( serialized, data );
}

void ItemSerializerTest::testExtraPart()
{
  AttributeFactory::registerAttribute<TestAttribute>();

  QByteArray data = "foo";
  Item item;
  item.setMimeType( "application/octet-stream" );
  ItemSerializer::deserialize( item, "EXTRA", data, 0 );

  QVERIFY( !item.hasPayload() );
  QVERIFY( item.loadedPayloadParts().isEmpty() );
  QCOMPARE( item.attributes().count(), 1 );
  QVERIFY( item.hasAttribute<TestAttribute>() );
  QCOMPARE( item.attribute<TestAttribute>()->data, data );

  QByteArray ser;
  int version = 0;
  ItemSerializer::serialize( item, "EXTRA", ser, version );
  QCOMPARE( ser, data );
}

#include "itemserializertest.moc"
