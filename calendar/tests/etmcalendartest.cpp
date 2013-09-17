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

#include "etmcalendartest.h"

#include "../etmcalendar.h"
#include <akonadi/itemcreatejob.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemmodifyjob.h>
#include <KCheckableProxyModel>

#include <QTestEventLoop>
#include <QSignalSpy>

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE( QSet<QByteArray> )


void ETMCalendarTest::createIncidence( const QString &uid )
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

void ETMCalendarTest::fetchCollection()
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

void ETMCalendarTest:: initTestCase()
{
    AkonadiTest::checkTestIsIsolated();

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

void ETMCalendarTest::cleanupTestCase()
{
    delete mCalendar;
}

void ETMCalendarTest::testCollectionChanged_data()
{
    QTest::addColumn<Akonadi::Collection>( "noRightsCollection" );
    Collection noRightsCollection = mCollection;
    noRightsCollection.setRights( Collection::Rights( Collection::CanCreateItem ) );
    QTest::newRow( "change rights" ) << noRightsCollection;
}

void ETMCalendarTest::testCollectionChanged()
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

void ETMCalendarTest::testIncidencesAdded()
{
    // Already tested above.
}

void ETMCalendarTest::testIncidencesModified()
{
    const QString uid = tr( "d" );
    const Item item = mCalendar->item( uid );
    QVERIFY( item.isValid() );
    QVERIFY( item.hasPayload() );
    item.payload<KCalCore::Incidence::Ptr>()->setSummary( tr( "foo33" ) );
    ItemModifyJob *job = new ItemModifyJob( item );
    AKVERIFYEXEC(job);
    mIncidencesToChange = 1;
    QTestEventLoop::instance().enterLoop( 10 );
    QVERIFY( !QTestEventLoop::instance().timeout() );
    QCOMPARE( mLastModifiedUid, uid );
    QCOMPARE( mCalendar->incidence( uid )->summary(), tr( "foo33" ) );
}

void ETMCalendarTest::testIncidencesDeleted()
{
    Event::List incidences = mCalendar->events();
    QCOMPARE( incidences.count(), 6 );
    const Item item = mCalendar->item( tr( "a" ) );
    QVERIFY( item.isValid() );
    QVERIFY( item.hasPayload() );
    ItemDeleteJob *job = new ItemDeleteJob( item );
    AKVERIFYEXEC(job);
    mIncidencesToDelete = 1;
    QTestEventLoop::instance().enterLoop( 10 );
    QVERIFY( !QTestEventLoop::instance().timeout() );
    QCOMPARE( mLastDeletedUid, tr( "a" ) );
    QVERIFY( !mCalendar->item( tr( "a" ) ).isValid() );
}

void ETMCalendarTest::testFilteredModel()
{
    QVERIFY( mCalendar->model() );
}

void ETMCalendarTest::testUnfilteredModel()
{
    QVERIFY( mCalendar->entityTreeModel() );
}

void ETMCalendarTest::testCheckableProxyModel()
{
    QVERIFY( mCalendar->checkableProxyModel() );
}

void ETMCalendarTest::testUnselectCollection()
{
    mIncidencesToAdd = mIncidencesToDelete = mCalendar->incidences().count();
    const int originalToDelete = mIncidencesToDelete;
    KCheckableProxyModel *checkable = mCalendar->checkableProxyModel();
    const QModelIndex firstIndex = checkable->index( 0, 0 );
    QVERIFY( firstIndex.isValid() );
    checkable->setData( firstIndex, Qt::Unchecked, Qt::CheckStateRole );

    if ( mIncidencesToDelete > 0 ) { // Actually they probably where deleted already
        //doesn't need the event loop, but just in case
        QTestEventLoop::instance().enterLoop( 10 );

        if ( QTestEventLoop::instance().timeout() ) {
            qDebug() << originalToDelete << mIncidencesToDelete;
            QVERIFY( false );
        }
    }
}

void ETMCalendarTest::testSelectCollection()
{
    KCheckableProxyModel *checkable = mCalendar->checkableProxyModel();
    const QModelIndex firstIndex = checkable->index( 0, 0 );
    QVERIFY( firstIndex.isValid() );
    checkable->setData( firstIndex, Qt::Checked, Qt::CheckStateRole );

    if ( mIncidencesToDelete > 0 ) {
        QTestEventLoop::instance().enterLoop( 10 );
        QVERIFY( !QTestEventLoop::instance().timeout() );
    }
}


void ETMCalendarTest::calendarIncidenceAdded( const Incidence::Ptr &incidence )
{
    Q_UNUSED( incidence );
    Q_ASSERT( false );
    --mIncidencesToAdd;
    if ( mIncidencesToAdd == 0 ) {
        QTestEventLoop::instance().exitLoop();
    }
}

void ETMCalendarTest::handleCollectionsAdded( const Akonadi::Collection::List & )
{
    QTestEventLoop::instance().exitLoop();
}

void ETMCalendarTest::calendarIncidenceChanged( const Incidence::Ptr &incidence )
{
    --mIncidencesToChange;
    mLastModifiedUid = incidence->uid();
    if ( mIncidencesToChange == 0 ) {
        QTestEventLoop::instance().exitLoop();
    }
}


void ETMCalendarTest::calendarIncidenceDeleted( const Incidence::Ptr &incidence )
{
    --mIncidencesToDelete;
    mLastDeletedUid = incidence->uid();
    if ( mIncidencesToDelete == 0 ) {
        QTestEventLoop::instance().exitLoop();
    }
}


QTEST_AKONADIMAIN( ETMCalendarTest, GUI )
