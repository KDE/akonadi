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

#include "../calendarbase.h"
#include "../incidencechanger.h"

#include <QTestEventLoop>
#include <akonadi/qtest_akonadi.h>
#include <Akonadi/Collection>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemCreateJob>

#include <QtCore/QObject>

using namespace Akonadi;
using namespace KCalCore;

static bool compareUids( const QStringList &_uids, const Incidence::List &incidences )
{
  QStringList uids = _uids;

  foreach( const KCalCore::Incidence::Ptr &incidence, incidences )
  {
    if ( uids.contains( incidence->uid() ) )
      uids.removeAll( incidence->uid() );
  }

  if ( uids.isEmpty() && _uids.count() == incidences.count() ) {
    return true;
  } else {
    qDebug() << uids.count() << incidences.count();
    return false;
  }
}

class CalendarBaseTest : public QObject
{
  Q_OBJECT
  Collection mCollection;
  CalendarBase *mCalendar;
  bool mExpectedSlotResult;
  QStringList mUids;
  QString mOneEventUid;
  QString mOneTodoUid;
  QString mOneJournalUid;
  QString mOneIncidenceUid;

  private slots:

    void fetchCollection()
    {
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
    }

    void createInitialIncidences()
    {
      mExpectedSlotResult = true;

      for( int i=0; i<5; ++i ) {
        Event::Ptr event = Event::Ptr( new Event() );
        event->setUid( QLatin1String( "event" ) + QString::number( i ) );
        event->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        mUids.append( event->uid() );
        QVERIFY( mCalendar->addEvent( event ) );
        QTestEventLoop::instance().enterLoop( 5 );
        QVERIFY( !QTestEventLoop::instance().timeout() );
      }
      mOneEventUid = mUids.last();

      for( int i=0; i<5; ++i ) {
        Todo::Ptr todo = Todo::Ptr( new Todo() );
        todo->setUid( QLatin1String( "todo" ) + QString::number( i ) );
        todo->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        mUids.append( todo->uid() );
        QVERIFY( mCalendar->addTodo( todo ) );
        QTestEventLoop::instance().enterLoop( 5 );
        QVERIFY( !QTestEventLoop::instance().timeout() );
      }
      mOneTodoUid = mUids.last();

      for( int i=0; i<5; ++i ) {
        Journal::Ptr journal = Journal::Ptr( new Journal() );
        journal->setUid( QLatin1String( "journal" ) + QString::number( i ) );
        journal->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        mUids.append( journal->uid() );
        QVERIFY( mCalendar->addJournal( journal ) );
        QTestEventLoop::instance().enterLoop( 5 );
        QVERIFY( !QTestEventLoop::instance().timeout() );
      }
      mOneJournalUid = mUids.last();

      for( int i=0; i<5; ++i ) {
        Incidence::Ptr incidence = Incidence::Ptr( new Event() );
        incidence->setUid( QLatin1String( "incidence" ) + QString::number( i ) );
        incidence->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        mUids.append( incidence->uid() );
        QVERIFY( mCalendar->addIncidence( incidence ) );
        QTestEventLoop::instance().enterLoop( 5 );
        QVERIFY( !QTestEventLoop::instance().timeout() );
      }
      mOneIncidenceUid = mUids.last();
    }

    void initTestCase()
    {
      fetchCollection();
      qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
      mCalendar = new CalendarBase();
      mCalendar->incidenceChanger()->setDefaultCollection( mCollection );
      connect( mCalendar, SIGNAL(createFinished(bool,QString)),
               SLOT(handleCreateFinished(bool,QString)) );

      connect( mCalendar, SIGNAL(deleteFinished(bool,QString)),
               SLOT(handleDeleteFinished(bool,QString)) );
      createInitialIncidences();
    }

    void cleanupTestCase()
    {
      delete mCalendar;
    }

    void testItem()
    { // No need for _data()
      foreach( const QString &uid, mUids ) {
        const Item item1 = mCalendar->item( uid );
        const Item item2 = mCalendar->item( item1.id() );
        QVERIFY( item1.isValid() );
        QVERIFY( item2.isValid() );
        QCOMPARE( item1.id(), item2.id() );
        QCOMPARE( item1.payload<KCalCore::Incidence::Ptr>()->uid(), uid );
        QCOMPARE( item2.payload<KCalCore::Incidence::Ptr>()->uid(), uid );
      }
    }

    void testChildIncidences_data()
    {
      QTest::addColumn<QString>( "parentUid" );
      QTest::addColumn<Akonadi::Item::Id>( "parentId" );
      QTest::addColumn<QStringList>( "childrenUids" );

      QTest::newRow( "Invalid parent" ) << "doesnt exist" << Item::Id( 404 ) << QStringList();
      Item::Id id = createTodo( tr( "parent1" ) );
      QVERIFY( id > -1 );
      QVERIFY( createTodo( tr( "child1" ),  tr( "parent1" ) ) > -1 );
      QVERIFY( createTodo( tr( "child2" ),  tr( "parent1" ) ) > -1 );
      QTest::newRow( "2 childs" ) << "parent1"
                                  << id << ( QStringList() <<  tr( "child1" ) << tr( "child2" ) );
    }

