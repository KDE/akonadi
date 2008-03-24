/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include <sys/types.h>

#include "collectionjobtest.h"
#include <qtest_kde.h>

#include "cachepolicy.h"
#include "collection.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionstatistics.h"
#include "collectionstatisticsjob.h"
#include "collectionpathresolver.h"
#include "collectionselectjob.h"
#include "control.h"
#include "item.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>



using namespace Akonadi;

QTEST_KDEMAIN( CollectionJobTest, NoGUI )

void CollectionJobTest::initTestCase()
{
  qRegisterMetaType<Akonadi::Collection::List>();
  Control::start();
}

static Collection findCol( const Collection::List &list, const QString &name ) {
  foreach ( const Collection col, list )
    if ( col.name() == name )
      return col;
  return Collection();
}

// list compare which ignores the order
template <class T> static void compareLists( const QList<T> &l1, const QList<T> &l2 )
{
  QCOMPARE( l1.count(), l2.count() );
  foreach ( const T entry, l1 ) {
    QVERIFY( l2.contains( entry ) );
  }
}

template <typename T> static T* extractAttribute( QList<Attribute*> attrs )
{
  T dummy;
  foreach ( Attribute* attr, attrs ) {
    if ( attr->type() == dummy.type() )
      return dynamic_cast<T*>( attr );
  }
  return 0;
}

static Collection::Id res1ColId = 6; // -1;
static Collection::Id res2ColId = 7; //-1;
static Collection::Id res3ColId = -1;
static Collection::Id searchColId = -1;

void CollectionJobTest::testTopLevelList( )
{
  // non-recursive top-level list
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Flat );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  // check if everything is there and has the correct types and attributes
  QCOMPARE( list.count(), 4 );
  Collection col;

  col = findCol( list, "res1" );
  QVERIFY( col.isValid() );
  res1ColId = col.id(); // for the next test
  QVERIFY( res1ColId > 0 );
  QCOMPARE( col.type(), Collection::Resource );
  QCOMPARE( col.parent(), Collection::root().id() );
  QCOMPARE( col.resource(), QLatin1String("akonadi_dummy_resource_1") );

  QVERIFY( findCol( list, "res2" ).isValid() );
  res2ColId = findCol( list, "res2" ).id();
  QVERIFY( res2ColId > 0 );
  QVERIFY( findCol( list, "res3" ).isValid() );
  res3ColId = findCol( list, "res3" ).id();
  QVERIFY( res3ColId > 0 );

  col = findCol( list, "Search" );
  searchColId = col.id();
  QVERIFY( col.isValid() );
  QCOMPARE( col.type(), Collection::VirtualParent );
  QCOMPARE( col.resource(), QLatin1String("akonadi_search_resource") );
}

void CollectionJobTest::testFolderList( )
{
  // recursive list of physical folders
  CollectionFetchJob *job = new CollectionFetchJob( Collection( res1ColId ), CollectionFetchJob::Recursive );
  QSignalSpy spy( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)) );
  QVERIFY( spy.isValid() );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  int count = 0;
  for ( int i = 0; i < spy.count(); ++i ) {
    Collection::List l = spy[i][0].value<Akonadi::Collection::List>();
    for ( int j = 0; j < l.count(); ++j ) {
      QVERIFY( list.count() > count + j );
      QCOMPARE( list[count + j].id(), l[j].id() );
    }
    count += l.count();
  }
  QCOMPARE( count, list.count() );

  // check if everything is there
  QCOMPARE( list.count(), 4 );
  Collection col;
  QStringList contentTypes;

  col = findCol( list, "foo" );
  QVERIFY( col.isValid() );
  QCOMPARE( col.parent(), res1ColId );
  QCOMPARE( col.type(), Collection::Folder );
  contentTypes << "message/rfc822" << "text/calendar" << "text/vcard" << "application/octet-stream";
  compareLists( col.contentMimeTypes(), contentTypes );

  QVERIFY( findCol( list, "bar" ).isValid() );
  QCOMPARE( findCol( list, "bar" ).parent(), col.id() );
  QVERIFY( findCol( list, "bla" ).isValid() );
}

void CollectionJobTest::testNonRecursiveFolderList( )
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection( res1ColId ), CollectionFetchJob::Local );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  QCOMPARE( list.count(), 1 );
  QVERIFY( findCol( list, "res1" ).isValid() );
}

