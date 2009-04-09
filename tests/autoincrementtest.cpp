/*
    Copyright (c) 2009 Thomas McGuire <mcguire@kde.org>

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
#include "autoincrementtest.h"

#include <Akonadi/Control>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>

#include <qtest_akonadi.h>
#include "test_utils.h"

using namespace Akonadi;

QTEST_AKONADIMAIN( AutoIncrementTest, NoGUI )

void AutoIncrementTest::initTestCase()
{
  QVERIFY( Control::start() );

  targetCollection = Collection( collectionIdFromPath( "res2/space folder" ) );
  QVERIFY( targetCollection.isValid() );
}

Akonadi::ItemCreateJob* AutoIncrementTest::createItemCreateJob()
{
  QString payload( "Hello world" );
  Item item( -1 );
  item.setMimeType( "application/octet-stream" );
  item.setPayload( payload );
  return new ItemCreateJob( item, targetCollection );
}

void AutoIncrementTest::testAutoIncrement()
{
  QList<Item> itemsToDelete;
  Item::Id lastId = -1;

  // Create 20 test items
  for( int i = 0; i < 20; i++ ) {
    ItemCreateJob *job = createItemCreateJob();
    QVERIFY( job->exec() );
    Item newItem = job->item();
    QVERIFY( newItem.id() > lastId );
    lastId = newItem.id();
    itemsToDelete.append( newItem );
  }

  // Delete the 20 item
  foreach( const Item &item, itemsToDelete ) {
    ItemDeleteJob *job = new ItemDeleteJob( item );
    QVERIFY( job->exec() );
  }

  // Restart the server, then test item creation again. The new id of the item
  // should be higher than all ids before.
  QVERIFY( Control::stop() );
  QVERIFY( Control::start() );

  ItemCreateJob *job = createItemCreateJob();
  QVERIFY( job->exec() );
  Item newItem = job->item();

  // If this fails, this is http://bugs.mysql.com/bug.php?id=199
  QVERIFY( newItem.id() > lastId );
  lastId = newItem.id();
}

