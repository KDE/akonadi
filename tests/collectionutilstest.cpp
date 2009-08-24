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

#include <qtest_akonadi.h>
#include <akonadi/collection.h>
#include "../collectionutils_p.h"

using namespace Akonadi;

class CollectionUtilsTest : public QObject
{
  Q_OBJECT
  private slots:
    void testHasValidHierarchicalRID_data()
    {
      QTest::addColumn<Collection>( "collection" );
      QTest::addColumn<bool>( "isHRID" );

      QTest::newRow( "empty" ) << Collection() << false;
      QTest::newRow( "root" ) << Collection::root() << true;
      Collection c;
      c.setParentCollection( Collection::root() );
      QTest::newRow( "one level not ok" ) << c << false;
      c.setRemoteId( "r1" );
      QTest::newRow( "one level ok" ) << c << true;
      Collection c2;
      c2.setParentCollection( c );
      QTest::newRow( "two level not ok" ) << c2 << false;
      c2.setRemoteId( "r2" );
      QTest::newRow( "two level ok" ) << c2 << true;
      c2.parentCollection().setRemoteId( QString() );
      QTest::newRow( "mid RID missing" ) << c2 << false;
    }

    void testHasValidHierarchicalRID()
    {
      QFETCH( Collection, collection );
      QFETCH( bool, isHRID );
      QCOMPARE( CollectionUtils::hasValidHierarchicalRID( collection ), isHRID );
    }
};

QTEST_AKONADIMAIN( CollectionUtilsTest, NoGUI )

#include "collectionutilstest.moc"
