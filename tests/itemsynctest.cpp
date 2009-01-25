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
#include <QSignalSpy>

#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE( KJob* )

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
      qRegisterMetaType<KJob*>();
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

    void testFullStreamingSync()
    {
      Item::List origItems = fetchItems();

      ItemSync* syncer = new ItemSync( Collection( 10 ) );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setTotalItems( origItems.count() );
      QTest::qWait( 10 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origItems.count(); ++i ) {
        Item::List l;
        l << origItems[i];
        syncer->setFullSyncItems( l );
        if ( i < origItems.count() - 1 )
          QTest::qWait( 10 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
      }
      QTest::qWait( 1000 ); // let it finish its job
      QCOMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QVERIFY( job );
      QCOMPARE( job->error(), 0 );

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

    void testIncrementalStreamingSync()
    {
      Item::List origItems = fetchItems();

      ItemSync* syncer = new ItemSync( Collection( 10 ) );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      QTest::qWait( 10 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origItems.count(); ++i ) {
        Item::List l;
        l << origItems[i];
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < origItems.count() - 1 )
          QTest::qWait( 10 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
      }
      syncer->deliveryDone();
      QTest::qWait( 1000 ); // let it finish its job
      QCOMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QVERIFY( job );
      QCOMPARE( job->error(), 0 );

      Item::List resultItems = fetchItems();
      QCOMPARE( resultItems.count(), origItems.count() );
    }

    void testEmptyIncrementalSync()
    {
      Item::List origItems = fetchItems();

      ItemSync* syncer = new ItemSync( Collection( 10 ) );
      syncer->setIncrementalSyncItems( Item::List(), Item::List() );
      QVERIFY( syncer->exec() );

      Item::List resultItems = fetchItems();
      QCOMPARE( resultItems.count(), origItems.count() );
    }
};

QTEST_AKONADIMAIN( ItemsyncTest, NoGUI )

#include "itemsynctest.moc"
