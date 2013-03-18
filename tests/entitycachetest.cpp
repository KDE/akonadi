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

#include "entitycache_p.h"

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
      QVERIFY( !cache.retrieve( 1 ).isValid() );

      FetchScope scope;
      scope.setAncestorRetrieval( FetchScope::All );

      cache.request( 1, scope );
      QVERIFY( !cache.isCached( 1 ) );
      QVERIFY( cache.isRequested( 1 ) );
      QVERIFY( !cache.retrieve( 1 ).isValid() );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 1 );
      QVERIFY( cache.isCached( 1 ) );
      QVERIFY( cache.isRequested( 1 ) );
      const T e1 = cache.retrieve( 1 );
      QCOMPARE( e1.id(), 1ll );
      QVERIFY( e1.parentCollection().isValid() );
      QVERIFY( !e1.parentCollection().remoteId().isEmpty() || e1.parentCollection() == Collection::root() );

      spy.clear();
      cache.request( 2, FetchScope() );
      cache.request( 3, FetchScope() );

      QVERIFY( !cache.isCached( 1 ) );
      QVERIFY( !cache.isRequested( 1 ) );
      QVERIFY( cache.isRequested( 2 ) );
      QVERIFY( cache.isRequested( 3 ) );

      cache.invalidate( 2 );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 2 );
      QVERIFY( cache.isCached( 2 ) );
      QVERIFY( cache.isCached( 3 ) );

      const T e2 = cache.retrieve( 2 );
      const T e3a = cache.retrieve( 3 );
      QCOMPARE( e3a.id(), 3ll );
      QVERIFY( !e2.isValid() );

      cache.invalidate( 3 );
      const T e3b = cache.retrieve( 3 );
      QVERIFY( !e3b.isValid() );

      spy.clear();
      // updating a cached entry removes it
      cache.update( 3, FetchScope() );
      cache.update( 3, FetchScope() );
      QVERIFY( !cache.isCached( 3 ) );
      QVERIFY( !cache.isRequested( 3 ) );
      QVERIFY( !cache.retrieve( 3 ).isValid() );

      // updating a pending entry re-fetches
      cache.request( 3, FetchScope() );
      cache.update( 3, FetchScope() );
      QVERIFY( !cache.isCached( 3 ) );
      QVERIFY( cache.isRequested( 3 ) );
      cache.update( 3, FetchScope() );
      QVERIFY( !cache.isCached( 3 ) );
      QVERIFY( cache.isRequested( 3 ) );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 3 );
      QVERIFY( cache.isCached( 3 ) );
      QVERIFY( cache.retrieve( 3 ).isValid() );
    }

    template <typename T, typename FetchJob, typename FetchScope>
    void testListCache()
    {
      EntityListCache<T, FetchJob, FetchScope> cache( 2 );
      QSignalSpy spy( &cache, SIGNAL(dataAvailable()) );
      QVERIFY( spy.isValid() );

      QList<Entity::Id> ids;
      ids << 1;
      QVERIFY( !cache.isCached( ids ) );
      QVERIFY( !cache.isRequested( ids ) );
      QVERIFY( cache.retrieve( ids ).isEmpty() );

      FetchScope scope;
      scope.setAncestorRetrieval( FetchScope::All );

      ids << 2;
      cache.request( ids, scope );
      QVERIFY( !cache.isCached( ids ) );
      QVERIFY( cache.isRequested( ids ) );
      QVERIFY( cache.retrieve( ids ).isEmpty() );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 1 );
      QVERIFY( cache.isCached( ids ) );
      QVERIFY( cache.isRequested( ids ) );
      const typename T::List entities = cache.retrieve( ids );
      QCOMPARE( entities.size(), ids.size() );
      const T e1 = entities.at(0);
      const T e2 = entities.at(1);
      QCOMPARE( e1.id(), 1ll );
      QCOMPARE( e2.id(), 2ll );
      QVERIFY( e1.parentCollection().isValid() );
      QVERIFY( e2.parentCollection().isValid() );
      QVERIFY( !e1.parentCollection().remoteId().isEmpty() || e1.parentCollection() == Collection::root() );
      QVERIFY( !e2.parentCollection().remoteId().isEmpty() || e2.parentCollection() == Collection::root() );

      spy.clear();
      QList<Entity::Id> ids2;
      ids2 << 3 << 4;
      cache.request( ids, FetchScope() );
      QList<Entity::Id> ids3;
      ids3 << 5 << 6;
      cache.request( ids, FetchScope() );

      QVERIFY( !cache.isCached( ids ) );
      QVERIFY( !cache.isRequested( ids  ) );
      QVERIFY( cache.isRequested( ids2 ) );
      QVERIFY( cache.isRequested( ids3 ) );

      cache.invalidate( ids2 );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 2 );
      QVERIFY( cache.isCached( ids2 ) );
      QVERIFY( cache.isCached( ids3 ) );

      const typename T::List entities2 = cache.retrieve( ids2 );
      QVERIFY( !entities2.isEmpty() );
      const T e3 = entities2.at(0);
      const T e4 = entities2.at(1);
      QCOMPARE( e3.id(), 3ll );
      QCOMPARE( e4.id(), 4ll );
      QVERIFY( !e3.isValid() );
      QVERIFY( !e4.isValid() );

      const typename T::List entities3 = cache.retrieve( ids3 );
      QVERIFY( !entities3.isEmpty() );
      const T e5a = entities3.at(0);
      const T e6a = entities3.at(1);
      QCOMPARE( e5a.id(), 5ll );
      QCOMPARE( e6a.id(), 6ll );
      QVERIFY( e5a.isValid() );
      QVERIFY( e6a.isValid() );

      cache.invalidate( ids3 );
      const typename T::List entities3b = cache.retrieve( ids3 );
      QVERIFY( !entities3b.isEmpty() );
      const T e5b = entities3b.at(0);
      const T e6b = entities3b.at(1);
      QVERIFY( !e5b.isValid() );
      QVERIFY( !e6b.isValid() );

      spy.clear();
      // updating a cached entry removes it
      cache.update( ids3, FetchScope() );
      cache.update( ids3, FetchScope() );
      QVERIFY( !cache.isCached( ids3 ) );
      QVERIFY( !cache.isRequested( ids3 ) );
      QVERIFY( cache.retrieve( ids3 ).isEmpty() );

      // updating a pending entry re-fetches
      cache.request( ids3, FetchScope() );
      cache.update( ids3, FetchScope() );
      QVERIFY( !cache.isCached( ids3 ) );
      QVERIFY( cache.isRequested( ids3 ) );
      cache.update( ids3, FetchScope() );
      QVERIFY( !cache.isCached( ids3 ) );
      QVERIFY( cache.isRequested( ids3 ) );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 3 );
      QVERIFY( cache.isCached( ids3 ) );
      QVERIFY( !cache.retrieve( ids3 ).isEmpty() );
    }

  private slots:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
    }
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

    void testListCacheGeneric_data()
    {
      QTest::addColumn<bool>( "collection" );
      QTest::newRow( "collection" ) << true;
      QTest::newRow( "item" ) << false;
    }

    void testListCacheGeneric()
    {
      QFETCH( bool, collection );
      if ( collection ) {
        testListCache<Collection, CollectionFetchJob, CollectionFetchScope>();
      } else {
        testListCache<Item, ItemFetchJob, ItemFetchScope>();
      }
    }

    void testItemCache()
    {
      ItemCache cache( 1 );
      QSignalSpy spy( &cache, SIGNAL(dataAvailable()) );
      QVERIFY( spy.isValid() );

      ItemFetchScope scope;
      scope.fetchFullPayload( true );
      cache.request( 1, scope );

      QTest::qWait( 1000 );
      QCOMPARE( spy.count(), 1 );
      QVERIFY( cache.isCached( 1 ) );
      QVERIFY( cache.isRequested( 1 ) );
      const Item item = cache.retrieve( 1 );
      QCOMPARE( item.id(), 1ll );
      QVERIFY( item.hasPayload<QByteArray>() );
    }
};

QTEST_AKONADIMAIN( EntityCacheTest, NoGUI )

#include "entitycachetest.moc"
