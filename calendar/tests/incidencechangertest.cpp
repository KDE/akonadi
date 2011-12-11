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

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE( QList<Akonadi::IncidenceChanger::ChangeType> )
Q_DECLARE_METATYPE( QList<bool> )
Q_DECLARE_METATYPE( QList<Akonadi::Collection::Right> )
Q_DECLARE_METATYPE( QList<Akonadi::Collection::Rights> )
Q_DECLARE_METATYPE( QList<Akonadi::IncidenceChanger::ResultCode> )

static Akonadi::Item item()
{
  Item item;
  Incidence::Ptr incidence = Incidence::Ptr( new Event() );
  incidence->setSummary( QLatin1String( "random summary" ) );
  item.setMimeType( incidence->mimeType() );
  item.setPayload<KCalCore::Incidence::Ptr>( incidence );
  return item;
}

static Akonadi::Item createItem( const Akonadi::Collection &collection )
{
  Item i = item();
  ItemCreateJob *createJob = new ItemCreateJob( i, collection );

  createJob->exec();
  return createJob->item();
}

class IncidenceChangerTest : public QObject
{
  Q_OBJECT
  Collection mCollection;

  QHash<int,IncidenceChanger::ResultCode> mExpectedResultByChangeId;
  IncidenceChanger *mChanger;

  int mIncidencesToDelete;
  int mIncidencesToAdd;
  int mIncidencesToModify;

  QHash<int,Akonadi::Item::Id> mItemIdByChangeId;
  QHash<QString,Akonadi::Item::Id> mItemIdByUid;
  int mChangeToWaitFor;

  private slots:
    void initTestCase()
    {
      mIncidencesToDelete = 0;
      mIncidencesToAdd    = 0;
      mIncidencesToModify = 0;

      mChangeToWaitFor = -1;
      //Control::start(); //TODO: uncomment when using testrunner
      qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
      qRegisterMetaType<QList<Akonadi::IncidenceChanger::ChangeType> >( "QList<Akonadi::IncidenceChanger::ChangeType>" );
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
      mCollection.setRights( Collection::Rights( Collection::AllRights ) );

      QVERIFY( mCollection.isValid() );
      QVERIFY( ( mCollection.rights() & Akonadi::Collection::CanCreateItem ) );

      mChanger = new IncidenceChanger( this );
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
      QTest::addColumn<bool>( "respectsCollectionRights" );
      QTest::addColumn<Akonadi::IncidenceChanger::DestinationPolicy>( "destinationPolicy" );
      QTest::addColumn<bool>( "failureExpected" );
      QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>( "expectedResultCode" );

      QTest::newRow( "Simple Creation1" ) << false << "SomeUid1" << "Summary1" << mCollection
                                          << Collection() << true
                                          << IncidenceChanger::DestinationPolicyNeverAsk
                                          << false << IncidenceChanger::ResultCodeSuccess;

      QTest::newRow( "Simple Creation2" ) << false << "SomeUid2" << "Summary2" << mCollection
                                          << Collection() << true
                                          << IncidenceChanger::DestinationPolicyNeverAsk
                                          << false << IncidenceChanger::ResultCodeSuccess;

      QTest::newRow( "Invalid incidence" ) << true << "SomeUid3" << "Summary3" << mCollection
                                           << Collection() << true
                                           << IncidenceChanger::DestinationPolicyNeverAsk
                                           << true;

      QTest::newRow( "Invalid collection" ) << false << "SomeUid4" << "Summary4" << Collection()
                                            << Collection() << true
                                            << IncidenceChanger::DestinationPolicyNeverAsk
                                            << false << IncidenceChanger::ResultCodeInvalidDefaultCollection;

      QTest::newRow( "Default collection" ) << false << "SomeUid5" << "Summary5" << Collection()
                                            << mCollection << true
                                            << IncidenceChanger::DestinationPolicyDefault
                                            << false << IncidenceChanger::ResultCodeSuccess;

      Collection collectionWithoutRights = Collection( mCollection.id() );
      collectionWithoutRights.setRights( Collection::Rights() );
      Q_ASSERT( ( mCollection.rights() & Akonadi::Collection::CanCreateItem ) );

      QTest::newRow( "No rights" ) << false << "SomeUid6" << "Summary6" << Collection()
                                   << collectionWithoutRights << true
                                   << IncidenceChanger::DestinationPolicyNeverAsk
                                   << false << IncidenceChanger::ResultCodeInvalidDefaultCollection;

      QTest::newRow( "No rights but its ok" ) << false << "SomeUid7" << "Summary7" << Collection()
                                              << collectionWithoutRights << false
                                              << IncidenceChanger::DestinationPolicyNeverAsk
                                              << false
                                              << IncidenceChanger::ResultCodeSuccess;
    }