void CollectionJobTest::testEmptyFolderList( )
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection( res3ColId ), CollectionFetchJob::Flat );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  QCOMPARE( list.count(), 0 );
}

void CollectionJobTest::testSearchFolderList( )
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection( searchColId ), CollectionFetchJob::Flat );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  QCOMPARE( list.count(), 3 );
  Collection col = findCol( list, "Test ?er" );
  QVERIFY( col.isValid() );
  QCOMPARE( col.type(), Collection::Virtual );
  QCOMPARE( col.resource(), QLatin1String("akonadi_search_resource" ) );
  QVERIFY( findCol( list, "all" ).isValid() );
  QVERIFY( findCol( list, "kde-core-devel" ).isValid() );
}

void CollectionJobTest::testResourceFolderList()
{
  // non-existing resource
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Flat );
  job->setResource( "i_dont_exist" );
  QVERIFY( !job->exec() );

  // recursive listing of all collections of an existing resource
  job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  job->setResource( "akonadi_dummy_resource_1" );
  QVERIFY( job->exec() );

  Collection::List list = job->collections();
  QCOMPARE( list.count(), 5 );
  QVERIFY( findCol( list, "res1" ).isValid() );
  QVERIFY( findCol( list, "foo" ).isValid() );
  QVERIFY( findCol( list, "bar" ).isValid() );
  QVERIFY( findCol( list, "bla" ).isValid() );
  int fooId = findCol( list, "foo" ).id();

  // limited listing of a resource
  job = new CollectionFetchJob( Collection( fooId ), CollectionFetchJob::Recursive );
  job->setResource( "akonadi_dummy_resource_1" );
  QVERIFY( job->exec() );

  list = job->collections();
  QCOMPARE( list.count(), 3 );
  QVERIFY( findCol( list, "bar" ).isValid() );
  QVERIFY( findCol( list, "bla" ).isValid() );
}

void CollectionJobTest::testCreateDeleteFolder_data()
{
  QTest::addColumn<Collection>("collection");
  QTest::addColumn<bool>("creatable");

  Collection col;
  QTest::newRow("empty") << col << false;
  col.setName( "new folder" );
  col.setParent( res3ColId );
  QTest::newRow("simple") << col << true;

  col.setParent( res3ColId );
  col.setName( "foo" );
  QTest::newRow( "existing in different resource" ) << col << true;

  col.setName( "mail folder" );
  QStringList mimeTypes;
  mimeTypes << "inode/directory" << "message/rfc822";
  col.setContentMimeTypes( mimeTypes );
  col.setRemoteId( "remote id" );
  CachePolicy policy;
  policy.setInheritFromParent( false );
  policy.setIntervalCheckTime( 60 );
  policy.setLocalParts( QStringList( Item::PartEnvelope ) );
  policy.setSyncOnDemand( true );
  policy.setCacheTimeout( 120 );
  col.setCachePolicy( policy );
  QTest::newRow( "complex" ) << col << true;

  col = Collection();
  col.setName( "New Folder" );
  col.setParent( searchColId );
  QTest::newRow( "search folder" ) << col << false;

  col.setParent( res2ColId );
  col.setName( "foo2" );
  QTest::newRow( "already existing" ) << col << false;

  col.setName( "Bla" );
  col.setParent( 2 );
  QTest::newRow( "already existing with different case" ) << col << true;

  CollectionPathResolver *resolver = new CollectionPathResolver( "res2/foo2", this );
  QVERIFY( resolver->exec() );
  col.setParent( resolver->collection() );
  col.setName( "new folder" );
  QTest::newRow( "parent noinferior" ) << col << false;

  col.setParent( INT_MAX );
  QTest::newRow( "missing parent" ) << col << false;
}

