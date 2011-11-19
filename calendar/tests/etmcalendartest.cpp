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

#include "../etmcalendar.h"
#include <Akonadi/ItemCreateJob>
#include <akonadi/qtest_akonadi.h>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionModifyJob>
#include <KCheckableProxyModel>
#include <QTestEventLoop>
#include <QSignalSpy>
using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE( QSet<QByteArray> )

class ETMCalendarTest : public QObject, KCalCore::Calendar::CalendarObserver
{
  Q_OBJECT
    ETMCalendar *mCalendar;
    Collection mCollection;
    int mIncidencesToAdd;

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

private Q_SLOTS:
    void initTestCase()
    {
      qRegisterMetaType<QSet<QByteArray> >("QSet<QByteArray>");
      fetchCollection();
      createIncidence( tr( "a" ) );
      createIncidence( tr( "b" ) );
      createIncidence( tr( "c" ) );
      createIncidence( tr( "d" ) );
      createIncidence( tr( "e" ) );
      createIncidence( tr( "f" ) );
      mCalendar = new ETMCalendar();
      connect( mCalendar, SIGNAL(collectionsAdded(Akonadi::Collection::List)),
               SLOT(handleCollectionsAdded(Akonadi::Collection::List)) );
      mIncidencesToAdd = 6;
      mCalendar->registerObserver( this );

      // Wait for the collection
      QTestEventLoop::instance().enterLoop( 10 );
      QVERIFY( !QTestEventLoop::instance().timeout() );

      KCheckableProxyModel *checkable = mCalendar->checkableProxyModel();
      const QModelIndex firstIndex = checkable->index( 0, 0 );
      QVERIFY( firstIndex.isValid() );
      checkable->setData( firstIndex, Qt::Checked, Qt::CheckStateRole );

      // Now wait for incidences
      QTestEventLoop::instance().enterLoop( 10 );
      QVERIFY( !QTestEventLoop::instance().timeout() );
    }

    void cleanupTestCase()
    {
      delete mCalendar;
    }

    void testCollectionChanged_data()
    {
      QTest::addColumn<Akonadi::Collection>( "noRightsCollection" );
      Collection noRightsCollection = mCollection;
      noRightsCollection.setRights( Collection::Rights( Collection::CanCreateItem ) );
      QTest::newRow( "change rights" ) << noRightsCollection;
    }

    void testCollectionChanged()
    {
      QFETCH( Akonadi::Collection, noRightsCollection );
      CollectionModifyJob *job = new CollectionModifyJob( mCollection, this );
      QSignalSpy spy( mCalendar, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)) );
      connect( mCalendar, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)),
               &QTestEventLoop::instance(), SLOT(exitLoop()) );
      AKVERIFYEXEC(job);
      QTestEventLoop::instance().enterLoop( 10 );
      QVERIFY( !QTestEventLoop::instance().timeout() );
      QCOMPARE( spy.count(), 1 );
      QCOMPARE( spy.at(0).count(), 2 );
      QCOMPARE( spy.at(0).at(0).value<Akonadi::Collection>(), mCollection );
      QVERIFY( spy.at(0).at(1).value<QSet<QByteArray> >().contains(QByteArray( "AccessRights" ) ) );
    }

    void testFilteredModel()
    {
      QVERIFY( mCalendar->filteredModel() );
    }

    void testUnfilteredModel()
    {
      QVERIFY( mCalendar->unfilteredModel() );
    }

    void testCheckableProxyModel()
    {
        QVERIFY( mCalendar->checkableProxyModel() );
    }

public Q_SLOTS:
  /** reimp */
  void calendarIncidenceAdded( const Incidence::Ptr &incidence )
  {
    Q_UNUSED( incidence );
    Q_ASSERT( false );
    --mIncidencesToAdd;
    if ( mIncidencesToAdd == 0 ) {
      QTestEventLoop::instance().exitLoop();
    }
  }

  void handleCollectionsAdded( const Akonadi::Collection::List & )
  {
    QTestEventLoop::instance().exitLoop();
  }

  /** reimp */
  void calendarIncidenceChanged( const Incidence::Ptr &incidence )
  {
    Q_UNUSED( incidence );
  }

  /** reimp */
  void calendarIncidenceDeleted( const Incidence::Ptr &incidence )
  {
    Q_UNUSED( incidence );
  }
};

QTEST_AKONADIMAIN( ETMCalendarTest, GUI )

#include "etmcalendartest.moc"
