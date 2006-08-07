/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "itemstoretest.h"
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <qtest_kde.h>

using namespace PIM;

QTEST_KDEMAIN( ItemStoreTest, NoGUI )

void ItemStoreTest::testFlagChange()
{
  ItemFetchJob *fjob = new ItemFetchJob( DataReference( "0", QString() ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  Item *item = fjob->items()[0];

  // add a flag
  item->setFlag( "added_test_flag" );
  Item::Flags expectedFlags = item->flags();
  ItemStoreJob *sjob = new ItemStoreJob( item );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( DataReference( "0", QString() ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item->flags().count(), expectedFlags.count() );
  Item::Flags diff = expectedFlags - item->flags();
  QVERIFY( diff.isEmpty() );

  // remove a flag
  item->unsetFlag( "added_test_flag" );
  expectedFlags = item->flags();
  sjob = new ItemStoreJob( item );
  QVERIFY( sjob->exec() );

  fjob = new ItemFetchJob( DataReference( "0", QString() ) );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items()[0];
  QCOMPARE( item->flags().count(), expectedFlags.count() );
  diff = expectedFlags - item->flags();
  QVERIFY( diff.isEmpty() );
}

#include "itemstoretest.moc"
