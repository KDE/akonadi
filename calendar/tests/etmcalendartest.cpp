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
#include <akonadi/itemdeletejob.h>
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


void ETMCalendarTest::createIncidence(const QString &uid)
{
    Item item;
    item.setMimeType( Event::eventMimeType() );
    Incidence::Ptr incidence = Incidence::Ptr( new Event() );
    incidence->setUid( uid );
    incidence->setDtStart( KDateTime::currentDateTime( KDateTime::UTC ) );
    incidence->setSummary( QLatin1String( "summary" ) );
    item.setPayload<KCalCore::Incidence::Ptr>( incidence );
    ItemCreateJob *job = new ItemCreateJob( item, mCollection, this );
    AKVERIFYEXEC( job );
}

void ETMCalendarTest::createTodo(const QString &uid, const QString &parentUid)
{
    Item item;
    item.setMimeType(Todo::todoMimeType());
    Todo::Ptr todo = Todo::Ptr(new Todo());
    todo->setUid(uid);

    todo->setRelatedTo(parentUid);

    todo->setSummary(QLatin1String("summary"));

    item.setPayload<KCalCore::Incidence::Ptr>(todo);
    ItemCreateJob *job = new ItemCreateJob(item, mCollection, this);
    mIncidencesToAdd++;
    mIncidencesToChange++;
    AKVERIFYEXEC(job);
}

void ETMCalendarTest::deleteIncidence(const QString &uid)
{
    ItemDeleteJob *job = new ItemDeleteJob(mCalendar->item(uid));
    mIncidencesToDelete++;
    AKVERIFYEXEC(job);
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
    mIncidencesToAdd = 0;
    mIncidencesToChange = 0;
    mIncidencesToDelete = 0;

    qRegisterMetaType<QSet<QByteArray> >("QSet<QByteArray>");
    fetchCollection();

    mCalendar = new ETMCalendar();
    connect( mCalendar, SIGNAL(collectionsAdded(Akonadi::Collection::List)),
             SLOT(handleCollectionsAdded(Akonadi::Collection::List)) );

    mCalendar->registerObserver( this );

    // Wait for the collection
    QTestEventLoop::instance().enterLoop( 10 );
    QVERIFY( !QTestEventLoop::instance().timeout() );

    KCheckableProxyModel *checkable = mCalendar->checkableProxyModel();
    const QModelIndex firstIndex = checkable->index( 0, 0 );
    QVERIFY( firstIndex.isValid() );
    checkable->setData( firstIndex, Qt::Checked, Qt::CheckStateRole );

    mIncidencesToAdd = 6;
    createIncidence( tr( "a" ) );
    createIncidence( tr( "b" ) );
    createIncidence( tr( "c" ) );
    createIncidence( tr( "d" ) );
    createIncidence( tr( "e" ) );
    createIncidence( tr( "f" ) );

    // Wait for incidences
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
    mIncidencesToChange = 6;
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
    mIncidencesToChange = 1;
    AKVERIFYEXEC(job);
    QTestEventLoop::instance().enterLoop( 10 );
    QVERIFY( !QTestEventLoop::instance().timeout() );
    QCOMPARE( mCalendar->incidence( uid )->summary(), tr( "foo33" ) );
    QVERIFY( item.revision() == mCalendar->item( item.id() ).revision() - 1 );
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
    Q_ASSERT( incidence );

    const QString id = incidence->customProperty( "VOLATILE", "AKONADI-ID" );
    QCOMPARE( id.toLongLong(), mCalendar->item( incidence->uid() ).id() );

    --mIncidencesToAdd;
    checkExitLoop();
}

void ETMCalendarTest::handleCollectionsAdded( const Akonadi::Collection::List & )
{
    QTestEventLoop::instance().exitLoop();
}

void ETMCalendarTest::calendarIncidenceChanged( const Incidence::Ptr &incidence )
{
    const QString id = incidence->customProperty( "VOLATILE", "AKONADI-ID" );
    QCOMPARE( id.toLongLong(), mCalendar->item( incidence->uid() ).id() );

    Akonadi::Item item = mCalendar->item(incidence->uid());
    Incidence::Ptr i2 = item.payload<KCalCore::Incidence::Ptr>();
    QCOMPARE(i2->summary(), incidence->summary());
    Incidence::Ptr i3 = mCalendar->incidence(incidence->uid());
    QCOMPARE(i3->summary(), incidence->summary());

    --mIncidencesToChange;

    checkExitLoop();
}

void ETMCalendarTest::calendarIncidenceDeleted( const Incidence::Ptr &incidence )
{
    const QString id = incidence->customProperty( "VOLATILE", "AKONADI-ID" );
    QVERIFY( !id.isEmpty() );

    --mIncidencesToDelete;
    mLastDeletedUid = incidence->uid();
    checkExitLoop();
}

