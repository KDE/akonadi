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

#include "test_utils.h"
#include "protocolhelper.cpp"

using namespace Akonadi;

class ProtocolHelperTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testItemSetToByteArray_data()
    {
      QTest::addColumn<Item::List>( "items" );
      QTest::addColumn<QByteArray>( "command" );
      QTest::addColumn<QByteArray>( "result" );
      QTest::addColumn<bool>( "shouldThrow" );

      Item u1; u1.setId( 1 );
      Item u2; u2.setId( 2 );
      Item u3; u3.setId( 3 );
      Item r1; r1.setRemoteId( "A" );
      Item r2; r2.setRemoteId( "B" );
      Item h1; h1.setRemoteId( "H1" ); h1.setParentCollection( Collection::root() );
      Item h2; h2.setRemoteId( "H2a" ); h2.parentCollection().setRemoteId( "H2b" ); h2.parentCollection().setParentCollection( Collection::root() );
      Item h3; h3.setRemoteId( "H3a" ); h3.parentCollection().setRemoteId( "H3b" );

      QTest::newRow( "empty" ) << Item::List() << QByteArray( "CMD" ) << QByteArray() << true;
      QTest::newRow( "single uid" ) << (Item::List() << u1) << QByteArray( "CMD" ) << QByteArray( " UID CMD 1" ) << false;
      QTest::newRow( "multi uid" ) << (Item::List() << u1 << u3) << QByteArray( "CMD" ) << QByteArray( " UID CMD 1,3" ) << false;
      QTest::newRow( "block uid" ) << (Item::List() << u1 << u2 << u3) << QByteArray( "CMD" ) << QByteArray( " UID CMD 1:3" ) << false;
      QTest::newRow( "single rid" ) << (Item::List() << r1) << QByteArray( "CMD" ) << QByteArray( " RID CMD (\"A\")" ) << false;
      QTest::newRow( "multi rid" ) << (Item::List() << r1 << r2) << QByteArray( "CMD" ) << QByteArray( " RID CMD (\"A\" \"B\")" ) << false;
      QTest::newRow( "invalid" ) << (Item::List() << Item()) << QByteArray( "CMD" ) << QByteArray() << true;
      QTest::newRow( "mixed" ) << (Item::List() << u1 << r1) << QByteArray( "CMD" ) << QByteArray() << true;
      QTest::newRow( "empty command, uid" ) << (Item::List() << u1) << QByteArray() << QByteArray( " UID 1" ) << false;
      QTest::newRow( "empty command, single rid" ) << (Item::List() << r1) << QByteArray() << QByteArray( " RID (\"A\")" ) << false;
      QTest::newRow( "single hrid" ) << (Item::List() << h1) << QByteArray( "CMD" ) << QByteArray( " HRID CMD ((-1 \"H1\") (0 \"\"))" ) << false;
      QTest::newRow( "single hrid 2" ) << (Item::List() << h2) << QByteArray( "CMD" ) << QByteArray( " HRID CMD ((-1 \"H2a\") (-2 \"H2b\") (0 \"\"))" ) << false;
      QTest::newRow( "mixed hrid/rid" ) << (Item::List() << h1 << r1) << QByteArray( "CMD" ) << QByteArray( " RID CMD (\"H1\" \"A\")" ) << false;
      QTest::newRow( "unterminated hrid" ) << (Item::List() << h3) << QByteArray( "CMD" ) << QByteArray( " RID CMD (\"H3a\")" ) << false;
    }

    void testItemSetToByteArray()
    {
      QFETCH( Item::List, items );
      QFETCH( QByteArray, command );
      QFETCH( QByteArray, result );
      QFETCH( bool, shouldThrow );

      bool didThrow = false;
      try {
        const QByteArray r = ProtocolHelper::entitySetToByteArray( items, command );
        qDebug() << r << result;
        QCOMPARE( r, result );
      } catch ( const std::exception &e ) {
        qDebug() << e.what();
        didThrow = true;
      }
      QCOMPARE( didThrow, shouldThrow );
    }

    void testAncestorParsing_data()
    {
      QTest::addColumn<QByteArray>( "input" );
      QTest::addColumn<Collection>( "parent" );

      const QByteArray b0 = "((0 \"\"))";
      QTest::newRow( "top-level" ) << b0 << Collection::root();

      const QByteArray b1 = "((42 \"net)\") (0 \"\"))";
      Collection c1;
      c1.setRemoteId( "net)" );
      c1.setId( 42 );
      c1.setParentCollection( Collection::root() );
      QTest::newRow( "till's obscure folder" ) << b1 << c1;
    }

    void testAncestorParsing()
    {
      QFETCH( QByteArray, input );
      QFETCH( Collection, parent );

      Item i;
      ProtocolHelper::parseAncestors( input, &i );
      QCOMPARE( i.parentCollection().id(), parent.id() );
      QCOMPARE( i.parentCollection().remoteId(), parent.remoteId() );
    }

    void testCollectionParsing_data()
    {
      QTest::addColumn<QByteArray>( "input" );
      QTest::addColumn<Collection>( "collection" );

      const QByteArray b1 = "2 1 (REMOTEID \"r2\" NAME \"n2\")";
      Collection c1;
      c1.setId( 2 );
      c1.setRemoteId( "r2" );
      c1.parentCollection().setId( 1 );
      c1.setName( "n2" );
      QTest::newRow( "no ancestors" ) << b1 << c1;

      const QByteArray b2 = "3 2 (REMOTEID \"r3\" ANCESTORS ((2 \"r2\") (1 \"r1\") (0 \"\")))";
      Collection c2;
      c2.setId( 3 );
      c2.setRemoteId( "r3" );
      c2.parentCollection().setId( 2 );
      c2.parentCollection().setRemoteId( "r2" );
      c2.parentCollection().parentCollection().setId( 1 );
      c2.parentCollection().parentCollection().setRemoteId( "r1" );
      c2.parentCollection().parentCollection().setParentCollection( Collection::root() );
      QTest::newRow( "ancestors" ) << b2 << c2;
    }

    void testCollectionParsing()
    {
      QFETCH( QByteArray, input );
      QFETCH( Collection, collection );

      Collection parsedCollection;
      ProtocolHelper::parseCollection( input, parsedCollection );

      QCOMPARE( parsedCollection.name(), collection.name() );

      while ( collection.isValid() || parsedCollection.isValid() ) {
        QCOMPARE( parsedCollection.id(), collection.id() );
        QCOMPARE( parsedCollection.remoteId(), collection.remoteId() );
        const Collection p1( parsedCollection.parentCollection() );
        const Collection p2( collection.parentCollection() );
        parsedCollection = p1;
        collection = p2;
      }
    }

    void testParentCollectionAfterCollectionParsing()
    {
      Collection parsedCollection;
      const QByteArray b = "111 222 (REMOTEID \"A\" ANCESTORS ((222 \"B\") (333 \"C\") (0 \"\"))";
      ProtocolHelper::parseCollection( b, parsedCollection );

      QList<qint64> ids;
      ids << 111 << 222 << 333 << 0;
      int i = 0;

      Collection col = parsedCollection;
      while ( col.isValid() ) {
        QCOMPARE( col.id(), ids[i++] );
        col = col.parentCollection();
      }
      QCOMPARE( i, 4 );
    }

    void testHRidToByteArray_data()
    {
      QTest::addColumn<Collection>( "collection" );
      QTest::addColumn<QByteArray>( "result" );

      QTest::newRow( "empty" ) << Collection() << QByteArray();
      QTest::newRow( "root" ) << Collection::root() << QByteArray( "(0 \"\")" );
      Collection c;
      c.setId(1);
      c.setParentCollection( Collection::root() );
      c.setRemoteId( "r1" );
      QTest::newRow( "one level" ) << c << QByteArray( "(1 \"r1\") (0 \"\")" );
      Collection c2;
      c2.setId(2);
      c2.setParentCollection( c );
      c2.setRemoteId( "r2" );
      QTest::newRow( "two level ok" ) << c2 << QByteArray( "(2 \"r2\") (1 \"r1\") (0 \"\")" );
    }

    void testHRidToByteArray()
    {
      QFETCH( Collection, collection );
      QFETCH( QByteArray, result );
      qDebug() << ProtocolHelper::hierarchicalRidToByteArray( collection ) << result;
      QCOMPARE( ProtocolHelper::hierarchicalRidToByteArray( collection ), result );
    }

    void testItemFetchScopeToByteArray_data()
    {
      QTest::addColumn<ItemFetchScope>( "scope" );
      QTest::addColumn<QByteArray>( "result" );

      QTest::newRow( "empty" ) << ItemFetchScope() << QByteArray( " EXTERNALPAYLOAD (UID COLLECTIONID FLAGS SIZE REMOTEID REMOTEREVISION DATETIME)\n" );

      ItemFetchScope scope;
      scope.fetchAllAttributes();
      scope.fetchFullPayload();
      scope.setAncestorRetrieval( Akonadi::ItemFetchScope::All );
      scope.setIgnoreRetrievalErrors( true );
      QTest::newRow( "full" ) << scope << QByteArray( " FULLPAYLOAD ALLATTR IGNOREERRORS ANCESTORS INF EXTERNALPAYLOAD (UID COLLECTIONID FLAGS SIZE REMOTEID REMOTEREVISION DATETIME)\n" );

      scope = ItemFetchScope();
      scope.setFetchModificationTime( false );
      scope.setFetchRemoteIdentification( false );
      QTest::newRow( "minimal" ) << scope << QByteArray( " EXTERNALPAYLOAD (UID COLLECTIONID FLAGS SIZE)\n" );
    }

    void testItemFetchScopeToByteArray()
    {
      QFETCH( ItemFetchScope, scope );
      QFETCH( QByteArray, result );
      qDebug() << ProtocolHelper::itemFetchScopeToByteArray( scope ) << result;
      QCOMPARE( ProtocolHelper::itemFetchScopeToByteArray( scope ), result );
    }
};

QTEST_KDEMAIN( ProtocolHelperTest, NoGUI )

#include "protocolhelpertest.moc"
