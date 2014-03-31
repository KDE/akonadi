  /*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "gidtest.h"

#include "control.h"
#include "testattribute.h"
#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <qtest_akonadi.h>
#include "test_utils.h"
#include <akonadi/gid/gidextractorinterface.h>
#include <akonadi/itemserializer_p.h>
#include <akonadi/itemserializerplugin.h>

using namespace Akonadi;

QTEST_AKONADIMAIN( GidTest, NoGUI )

bool TestSerializer::deserialize( Akonadi::Item& item, const QByteArray& label, QIODevice& data, int version )
{
  qDebug() << item.id();
  if ( label != Akonadi::Item::FullPayload )
    return false;
  Q_UNUSED( version );

  item.setPayload( data.readAll() );
  return true;
}

void TestSerializer::serialize( const Akonadi::Item& item, const QByteArray& label, QIODevice& data, int &version )
{
  qDebug();
  Q_ASSERT( label == Akonadi::Item::FullPayload );
  Q_UNUSED( label );
  Q_UNUSED( version );
  data.write( item.payload<QByteArray>() );
}

QString TestSerializer::extractGid( const Akonadi::Item& item ) const {
  if (item.gid().isEmpty()) {
      return item.url().url();
  }
  return item.gid();
}

void GidTest::initTestCase()
{
  AkonadiTest::checkTestIsIsolated();
  Control::start();

  AkonadiTest::setAllResourcesOffline();
  Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance("akonadi_knut_resource_0");
  QVERIFY(agent.isValid());
  agent.setIsOnline(true);

  ItemSerializer::overridePluginLookup( new TestSerializer );
}

void GidTest::testSetAndFetch_data()
{
  QTest::addColumn<Akonadi::Item::List>("input");
  QTest::addColumn<Akonadi::Item>("toFetch");
  QTest::addColumn<Akonadi::Item::List>("expected");

    Item item1(1);
    item1.setGid("gid1");
    Item item2(2);
    item2.setGid("gid2");
    Item toFetch;
    toFetch.setGid("gid1");
    QTest::newRow("single") << (Item::List() << item1) << toFetch << (Item::List() << item1);
    QTest::newRow("multi") << (Item::List() << item1 << item2) << toFetch << (Item::List() << item1);
  {
    Item item3(3);
    item2.setGid("gid1");
    QTest::newRow("multi") << (Item::List() << item1 << item2 << item3) << toFetch << (Item::List() << item1 << item3);
  }
}

static void fetchAndSetGid(Item item) {
    ItemFetchJob *prefetchjob = new ItemFetchJob( item );
    prefetchjob->fetchScope().fetchFullPayload();
    AKVERIFYEXEC( prefetchjob );
    Item fetchedItem = prefetchjob->items()[0];

    //Write the gid to the db
    fetchedItem.setGid(item.gid());
    ItemModifyJob *store = new ItemModifyJob( fetchedItem );
    store->setUpdateGid( true );
    AKVERIFYEXEC( store );
}

void GidTest::testSetAndFetch()
{
  QFETCH(Item::List, input);
  QFETCH(Item, toFetch);
  QFETCH(Item::List, expected);

  foreach (Item item, input) {
    fetchAndSetGid(item);
  }

  ItemFetchJob *fetch = new ItemFetchJob( toFetch, this );
  fetch->fetchScope().setFetchGid( true );
  AKVERIFYEXEC( fetch );
  Item::List fetched = fetch->items();
  QCOMPARE( fetched.count(), expected.size() );
  foreach (Item item, expected) {
    QVERIFY(expected.removeOne(item));
  }
  QVERIFY(expected.isEmpty());
}

void GidTest::testCreate()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo/bar", this );
  AKVERIFYEXEC( resolver );
  int colId = resolver->collection();

  Item item;
  item.setMimeType("application/octet-stream");
  item.setPayload<QByteArray>(QByteArray("test"));
  item.setGid("createGid");
  ItemCreateJob *createJob = new ItemCreateJob(item, Collection(colId), this);
  AKVERIFYEXEC(createJob);
  ItemFetchJob *fetch = new ItemFetchJob( item, this );
  AKVERIFYEXEC( fetch );
  Item::List fetched = fetch->items();
  QCOMPARE( fetched.count(), 1 );
}

void GidTest::testSetWithIgnorePayload()
{
  Item item(5);
  ItemFetchJob *prefetchjob = new ItemFetchJob( item );
  prefetchjob->fetchScope().fetchFullPayload();
  AKVERIFYEXEC( prefetchjob );
  Item fetchedItem = prefetchjob->items()[0];
  QVERIFY( fetchedItem.gid().isEmpty() );

  //Write the gid to the db
  fetchedItem.setGid( "gid5" );
  ItemModifyJob *store = new ItemModifyJob( fetchedItem );
  store->setIgnorePayload( true );
  store->setUpdateGid( true );
  AKVERIFYEXEC( store );
  Item toFetch;
  toFetch.setGid( "gid5" );
  ItemFetchJob *fetch = new ItemFetchJob( toFetch, this );
  AKVERIFYEXEC( fetch );
  Item::List fetched = fetch->items();
  QCOMPARE( fetched.count(), 1 );
  QCOMPARE( fetched.at(0).id(), Item::Id(5) );
}

void GidTest::testFetchScope()
{

  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo/bar", this );
  AKVERIFYEXEC( resolver );
  int colId = resolver->collection();

  Item item;
  item.setMimeType("application/octet-stream");
  item.setPayload<QByteArray>(QByteArray("test"));
  item.setGid("createGid2");
  ItemCreateJob *createJob = new ItemCreateJob(item, Collection(colId), this);
  AKVERIFYEXEC(createJob);
  {
    ItemFetchJob *fetch = new ItemFetchJob( item, this );
    AKVERIFYEXEC( fetch );
    Item::List fetched = fetch->items();
    QCOMPARE( fetched.count(), 1 );
    QVERIFY( fetched.at(0).gid().isNull() );
  }
  {
    ItemFetchJob *fetch = new ItemFetchJob( item, this );
    fetch->fetchScope().setFetchGid( true );
    AKVERIFYEXEC( fetch );
    Item::List fetched = fetch->items();
    QCOMPARE( fetched.count(), 1 );
    QVERIFY( !fetched.at(0).gid().isNull() );
  }
}