void ETMCalendarTest::testSubTodos()
{
    mIncidencesToAdd = 0;
    mIncidencesToChange = 0;
    mIncidencesToDelete = 0;

    createTodo(tr("ta"), QString());
    createTodo(tr("tb"), QString());
    createTodo(tr("tb.1"), tr("tb"));
    createTodo(tr("tb.1.1"), tr("tb.1"));
    createTodo(tr("tb.2"), tr("tb"));
    createTodo(tr("tb.3"), tr("tb"));
    waitForIt();

    QVERIFY(mCalendar->childIncidences(tr("ta")).isEmpty());
    QCOMPARE(mCalendar->childIncidences(tr("tb")).count(), 3);
    QCOMPARE(mCalendar->childIncidences(tr("tb.1")).count(), 1);
    QVERIFY(mCalendar->childIncidences(tr("tb.1.1")).isEmpty());
    QVERIFY(mCalendar->childIncidences(tr("tb.2")).isEmpty());

    // Kill a child
    deleteIncidence(tr("tb.3"));
    waitForIt();

    QCOMPARE(mCalendar->childIncidences(tr("tb")).count(), 2);
    QCOMPARE(mCalendar->childItems(tr("tb")).count(), 2);
    QVERIFY(!mCalendar->incidence(tr("tb.3")));

    // Move a top-level to-do to a new parent
    Incidence::Ptr ta = mCalendar->incidence(tr("ta"));
    Item ta_item = mCalendar->item(tr("ta"));
    ta_item.setPayload(ta);
    QVERIFY(ta);
    ta->setRelatedTo(tr("tb"));
    mIncidencesToChange = 1;

    ItemModifyJob *job = new ItemModifyJob(ta_item);
    AKVERIFYEXEC(job);
    waitForIt();

    QCOMPARE(mCalendar->childIncidences(tr("tb")).count(), 3);

    // Move it to another parent now
    ta = mCalendar->incidence(tr("ta"));
    ta_item = mCalendar->item(tr("ta"));
    ta->setRelatedTo(tr("tb.2"));
    ta_item.setPayload(ta);
    mIncidencesToChange = 1;
    job = new ItemModifyJob(ta_item);
    AKVERIFYEXEC(job);
    waitForIt();

    QCOMPARE(mCalendar->childIncidences(tr("tb")).count(), 2);
    QCOMPARE(mCalendar->childIncidences(tr("tb.2")).count(), 1);

    // Now unparent it
    ta = mCalendar->incidence(tr("ta"));
    ta_item = mCalendar->item(tr("ta"));
    ta->setRelatedTo(QString());
    ta_item.setPayload(ta);
    mIncidencesToChange = 1;
    job = new ItemModifyJob(ta_item);
    AKVERIFYEXEC(job);
    waitForIt();

    QCOMPARE(mCalendar->childIncidences(tr("tb")).count(), 2);
    QVERIFY(mCalendar->childIncidences(tr("tb.2")).isEmpty());
}

void ETMCalendarTest::testNotifyObserverBug()
{
    // When an observer's calendarIncidenceChanged(Incidence) method got called
    // and that observer then called calendar->item(incidence->uid()) the retrieved item would still
    // have the old payload, because CalendarBase::updateItem() was still on the stack
    // and would only update after calendarIncidenceChanged() returned.
    // This test ensure that doesnt happen.
    const QLatin1String uid("todo-notify-bug");
    createTodo(uid, QString());
    waitForIt();

    Akonadi::Item item = mCalendar->item(uid);
    KCalCore::Incidence::Ptr incidence = KCalCore::Incidence::Ptr(mCalendar->incidence(uid)->clone());
    QVERIFY(item.isValid());

    // Modify it
    mIncidencesToChange = 1;
    incidence->setSummary(QLatin1String("new-summary"));
    item.setPayload(incidence);
    ItemModifyJob *job = new ItemModifyJob(item);
    AKVERIFYEXEC(job);

    // The test will now happen inside ETMCalendarTest::calendarIncidenceChanged()
    waitForIt();
}

void ETMCalendarTest::testUidChange()
{
    const QLatin1String originalUid("original-uid");
    const QLatin1String newUid("new-uid");
    createTodo(originalUid, QString());
    waitForIt();

    KCalCore::Incidence::Ptr clone = Incidence::Ptr(mCalendar->incidence(originalUid)->clone());
    QCOMPARE(clone->uid(), originalUid);

    Akonadi::Item item = mCalendar->item(originalUid);
    clone->setUid(newUid);
    QVERIFY(item.isValid());
    item.setPayload(clone);
    mIncidencesToChange = 1;
    ItemModifyJob *job = new ItemModifyJob(item);
    AKVERIFYEXEC(job);

    waitForIt();

    // Check that stuff still works fine
    KCalCore::Incidence::Ptr incidence = mCalendar->incidence(originalUid);
    QVERIFY(!incidence);
    incidence = mCalendar->incidence(newUid);
    QCOMPARE(incidence->uid(), newUid);

    item = mCalendar->item(originalUid);
    QVERIFY(!item.isValid());

    item = mCalendar->item(newUid);
    QVERIFY(item.isValid());
}

void ETMCalendarTest::waitForIt()
{
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void ETMCalendarTest::checkExitLoop()
{
    //qDebug() << "checkExitLoop: current state: " << mIncidencesToDelete << mIncidencesToAdd << mIncidencesToChange;
    if (mIncidencesToDelete == 0 && mIncidencesToAdd == 0 && mIncidencesToChange == 0) {
        QTestEventLoop::instance().exitLoop();
    }
}

QTEST_AKONADIMAIN( ETMCalendarTest, GUI )
