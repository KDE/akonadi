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

#include <libakonadi/control.h>
#include <libakonadi/itemcopyjob.h>
#include <libakonadi/itemfetchjob.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class ItemCopyTest : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
    }

    void testCopy()
    {
      const Collection target( 8 );
      Item item( DataReference( 1, QString() ) );

      ItemCopyJob *copy = new ItemCopyJob( item, target );
      QVERIFY( copy->exec() );

      ItemFetchJob *fetch = new ItemFetchJob( target );
      fetch->fetchAllParts();
      QVERIFY( fetch->exec() );
      QCOMPARE( fetch->items().count(), 1 );
    }

    void testIlleagalCopy()
    {
      // empty item list
      ItemCopyJob *copy = new ItemCopyJob( Item::List(), Collection::root() );
      QVERIFY( !copy->exec() );

      // non-existing target
      copy = new ItemCopyJob( Item( DataReference( 1, QString() ) ), Collection( INT_MAX ) );
      QVERIFY( !copy->exec() );

      // non-existing source
      copy = new ItemCopyJob( Item( DataReference( INT_MAX, QString() ) ), Collection::root() );
      QVERIFY( !copy->exec() );
    }

};

QTEST_KDEMAIN( ItemCopyTest, NoGUI )

#include "itemcopytest.moc"
