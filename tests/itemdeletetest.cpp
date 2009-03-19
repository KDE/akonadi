/*
    Copyright (c) 20089 Volker Krause <vkrause@kde.org>

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
#include <akonadi/collection.h>
#include <akonadi/collectionpathresolver_p.h>
#include <akonadi/collectionselectjob.h>
#include <akonadi/control.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/transactionjobs.h>

#include <QtCore/QObject>

#include <sys/types.h>

using namespace Akonadi;

class ItemDeleteTest : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
    }

    void testIllegalDelete()
    {
      ItemDeleteJob *djob = new ItemDeleteJob( Item( INT_MAX ), this );
      QVERIFY( !djob->exec() );

      // make sure a failed delete doesn't leave a transaction open (the kpilot bug)
      TransactionRollbackJob *tjob = new TransactionRollbackJob( this );
      QVERIFY( !tjob->exec() );
    }

    void testDelete()
    {
      ItemDeleteJob *djob = new ItemDeleteJob( Item( 1 ), this );
      QVERIFY( djob->exec() );

      ItemFetchJob *fjob = new ItemFetchJob( Item( 1 ), this );
      fjob->exec();
      QCOMPARE( fjob->items().count(), 0 );
    }

    void testDeleteFromUnselectedCollection()
    {
      const QString path = QLatin1String( "res1" ) +
                           CollectionPathResolver::pathDelimiter() +
                           QLatin1String( "foo" );
      CollectionPathResolver *rjob = new CollectionPathResolver( path, this );
      QVERIFY( rjob->exec() );

      ItemFetchJob *fjob = new ItemFetchJob( Collection( rjob->collection() ), this );
      QVERIFY( fjob->exec() );

      const Item::List items = fjob->items();
      QVERIFY( items.count() > 0 );

      CollectionSelectJob *sjob = new CollectionSelectJob( Collection( 2 ), this );
      QVERIFY( sjob->exec() );

      ItemDeleteJob *djob = new ItemDeleteJob( items[ 0 ], this );
      QVERIFY( djob->exec() );

      fjob = new ItemFetchJob( items[ 0 ], this );
      fjob->exec();
      QCOMPARE( fjob->items().count(), 0 );
    }
};

QTEST_AKONADIMAIN( ItemDeleteTest, NoGUI )

#include "itemdeletetest.moc"
