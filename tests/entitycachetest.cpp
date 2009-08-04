/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "entitycache.cpp"

#include <QSignalSpy>
#include <qtest_akonadi.h>

using namespace Akonadi;

class EntityCacheTest : public QObject
{
  Q_OBJECT
  private:
    template <typename T, typename FetchJob, typename FetchScope>
    void testCache()
    {
      EntityCache<T, FetchJob, FetchScope> cache( 2 );
      QSignalSpy spy( &cache, SIGNAL(dataAvailable()) );
      QVERIFY( spy.isValid() );

      QVERIFY( !cache.isCached( 1 ) );
      QVERIFY( !cache.isRequested( 1 ) );

      cache.request( 1, FetchScope() );
      QVERIFY( !cache.isCached( 1 ) );
      QVERIFY( cache.isRequested( 1 ) );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 1 );
      QVERIFY( cache.isCached( 1 ) );
      QVERIFY( cache.isRequested( 1 ) );
      const T e1 = cache.retrieve( 1 );
      QCOMPARE( e1.id(), 1ll );

      spy.clear();
      cache.request( 2, FetchScope() );
      cache.request( 3, FetchScope() );

      QVERIFY( !cache.isCached( 1 ) );
      QVERIFY( !cache.isRequested( 1 ) );
      QVERIFY( cache.isRequested( 2 ) );
      QVERIFY( cache.isRequested( 3 ) );
    }

  private slots:
    void testCacheGeneric_data()
    {
      QTest::addColumn<bool>( "collection" );
      QTest::newRow( "collection" ) << true;
      QTest::newRow( "item" ) << false;
    }

    void testCacheGeneric()
    {
      QFETCH( bool, collection );
      if ( collection )
        testCache<Collection, CollectionFetchJob, CollectionFetchScope>();
      else
        testCache<Item, ItemFetchJob, ItemFetchScope>();
    }
};

QTEST_AKONADIMAIN( EntityCacheTest, NoGUI )

#include "entitycachetest.moc"
