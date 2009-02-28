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

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/control.h>
#include <akonadi/collectioncopyjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <QtCore/QObject>

#include "test_utils.h"

using namespace Akonadi;

class CollectionCopyTest : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
      // switch target resources offline to reduce interference from them
      foreach ( Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances() ) {
        if ( agent.identifier() == "akonadi_knut_resource_2" )
          agent.setIsOnline( false );
      }
    }

    void testCopy()
    {
      const Collection target( collectionIdFromPath( "res3" ) );
      Collection source( collectionIdFromPath( "res1/foo" ) );
      QVERIFY( target.isValid() );
      QVERIFY( source.isValid() );

      // obtain reference listing
      CollectionFetchJob *fetch = new CollectionFetchJob( source, CollectionFetchJob::Base );
      QVERIFY( fetch->exec() );
      QCOMPARE( fetch->collections().count(), 1 );
      source = fetch->collections().first();
      QVERIFY( source.isValid() );

      fetch = new CollectionFetchJob( source, CollectionFetchJob::Recursive );
      QVERIFY( fetch->exec() );
      QHash<Collection, Item::List> referenceData;
      Collection::List cols = fetch->collections();
      cols << source;
      foreach ( const Collection c, cols ) {
        ItemFetchJob *job = new ItemFetchJob( c, this );
        QVERIFY( job->exec() );
        referenceData.insert( c, job->items() );
      }

      // actually copy the collection
      CollectionCopyJob *copy = new CollectionCopyJob( source, target );
      QVERIFY( copy->exec() );

      // list destination and check if everything has arrived
      CollectionFetchJob *list = new CollectionFetchJob( target, CollectionFetchJob::Recursive );
      QVERIFY( list->exec() );
      cols = list->collections();
      QCOMPARE( cols.count(), referenceData.count() );
      for ( QHash<Collection, Item::List>::ConstIterator it = referenceData.constBegin(); it != referenceData.constEnd(); ++it ) {
        QVERIFY( !cols.contains( it.key() ) );
        Collection col;
        foreach ( const Collection &c, cols ) {
          if ( it.key().name() == c.name() )
            col = c;
        }

        QVERIFY( col.isValid() );
        QCOMPARE( col.resource(), QLatin1String("akonadi_knut_resource_2") );
        QVERIFY( col.remoteId().isEmpty() );
        ItemFetchJob *job = new ItemFetchJob( col, this );
        job->fetchScope().fetchFullPayload();
        job->fetchScope().setCacheOnly( true );
        QVERIFY( job->exec() );
        QCOMPARE( job->items().count(), it.value().count() );
        foreach ( const Item &item, job->items() ) {
          QVERIFY( !it.value().contains( item ) );
          QVERIFY( item.remoteId().isEmpty() );
          QEXPECT_FAIL( "", "server bug", Continue );
          QVERIFY( item.hasPayload() );
        }
      }
    }

    void testIlleagalCopy()
    {
      // invalid source
      CollectionCopyJob* copy = new CollectionCopyJob( Collection(), Collection( 1 ) );
      QVERIFY( !copy->exec() );

      // non-existing source
      copy = new CollectionCopyJob( Collection( INT_MAX ), Collection( 1 ) );
      QVERIFY( !copy->exec() );

      // invalid target
      copy = new CollectionCopyJob( Collection( 1 ), Collection() );
      QVERIFY( !copy->exec() );

      // non-existing target
      copy = new CollectionCopyJob( Collection( 1 ), Collection( INT_MAX ) );
      QVERIFY( !copy->exec() );
    }

};

QTEST_AKONADIMAIN( CollectionCopyTest, NoGUI )

#include "collectioncopytest.moc"
