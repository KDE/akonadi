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

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>

#include "../akonadi/collectionsync.cpp"

#include <krandom.h>

#include <QtCore/QObject>
#include <QSignalSpy>

#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE( KJob* )

class CollectionSyncTest : public QObject
{
  Q_OBJECT
  private:
    Collection::List fetchCollections( const QString &res )
    {
      CollectionFetchJob *fetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
      fetch->setResource( res );
      Q_ASSERT( fetch->exec() );
      Q_ASSERT( !fetch->collections().isEmpty() );
      return fetch->collections();
    }

  private slots:
    void initTestCase()
    {
      Control::start();
      qRegisterMetaType<KJob*>();

      // switch all resources offline to reduce interference from them
      foreach ( Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances() )
        agent.setIsOnline( false );
    }

    void testFullSync()
    {
      Collection::List origCols = fetchCollections( "akonadi_knut_resource_0" );

      CollectionSync* syncer = new CollectionSync( "akonadi_knut_resource_0" );
      syncer->setRemoteCollections( origCols );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols = fetchCollections( "akonadi_knut_resource_0" );
      QCOMPARE( resultCols.count(), origCols.count() );
    }

    void testFullStreamingSync()
    {
      Collection::List origCols = fetchCollections( "akonadi_knut_resource_0" );

      CollectionSync* syncer = new CollectionSync( "akonadi_knut_resource_0" );
      syncer->setAutoDelete( false );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      // ### streaming not implemented yet
//       syncer->setTotalItems( origCols.count() );
      QTest::qWait( 10 );
      QCOMPARE( spy.count(), 0 );
// 
      for ( int i = 0; i < origCols.count(); ++i ) {
//         Item::List l;
//         l << origCols[i];
//         syncer->setFullSyncItems( l );
//         if ( i < origCols.count() - 1 )
//           QTest::qWait( 10 ); // enter the event loop so itemsync actually can do something
//         QCOMPARE( spy.count(), 0 );
      }
//       QTest::qWait( 1000 ); // let it finish its job
//       QCOMPARE( spy.count(), 1 );
//       KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
//       QCOMPARE( job, syncer );
//       QCOMPARE( job->error(), 0 );
// 
      Collection::List resultCols = fetchCollections( "akonadi_knut_resource_0" );
      QCOMPARE( resultCols.count(), origCols.count() );

      delete syncer;
    }

    void testIncrementalSync()
    {
      Collection::List origCols = fetchCollections( "akonadi_knut_resource_0" );

      CollectionSync* syncer = new CollectionSync( "akonadi_knut_resource_0" );
      syncer->setRemoteCollections( origCols, Collection::List() );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols = fetchCollections( "akonadi_knut_resource_0" );
      QCOMPARE( resultCols.count(), origCols.count() );

      Collection::List delCols;
      delCols << resultCols.front();
      resultCols.pop_front();

      // ### not implemented yet I guess
#if 0
      Collection colWithOnlyRemoteId;
      colWithOnlyRemoteId.setRemoteId( resultCols.front().remoteId() );
      delCols << colWithOnlyRemoteId;
      resultCols.pop_front();
#endif

#if 0
      // ### should this work?
      Collection colWithRandomRemoteId;
      colWithRandomRemoteId.setRemoteId( KRandom::randomString( 100 ) );
      delCols << colWithRandomRemoteId;
#endif

      syncer = new CollectionSync( "akonadi_knut_resource_0" );
      syncer->setRemoteCollections( resultCols, delCols );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols2 = fetchCollections( "akonadi_knut_resource_0" );
      QCOMPARE( resultCols2.count(), resultCols.count() );
    }

    void testIncrementalStreamingSync()
    {
      Collection::List origCols = fetchCollections( "akonadi_knut_resource_0" );

      CollectionSync* syncer = new CollectionSync( "akonadi_knut_resource_0" );
      syncer->setAutoDelete( false );
      QSignalSpy spy( syncer, SIGNAL(result(KJob*)) );
      QVERIFY( spy.isValid() );
      // ### not implemented yet
//       syncer->setStreamingEnabled( true );
      QTest::qWait( 10 );
      QCOMPARE( spy.count(), 0 );

      for ( int i = 0; i < origCols.count(); ++i ) {
/*        Item::List l;
        l << origCols[i];
        syncer->setIncrementalSyncItems( l, Item::List() );
        if ( i < origCols.count() - 1 )
          QTest::qWait( 10 ); // enter the event loop so itemsync actually can do something
        QCOMPARE( spy.count(), 0 );*/
      }
//       syncer->deliveryDone();
//       QTest::qWait( 1000 ); // let it finish its job
//       QCOMPARE( spy.count(), 1 );
//       KJob *job = spy.at( 0 ).at( 0 ).value<KJob*>();
//       QCOMPARE( job, syncer );
//       QCOMPARE( job->error(), 0 );

      Collection::List resultCols = fetchCollections( "akonadi_knut_resource_0" );
      QCOMPARE( resultCols.count(), origCols.count() );

      delete syncer;
    }

    void testEmptyIncrementalSync()
    {
      Collection::List origCols = fetchCollections( "akonadi_knut_resource_0" );

      CollectionSync* syncer = new CollectionSync( "akonadi_knut_resource_0" );
      syncer->setRemoteCollections( Collection::List(), Collection::List() );
      AKVERIFYEXEC( syncer );

      Collection::List resultCols = fetchCollections( "akonadi_knut_resource_0" );
      QCOMPARE( resultCols.count(), origCols.count() );
    }
};

QTEST_AKONADIMAIN( CollectionSyncTest, NoGUI )

#include "collectionsynctest.moc"
