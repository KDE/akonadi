/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "monitortest.h"
#include "test_utils.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/monitor.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/control.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemmovejob.h>
#include <akonadi/searchcreatejob.h>

#include <QtCore/QVariant>
#include <QtGui/QApplication>
#include <QtTest/QSignalSpy>
#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN( MonitorTest, NoGUI )

static Collection res3;

Q_DECLARE_METATYPE(Akonadi::Collection::Id)
Q_DECLARE_METATYPE(QSet<QByteArray>)

void MonitorTest::initTestCase()
{
  Control::start();

  res3 = Collection( collectionIdFromPath( "res3" ) );

  // switch all resources offline to reduce interference from them
  foreach ( Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances() )
    agent.setIsOnline( false );
}

void MonitorTest::testMonitor_data()
{
  QTest::addColumn<bool>( "fetchCol" );
  QTest::newRow( "with collection fetching" ) << true;
  QTest::newRow( "without collection fetching" ) << false;
}

void MonitorTest::testMonitor()
{
  QFETCH( bool, fetchCol );

  Monitor *monitor = new Monitor( this );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->fetchCollection( fetchCol );
  monitor->itemFetchScope().fetchFullPayload();

  // monitor signals
  qRegisterMetaType<Akonadi::Collection>();
  qRegisterMetaType<Akonadi::Collection::Id>();
  qRegisterMetaType<Akonadi::Item>();
  qRegisterMetaType<Akonadi::CollectionStatistics>();
  qRegisterMetaType<QSet<QByteArray> >();
  QSignalSpy caddspy( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  QSignalSpy cmodspy( monitor, SIGNAL(collectionChanged(const Akonadi::Collection&)) );
  QSignalSpy crmspy( monitor, SIGNAL(collectionRemoved(const Akonadi::Collection&)) );
  QSignalSpy cstatspy( monitor, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );
  QSignalSpy iaddspy( monitor, SIGNAL(itemAdded(const Akonadi::Item&, const Akonadi::Collection&)) );
  QSignalSpy imodspy( monitor, SIGNAL(itemChanged(const Akonadi::Item&, const QSet<QByteArray>&)) );
  QSignalSpy imvspy( monitor, SIGNAL(itemMoved(const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection&)) );
  QSignalSpy irmspy( monitor, SIGNAL(itemRemoved(const Akonadi::Item&)) );

  QVERIFY( caddspy.isValid() );
  QVERIFY( cmodspy.isValid() );
  QVERIFY( crmspy.isValid() );
  QVERIFY( cstatspy.isValid() );
  QVERIFY( iaddspy.isValid() );
  QVERIFY( imodspy.isValid() );
  QVERIFY( imvspy.isValid() );
  QVERIFY( irmspy.isValid() );

  // create a collection
  Collection monitorCol;
  monitorCol.setParent( res3 );
  monitorCol.setName( "monitor" );
  CollectionCreateJob *create = new CollectionCreateJob( monitorCol, this );
  QVERIFY( create->exec() );
  monitorCol = create->collection();
  QVERIFY( monitorCol.isValid() );
  QTest::qWait(1000); // make sure the DBus signal has been processed

  QCOMPARE( caddspy.count(), 1 );
  QList<QVariant> arg = caddspy.takeFirst();
  Collection col = arg.at(0).value<Collection>();
  QCOMPARE( col, monitorCol );
  if ( fetchCol )
    QCOMPARE( col.name(), QString("monitor") );
  Collection parent = arg.at(1).value<Collection>();
  QCOMPARE( parent, res3 );

  QVERIFY( cmodspy.isEmpty() );
  QVERIFY( crmspy.isEmpty() );
  QVERIFY( cstatspy.isEmpty() );
  QVERIFY( iaddspy.isEmpty() );
  QVERIFY( imodspy.isEmpty() );
  QVERIFY( imvspy.isEmpty() );
  QVERIFY( irmspy.isEmpty() );

  // add an item
  Item newItem;
  newItem.setMimeType( "application/octet-stream" );
  ItemCreateJob *append = new ItemCreateJob( newItem, monitorCol, this );
  QVERIFY( append->exec() );
  Item monitorRef = append->item();
  QVERIFY( monitorRef.isValid() );
  QTest::qWait(1000);

  QCOMPARE( cstatspy.count(), 1 );
  arg = cstatspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Akonadi::Collection::Id>(), monitorCol.id() );

  /*
     qRegisterMetaType<Akonadi::Collection::Id>() registers the type with a
     name of "qlonglong".  Doing
     qRegisterMetaType<Akonadi::Collection::Id>( "Akonadi::Collection::Id" ) 
     doesn't help.

     The problem here is that Akonadi::Collection::Id is a typedef to qlonglong,
     and qlonglong is already a registered meta type.  So the signal spy will
     give us a QVariant of type Akonadi::Collection::Id, but calling
     .value<Akonadi::Collection::Id>() on that variant will in fact end up
     calling qvariant_cast<qlonglong>.  From the point of view of QMetaType,
     Akonadi::Collection::Id and qlonglong are different types, so QVariant
     can't convert, and returns a default-constructed qlonglong, zero.

     When connecting to a real slot (without QSignalSpy), this problem is
     avoided, because the casting is done differently (via a lot of void
     pointers).

     The docs say nothing about qRegisterMetaType -ing a typedef, so I'm not
     sure if this is a bug or not. (cberzan)
   */

  QCOMPARE( iaddspy.count(), 1 );
  arg = iaddspy.takeFirst();
  Item item = arg.at( 0 ).value<Item>();
  QCOMPARE( item, monitorRef );
  QCOMPARE( item.mimeType(), QString::fromLatin1(  "application/octet-stream" ) );
  Collection collection = arg.at( 1 ).value<Collection>();
  QCOMPARE( collection.id(), monitorCol.id() );

  QVERIFY( caddspy.isEmpty() );
  QVERIFY( cmodspy.isEmpty() );
  QVERIFY( crmspy.isEmpty() );
  QVERIFY( imodspy.isEmpty() );
  QVERIFY( imvspy.isEmpty() );
  QVERIFY( irmspy.isEmpty() );

  // modify an item
  item.setPayload<QByteArray>( "some new content" );
  ItemModifyJob *store = new ItemModifyJob( item, this );
  QVERIFY( store->exec() );
  QTest::qWait(1000);

  QCOMPARE( cstatspy.count(), 1 );
  arg = cstatspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Collection::Id>(), monitorCol.id() );

  QCOMPARE( imodspy.count(), 1 );
  arg = imodspy.takeFirst();
  item = arg.at( 0 ).value<Item>();
  QCOMPARE( monitorRef, item );
  QCOMPARE( item.payload<QByteArray>(), QByteArray( "some new content" ) );
  QSet<QByteArray> parts = arg.at( 1 ).value<QSet<QByteArray> >();
  QCOMPARE( parts, QSet<QByteArray>() << "PLD:RFC822" );

  QVERIFY( caddspy.isEmpty() );
  QVERIFY( cmodspy.isEmpty() );
  QVERIFY( crmspy.isEmpty() );
  QVERIFY( iaddspy.isEmpty() );
  QVERIFY( imvspy.isEmpty() );
  QVERIFY( irmspy.isEmpty() );

  // move an item
  ItemMoveJob *move = new ItemMoveJob( item, res3 );
  QVERIFY( move->exec() );
  QTest::qWait( 1000 );

  QCOMPARE( cstatspy.count(), 1 );
  arg = cstatspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Collection::Id>(), monitorCol.id() );

  QCOMPARE( imvspy.count(), 1 );
  arg = imvspy.takeFirst();
  item = arg.at( 0 ).value<Item>(); // the item
  QCOMPARE( monitorRef, item );
  col = arg.at( 1 ).value<Collection>(); // the source collection
  QCOMPARE( col.id(), monitorCol.id() );
  col = arg.at( 2 ).value<Collection>(); // the destination collection
  QCOMPARE( col.id(), res3.id() );

  QCOMPARE( cmodspy.count(), 2 );
  arg = cmodspy.takeFirst();
  Collection col1 = arg.at( 0 ).value<Collection>();
  arg = cmodspy.takeFirst();
  Collection col2 = arg.at( 0 ).value<Collection>();
  // source and dest collections, in any order
  QVERIFY( ( col1.id() == monitorCol.id() && col2.id() == res3.id() ) ||
           ( col2.id() == monitorCol.id() && col1.id() == res3.id() ) );

  QVERIFY( caddspy.isEmpty() );
  QVERIFY( crmspy.isEmpty() );
  QVERIFY( iaddspy.isEmpty() );
  QVERIFY( imodspy.isEmpty() );
  QVERIFY( irmspy.isEmpty() );

  // delete an item
  ItemDeleteJob *del = new ItemDeleteJob( monitorRef, this );
  QVERIFY( del->exec() );
  QTest::qWait(1000);

  QCOMPARE( cstatspy.count(), 1 );
  arg = cstatspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Collection::Id>(), monitorCol.id() );
  cmodspy.clear();

  QCOMPARE( irmspy.count(), 1 );
  arg = irmspy.takeFirst();
  Item ref = qvariant_cast<Item>( arg.at(0) );
  QCOMPARE( monitorRef, ref );

  QVERIFY( caddspy.isEmpty() );
  QVERIFY( cmodspy.isEmpty() );
  QVERIFY( crmspy.isEmpty() );
  QVERIFY( iaddspy.isEmpty() );
  QVERIFY( imodspy.isEmpty() );
  QVERIFY( imvspy.isEmpty() );
  imvspy.clear();

  // modify a collection
  monitorCol.setName( "changed name" );
  CollectionModifyJob *mod = new CollectionModifyJob( monitorCol, this );
  QVERIFY( mod->exec() );
  QTest::qWait(1000);

  QCOMPARE( cmodspy.count(), 1 );
  arg = cmodspy.takeFirst();
  col = arg.at(0).value<Collection>();
  QCOMPARE( col, monitorCol );
  if ( fetchCol )
    QCOMPARE( col.name(), QString("changed name") );

  QVERIFY( caddspy.isEmpty() );
  QVERIFY( crmspy.isEmpty() );
  QVERIFY( cstatspy.isEmpty() );
  QVERIFY( iaddspy.isEmpty() );
  QVERIFY( imodspy.isEmpty() );
  QVERIFY( imvspy.isEmpty() );
  QVERIFY( irmspy.isEmpty() );

  // delete a collection
  CollectionDeleteJob *cdel = new CollectionDeleteJob( monitorCol, this );
  QVERIFY( cdel->exec() );
  QTest::qWait(1000);

  QCOMPARE( crmspy.count(), 1 );
  arg = crmspy.takeFirst();
  QCOMPARE( arg.at(0).value<Collection>().id(), monitorCol.id() );

  QVERIFY( caddspy.isEmpty() );
  QVERIFY( cmodspy.isEmpty() );
  QVERIFY( cstatspy.isEmpty() );
  QVERIFY( iaddspy.isEmpty() );
  QVERIFY( imodspy.isEmpty() );
  QVERIFY( imvspy.isEmpty() );
  QVERIFY( irmspy.isEmpty() );
}

void MonitorTest::testVirtualCollectionsMonitoring()
{
  Monitor *monitor = new Monitor( this );
  monitor->setCollectionMonitored( Collection( 1 ) );   // top-level 'Search' collection

  QSignalSpy caddspy( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  QVERIFY( caddspy.isValid() );

  SearchCreateJob *job = new SearchCreateJob( "Test search collection", "test-search-query" );
  QVERIFY( job->exec() );
  QTest::qWait( 1000 );
  QCOMPARE( caddspy.count(), 1 );
}

#include "monitortest.moc"