    void testChildIncidences()
    {
      QFETCH( QString, parentUid );
      QFETCH( Akonadi::Item::Id, parentId );
      QFETCH( QStringList, childrenUids );
      KCalCore::Incidence::List childs = mCalendar->childIncidences( parentId );
      QVERIFY( compareUids( childrenUids, childs ) );
      childs = mCalendar->childIncidences( parentUid );
      QVERIFY( compareUids( childrenUids, childs ) );
    }

    void testDelete()
    { // No need for _data()
      const Item event = mCalendar->item( mOneEventUid );
      QVERIFY( event.isValid() );
      const Item todo = mCalendar->item( mOneTodoUid );
      QVERIFY( todo.isValid() );
      const Item journal = mCalendar->item( mOneJournalUid );
      QVERIFY( journal.isValid() );
      const Item incidence = mCalendar->item( mOneIncidenceUid );
      QVERIFY( incidence.isValid() );

      mExpectedSlotResult = true;
      QVERIFY( mCalendar->deleteEvent( event.payload<KCalCore::Event::Ptr>() ) );
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );

      QVERIFY( mCalendar->deleteTodo( todo.payload<KCalCore::Todo::Ptr>() ) );
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );

      QVERIFY( mCalendar->deleteJournal( journal.payload<KCalCore::Journal::Ptr>() ) );
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );

      QVERIFY( mCalendar->deleteIncidence( incidence.payload<KCalCore::Incidence::Ptr>() ) );
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );

      ItemFetchJob *job1 = new ItemFetchJob( event, this );
      ItemFetchJob *job2 = new ItemFetchJob( todo, this );
      ItemFetchJob *job3 = new ItemFetchJob( journal, this );
      ItemFetchJob *job4 = new ItemFetchJob( incidence, this );
      QVERIFY( !job1->exec() );
      QVERIFY( !job2->exec() );
      QVERIFY( !job3->exec() );
      QVERIFY( !job4->exec() );
      QVERIFY( mCalendar->item( event.id() ) == Item() );
      QVERIFY( mCalendar->item( todo.id() ) == Item() );
      QVERIFY( mCalendar->item( journal.id() ) == Item() );
      QVERIFY( mCalendar->item( incidence.id() ) == Item() );
    }

    void testDeleteAll()
    { // No need for _data()
      mCalendar->deleteAllEvents();
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );
      QVERIFY( mCalendar->events().isEmpty() );
      QVERIFY( !mCalendar->journals().isEmpty() );
      QVERIFY( !mCalendar->todos().isEmpty() );

      mCalendar->deleteAllTodos();
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );
      QVERIFY( mCalendar->events().isEmpty() );
      QVERIFY( !mCalendar->journals().isEmpty() );
      QVERIFY( mCalendar->todos().isEmpty() );

      mCalendar->deleteAllJournals();
      QTestEventLoop::instance().enterLoop( 5 );
      QVERIFY( !QTestEventLoop::instance().timeout() );
      QVERIFY( mCalendar->events().isEmpty() );
      QVERIFY( mCalendar->journals().isEmpty() );
      QVERIFY( mCalendar->todos().isEmpty() );

      QVERIFY( mCalendar->incidences().isEmpty() );

      foreach( const QString &uid, mUids ) {
        QCOMPARE( mCalendar->item( uid ), Item() );
      }
    }

public Q_SLOTS:
    void handleCreateFinished( bool success, const QString &errorString )
    {
      if ( !success )
        qDebug() << "handleCreateFinished(): " << errorString;
      QCOMPARE( success, mExpectedSlotResult );
      QTestEventLoop::instance().exitLoop();
    }

    void handleDeleteFinished( bool success, const QString &errorString )
    {
      if ( !success )
        qDebug() << "handleDeleteFinished(): " << errorString;
      QCOMPARE( success, mExpectedSlotResult );
      QTestEventLoop::instance().exitLoop();
    }
private:
  Item::Id createTodo( const QString &uid, const QString &parentUid = QString() )
  {
    Todo::Ptr todo = Todo::Ptr( new Todo() );
    todo->setUid( uid );
    todo->setSummary( QLatin1String( "summary" ) );
    if ( !parentUid.isEmpty() ) {
      todo->setRelatedTo( parentUid );
    }
    mCalendar->addTodo( todo );
    QTestEventLoop::instance().enterLoop( 5 );
    //QVERIFY( !QTestEventLoop::instance().timeout() );

    return mCalendar->item( uid ).id();
  }
};

QTEST_AKONADIMAIN( CalendarBaseTest, GUI )

#include "calendarbasetest.moc"
