/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsync.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class ItemsyncTest : public QObject
{
  Q_OBJECT
  private:
    Item::List fetchItems()
    {
      ItemFetchJob *fetch = new ItemFetchJob( Collection( 10 ) );
      fetch->fetchScope().fetchFullPayload();
      fetch->fetchScope().fetchAllAttributes();
      Q_ASSERT( fetch->exec() );
      Q_ASSERT( !fetch->items().isEmpty() );
      return fetch->items();
    }

  private slots:
    void initTestCase()
    {
      Control::start();
    }

    void testFullSync()
    {
      Item::List origItems = fetchItems();

      ItemSync* syncer = new ItemSync( Collection( 10 ) );
      syncer->setFullSyncItems( origItems );
      QVERIFY( syncer->exec() );

      Item::List resultItems = fetchItems();
      QCOMPARE( resultItems.count(), origItems.count() );
    }

    void testPartialSync()
    {
      Item::List origItems = fetchItems();

      ItemSync* syncer = new ItemSync( Collection( 10 ) );
      syncer->setTotalItems( origItems.count() );
      foreach ( const Item &item, origItems ) {
        Item::List l;
        l << item;
        syncer->setPartSyncItems( l );
      }
      QVERIFY( syncer->exec() );

      Item::List resultItems = fetchItems();
      QCOMPARE( resultItems.count(), origItems.count() );
    }

    void testIncrementalSync()
    {
      Item::List origItems = fetchItems();

      ItemSync* syncer = new ItemSync( Collection( 10 ) );
      syncer->setIncrementalSyncItems( origItems, Item::List() );
      QVERIFY( syncer->exec() );

      Item::List resultItems = fetchItems();
      QCOMPARE( resultItems.count(), origItems.count() );
    }
};

QTEST_KDEMAIN( ItemsyncTest, NoGUI )

#include "itemsynctest.moc"
