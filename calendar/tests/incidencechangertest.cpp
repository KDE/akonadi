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
    }

    void testDeleting()
    {
      /*
      int changeId;
      const Item::List incidences = mCalendar->rawIncidences();

      { // Delete 5 incidences, previously created
        foreach( const Item &item, incidences ) {
          mPendingDeletesInETM.append( item.payload<Incidence::Ptr>()->uid() );
        }
        changeId = mChanger->deleteIncidences( incidences );
        mKnownChangeIds.insert( changeId );
        QVERIFY( changeId != -1 );
        waitForSignals();
      }

      { // Delete something already deleted
        mWaitingForIncidenceChangerSignals = true;
        changeId = mChanger->deleteIncidences( incidences );
        mKnownChangeIds.insert( changeId );
        QVERIFY( changeId != -1 );
        mExpectedResult = IncidenceChanger::ResultCodeAlreadyDeleted;
        waitForSignals();
      }

      { // If we provide an empty list, a job won't be created
        changeId = mChanger->deleteIncidences( Item::List() );
        mKnownChangeIds.insert( changeId );
        QVERIFY( changeId == -1 );
      }

      { // If we provide a list with at least one invalid item, a job won't be created
        Item::List list;
        list << Item();
        changeId = mChanger->deleteIncidences( list );
        QVERIFY( changeId == -1 );
      }
*/
    }

    void testModifying()
    {
      /*
      int changeId;

      // First create an incidence
      const QString uid( "uid");
      const QString summary( "summary");
      Incidence::Ptr incidence( new Event() );
      incidence->setUid( uid );
      incidence->setSummary( summary );
      mWaitingForIncidenceChangerSignals = true;
      changeId = mChanger->createIncidence( incidence, mCollection );
      QVERIFY( changeId != -1 );
      mKnownChangeIds.insert( changeId );
      waitForSignals();

      { // Just a summary change
        Item item = mCalendar->itemForIncidenceUid( uid );
        QVERIFY( item.isValid() );
        item.payload<Incidence::Ptr>()->setSummary( "summary2" );
        mWaitingForIncidenceChangerSignals = true;
        changeId = mChanger->modifyIncidence( item );
        QVERIFY( changeId != -1 );
        mKnownChangeIds.insert( changeId );
        waitForSignals();
        item = mCalendar->itemForIncidenceUid( uid );
        QVERIFY( item.isValid() );
        QVERIFY( item.payload<Incidence::Ptr>()->summary() == "summary2" );
      }

      { // Invalid item
        changeId = mChanger->modifyIncidence( Item() );
        QVERIFY( changeId == -1 );
      }

      { // Delete it and try do modify it, should result in error
        Item item = mCalendar->itemForIncidenceUid( uid );
        QVERIFY( item.isValid() );
        mPendingDeletesInETM.append( uid );
        changeId = mChanger->deleteIncidence( item );
        QVERIFY( changeId != -1 );
        mKnownChangeIds.insert( changeId );
        waitForSignals();

        mWaitingForIncidenceChangerSignals = true;
        changeId = mChanger->modifyIncidence( item );
        mKnownChangeIds.insert( changeId );
        mExpectedResult = IncidenceChanger::ResultCodeAlreadyDeleted;
        QVERIFY( changeId != -1 );
        waitForSignals();
      }
      */
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
