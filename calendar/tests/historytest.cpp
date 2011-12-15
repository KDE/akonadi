/*
    Copyright (c) 2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "../history.h"
#include "../incidencechanger.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <akonadi/qtest_akonadi.h>

#include <KCalCore/Event>

#include <QTestEventLoop>

using namespace Akonadi;
using namespace KCalCore;

enum SignalType {
  DeletionSignal,
  CreationSignal,
  ModificationSignal,
  UndoSignal,
  RedoSignal,
  NumSignals
};

class HistoryTest : public QObject
{
  Q_OBJECT
    Collection mCollection;
    IncidenceChanger *mChanger;
    History *mHistory;
    QHash<SignalType, int> mPendingSignals;
    QHash<int, Akonadi::Item> mItemByChangeId;
    QList<int> mKnownChangeIds;

    void createIncidence( const QString &uid )
    {
      Item item;
      item.setMimeType( Event::eventMimeType() );
      Incidence::Ptr incidence = Incidence::Ptr( new Event() );
      incidence->setUid( uid );
      incidence->setSummary( QLatin1String( "summary" ) );
      item.setPayload<KCalCore::Incidence::Ptr>( incidence );
      ItemCreateJob *job = new ItemCreateJob( item, mCollection, this );
      AKVERIFYEXEC( job );
    }

    static Akonadi::Item item()
    {
      Item item;
      Incidence::Ptr incidence = Incidence::Ptr( new Event() );
      incidence->setSummary( QLatin1String( "some summary" ) );
      item.setMimeType( incidence->mimeType() );
      item.setPayload<KCalCore::Incidence::Ptr>( incidence );
      return item;
    }

    void fetchCollection()
    {
      CollectionFetchJob *job = new CollectionFetchJob( Collection::root(),
                                                        CollectionFetchJob::Recursive,
                                                        this );
      // Get list of collections
      job->fetchScope().setContentMimeTypes( QStringList()
                                   << QLatin1String( "application/x-vnd.akonadi.calendar.event" ) );
      AKVERIFYEXEC( job );

      // Find our collection
      Collection::List collections = job->collections();
      QVERIFY( !collections.isEmpty() );
      mCollection = collections.first();

      QVERIFY( mCollection.isValid() );
    }
private Q_SLOTS:
    void initTestCase()
    {
      fetchCollection();
      qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
      qRegisterMetaType<QList<Akonadi::IncidenceChanger::ChangeType> >( "QList<Akonadi::IncidenceChanger::ChangeType>" );
      qRegisterMetaType<QVector<Akonadi::Item::Id> >( "QVector<Akonadi::Item::Id>" );
      mChanger = new IncidenceChanger( this );
      mChanger->setShowDialogsOnError( false );
      mChanger->setHistoryEnabled( true );
      mHistory = mChanger->history();
      mChanger->setDefaultCollection( mCollection );
      connect( mChanger,
               SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
               SLOT(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );

      connect( mChanger,
               SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
               SLOT(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)) );

      connect( mChanger,
               SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
               SLOT(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );

      connect( mHistory, SIGNAL(undone(Akonadi::History::ResultCode)),
               SLOT(handleUndone(Akonadi::History::ResultCode)));

      connect( mHistory, SIGNAL(redone(Akonadi::History::ResultCode)),
               SLOT(handleRedone(Akonadi::History::ResultCode)));
    }

    void testUndoCreation_data()
    {
      QTest::addColumn<Akonadi::Item>( "item" );
      QTest::newRow("item1") << item();
    }

    void testUndoCreation()
    {
      QFETCH( Akonadi::Item, item );
      mPendingSignals[CreationSignal] = 1;
      QCOMPARE( mHistory->redoCount(), 0 );
      QCOMPARE( mHistory->undoCount(), 0 );
      QVERIFY( item.hasPayload() );
      const int changeId = mChanger->createIncidence( item.payload<KCalCore::Incidence::Ptr>() );
      QVERIFY( changeId > 0 );
      mKnownChangeIds << changeId;
      waitForSignals();

      // Check that it was created
      ItemFetchJob *job = new ItemFetchJob( mItemByChangeId.value( changeId ) );
      AKVERIFYEXEC( job );

      QCOMPARE( mHistory->redoCount(), 0 );
      QCOMPARE( mHistory->undoCount(), 1 );

      //undo it
      mPendingSignals[UndoSignal] = 1;
      mHistory->undo();
      waitForSignals();

      QCOMPARE( mHistory->redoCount(), 1 );
      QCOMPARE( mHistory->undoCount(), 0 );

      mHistory->clear();
      QCOMPARE( mHistory->redoCount(), 0 );
      QCOMPARE( mHistory->undoCount(), 0 );
    }

private:
    void waitForSignals()
    {
      bool somethingToWaitFor = false;
      for ( int i=0; i<NumSignals; ++i ) {
        if ( mPendingSignals.value( static_cast<SignalType>(i) ) ) {
          somethingToWaitFor = true;
          break;
        }
      }

      if ( !somethingToWaitFor )
        return;

      QTestEventLoop::instance().enterLoop( 10 );

      if ( QTestEventLoop::instance().timeout() ) {
        for ( int i=0; i<NumSignals; ++i ) {
          qDebug() << mPendingSignals.value( static_cast<SignalType>(i) );
        }
      }

      QVERIFY( !QTestEventLoop::instance().timeout() );
    }

public Q_SLOTS:
    void deleteFinished( int changeId,
                         const QVector<Akonadi::Item::Id> &deletedIds,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorMessage )
    {
      QVERIFY( changeId != -1 );

      if ( !mKnownChangeIds.contains(changeId) )
        return;

      --mPendingSignals[DeletionSignal];

      if ( resultCode != IncidenceChanger::ResultCodeSuccess ) {
        kDebug() << "Error string is " << errorMessage;
      } else {
        QVERIFY( !deletedIds.isEmpty() );
        foreach( Akonadi::Item::Id id , deletedIds ) {
          QVERIFY( id != -1 );
        }
      }
      maybeQuitEventLoop();
    }

    void createFinished( int changeId,
                         const Akonadi::Item &item,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorString )
    {
      QVERIFY( changeId != -1 );

      if ( !mKnownChangeIds.contains(changeId) )
        return;

      --mPendingSignals[CreationSignal];

      if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
        QVERIFY( item.isValid() );
        QVERIFY( item.parentCollection().isValid() );
        mItemByChangeId.insert( changeId, item );
        QVERIFY( item.hasPayload() );
        Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
        //mItemIdByUid.insert( incidence->uid(), item.id() );
      } else {
        kDebug() << "Error string is " << errorString;
      }

      maybeQuitEventLoop();
    }

    void modifyFinished( int changeId,
                         const Akonadi::Item &item,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorString )
    {
      --mPendingSignals[ModificationSignal];

      if ( !mKnownChangeIds.contains(changeId) )
        return;

      QVERIFY( changeId != -1 );

      if ( resultCode == IncidenceChanger::ResultCodeSuccess )
        QVERIFY( item.isValid() );
      else
        kDebug() << "Error string is " << errorString;

      maybeQuitEventLoop();
    }

    void handleRedone( Akonadi::History::ResultCode result )
    {
      QCOMPARE( result, History::ResultCodeSuccess );
      --mPendingSignals[RedoSignal];
      maybeQuitEventLoop();
    }

    void handleUndone( Akonadi::History::ResultCode result )
    {
      QCOMPARE( result, History::ResultCodeSuccess );
      --mPendingSignals[UndoSignal];
      maybeQuitEventLoop();
    }

    void maybeQuitEventLoop()
    {
      for ( int i=0; i<NumSignals; ++i ) {
        if ( mPendingSignals.value( static_cast<SignalType>(i) ) > 0 )
          return;
      }
      QTestEventLoop::instance().exitLoop();
    }
};

QTEST_AKONADIMAIN( HistoryTest, GUI )

#include "HistoryTest.moc"
