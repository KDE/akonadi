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

#include <agentinstance.h>
#include <agentmanager.h>
#include <collection.h>
#include <collectionstatistics.h>
#include <control.h>
#include <itemcopyjob.h>
#include <itemfetchjob.h>
#include <itemfetchscope.h>

#include <QtCore/QObject>

#include "test_utils.h"
#include <qtest_akonadi.h>

using namespace Akonadi;

class ItemCopyTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      Control::start();
      // switch target resources offline to reduce interference from them
      foreach ( Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances() ) { //krazy:exclude=foreach
        if ( agent.identifier() == QLatin1String("akonadi_knut_resource_2") )
          agent.setIsOnline( false );
      }
    }

    void testCopy()
    {
      const Collection target( collectionIdFromPath( QLatin1String("res3") ) );
      QVERIFY( target.isValid() );

      ItemCopyJob *copy = new ItemCopyJob( Item( 1 ), target );
      AKVERIFYEXEC( copy );

      Item source( 1 );
      ItemFetchJob *sourceFetch = new ItemFetchJob( source );
      AKVERIFYEXEC( sourceFetch );
      source = sourceFetch->items().first();

      ItemFetchJob *fetch = new ItemFetchJob( target );
      fetch->fetchScope().fetchFullPayload();
      fetch->fetchScope().fetchAllAttributes();
      fetch->fetchScope().setCacheOnly( true );
      AKVERIFYEXEC( fetch );
      QCOMPARE( fetch->items().count(), 1 );

      Item item = fetch->items().first();
      QVERIFY( item.hasPayload() );
      QVERIFY( source.size() > 0 );
      QVERIFY( item.size() > 0 );
      QCOMPARE( item.size(), source.size() );
      QCOMPARE( item.attributes().count(), 1 );
      QVERIFY( item.remoteId().isEmpty() );
      QEXPECT_FAIL("", "statistics are not properly updated after copy", Abort);
      QCOMPARE( target.statistics().count(), 1ll );
    }

    void testIlleagalCopy()
    {
      // empty item list
      ItemCopyJob *copy = new ItemCopyJob( Item::List(), Collection::root() );
      QVERIFY( !copy->exec() );

      // non-existing target
      copy = new ItemCopyJob( Item( 1 ), Collection( INT_MAX ) );
      QVERIFY( !copy->exec() );

      // non-existing source
      copy = new ItemCopyJob( Item( INT_MAX ), Collection::root() );
      QVERIFY( !copy->exec() );
    }

};

QTEST_AKONADIMAIN( ItemCopyTest )

#include "itemcopytest.moc"
