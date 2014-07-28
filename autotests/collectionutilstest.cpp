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

#include <qtest_akonadi.h>
#include "collection.h"
#include "../collectionutils.h"

using namespace Akonadi;

class CollectionUtilsTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testHasValidHierarchicalRID_data()
    {
      QTest::addColumn<Collection>( "collection" );
      QTest::addColumn<bool>( "isHRID" );

      QTest::newRow( "empty" ) << Collection() << false;
      QTest::newRow( "root" ) << Collection::root() << true;
      Collection c;
      c.setParentCollection( Collection::root() );
      QTest::newRow( "one level not ok" ) << c << false;
      c.setRemoteId( QLatin1String("r1") );
      QTest::newRow( "one level ok" ) << c << true;
      Collection c2;
      c2.setParentCollection( c );
      QTest::newRow( "two level not ok" ) << c2 << false;
      c2.setRemoteId( QLatin1String("r2") );
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

    void testPersistentParentCollection()
    {
      Collection col1(1);
      Collection col2(2);
      Collection col3(3);

      col2.setParentCollection(col3);
      col1.setParentCollection(col2);

      Collection assigned = col1;
      QCOMPARE(assigned.parentCollection(), col2);
      QCOMPARE(assigned.parentCollection().parentCollection(), col3);

      Collection copied(col1);
      QCOMPARE(copied.parentCollection(), col2);
      QCOMPARE(copied.parentCollection().parentCollection(), col3);
    }
};

QTEST_AKONADIMAIN( CollectionUtilsTest )

#include "collectionutilstest.moc"