void CollectionJobTest::testCreateDeleteFolder()
{
  QFETCH( Collection, collection );
  QFETCH( bool, creatable );

  CollectionCreateJob *createJob = new CollectionCreateJob( collection, this );
  QCOMPARE( createJob->exec(), creatable );
  if ( !creatable )
    return;

  Collection createdCol = createJob->collection();
  QVERIFY( createdCol.isValid() );
  QCOMPARE( createdCol.name(), collection.name() );
  QCOMPARE( createdCol.parent(), collection.parent() );
  QCOMPARE( createdCol.remoteId(), collection.remoteId() );
  QCOMPARE( createdCol.cachePolicy(), collection.cachePolicy() );

  CollectionFetchJob *listJob = new CollectionFetchJob( Collection( collection.parent() ), CollectionFetchJob::Flat, this );
  QVERIFY( listJob->exec() );
  Collection listedCol = findCol( listJob->collections(), collection.name() );
  QCOMPARE( listedCol, createdCol );
  QCOMPARE( listedCol.remoteId(), collection.remoteId() );
  QCOMPARE( listedCol.cachePolicy(), collection.cachePolicy() );

  // fetch parent to compare inherited collection properties
  Collection parentCol = Collection::root();
  if ( collection.parent() > 0 ) {
    CollectionFetchJob *listJob = new CollectionFetchJob( Collection( collection.parent() ), CollectionFetchJob::Local, this );
    QVERIFY( listJob->exec() );
    QCOMPARE( listJob->collections().count(), 1 );
    parentCol = listJob->collections().first();
  }

  if ( collection.contentMimeTypes().isEmpty() )
    compareLists( listedCol.contentMimeTypes(), parentCol.contentMimeTypes() );
  else
    compareLists( listedCol.contentMimeTypes(), collection.contentMimeTypes() );

  if ( collection.resource().isEmpty() )
    QCOMPARE( listedCol.resource(), parentCol.resource() );
  else
    QCOMPARE( listedCol.resource(), collection.resource() );

  CollectionDeleteJob *delJob = new CollectionDeleteJob( createdCol, this );
  QVERIFY( delJob->exec() );

  listJob = new CollectionFetchJob( Collection( collection.parent() ), CollectionFetchJob::Flat, this );
  QVERIFY( listJob->exec() );
  QVERIFY( !findCol( listJob->collections(), collection.name() ).isValid() );
}

void CollectionJobTest::testIllegalDeleteFolder()
{
  // non-existing folder
  CollectionDeleteJob *del = new CollectionDeleteJob( Collection( INT_MAX ), this );
  QVERIFY( !del->exec() );

  // root
  del = new CollectionDeleteJob( Collection::root(), this );
  QVERIFY( !del->exec() );
}

void CollectionJobTest::testStatistics()
{
  // empty folder
  CollectionStatisticsJob *statistics =
                   new CollectionStatisticsJob( Collection( res1ColId ), this );
  QVERIFY( statistics->exec() );

  CollectionStatistics s = statistics->statistics();
  QCOMPARE( s.count(), 0 );
  QCOMPARE( s.unreadCount(), 0 );

  // folder with attributes and content
  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo", this );;
  QVERIFY( resolver->exec() );
  statistics = new CollectionStatisticsJob( Collection( resolver->collection() ), this );
  QVERIFY( statistics->exec() );

  s = statistics->statistics();
  QCOMPARE( s.count(), 15 );
  QCOMPARE( s.unreadCount(), 14 );
}

void CollectionJobTest::testModify()
{
  QStringList reference;
  reference << "text/calendar" << "text/vcard" << "message/rfc822" << "application/octet-stream";

  Collection col;
  CollectionFetchJob *ljob = new CollectionFetchJob( Collection( res1ColId ), CollectionFetchJob::Flat );
  QVERIFY( ljob->exec() );
  col = findCol( ljob->collections(), "foo" );
  QVERIFY( col.isValid() );

  // test noop modify
  CollectionModifyJob *mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );

  ljob = new CollectionFetchJob( col, CollectionFetchJob::Local, this );
  QVERIFY( ljob->exec() );
  QCOMPARE( ljob->collections().count(), 1 );
  col = ljob->collections().first();
  compareLists( col.contentMimeTypes(), reference );

  // test clearing content types
  col.setContentMimeTypes( QStringList() );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );

  ljob = new CollectionFetchJob( col, CollectionFetchJob::Local, this );
  QVERIFY( ljob->exec() );
  QCOMPARE( ljob->collections().count(), 1 );
  col = ljob->collections().first();
  QVERIFY( col.contentMimeTypes().isEmpty() );

  // test setting contnet types
  col.setContentMimeTypes( reference );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );

  ljob = new CollectionFetchJob( col, CollectionFetchJob::Local, this );
  QVERIFY( ljob->exec() );
  QCOMPARE( ljob->collections().count(), 1 );
  col = ljob->collections().first();
  compareLists( col.contentMimeTypes(), reference );

  // renaming
  col.setName( "foo (renamed)" );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );

  ljob = new CollectionFetchJob( col, CollectionFetchJob::Local, this );
  QVERIFY( ljob->exec() );
  QCOMPARE( ljob->collections().count(), 1 );
  col = ljob->collections().first();
  QCOMPARE( col.name(), QString( "foo (renamed)" ) );

  col.setName( "foo" );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );
}

