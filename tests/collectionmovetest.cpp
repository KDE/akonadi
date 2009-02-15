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

using namespace Akonadi;

static Collection findCol( const Collection::List &list, const QString &name ) {
  foreach ( const Collection &col, list )
    if ( col.name() == name )
      return col;
    return Collection();
}

class CollectionMoveTest : public QObject
{
  Q_OBJECT
  private slots:
    void testIllegalMove_data()
    {
      QTest::addColumn<Collection>( "source" );
      QTest::addColumn<Collection>( "destination" );
      
      const Collection res1( collectionIdFromPath( "res1" ) );
      QVERIFY( res1.isValid() );
      const Collection res1foo( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( res1foo.isValid() );
      
      QTest::newRow( "non-existing-target" ) << res1 << Collection( INT_MAX );
      QTest::newRow( "root" ) << Collection::root() << res1;
      QTest::newRow( "move-into-child" ) << res1 << res1foo;
    }
    
    void testIllegalMove()
    {
      QFETCH( Collection, source );
      QFETCH( Collection, destination );
      
      source.setParent( destination );
      CollectionModifyJob* mod = new CollectionModifyJob( source, this );
      QVERIFY( !mod->exec() );
    }

    void testMove()
    {
      const Collection res1( collectionIdFromPath( "res1" ) );
      QVERIFY( res1.isValid() );
      const Collection res2( collectionIdFromPath( "res2" ) );
      QVERIFY( res2.isValid() );

      Collection col( res1 );
      col.setParent( res2 );
      CollectionModifyJob *mod = new CollectionModifyJob( col, this );
      QVERIFY( mod->exec() );
      
      CollectionFetchJob *ljob = new CollectionFetchJob( Collection( res2 ), CollectionFetchJob::Recursive );
      QVERIFY( ljob->exec() );
      Collection::List list = ljob->collections();
      
      QCOMPARE( list.count(), 7 );
      QVERIFY( findCol( list, "res1" ).isValid() );
      QVERIFY( findCol( list, "foo" ).isValid() );
      QVERIFY( findCol( list, "bar" ).isValid() );
      QVERIFY( findCol( list, "bla" ).isValid() );
      
      ljob = new CollectionFetchJob( res1, CollectionFetchJob::Base );
      QVERIFY( ljob->exec() );
      list = ljob->collections();
      
      QCOMPARE( list.count(), 1 );
      col = list.first();
      QCOMPARE( col.name(), QLatin1String("res1") );
      QCOMPARE( col.parent(), res2.id() );
      
      // cleanup
      col.setParent( Collection::root() );
      mod = new CollectionModifyJob( col, this );
      QVERIFY( mod->exec() );
    }
};

QTEST_AKONADIMAIN( CollectionMoveTest, NoGUI )

#include "collectionmovetest.moc"
