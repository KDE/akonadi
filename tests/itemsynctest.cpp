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
#include <akonadi/itemcreatejob.h>

#include <krandom.h>

#include <QtCore/QObject>
#include <QSignalSpy>

#include <qtest_akonadi.h>
#include <monitor.h>

using namespace Akonadi;

Q_DECLARE_METATYPE( KJob* )
Q_DECLARE_METATYPE( ItemSync::TransactionMode )

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
      return fetch->items();
    }

  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      Control::start();
      AkonadiTest::setAllResourcesOffline();
      qRegisterMetaType<KJob*>();
      qRegisterMetaType<ItemSync::TransactionMode>();
    }

    static Item modifyItem(Item item)
    {
      static int counter = 0;
      item.setFlag(QByteArray("\\READ")+ QByteArray::number(counter));
      counter++;
      return item;
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
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setTransactionMode(ItemSync::SingleTransaction);
      QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
      QVERIFY(transactionSpy.isValid());
      syncer->setFullSyncItems( origItems );
      AKVERIFYEXEC( syncer );
      QCOMPARE(transactionSpy.count(), 1);

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );
      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QCOMPARE(changedSpy.count(), 0);
    }

    void testFullStreamingSync_data()
    {
      QTest::addColumn<ItemSync::TransactionMode>("transactionMode");
      QTest::addColumn<bool>("goToEventLoopAfterAddingItems");

      QTest::newRow("single transaction, no eventloop") << ItemSync::SingleTransaction << false;
      QTest::newRow("multi transaction, no eventloop") << ItemSync::MultipleTransactions << false;
      QTest::newRow("single transaction, with eventloop") << ItemSync::SingleTransaction << true;
      QTest::newRow("multi transaction, with eventloop") << ItemSync::MultipleTransactions << true;
    }

    void testFullStreamingSync()
    {
      QFETCH(ItemSync::TransactionMode, transactionMode);
      QFETCH(bool, goToEventLoopAfterAddingItems);

      const Collection col = Collection( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( col.isValid() );
      Item::List origItems = fetchItems( col );
      QCOMPARE(origItems.size(), 15);

      Akonadi::Monitor monitor;
      monitor.setCollectionMonitored(col);
      QSignalSpy deletedSpy(&monitor, SIGNAL(itemRemoved(Akonadi::Item)));
      QVERIFY(deletedSpy.isValid());
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
      QVERIFY(transactionSpy.isValid());
      syncer->setTransactionMode(transactionMode);
      syncer->setBatchSize(1);
      syncer->setAutoDelete( false );
      syncer->setStreamingEnabled(true);
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setTotalItems( origItems.count() );
      QTest::qWait( 0 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origItems.count(); ++i ) {
        Item::List l;
        //Modify to trigger a changed signal
        l << modifyItem(origItems[i]);
        syncer->setFullSyncItems( l );
        if (goToEventLoopAfterAddingItems) {
          QTest::qWait(0);
        }
        if ( i < origItems.count() - 1 ) {
          QCOMPARE( spy.count(), 0 );
        }
      }
      syncer->deliveryDone();
      QTRY_COMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QCOMPARE( job, syncer );
      QCOMPARE( job->error(), 0 );
      if (transactionMode == ItemSync::SingleTransaction) {
        QCOMPARE(transactionSpy.count(), 1);
      }
      if (transactionMode == ItemSync::MultipleTransactions) {
        QCOMPARE(transactionSpy.count(), origItems.count());
      }

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      delete syncer;
      QTest::qWait(100);
      QTRY_COMPARE(deletedSpy.count(), 0);
      QTRY_COMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), origItems.count());
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
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      {
        ItemSync* syncer = new ItemSync( col );
        QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
        QVERIFY(transactionSpy.isValid());
        syncer->setTransactionMode(ItemSync::SingleTransaction);
        syncer->setIncrementalSyncItems( origItems, Item::List() );
        AKVERIFYEXEC( syncer );
        QCOMPARE(transactionSpy.count(), 1);
      }

      QTest::qWait(100);
      QTRY_COMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), 0);
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

      {
        ItemSync *syncer = new ItemSync( col );
        syncer->setTransactionMode(ItemSync::SingleTransaction);
        QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
        QVERIFY(transactionSpy.isValid());
        syncer->setIncrementalSyncItems( resultItems, delItems );
        AKVERIFYEXEC(syncer);
        QCOMPARE(transactionSpy.count(), 1);
      }

      Item::List resultItems2 = fetchItems( col );
      QCOMPARE( resultItems2.count(), resultItems.count() );

      QTest::qWait(100);
      QTRY_COMPARE(deletedSpy.count(), 2);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), 0);
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
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setTransactionMode(ItemSync::SingleTransaction);
      QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
      QVERIFY(transactionSpy.isValid());
      syncer->setAutoDelete( false );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      QTest::qWait( 0 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origItems.count(); ++i ) {
        Item::List l;
        //Modify to trigger a changed signal
        l << modifyItem(origItems[i]);
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < origItems.count() - 1 ) {
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
        }
        QCOMPARE( spy.count(), 0 );
      }
      syncer->deliveryDone();
      QTRY_COMPARE( spy.count(), 1 );
      KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
      QCOMPARE( job, syncer );
      QCOMPARE( job->error(), 0 );
      QCOMPARE(transactionSpy.count(), 1);

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      delete syncer;

      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), origItems.size());
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
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      syncer->setTransactionMode(ItemSync::SingleTransaction);
      QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
      QVERIFY(transactionSpy.isValid());
      syncer->setIncrementalSyncItems( Item::List(), Item::List() );
      AKVERIFYEXEC( syncer );
      //It would be better if we didn't have a transaction at all, but so far the transaction is still created
      QCOMPARE(transactionSpy.count(), 1);

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
      QSignalSpy addedSpy(&monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)));
      QVERIFY(addedSpy.isValid());
      QSignalSpy changedSpy(&monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
      QVERIFY(changedSpy.isValid());

      ItemSync* syncer = new ItemSync( col );
      QSignalSpy transactionSpy(syncer, SIGNAL(transactionCommitted()));
      QVERIFY(transactionSpy.isValid());
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      syncer->setStreamingEnabled( true );
      syncer->setTransactionMode(ItemSync::MultipleTransactions);
      QTest::qWait( 0 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < syncer->batchSize(); ++i ) {
        Item::List l;
        //Modify to trigger a changed signal
        l << modifyItem(origItems[i]);
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < (syncer->batchSize() - 1) ) {
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
        }
        QCOMPARE( spy.count(), 0 );
      }
      QTest::qWait(100);
      //this should process one batch of batchSize() items
      QTRY_COMPARE(changedSpy.count(), syncer->batchSize());
      QCOMPARE(transactionSpy.count(), 1); //one per batch

      for ( int i = syncer->batchSize(); i < origItems.count(); ++i ) {
        Item::List l;
        //Modify to trigger a changed signal
        l << modifyItem(origItems[i]);
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < origItems.count() - 1 ) {
          QTest::qWait( 0 ); // enter the event loop so itemsync actually can do something
        }
        QCOMPARE( spy.count(), 0 );
      }

      syncer->deliveryDone();
      QTRY_COMPARE( spy.count(), 1 );
      QCOMPARE(transactionSpy.count(), 2); //one per batch
      QTest::qWait(100);

      Item::List resultItems = fetchItems( col );
      QCOMPARE( resultItems.count(), origItems.count() );

      QTest::qWait(100);
      QCOMPARE(deletedSpy.count(), 0);
      QCOMPARE(addedSpy.count(), 0);
      QTRY_COMPARE(changedSpy.count(), resultItems.count());
    }

    void testGidMerge()
    {
       Collection col(collectionIdFromPath("res3"));
       {
          Item item("application/octet-stream");
          item.setRemoteId("rid1");
          item.setGid("gid1");
          item.setPayload<QByteArray>("payload1");
          ItemCreateJob *job = new ItemCreateJob(item, col);
          AKVERIFYEXEC(job);
       }
       {
          Item item("application/octet-stream");
          item.setRemoteId("rid2");
          item.setGid("gid2");
          item.setPayload<QByteArray>("payload1");
          ItemCreateJob *job = new ItemCreateJob(item, col);
          AKVERIFYEXEC(job);
       }
       Item modifiedItem("application/octet-stream");
       modifiedItem.setRemoteId("rid3");
       modifiedItem.setGid("gid2");
       modifiedItem.setPayload<QByteArray>("payload2");

       ItemSync* syncer = new ItemSync(col);
       syncer->setTransactionMode(ItemSync::MultipleTransactions);
       syncer->setIncrementalSyncItems(Item::List() << modifiedItem, Item::List());
       AKVERIFYEXEC(syncer);

       Item::List resultItems = fetchItems(col);
       QCOMPARE(resultItems.count(), 2);

       ItemFetchJob *fetchJob = new ItemFetchJob(modifiedItem);
       fetchJob->fetchScope().fetchFullPayload();
       AKVERIFYEXEC(fetchJob);
       QCOMPARE(fetchJob->items().size(), 1);
       QCOMPARE(fetchJob->items().first().payload<QByteArray>(), QByteArray("payload2"));
       QCOMPARE(fetchJob->items().first().remoteId(), QString::fromLatin1("rid3"));
    }

};

QTEST_AKONADIMAIN( ItemsyncTest, NoGUI )

#include "itemsynctest.moc"