void CollectionJobTest::testMove()
{
  Collection col( res1ColId );
  col.setParent( res2ColId );
  CollectionModifyJob *mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );

  CollectionFetchJob *ljob = new CollectionFetchJob( Collection( res2ColId ), CollectionFetchJob::Recursive );
  QVERIFY( ljob->exec() );
  Collection::List list = ljob->collections();

  QCOMPARE( list.count(), 7 );
  QVERIFY( findCol( list, "res1" ).isValid() );
  QVERIFY( findCol( list, "foo" ).isValid() );
  QVERIFY( findCol( list, "bar" ).isValid() );
  QVERIFY( findCol( list, "bla" ).isValid() );

  ljob = new CollectionFetchJob( Collection( res1ColId ), CollectionFetchJob::Local );
  QVERIFY( ljob->exec() );
  list = ljob->collections();

  QCOMPARE( list.count(), 1 );
  col = list.first();
  QCOMPARE( col.name(), QLatin1String("res1") );
  QCOMPARE( col.parent(), res2ColId );

  // cleanup
  col.setParent( Collection::root() );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );
}

void CollectionJobTest::testIllegalModify()
{
  // non-existing collection
  Collection col( INT_MAX );
  col.setParent( res1ColId );
  CollectionModifyJob *mod = new CollectionModifyJob( col, this );
  QVERIFY( !mod->exec() );

  // rename to already existing name
  col = Collection( res1ColId );
  col.setName( "res2" );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( !mod->exec() );

  // move to non-existing target
  col = Collection( res1ColId );
  col.setParent( INT_MAX );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( !mod->exec() );

  // moving root
  col = Collection::root();
  col.setParent( res1ColId );
  mod = new CollectionModifyJob( col, this );
  QVERIFY( !mod->exec() );
}

void CollectionJobTest::testUtf8CollectionName()
{
  QString folderName = QString::fromUtf8( "ä" );

  // create collection
  Collection col;
  col.setParent( res3ColId );
  col.setName( folderName );
  CollectionCreateJob *create = new CollectionCreateJob( col, this );
  QVERIFY( create->exec() );
  col = create->collection();
  QVERIFY( col.isValid() );

  // list parent
  CollectionFetchJob *list = new CollectionFetchJob( Collection( res3ColId ), CollectionFetchJob::Recursive, this );
  QVERIFY( list->exec() );
  QCOMPARE( list->collections().count(), 1 );
  QCOMPARE( col, list->collections().first() );
  QCOMPARE( col.name(), folderName );

  // modify collection
  col.setContentMimeTypes( QStringList( "message/rfc822'" ) );
  CollectionModifyJob *modify = new CollectionModifyJob( col, this );
  QVERIFY( modify->exec() );

  // collection statistics
  CollectionStatisticsJob *statistics = new CollectionStatisticsJob( col, this );
  QVERIFY( statistics->exec() );
  CollectionStatistics s = statistics->statistics();
  QCOMPARE( s.count(), 0 );
  QCOMPARE( s.unreadCount(), 0 );

  // delete collection
  CollectionDeleteJob *del = new CollectionDeleteJob( col, this );
  QVERIFY( del->exec() );
}

void CollectionJobTest::testMultiList()
{
  Collection::List req;
  req << Collection( res1ColId ) << Collection( res2ColId );
  CollectionFetchJob* job = new CollectionFetchJob( req, this );
  QVERIFY( job->exec() );

  Collection::List res;
  res = job->collections();
  compareLists( res, req );
}

void CollectionJobTest::testSelect()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo", this );;
  QVERIFY( resolver->exec() );
  Collection col( resolver->collection() );

  CollectionSelectJob *job = new CollectionSelectJob( col, this );
  QVERIFY( job->exec() );
  QCOMPARE( job->unseen(), -1 );

  job = new CollectionSelectJob( col, this );
  job->setRetrieveStatus( true );
  QVERIFY( job->exec() );
  QVERIFY( job->unseen() > -1 );

  job = new CollectionSelectJob( Collection::root(), this );
  QVERIFY( job->exec() );

  job = new CollectionSelectJob( Collection( INT_MAX ), this );
  QVERIFY( !job->exec() );
}

#include "collectionjobtest.moc"
