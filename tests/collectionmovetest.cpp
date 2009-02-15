/*
    Copyright (c) 2006, 2009 Volker Krause <vkrause@kde.org>

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

#include "test_utils.h"

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QHash>

using namespace Akonadi;

class CollectionMoveTest : public QObject
{
  Q_OBJECT
  private slots:
    void testIllegalMove_data()
    {
      QTest::addColumn<Collection>( "source" );
      QTest::addColumn<Collection>( "destination" );

      const Collection res1( collectionIdFromPath( "res1" ) );
      const Collection res1foo( collectionIdFromPath( "res1/foo" ) );
      const Collection res1bla( collectionIdFromPath( "res1/foo/bar/bla" ) );

      QTest::newRow( "non-existing-target" ) << res1 << Collection( INT_MAX );
      QTest::newRow( "root" ) << Collection::root() << res1;
      QTest::newRow( "move-into-child" ) << res1 << res1foo;
      QTest::newRow( "same-name-in-target" ) << res1bla << res1foo;
      QTest::newRow( "non-existing-source" ) << Collection( INT_MAX ) << res1;
    }

    void testIllegalMove()
    {
      QFETCH( Collection, source );
      QFETCH( Collection, destination );
      QVERIFY( source.isValid() );
      QVERIFY( destination.isValid() );

      source.setParent( destination );
      CollectionModifyJob* mod = new CollectionModifyJob( source, this );
      QVERIFY( !mod->exec() );
    }

    void testMove_data()
    {
      QTest::addColumn<Collection>( "source" );
      QTest::addColumn<Collection>( "destination" );
      QTest::addColumn<bool>( "crossResource" );

      QTest::newRow( "inter-resource" ) << Collection( collectionIdFromPath( "res1" ) )
                                        << Collection( collectionIdFromPath( "res2" ) )
                                        << true;
      QTest::newRow( "intra-resource" ) << Collection( collectionIdFromPath( "res1/foo/bla" ) )
                                        << Collection( collectionIdFromPath( "res1/foo/bar/bla" ) )
                                        << false;
    }

    // TODO: test signals
    void testMove()
    {
      QFETCH( Collection, source );
      QFETCH( Collection, destination );
      QFETCH( bool, crossResource );
      QVERIFY( source.isValid() );
      QVERIFY( destination.isValid() );

      CollectionFetchJob *fetch = new CollectionFetchJob( source, CollectionFetchJob::Base, this );
      QVERIFY( fetch->exec() );
      QCOMPARE( fetch->collections().count(), 1 );
      source = fetch->collections().first();

      // obtain reference listing
      fetch = new CollectionFetchJob( source, CollectionFetchJob::Recursive );
      QVERIFY( fetch->exec() );
      QHash<Collection, Item::List> referenceData;
      foreach ( const Collection c, fetch->collections() ) {
        ItemFetchJob *job = new ItemFetchJob( c, this );
        QVERIFY( job->exec() );
        referenceData.insert( c, job->items() );
      }

      // move collection
      Collection col( source );
      col.setParent( destination );
      CollectionModifyJob *mod = new CollectionModifyJob( col, this );
      QVERIFY( mod->exec() );

      // check if source was modified correctly
      CollectionFetchJob *ljob = new CollectionFetchJob( source, CollectionFetchJob::Base );
      QVERIFY( ljob->exec() );
      Collection::List list = ljob->collections();

      QCOMPARE( list.count(), 1 );
      col = list.first();
      QCOMPARE( col.name(), source.name() );
      QCOMPARE( col.parent(), destination.id() );

      // list destination and check if everything is still there
      ljob = new CollectionFetchJob( destination, CollectionFetchJob::Recursive );
      QVERIFY( ljob->exec() );
      list = ljob->collections();

      QVERIFY( list.count() >= referenceData.count() );
      for ( QHash<Collection, Item::List>::ConstIterator it = referenceData.constBegin(); it != referenceData.constEnd(); ++it ) {
        QVERIFY( list.contains( it.key() ) );
        if ( crossResource ) {
          QVERIFY( list[ list.indexOf( it.key() ) ].resource() != it.key().resource() );
        } else {
          QCOMPARE( list[ list.indexOf( it.key() ) ].resource(), it.key().resource() );
        }
        ItemFetchJob *job = new ItemFetchJob( it.key(), this );
        job->fetchScope().fetchFullPayload();
        QVERIFY( job->exec() );
        QCOMPARE( job->items().count(), it.value().count() );
        foreach ( const Item item, job->items() ) {
          QVERIFY( it.value().contains( item ) );
          QEXPECT_FAIL( "inter-resource", "server bug", Continue );
          QVERIFY( item.hasPayload() );
        }
      }

      // cleanup
      col.setParent( source.parent() );
      mod = new CollectionModifyJob( col, this );
      QVERIFY( mod->exec() );
    }
};

QTEST_AKONADIMAIN( CollectionMoveTest, NoGUI )

#include "collectionmovetest.moc"
