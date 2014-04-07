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

#include "test_utils.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsync.h>

#include <krandom.h>

#include <QtCore/QObject>
#include <QSignalSpy>

#include <qtest_akonadi.h>
#include <monitor.h>

using namespace Akonadi;

Q_DECLARE_METATYPE( KJob* )

class ItemsyncTest : public QObject
{
  Q_OBJECT
  private:
    Item::List fetchItems( const Collection &col )
    {
      kDebug() << col.remoteId();
      ItemFetchJob *fetch = new ItemFetchJob( col, this );
      fetch->fetchScope().fetchFullPayload();
      fetch->fetchScope().fetchAllAttributes();
      fetch->fetchScope().setCacheOnly( true ); // resources are switched off anyway
      Q_ASSERT( fetch->exec() );
//       Q_ASSERT( !fetch->items().isEmpty() );
      return fetch->items();
    }

  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      Control::start();
      AkonadiTest::setAllResourcesOffline();
      qRegisterMetaType<KJob*>();
    }

    void testFullSync()
    {
      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );

      //Since the item sync affects the knut resource we ensure we actually managed to load all items
      //This needs to be adjusted should the testdataset change
      QCOMPARE(origItems.size(), 15);

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setFullSyncItems( origItems );
      AKVERIFYEXEC( syncer );

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );
      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QCOMPARE(changedSpy.count(), 0);
    }

    void testFullStreamingSync()
    {
      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );
      QCOMPARE(origItems.size(), 15);

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setAutoDelete( false );
      syncer->setStreamingEnabled(true);
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setTotalItems( origItems.count() );
      QTest::qWait( 0 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origItems.count(); ++i ) {
        Item::List l;
        l << origItems[i];
        syncer->setFullSyncItems( l );
        if ( i < origItems.count() - 1 ) {
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
          QCOMPARE( spy.count(), 0 );
//           QCOMPARE(syncer->percent(), (unsigned long)(i*100/origItems.count()));
        }
      }
      syncer->deliveryDone();
      QTRY_COMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QCOMPARE( job, syncer );
      QCOMPARE( job->error(), 0 );

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      delete syncer;
      QTest::qWait(100);
      QTRY_COMPARE(deletedSpy.count(), 0);
      QTRY_COMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), 0);
    }

    void testIncrementalSync()
    {

      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );
      QCOMPARE(origItems.size(), 15);

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setIncrementalSyncItems( origItems, Item::List() );
      AKVERIFYEXEC( syncer );

      QTest::qWait(100);
      QTRY_COMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), origItems.count());
      deletedSpy.clear();
      addedSpy.clear();
      changedSpy.clear();

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      Item::List delItems;
      delItems << resultItems.takeFirst();

      Item itemWithOnlyRemoteId;
      itemWithOnlyRemoteId.setRemoteId( resultItems.front().remoteId() );
      delItems << itemWithOnlyRemoteId;
      resultItems.takeFirst();

      Item itemWithRandomRemoteId;
      itemWithRandomRemoteId.setRemoteId( KRandom::randomString( 100 ) );
      delItems << itemWithRandomRemoteId;

      syncer = new ItemSync( col );
      syncer->setIncrementalSyncItems( resultItems, delItems );

      Item::List resultItems2 = fetchItems( col );
      QCOMPARE( resultItems2.count(), resultItems.count() );

      QTest::qWait(100);
      QTRY_COMPARE(deletedSpy.count(), 2);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), resultItems.count());
    }

    void testIncrementalStreamingSync()
    {
      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setAutoDelete( false );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      QTest::qWait( 0 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origItems.count(); ++i ) {
        Item::List l;
        l << origItems[i];
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < origItems.count() - 1 )
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
//         kDebug() << syncer->percent();
//         QCOMPARE(syncer->percent(), (unsigned long)(i*100/origItems.count()));
      }
      syncer->deliveryDone();
      QTRY_COMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QCOMPARE( job, syncer );
      QCOMPARE( job->error(), 0 );

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      delete syncer;

      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), origItems.count());
    }

    void testEmptyIncrementalSync()
    {
      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setIncrementalSyncItems( Item::List(), Item::List() );
      AKVERIFYEXEC( syncer );

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QCOMPARE(changedSpy.count(), 0);
    }

    void testIncrementalStreamingSyncBatchProcessing()
    {
      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      syncer->setTransactionMode(ItemSync::MultipleTransactions);
      QTest::qWait( 0 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < ItemSync::batchSize(); ++i ) {
        Item::List l;
        l << origItems[i];
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < 9 )
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
      }
      QTest::qWait(100);
      //this should process one batch of 10 items
      QTRY_COMPARE(changedSpy.count(), ItemSync::batchSize());

      for ( int i = ItemSync::batchSize(); i < origItems.count(); ++i ) {
        Item::List l;
        l << origItems[i];
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < origItems.count() - 1 )
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );
      }

      syncer->deliveryDone();
      QTRY_COMPARE( spy.count(), 1 );
      QTest::qWait(100);

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), resultItems.count());
    }

};

QTEST_AKONADIMAIN( ItemsyncTest, NoGUI )

#include "itemsynctest.moc"
