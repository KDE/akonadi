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

#include <akonadi/attributefactory.h>
#include <akonadi/item.h>
#include <akonadi/itemserializer_p.h>

#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( ItemSerializerTest, NoGUI )

void ItemSerializerTest::testEmptyPayload()
{
  // should not crash
  QByteArray data;
  Item item;
  ItemSerializer::deserialize( item, Item::FullPayload, data, 0, false );
  QVERIFY( data.isEmpty() );
}

void ItemSerializerTest::testDefaultSerializer_data()
{
  QTest::addColumn<QByteArray>( "serialized" );

  QTest::newRow( "null" ) << QByteArray();
  QTest::newRow( "empty" ) << QByteArray( "" );
  QTest::newRow( "nullbytei" ) << QByteArray( "\0", 1 );
  QTest::newRow( "mixed" ) << QByteArray( "\0\r\n\0bla", 7 );
}

void ItemSerializerTest::testDefaultSerializer()
{
  QFETCH( QByteArray, serialized );
  Item item;
  item.setMimeType( "application/octet-stream" );
  ItemSerializer::deserialize( item, Item::FullPayload, serialized, 0, false );

  QVERIFY( item.hasPayload<QByteArray>() );
  QCOMPARE( item.payload<QByteArray>(), serialized );

  QByteArray data;
  int version = 0;
  ItemSerializer::serialize( item, Item::FullPayload, data, version );
  QCOMPARE( data, serialized );
  QEXPECT_FAIL( "null", "Serializer cannot distinguish null vs. empty", Continue );
  QCOMPARE( data.isNull(), serialized.isNull() );
}