    void testCreating()
    {
      QFETCH( bool, sendInvalidIncidence );
      QFETCH( QString, uid );
      QFETCH( QString, summary );
      QFETCH( Akonadi::Collection, destinationCollection );
      QFETCH( Akonadi::Collection, defaultCollection );
      QFETCH( bool, respectsCollectionRights );
      QFETCH( Akonadi::IncidenceChanger::DestinationPolicy, destinationPolicy );
      QFETCH( bool, failureExpected );

      Incidence::Ptr incidence;

      if ( !sendInvalidIncidence ) {
        incidence = Incidence::Ptr( new Event() );
        incidence->setUid( uid );
        incidence->setSummary( summary );
      }
      mChanger->setRespectsCollectionRights( respectsCollectionRights );
      mChanger->setDestinationPolicy( destinationPolicy );
      mChanger->setDefaultCollection( defaultCollection );
      const int changeId = mChanger->createIncidence( incidence, destinationCollection );

      QVERIFY( failureExpected ^ ( changeId != -1 ) );

      if ( !failureExpected ) {
        QFETCH( Akonadi::IncidenceChanger::ResultCode, expectedResultCode );
        mIncidencesToAdd = 1;

        mExpectedResultByChangeId.insert( changeId, expectedResultCode );
        waitForSignals();

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
      mItemIdByChangeId.clear();
    }

    void testDeleting_data()
    {
      QTest::addColumn<Akonadi::Item::List>( "items" );
      QTest::addColumn<bool>( "respectsCollectionRights" );
      QTest::addColumn<bool>( "failureExpected" );
      QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>( "expectedResultCode" );


      QTest::newRow( "Delete empty list" )   << Item::List() << true << true;
      QTest::newRow( "Delete invalid item" ) << (Item::List() << Item()) << true << true;

      ItemFetchJob *fetchJob = new ItemFetchJob( mCollection );
      fetchJob->fetchScope().fetchFullPayload();
      AKVERIFYEXEC( fetchJob );
      Item::List items = fetchJob->items();

      // 5 Incidences were created in testCreating(). Keep this in sync.
      QVERIFY( items.count() == 4 );
      QTest::newRow( "Simple delete" ) << (Item::List() << items.at( 0 ) ) << true << false
                                       << IncidenceChanger::ResultCodeSuccess;

      QTest::newRow( "Delete already deleted" ) << ( Item::List() << items.at( 0 ) ) << true
                                                << false
                                                << IncidenceChanger::ResultCodeAlreadyDeleted;

      QTest::newRow( "Delete all others" ) << ( Item::List() << items.at( 1 ) << items.at( 2 ) )
                                           << true << false << IncidenceChanger::ResultCodeSuccess;

      Collection collectionWithoutRights = Collection( mCollection.id() );
      collectionWithoutRights.setRights( Collection::Rights() );
      Item item = items.at( 3 );
      item.setParentCollection( collectionWithoutRights );
      QTest::newRow( "Delete can't delete" ) << ( Item::List() << item )
                                             << true << false
                                             << IncidenceChanger::ResultCodePermissions;
    }

    void testDeleting()
    {
      QFETCH( Akonadi::Item::List, items );
      QFETCH( bool, respectsCollectionRights );
      QFETCH( bool, failureExpected );
      mChanger->setRespectsCollectionRights( respectsCollectionRights );
      const int changeId = mChanger->deleteIncidences( items );

      QVERIFY( failureExpected ^ ( changeId != -1 ) );

      if ( !failureExpected ) {
        QFETCH( Akonadi::IncidenceChanger::ResultCode, expectedResultCode );
        mIncidencesToDelete = 1;
        mExpectedResultByChangeId.insert( changeId, expectedResultCode );
        waitForSignals();

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
      }
    }

    void testModifying_data()
    {
      QTest::addColumn<Akonadi::Item>( "item" );
      QTest::addColumn<QString>( "newSummary" );
      QTest::addColumn<bool>( "respectsCollectionRights" );
      QTest::addColumn<int>( "expectedRevision" );
      QTest::addColumn<bool>( "failureExpected" );
      QTest::addColumn<Akonadi::IncidenceChanger::ResultCode>( "expectedResultCode" );

      QTest::newRow( "Invalid item" ) << Item() << QString() << true << 0 << true;
      QTest::newRow( "Valid item, invalid payload" ) << Item(1) << QString() << true << 0 << true;

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

      QTest::newRow("Change summary") << item << "New Summary" << true << 1 << false
                                      << IncidenceChanger::ResultCodeSuccess;

      Collection collectionWithoutRights = Collection( mCollection.id() );
      collectionWithoutRights.setRights( Collection::Rights() );
      item.setParentCollection( collectionWithoutRights );

      QTest::newRow("Can't change") << item << "New Summary" << true << 1 << false
                                    << IncidenceChanger::ResultCodePermissions;
    }

    void testModifying()
    {
      QFETCH( Akonadi::Item, item );
      QFETCH( QString, newSummary );
      QFETCH( bool, respectsCollectionRights );
      QFETCH( int, expectedRevision );
      QFETCH( bool, failureExpected );

      mChanger->setRespectsCollectionRights( respectsCollectionRights );
      const int changeId = mChanger->modifyIncidence( item );
      QVERIFY( failureExpected ^ ( changeId != -1 ) );

      if ( !failureExpected ) {
        QFETCH( Akonadi::IncidenceChanger::ResultCode, expectedResultCode );

        mIncidencesToModify = 1;
        mExpectedResultByChangeId.insert( changeId, expectedResultCode );
        waitForSignals();
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
        delete fetchJob;
      }
    }

    void testMassModifyForConflicts_data()
    {
      QTest::addColumn<Akonadi::Item>( "item" );
      QTest::addColumn<bool>( "waitForPreviousJob" );
      QTest::addColumn<int>( "numberOfModifications" );

      // Create an incidence
      Item item;
      item.setMimeType( Event::eventMimeType() );
      Incidence::Ptr incidence = Incidence::Ptr( new Event() );
      incidence->setUid( QLatin1String( "test123uid" ) );
      incidence->setSummary( QLatin1String( "summary" ) );
      item.setPayload<KCalCore::Incidence::Ptr>( incidence );
      ItemCreateJob *job = new ItemCreateJob( item, mCollection, this );
      AKVERIFYEXEC( job );
      item = job->item();

      QTest::newRow("15 modifications in sequence") << item << true  << 15;
      QTest::newRow("15 modifications in parallel") << item << false << 15;
    }

    void testMassModifyForConflicts()
    {
      QFETCH( Akonadi::Item, item );
      QFETCH( bool, waitForPreviousJob );
      QFETCH( int, numberOfModifications );

      Q_ASSERT( numberOfModifications > 0 );
      int changeId = -1;
      for( int i=0; i<numberOfModifications; ++i ) {
        Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
        Q_ASSERT( incidence );
        incidence->setSummary( QString::number( i ) );
        changeId = mChanger->modifyIncidence( item );
        QVERIFY( changeId != -1 );

        if ( waitForPreviousJob ) {
          mIncidencesToModify = 1;
          mExpectedResultByChangeId.insert( changeId, IncidenceChanger::ResultCodeSuccess );
          waitForSignals();
          ItemFetchJob *fetchJob = new ItemFetchJob( item, this );
          fetchJob->fetchScope().fetchFullPayload();
          AKVERIFYEXEC( fetchJob );
          QVERIFY( fetchJob->items().count() == 1 );
          QCOMPARE( fetchJob->items().first().payload<KCalCore::Incidence::Ptr>()->summary(),
                    QString::number( i ) );
        }
      }

      if ( !waitForPreviousJob ) {
        // Wait for the last one only.
        mExpectedResultByChangeId.insert( changeId, IncidenceChanger::ResultCodeSuccess );
        waitForChange( changeId );
        ItemFetchJob *fetchJob = new ItemFetchJob( item, this );
        fetchJob->fetchScope().fetchFullPayload();
        AKVERIFYEXEC( fetchJob );
        QVERIFY( fetchJob->items().count() == 1 );
        QCOMPARE( fetchJob->items().first().payload<KCalCore::Incidence::Ptr>()->summary(),
                  QString::number( numberOfModifications-1 ) );
      }
    }

      void testAtomicOperations_data()
      {
        QTest::addColumn<Akonadi::Item::List>( "items" );
        QTest::addColumn<QList<Akonadi::IncidenceChanger::ChangeType> >( "changeTypes" );
        QTest::addColumn<QList<bool> >( "failureExpectedList" );
        QTest::addColumn<QList<Akonadi::IncidenceChanger::ResultCode> >( "expectedResults" );
        QTest::addColumn<QList<Akonadi::Collection::Rights> >( "rights" );

        Akonadi::Item::List items;
        QList<Akonadi::IncidenceChanger::ChangeType> changeTypes;
        QList<bool> failureExpectedList;
        QList<Akonadi::IncidenceChanger::ResultCode> expectedResults;
        QList<Akonadi::Collection::Rights> rights;
        const Collection::Rights allRights = Collection::AllRights;
        const Collection::Rights noRights = Collection::Rights();
        //------------------------------------------------------------------------------------------
        // Create two incidences, should succeed
        items << item() << item() ;
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList << false << false;
        expectedResults << IncidenceChanger::ResultCodeSuccess << IncidenceChanger::ResultCodeSuccess;
        rights << allRights << allRights;

        QTest::newRow( "create two - success " ) << items << changeTypes << failureExpectedList
                                                 << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeModify;
        items.clear();
        items << createItem( mCollection ) << createItem( mCollection );

        QTest::newRow( "modify two - success " ) << items << changeTypes << failureExpectedList
                                                 << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeDelete;
        QTest::newRow( "delete two - success " ) << items << changeTypes << failureExpectedList
                                                 << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // Creation succeeds but deletion doesnt ( invalid item case )
        items.clear();
        items << item() << Item(); // Invalid item on purpose
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
        failureExpectedList.clear();
        failureExpectedList << false << true;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow( "create,try delete" ) << items << changeTypes << failureExpectedList
                                             << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // deletion doesn't succeed, but creation does ( invalid item case )
        items.clear();
        items << Item() << item(); // Invalid item on purpose
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList.clear();
        failureExpectedList << true << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow( "try delete,create" ) << items << changeTypes << failureExpectedList
                                            << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // Creation succeeds but deletion doesnt ( valid, inexistant item case )
        items.clear();
        items << item() << Item(10101010);
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeJobError;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow( "create,try delete v2" ) << items << changeTypes << failureExpectedList
                                                << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // deletion doesn't succeed, but creation does ( valid, inexistant item case )
        items.clear();
        items << Item(10101010) << item();
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeJobError
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow( "try delete,create v2" ) << items << changeTypes << failureExpectedList
                                                << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // deletion doesn't succeed, but creation does ( NO ACL case )
        items.clear();
        items << createItem( mCollection ) << item();
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeDelete << IncidenceChanger::ChangeTypeCreate;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << noRights << allRights;

        QTest::newRow( "try delete(ACL),create" ) << items << changeTypes << failureExpectedList
                                                  << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        //Creation succeeds but deletion doesnt ( NO ACL case )
        items.clear();
        items << item() << createItem( mCollection );
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeDelete;
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << allRights << noRights;

        QTest::newRow( "create,try delete(ACL)" ) << items << changeTypes << failureExpectedList
                                                  << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 1 successfull modification, 1 failed creation
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << createItem( mCollection ) << Item();
        failureExpectedList.clear();
        failureExpectedList << false << true;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow( "modify,try create" ) << items << changeTypes << failureExpectedList
                                             << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 1 successfull modification, 1 failed creation
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeModify << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << createItem( mCollection ) << item();
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << allRights << noRights;

        QTest::newRow( "modify,try create v2" ) << items << changeTypes << failureExpectedList
                                                << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 1 failed creation, 1 successfull modification
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeModify;
        items.clear();
        items << Item() << createItem( mCollection );
        failureExpectedList.clear();
        failureExpectedList << true << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << allRights;

        QTest::newRow( "try create,modify" ) << items << changeTypes << failureExpectedList
                                             << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 1 failed creation, 1 successfull modification
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeModify;
        items.clear();
        items << item() << createItem( mCollection );
        failureExpectedList.clear();
        failureExpectedList << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << noRights << allRights;

        QTest::newRow( "try create,modify v2" ) << items << changeTypes << failureExpectedList
                                                << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 4 creations, last one fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions;
        rights.clear();
        rights << allRights << allRights << allRights << noRights;

        QTest::newRow( "create 4, last fails" ) << items << changeTypes << failureExpectedList
                                                << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 4 creations, first one fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << noRights << allRights << allRights << allRights;

        QTest::newRow( "create 4, first fails" ) << items << changeTypes << failureExpectedList
                                                << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        // 4 creations, second one fails
        changeTypes.clear();
        changeTypes << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate
                    << IncidenceChanger::ChangeTypeCreate << IncidenceChanger::ChangeTypeCreate;
        items.clear();
        items << item() << item() << item() << item();
        failureExpectedList.clear();
        failureExpectedList << false << false << false << false;
        expectedResults.clear();
        expectedResults << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodePermissions
                        << IncidenceChanger::ResultCodeRolledback
                        << IncidenceChanger::ResultCodeRolledback;
        rights.clear();
        rights << allRights << noRights << allRights << allRights;

        QTest::newRow( "create 4, second fails" ) << items << changeTypes << failureExpectedList
                                                  << expectedResults << rights;
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------
      }

      void testAtomicOperations()
      {
        QFETCH( Akonadi::Item::List, items );
        QFETCH( QList<Akonadi::IncidenceChanger::ChangeType>, changeTypes );
        QFETCH( QList<bool>, failureExpectedList );
        QFETCH( QList<Akonadi::IncidenceChanger::ResultCode>, expectedResults );
        QFETCH( QList<Akonadi::Collection::Rights>, rights );

        QCOMPARE( items.count(), changeTypes.count() );
        QCOMPARE( items.count(), failureExpectedList.count() );
        QCOMPARE( items.count(), expectedResults.count() );
        QCOMPARE( items.count(), rights.count() );

        mChanger->setDefaultCollection( mCollection );
        mChanger->setRespectsCollectionRights( true );
        mChanger->setDestinationPolicy( IncidenceChanger::DestinationPolicyNeverAsk );
        mChanger->startAtomicOperation();
        mIncidencesToAdd = 0;
        mIncidencesToModify = 0;
        mIncidencesToDelete = 0;
        for( int i=0; i<items.count(); ++i ) {
          mCollection.setRights( rights[i] );
          mChanger->setDefaultCollection( mCollection );
          const Akonadi::Item item = items[i];
          int changeId = -1;
          switch( changeTypes[i] ) {
            case IncidenceChanger::ChangeTypeCreate:
              changeId = mChanger->createIncidence( item.hasPayload() ? item.payload<KCalCore::Incidence::Ptr>() : Incidence::Ptr() );
              if ( changeId != -1 )
                ++mIncidencesToAdd;
            break;
            case IncidenceChanger::ChangeTypeDelete:
              changeId = mChanger->deleteIncidence( item );
              if ( changeId != -1 )
                ++mIncidencesToDelete;
            break;
            case IncidenceChanger::ChangeTypeModify:
              QVERIFY( item.isValid() );
              QVERIFY( item.hasPayload<KCalCore::Incidence::Ptr>() );
              item.payload<KCalCore::Incidence::Ptr>()->setSummary( QLatin1String( "Changed" ) );
              changeId = mChanger->modifyIncidence( item );
              if ( changeId != -1 )
                ++mIncidencesToModify;
            break;
            default:
              QVERIFY( false );
          }
          QVERIFY( !( ( changeId == -1 ) ^ failureExpectedList[i] ) );
          if ( changeId != -1 ) {
            mExpectedResultByChangeId.insert( changeId, expectedResults[i] );
          }
        }
        mChanger->endAtomicOperation();

        waitForSignals();

        //Validate:
        for( int i=0; i<items.count(); ++i ) {
          const bool expectedSuccess = ( expectedResults[i] == IncidenceChanger::ResultCodeSuccess );
          mCollection.setRights( rights[i] );

          Akonadi::Item item = items[i];

          switch( changeTypes[i] ) {
            case IncidenceChanger::ChangeTypeCreate:
              if ( expectedSuccess ) {
                QVERIFY( item.hasPayload<KCalCore::Incidence::Ptr>() );
                Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
                QVERIFY( incidence );
                QVERIFY( !incidence->uid().isEmpty() );
                QVERIFY( mItemIdByUid.contains( incidence->uid() ) );
                ItemFetchJob *fJob = new ItemFetchJob( Item( mItemIdByUid.value( incidence->uid() ) ) );
                fJob->fetchScope().fetchFullPayload();
                AKVERIFYEXEC( fJob );
                QCOMPARE( fJob->items().count(), 1 );
                QVERIFY( fJob->items().first().isValid() );
                QVERIFY( fJob->items().first().hasPayload() );
                QVERIFY( fJob->items().first().hasPayload<KCalCore::Incidence::Ptr>() );
                QCOMPARE( item.payload<KCalCore::Incidence::Ptr>()->uid(),
                          fJob->items().first().payload<KCalCore::Incidence::Ptr>()->uid() );
              }
            break;
            case IncidenceChanger::ChangeTypeDelete:
              if ( expectedSuccess ) {
                ItemFetchJob *fJob = new ItemFetchJob( Item( item.id() ) );
                QVERIFY( !fJob->exec() );
              }
            break;
            case IncidenceChanger::ChangeTypeModify:
              if ( expectedSuccess ) {
                ItemFetchJob *fJob = new ItemFetchJob( Item( item.id() ) );
                fJob->fetchScope().fetchFullPayload();
                AKVERIFYEXEC( fJob );
                QCOMPARE( item.payload<KCalCore::Incidence::Ptr>()->summary(),
                          fJob->items().first().payload<KCalCore::Incidence::Ptr>()->summary() );
              }
            break;
            default:
              QVERIFY( false );
          }
        }
        qDebug() << "testAtomicOperations END";
      }

  public Q_SLOTS:

    void waitForSignals()
    {

      kDebug() << "Remaining: " << mIncidencesToAdd << mIncidencesToDelete << mIncidencesToModify;
      QTestEventLoop::instance().enterLoop( 10 );
      QVERIFY( !QTestEventLoop::instance().timeout() );
    }

    // Waits for a specific change
    void waitForChange( int changeId )
    {
      mChangeToWaitFor = changeId;

      int i = 0;
      while ( mChangeToWaitFor != -1 && i++ < 10) { // wait 10 seconds max.
        QTest::qWait( 100 );
      }

      QVERIFY( mChangeToWaitFor == -1 );
    }

  void deleteFinished( int changeId,
                       const QVector<Akonadi::Item::Id> &deletedIds,
                       Akonadi::IncidenceChanger::ResultCode resultCode,
                       const QString &errorMessage )
  {
    QVERIFY( changeId != -1 );

    if ( resultCode != IncidenceChanger::ResultCodeSuccess ) {
      kDebug() << "Error string is " << errorMessage;
    } else {
      QVERIFY( !deletedIds.isEmpty() );
      foreach( Akonadi::Item::Id id , deletedIds ) {
        QVERIFY( id != -1 );
      }
    }

    if ( resultCode != mExpectedResultByChangeId[changeId] ) {
      qDebug() << "deleteFinished: Expected " << mExpectedResultByChangeId[changeId] << " got " << resultCode
               << ( deletedIds.isEmpty() ? -1 : deletedIds.first() );
    }
    QCOMPARE( resultCode, mExpectedResultByChangeId[changeId] );
    mChangeToWaitFor = -1;

    --mIncidencesToDelete;
    maybeQuitEventLoop();
  }

  void createFinished( int changeId,
                       const Akonadi::Item &item,
                       Akonadi::IncidenceChanger::ResultCode resultCode,
                       const QString &errorString )
  {
    QVERIFY( changeId != -1 );

    if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
      QVERIFY( item.isValid() );
      QVERIFY( item.parentCollection().isValid() );
      mItemIdByChangeId.insert( changeId, item.id() );
      QVERIFY( item.hasPayload() );
      Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
      mItemIdByUid.insert( incidence->uid(), item.id() );
    } else {
      kDebug() << "Error string is " << errorString;
    }

    if ( resultCode != mExpectedResultByChangeId[changeId] ) {
      qDebug() << "createFinished: Expected " << mExpectedResultByChangeId[changeId] << " got " << resultCode << " for id=" << item.id();
    }
    QCOMPARE( resultCode, mExpectedResultByChangeId[changeId] );
    mChangeToWaitFor = -1;

    --mIncidencesToAdd;
    qDebug() << "Createfinished " << mIncidencesToAdd;
    maybeQuitEventLoop();
  }

