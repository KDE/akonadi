/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "test_utils.h"

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmovejob.h>
#include <akonadi/itemfetchscope.h>

#include <QtCore/QObject>

#include <qtest_akonadi.h>

using namespace Akonadi;

class ItemMoveTest: public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
    }

    // TODO: test inter and intra resource moves, check content on cold caches
    void testMove()
    {
      Collection col( collectionIdFromPath( "res3" ) );
      QVERIFY( col.isValid() );

      ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
      prefetchjob->exec();
      Item item = prefetchjob->items()[0];

      ItemMoveJob *store = new ItemMoveJob( item, col, this );
      QVERIFY( store->exec() );

      ItemFetchJob *fetch = new ItemFetchJob( col, this );
      fetch->fetchScope().fetchFullPayload();
      QVERIFY( fetch->exec() );
      QCOMPARE( fetch->items().count(), 1 );
      item = fetch->items().first();
      QVERIFY( item.hasPayload() );
    }

    void testIllegalMove()
    {
      Collection col( collectionIdFromPath( "res2" ) );
      QVERIFY( col.isValid() );

      ItemFetchJob *prefetchjob = new ItemFetchJob( Item( 1 ) );
      QVERIFY( prefetchjob->exec() );
      QCOMPARE( prefetchjob->items().count(), 1 );
      Item item = prefetchjob->items()[0];

      // move into invalid collection
      ItemMoveJob *store = new ItemMoveJob( item, Collection( INT_MAX ), this );
      QVERIFY( !store->exec() );

      // move item into folder that doesn't support its mimetype
      store = new ItemMoveJob( item, col, this );
      QEXPECT_FAIL( "", "Check not yet implemented by the server.", Continue );
      QVERIFY( !store->exec() );
    }
};

QTEST_AKONADIMAIN( ItemMoveTest, NoGUI )

#include "itemmovetest.moc"
