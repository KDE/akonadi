/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/linkjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/unlinkjob.h>
#include <akonadi/monitor.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/searchcreatejob.h>

#include <QtCore/QObject>

#include <qtest_akonadi.h>

using namespace Akonadi;

class LinkTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      Control::start();
    }

    void testLink()
    {
      SearchCreateJob *create = new SearchCreateJob( "linkTestFolder", "dummy query", this );
      AKVERIFYEXEC( create );

      CollectionFetchJob *list = new CollectionFetchJob( Collection( 1 ), CollectionFetchJob::Recursive, this );
      AKVERIFYEXEC( list );
      Collection col;
      foreach ( const Collection &c, list->collections() ) {
        if ( c.name() == "linkTestFolder" ) {
          col = c;
        }
      }
      QVERIFY( col.isValid() );

      Item::List items;
      items << Item( 3 ) << Item( 4 ) << Item( 6 );

      Monitor *monitor = new Monitor( this );
      monitor->setCollectionMonitored( col );
      monitor->itemFetchScope().fetchFullPayload();

      qRegisterMetaType<Akonadi::Collection>();
      qRegisterMetaType<Akonadi::Item>();
      QSignalSpy lspy( monitor, SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)) );
      QSignalSpy uspy( monitor, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)) );
      QVERIFY( lspy.isValid() );
      QVERIFY( uspy.isValid() );

      LinkJob *link = new LinkJob( col, items, this );
      AKVERIFYEXEC( link );

      QTRY_COMPARE( lspy.count(), 3 );
      QTest::qWait( 100 );
      QVERIFY( uspy.isEmpty() );

      QList<QVariant> arg = lspy.takeFirst();
      Item item = arg.at( 0 ).value<Item>();
      QCOMPARE( item.mimeType(), QString::fromLatin1(  "application/octet-stream" ) );
      QVERIFY( item.hasPayload<QByteArray>() );

      lspy.clear();

      ItemFetchJob *fetch = new ItemFetchJob( col );
      AKVERIFYEXEC( fetch );
      QCOMPARE( fetch->items().count(), 3 );
      foreach ( const Item &item, fetch->items() ) {
        QVERIFY( items.contains( item ) );
      }

      UnlinkJob *unlink = new UnlinkJob( col, items, this );
      AKVERIFYEXEC( unlink );

      QTRY_COMPARE( uspy.count(), 3 );
      QTest::qWait( 100 );
      QVERIFY( lspy.isEmpty() );

      fetch = new ItemFetchJob( col );
      AKVERIFYEXEC( fetch );
      QCOMPARE( fetch->items().count(), 0 );
    }

};

QTEST_AKONADIMAIN( LinkTest, NoGUI )

#include "linktest.moc"
