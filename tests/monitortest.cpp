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

#include <libakonadi/monitor.h>
#include <libakonadi/collectioncreatejob.h>
#include <libakonadi/collectiondeletejob.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/control.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>

#include <QtCore/QVariant>
#include <QtGui/QApplication>
#include <QtTest/QSignalSpy>
#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( MonitorTest, NoGUI )

static Collection res3;

void MonitorTest::initTestCase()
{
  Control::start();

  // get the collections we run the tests on
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  foreach ( const Collection col, list )
    if ( col.name() == "res3" )
      res3 = col;
}

void MonitorTest::testMonitor()
{
  Monitor *monitor = new Monitor( this );
  monitor->monitorCollection( Collection::root() );
  monitor->fetchCollection( true );
  monitor->addFetchPart( ItemFetchJob::PartAll );

  // monitor signals
  qRegisterMetaType<Akonadi::DataReference>();
  qRegisterMetaType<Akonadi::Collection>();
  qRegisterMetaType<Akonadi::Item>();
  qRegisterMetaType<Akonadi::CollectionStatus>();
  QSignalSpy caspy( monitor, SIGNAL(collectionAdded(const Akonadi::Collection&)) );
  QSignalSpy cmspy( monitor, SIGNAL(collectionChanged(const Akonadi::Collection&)) );
  QSignalSpy crspy( monitor, SIGNAL(collectionRemoved(int,QString)) );
  QSignalSpy csspy( monitor, SIGNAL(collectionStatusChanged(int,Akonadi::CollectionStatus)) );
  QSignalSpy iaspy( monitor, SIGNAL(itemAdded(const Akonadi::Item&, const Akonadi::Collection&)) );
  QSignalSpy imspy( monitor, SIGNAL(itemChanged(const Akonadi::Item&, const QStringList&)) );
  QSignalSpy irspy( monitor, SIGNAL(itemRemoved(Akonadi::DataReference)) );

  QVERIFY( caspy.isValid() );
  QVERIFY( cmspy.isValid() );
  QVERIFY( crspy.isValid() );
  QVERIFY( csspy.isValid() );
  QVERIFY( iaspy.isValid() );
  QVERIFY( imspy.isValid() );
  QVERIFY( irspy.isValid() );

  // create a collection
  CollectionCreateJob *create = new CollectionCreateJob( res3, "monitor", this );
  QVERIFY( create->exec() );
  Collection monitorCol = create->collection();
  QVERIFY( monitorCol.isValid() );
  QTest::qWait(1000); // make sure the DBus signal has been processed

  QCOMPARE( caspy.count(), 1 );
  QList<QVariant> arg = caspy.takeFirst();
  Collection col = arg.at(0).value<Collection>();
  QCOMPARE( col, monitorCol );
  QCOMPARE( col.name(), QString("monitor") );

  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( csspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // add an item
  ItemAppendJob *append = new ItemAppendJob( monitorCol, "message/rfc822", this );
  QVERIFY( append->exec() );
  DataReference monitorRef = append->reference();
  QVERIFY( !monitorRef.isNull() );
  QTest::qWait(1000);

  QCOMPARE( csspy.count(), 1 );
  arg = csspy.takeFirst();
  QCOMPARE( arg.at(0).toInt(), monitorCol.id() );

  QCOMPARE( iaspy.count(), 1 );
  arg = iaspy.takeFirst();
  Item item = arg.at( 0 ).value<Item>();
  QCOMPARE( item.reference(), monitorRef );
  QCOMPARE( item.mimeType(), QString::fromLatin1(  "message/rfc822" ) );
  Collection collection = arg.at( 1 ).value<Collection>();
  QCOMPARE( collection.id(), monitorCol.id() );
  QCOMPARE( collection.remoteId(), monitorCol.remoteId() );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // modify an item
  ItemStoreJob *store = new ItemStoreJob( monitorRef, this );
  store->setData( QByteArray( "some new content" ) );
  QVERIFY( store->exec() );
  QTest::qWait(1000);

  QCOMPARE( csspy.count(), 1 );
  arg = csspy.takeFirst();
  QCOMPARE( arg.at(0).toInt(), monitorCol.id() );

  QCOMPARE( imspy.count(), 1 );
  arg = imspy.takeFirst();
  item = arg.at( 0 ).value<Item>();
  QCOMPARE( monitorRef, item.reference() );
  QCOMPARE( item.data(), QByteArray( "some new content" ) );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // delete an item
  ItemDeleteJob *del = new ItemDeleteJob( monitorRef, this );
  QVERIFY( del->exec() );
  QTest::qWait(1000);

  QCOMPARE( csspy.count(), 1 );
  arg = csspy.takeFirst();
  QCOMPARE( arg.at(0).toInt(), monitorCol.id() );
  cmspy.clear();

  QCOMPARE( irspy.count(), 1 );
  arg = irspy.takeFirst();
  DataReference ref = qvariant_cast<DataReference>( arg.at(0) );
  QCOMPARE( monitorRef, ref );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  imspy.clear();

  // delete a collection
  CollectionDeleteJob *cdel = new CollectionDeleteJob( monitorCol, this );
  QVERIFY( cdel->exec() );
  QTest::qWait(1000);

  QCOMPARE( crspy.count(), 1 );
  arg = crspy.takeFirst();
  QCOMPARE( arg.at(0).toInt(), monitorCol.id() );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( csspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );
}

#include "monitortest.moc"
