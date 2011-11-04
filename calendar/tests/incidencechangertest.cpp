/*
    Copyright (c) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "../incidencechanger.h"

#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/qtest_akonadi.h>

#include <Akonadi/Item>
#include <Akonadi/Collection>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemCreateJob>
#include <akonadi/itemfetchscope.h>

#include <KCalCore/Event>
#include <KCalCore/Journal>
#include <KCalCore/Todo>

#include <QtCore/QObject>
#include <QPushButton>

using namespace Akonadi;
using namespace KCalCore;

class IncidenceChangerTest : public QObject
{
  Q_OBJECT
  Collection mCollection;

  bool mWaitingForIncidenceChangerSignals;
  IncidenceChanger::ResultCode mExpectedResult;
  IncidenceChanger *mChanger;

  QSet<int> mKnownChangeIds;
  QHash<int,Akonadi::Item::Id> mItemIdByChangeId;

  private slots:
    void initTestCase()
    {
      mWaitingForIncidenceChangerSignals = false;
      mExpectedResult = IncidenceChanger::ResultCodeSuccess;
      //Control::start(); //TODO: uncomment when using testrunner
      qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
      CollectionFetchJob *job = new CollectionFetchJob( Collection::root(),
                                                        CollectionFetchJob::Recursive,
                                                        this );
      // Get list of collections
      job->fetchScope().setContentMimeTypes( QStringList() << QLatin1String( "application/x-vnd.akonadi.calendar.event" ) );
      AKVERIFYEXEC( job );

      // Find our collection
      Collection::List collections = job->collections();
      QVERIFY( !collections.isEmpty() );
      mCollection = collections.first();

      QVERIFY( mCollection.isValid() );

      mChanger = new IncidenceChanger();
      mChanger->setShowDialogsOnError( false );

      connect( mChanger, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
               SLOT(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );

      connect( mChanger, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
               SLOT(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)) );

      connect( mChanger,SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
               SLOT(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );
    }

    void testCreating_data()
    {
      QTest::addColumn<bool>( "sendInvalidIncidence" );
      QTest::addColumn<QString>( "uid" );
      QTest::addColumn<QString>( "summary" );
      QTest::addColumn<Akonadi::Collection>( "destinationCollection" );
      QTest::addColumn<Akonadi::Collection>( "defaultCollection" );
      QTest::addColumn<Akonadi::IncidenceChanger::DestinationPolicy>( "destinationPolicy" );
      QTest::addColumn<bool>( "failureExpected" );
      QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>( "expectedResultCode" );


      QTest::newRow( "Simple Creation1" ) << false << "SomeUid1" << "Summary1" << mCollection
                                          << Collection()
                                          << IncidenceChanger::DestinationPolicyNeverAsk
                                          << false << IncidenceChanger::ResultCodeSuccess;

      QTest::newRow( "Simple Creation2" ) << false << "SomeUid2" << "Summary2" << mCollection
                                          << Collection()
                                          << IncidenceChanger::DestinationPolicyNeverAsk
                                          << false << IncidenceChanger::ResultCodeSuccess;

      QTest::newRow( "Invalid incidence" ) << true << "SomeUid3" << "Summary3" << mCollection
                                           << Collection()
                                           << IncidenceChanger::DestinationPolicyNeverAsk
                                           << true;

      QTest::newRow( "Invalid collection" ) << false << "SomeUid4" << "Summary4" << Collection()
                                            << Collection()
                                            << IncidenceChanger::DestinationPolicyNeverAsk
                                            << false << IncidenceChanger::ResultCodeInvalidDefaultCollection;

      QTest::newRow( "Default collection" ) << false << "SomeUid5" << "Summary5" << Collection()
                                            << mCollection
                                            << IncidenceChanger::DestinationPolicyDefault
                                            << false << IncidenceChanger::ResultCodeSuccess;
    }

    void testCreating()
    {
      QFETCH( bool, sendInvalidIncidence );
      QFETCH( QString, uid );
      QFETCH( QString, summary );
      QFETCH( Akonadi::Collection, destinationCollection );
      QFETCH( Akonadi::Collection, defaultCollection );
      QFETCH( Akonadi::IncidenceChanger::DestinationPolicy, destinationPolicy );
      QFETCH( bool, failureExpected );

      Incidence::Ptr incidence;

      if ( !sendInvalidIncidence ) {
        incidence = Incidence::Ptr( new Event() );
        incidence->setUid( uid );
        incidence->setSummary( summary );
      }

      mChanger->setDestinationPolicy( destinationPolicy );
      mChanger->setDefaultCollection( defaultCollection );
      const int changeId = mChanger->createIncidence( incidence, destinationCollection );

      QVERIFY( failureExpected ^ ( changeId != -1 ) );

      if ( !failureExpected ) {
        QFETCH( Akonadi::IncidenceChanger::ResultCode, expectedResultCode );
        mKnownChangeIds.insert( changeId );
        waitForSignals( expectedResultCode );

        if ( expectedResultCode == IncidenceChanger::ResultCodeSuccess && !failureExpected ) {
          Item item;
          QVERIFY( mItemIdByChangeId.contains( changeId ) );
          item.setId( mItemIdByChangeId.value( changeId ) );
          ItemFetchJob *fetchJob = new ItemFetchJob( item, this );
          fetchJob->fetchScope().fetchFullPayload();
          AKVERIFYEXEC( fetchJob );
          QVERIFY( !fetchJob->items().isEmpty() );
          Item retrievedItem = fetchJob->items().first();
          QVERIFY( retrievedItem.isValid() );
          QVERIFY( retrievedItem.hasPayload() );
          QVERIFY( retrievedItem.hasPayload<KCalCore::Event::Ptr>() );
          QVERIFY( retrievedItem.hasPayload<KCalCore::Incidence::Ptr>() );
          Incidence::Ptr incidence = retrievedItem.payload<KCalCore::Incidence::Ptr>();
          QCOMPARE( incidence->summary(), summary );
          QCOMPARE( incidence->uid(), uid );
        }
      }
      mKnownChangeIds.clear();
      mItemIdByChangeId.clear();
    }

    void testDeleting_data()
    {
      QTest::addColumn<Akonadi::Item::List>( "items" );
      QTest::addColumn<bool>( "failureExpected" );
      QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>( "expectedResultCode" );


      QTest::newRow( "Delete empty list" )   << Item::List() << true;
      QTest::newRow( "Delete invalid item" ) << (Item::List() << Item()) << true;

      ItemFetchJob *fetchJob = new ItemFetchJob( mCollection );
      fetchJob->fetchScope().fetchFullPayload();
      AKVERIFYEXEC( fetchJob );
      Item::List items = fetchJob->items();

      // 3 Incidences were created in testCreating(). Keep this in sync.
      QVERIFY( items.count() == 3 );
      QTest::newRow( "Simple delete" ) << (Item::List() << items.at( 0 ) ) << false
                                       << IncidenceChanger::ResultCodeSuccess;

      QTest::newRow( "Delete already deleted" ) << ( Item::List() << items.at( 0 ) ) << false
                                                << IncidenceChanger::ResultCodeAlreadyDeleted;

      QTest::newRow( "Delete all others" ) << ( Item::List() << items.at( 1 ) << items.at( 2 ) )
                                           << false << IncidenceChanger::ResultCodeSuccess;
    }

    void testDeleting()
    {
      QFETCH( Akonadi::Item::List, items );
      QFETCH( bool, failureExpected );
      const int changeId = mChanger->deleteIncidences( items );

      QVERIFY( failureExpected ^ ( changeId != -1 ) );

      if ( !failureExpected ) {
        mKnownChangeIds.insert( changeId );
        QFETCH( Akonadi::IncidenceChanger::ResultCode, expectedResultCode );

        waitForSignals( expectedResultCode );

        if ( expectedResultCode == IncidenceChanger::ResultCodeSuccess ) {
          // Check that the incidence was really deleted
          Item item;
          foreach( const Akonadi::Item &item, items ) {
            ItemFetchJob *fetchJob = new ItemFetchJob( item, this );
            fetchJob->fetchScope().fetchFullPayload();
            QVERIFY( !fetchJob->exec() );
            QVERIFY( fetchJob->items().isEmpty() );
            delete fetchJob;
          }
        }
        mKnownChangeIds.clear();
      }
    }

    void testModifying_data()
    {
      QTest::addColumn<Akonadi::Item>( "item" );
      QTest::addColumn<QString>( "newSummary" );
      QTest::addColumn<int>( "expectedRevision" );
      QTest::addColumn<bool>( "failureExpected" );
      QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>( "expectedResultCode" );

      QTest::newRow( "Invalid item" ) << Item() << QString() << 0 << true;
      QTest::newRow( "Valid item, invalid payload" ) << Item(1) << QString() << 0 << true;

      Item item;
      item.setMimeType( Event::eventMimeType() );
      Incidence::Ptr incidence = Incidence::Ptr( new Event() );
      incidence->setUid( QLatin1String( "test123uid" ) );
      incidence->setSummary( QLatin1String( "summary" ) );
      item.setPayload<KCalCore::Incidence::Ptr>( incidence );
      ItemCreateJob *job = new ItemCreateJob( item, mCollection, this );
      AKVERIFYEXEC( job );
      item = job->item();
      incidence->setSummary( QLatin1String( "New Summary" ) );
      item.setPayload<KCalCore::Incidence::Ptr>( incidence );

      QTest::newRow("Change summary") << item << "New Summary" << 1 << false
                                      << IncidenceChanger::ResultCodeSuccess;
    }

    void testModifying()
    {
      QFETCH( Akonadi::Item, item );
      QFETCH( QString, newSummary );
      QFETCH( int, expectedRevision );
      QFETCH( bool, failureExpected );

      const int changeId = mChanger->modifyIncidence( item );
      QVERIFY( failureExpected ^ ( changeId != -1 ) );

      if ( !failureExpected ) {
        QFETCH( Akonadi::IncidenceChanger::ResultCode, expectedResultCode );
        mKnownChangeIds.insert( changeId );
        waitForSignals( expectedResultCode );
        ItemFetchJob *fetchJob = new ItemFetchJob( item, this );
        fetchJob->fetchScope().fetchFullPayload();
        AKVERIFYEXEC( fetchJob );
        QVERIFY( fetchJob->items().count() == 1 );
        Item fetchedItem = fetchJob->items().first();
        QVERIFY( fetchedItem.isValid() );
        QVERIFY( fetchedItem.hasPayload<KCalCore::Incidence::Ptr>() );
        Incidence::Ptr incidence = fetchedItem.payload<KCalCore::Incidence::Ptr>();
        QCOMPARE( incidence->summary(), newSummary );
        QCOMPARE( incidence->revision(), expectedRevision );
        mKnownChangeIds.clear();
        delete fetchJob;
      }
    }

    void testMassModifyForConflicts()
    {
      /*
      int changeId;

      // First create an incidence
      const QString uid( "uid");
      const QString summary( "summary");
      Incidence::Ptr incidence( new Event() );
      incidence->setUid( uid );
      incidence->setSummary( summary );
      mPendingInsertsInETM.append( uid );
      changeId = mChanger->createIncidence( incidence,
                                            mCollection );
      QVERIFY( changeId != -1 );
      mKnownChangeIds.insert( changeId );
      waitForSignals();

      kDebug() << "Doing 30 modifications, but waiting for jobs to end before starting a new one.";
      const int LIMIT = 30;
      for ( int i = 0; i < LIMIT; ++i ) {
        mWaitingForIncidenceChangerSignals = true;
        mPendingUpdatesInETM.append( uid );
        Item item = mCalendar->itemForIncidenceUid( uid );
        QVERIFY( item.isValid() );
        item.payload<Incidence::Ptr>()->setSummary( QString::number( i ) );
        int changeId = mChanger->modifyIncidence( item );
        QVERIFY( changeId > -1 );
        mKnownChangeIds.insert( changeId );
        waitForSignals();
      }

      Item item = mCalendar->itemForIncidenceUid( uid );
      QVERIFY( item.isValid() );

      kDebug() << "Doing 30 modifications, and not for jobs to end before starting a new one.";

      for ( int i = 0; i < LIMIT; ++i ) {
        item.payload<Incidence::Ptr>()->setSummary( QString::number( i ) );
        const int changeId = mChanger->modifyIncidence( item );
        QVERIFY( changeId > -1 );
        mKnownChangeIds.insert( changeId );

        if ( i == LIMIT-1 ) {
          // Let's catch the last signal, so we don't exit our test with jobs still running
          mWaitingForIncidenceChangerSignals = true;
        }
        QTest::qWait( 100 );
      }
      waitForSignals();

      // Cleanup, delete the incidence
      item = mCalendar->itemForIncidenceUid( uid );
      QVERIFY( item.isValid() );
      mPendingDeletesInETM.append( uid );
      changeId = mChanger->deleteIncidence( item );
      QVERIFY( changeId != -1 );
      mKnownChangeIds.insert( changeId );
      waitForSignals();
      */
    }

  public Q_SLOTS:

    void waitForSignals( Akonadi::IncidenceChanger::ResultCode expectedResultCode )
    {
      mWaitingForIncidenceChangerSignals = true;
      mExpectedResult = expectedResultCode;

      int i = 0;
      while ( mWaitingForIncidenceChangerSignals && i++ < 10) { // wait 10 seconds max.
        QTest::qWait( 1000 );
      }

      QVERIFY( !mWaitingForIncidenceChangerSignals );
    }

  void deleteFinished( int changeId,
                       const QVector<Akonadi::Item::Id> &deletedIds,
                       Akonadi::IncidenceChanger::ResultCode resultCode,
                       const QString &errorMessage )
  {
    Q_UNUSED( deletedIds );
    QVERIFY( mKnownChangeIds.contains( changeId ) );
    QVERIFY( changeId != -1 );

    if ( resultCode != IncidenceChanger::ResultCodeSuccess ) {
      kDebug() << "Error string is " << errorMessage;
    } else {
      QVERIFY( !deletedIds.isEmpty() );
      foreach( Akonadi::Item::Id id , deletedIds ) {
        QVERIFY( id != -1 );
      }
    }

    QVERIFY( resultCode == mExpectedResult );
    mExpectedResult = IncidenceChanger::ResultCodeSuccess;
    mWaitingForIncidenceChangerSignals = false;
  }

  void createFinished( int changeId,
                       const Akonadi::Item &item,
                       Akonadi::IncidenceChanger::ResultCode resultCode,
                       const QString &errorString )
  {
    QVERIFY( mKnownChangeIds.contains( changeId ) );
    QVERIFY( changeId != -1 );

    if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
      QVERIFY( item.isValid() );
      QVERIFY( item.parentCollection().isValid() );
      mItemIdByChangeId.insert( changeId, item.id() );
    } else {
      kDebug() << "Error string is " << errorString;
    }

    QVERIFY( resultCode == mExpectedResult );
    mExpectedResult = IncidenceChanger::ResultCodeSuccess;
    mWaitingForIncidenceChangerSignals = false;
  }

  void modifyFinished( int changeId,
                       const Akonadi::Item &item,
                       Akonadi::IncidenceChanger::ResultCode resultCode,
                       const QString &errorString )
  {
    Q_UNUSED( item );
    QVERIFY( mKnownChangeIds.contains( changeId ) );
    QVERIFY( changeId != -1 );

    if ( resultCode == IncidenceChanger::ResultCodeSuccess )
      QVERIFY( item.isValid() );
    else
      kDebug() << "Error string is " << errorString;

    QVERIFY( resultCode == mExpectedResult );

    mExpectedResult = IncidenceChanger::ResultCodeSuccess;
    mWaitingForIncidenceChangerSignals = false;
  }
};

QTEST_AKONADIMAIN( IncidenceChangerTest, NoGUI )

#include "incidencechangertest.moc"
