/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "searchjobtest.h"

#include <akonadi/collection.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/searchcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/monitor.h>
#include <akonadi/searchquery.h>

#include "collectionutils.h"

#include <qtest_akonadi.h>

QTEST_AKONADIMAIN( SearchJobTest, NoGUI )

using namespace Akonadi;

void SearchJobTest::initTestCase()
{
  AkonadiTest::checkTestIsIsolated();
  AkonadiTest::setAllResourcesOffline();
  Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance("akonadi_knut_resource_0");
  QVERIFY(agent.isValid());
  agent.setIsOnline(true);
}

void SearchJobTest::testCreateDeleteSearch()
{
  Akonadi::SearchQuery query;
  query.addTerm(Akonadi::SearchTerm("plugin", 1));
  query.addTerm(Akonadi::SearchTerm("resource", 2));
  query.addTerm(Akonadi::SearchTerm("plugin", 3));
  query.addTerm(Akonadi::SearchTerm("resource", 4));

  // Create collection
  SearchCreateJob *create = new SearchCreateJob( "search123456", query, this );
  create->setRemoteSearchEnabled(false);
  AKVERIFYEXEC( create );
  const Collection created = create->createdCollection();
  QVERIFY( created.isValid() );

  // Fetch "Search" collection, check the search collection has been created
  CollectionFetchJob *list = new CollectionFetchJob( Collection( 1 ), CollectionFetchJob::Recursive, this );
  AKVERIFYEXEC( list );
  Collection::List cols = list->collections();
  Collection col;
  foreach ( const Collection &c, cols ) {
    if ( c.name() == "search123456" ) {
      col = c;
    }
  }
  QVERIFY( col == created );
  QCOMPARE( col.parentCollection().id(), 1LL );
  QVERIFY( col.isVirtual() );

  // Fetch items in the search collection, check whether they are there
  ItemFetchJob *fetch = new ItemFetchJob( created, this );
  AKVERIFYEXEC( fetch );
  const Item::List items = fetch->items();
  QCOMPARE( items.count(), 2 );

  CollectionDeleteJob *delJob = new CollectionDeleteJob( col, this );
  AKVERIFYEXEC( delJob );
}

void SearchJobTest::testModifySearch()
{
  Akonadi::SearchQuery query;
  query.addTerm(Akonadi::SearchTerm("plugin", 1));
  query.addTerm(Akonadi::SearchTerm("resource", 2));

  // make sure there is a virtual collection
  SearchCreateJob *create = new SearchCreateJob( "search123456", query, this );
  AKVERIFYEXEC( create );
  Collection created = create->createdCollection();
  QVERIFY( created.isValid() );
}