  void modifyFinished( int changeId,
                       const Akonadi::Item &item,
                       Akonadi::IncidenceChanger::ResultCode resultCode,
                       const QString &errorString )
  {
    Q_UNUSED( item );
    QVERIFY( changeId != -1 );

    if ( resultCode == IncidenceChanger::ResultCodeSuccess )
      QVERIFY( item.isValid() );
    else
      kDebug() << "Error string is " << errorString;

    if ( resultCode != mExpectedResultByChangeId[changeId] ) {
      qDebug() << "modifyFinished: Expected " << mExpectedResultByChangeId[changeId] << " got " << resultCode << " for id=" << item.id();
    }

    QCOMPARE( resultCode, mExpectedResultByChangeId[changeId] );

    mChangeToWaitFor = -1;
    --mIncidencesToModify;
    maybeQuitEventLoop();
  }

  void maybeQuitEventLoop()
  {
      if ( mIncidencesToDelete == 0 && mIncidencesToAdd == 0 && mIncidencesToModify == 0 )
        QTestEventLoop::instance().exitLoop();
  }

  void testDefaultCollection()
  {
    const Collection newCollection( 42 );
    IncidenceChanger changer;
    QCOMPARE( changer.defaultCollection(), Collection() );
    changer.setDefaultCollection( newCollection );
    QCOMPARE( changer.defaultCollection(), newCollection );
  }

  void testDestinationPolicy()
  {
    IncidenceChanger changer;
    QCOMPARE( changer.destinationPolicy(), IncidenceChanger::DestinationPolicyDefault );
    changer.setDestinationPolicy( IncidenceChanger::DestinationPolicyNeverAsk );
    QCOMPARE( changer.destinationPolicy(), IncidenceChanger::DestinationPolicyNeverAsk );
  }

  void testDialogsOnError()
  {
    IncidenceChanger changer;
    QCOMPARE( changer.showDialogsOnError(), true );
    changer.setShowDialogsOnError( false );
    QCOMPARE( changer.showDialogsOnError(), false );
  }

  void testRespectsCollectionRights()
  {
    IncidenceChanger changer;
    QCOMPARE( changer.respectsCollectionRights(), true );
    changer.setRespectsCollectionRights( false );
    QCOMPARE( changer.respectsCollectionRights(), false );
  }
};

QTEST_AKONADIMAIN( IncidenceChangerTest, GUI )

#include "incidencechangertest.moc"
