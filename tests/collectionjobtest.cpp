/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "collectionjobtest.h"
#include <qtest_kde.h>

#include "collection.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionlistjob.h"
#include "collectionmodifyjob.h"
#include "collectionstatusjob.h"
#include "messagecollectionattribute.h"

using namespace PIM;

QTEST_KDEMAIN( CollectionJobTest, NoGUI );

static Collection* findCol( const Collection::List &list, const QByteArray &path ) {
  foreach ( Collection* col, list )
    if ( col->path() == path )
      return col;
  return 0;
}

// list compare which ignores the order
template <class T> static void compareLists( const QList<T> &l1, const QList<T> &l2 )
{
  QCOMPARE( l1.count(), l2.count() );
  foreach ( const T entry, l1 ) {
    QVERIFY( l2.contains( entry ) );
  }
}

 // ### These tests rely on an running akonadi server which uses the test database!!
void CollectionJobTest::testTopLevelList( )
{
  // non-recursive top-level list
  CollectionListJob *job = new CollectionListJob( Collection::root(), false );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  delete job;

  // check if everything is there and has the correct types and attributes
  QCOMPARE( list.count(), 4 );
  Collection *col;

  col = findCol( list, "res1" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::Resource );

  QVERIFY( findCol( list, "res2" ) != 0 );
  QVERIFY( findCol( list, "res3" ) != 0 );

  col = findCol( list, "Search" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::VirtualParent );
}

void CollectionJobTest::testFolderList( )
{
  // recursive list of physical folders
  CollectionListJob *job = new CollectionListJob( "/res1", true );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  delete job;

  // check if everything is there
  QCOMPARE( list.count(), 4 );
  Collection *col;
  QList<QByteArray> contentTypes;

  col = findCol( list, "res1/foo" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::Folder );
  contentTypes << "message/rfc822" << "text/calendar" << "text/vcard";
  compareLists( col->contentTypes(), contentTypes );

  QVERIFY( findCol( list, "res1/foo/bar" ) != 0 );
  QVERIFY( findCol( list, "res1/foo/bla" ) != 0 );

  col = findCol( list, "res1/foo/bar/bla" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::Folder );
  contentTypes.clear();
  contentTypes << Collection::collectionMimeType();
  compareLists( col->contentTypes(), contentTypes );
}

void CollectionJobTest::testNonRecursiveFolderList( )
{
  CollectionListJob *job = new CollectionListJob( "/res1", false );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  delete job;

  QCOMPARE( list.count(), 1 );
  QVERIFY( findCol( list, "res1/foo" ) );
}

void CollectionJobTest::testEmptyFolderList( )
{
  CollectionListJob *job = new CollectionListJob( "/res3", false );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  delete job;

  QCOMPARE( list.count(), 0 );
}

void CollectionJobTest::testSearchFolderList( )
{
  CollectionListJob *job = new CollectionListJob( Collection::root() + Collection::delimiter() + Collection::searchFolder(), false );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  delete job;

  QCOMPARE( list.count(), 3 );
  Collection *col = findCol( list, "Search/Test ?er" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::Virtual );
  QVERIFY( findCol( list, "Search/all" ) != 0 );
  QVERIFY( findCol( list, "Search/kde-core-devel" ) != 0 );
}

void CollectionJobTest::testResourceFolderList()
{
  // non-existing resource
  CollectionListJob *job = new CollectionListJob( Collection::root() );
  job->setResource( "i_dont_exist" );
  QVERIFY( !job->exec() );

  // recursive listing of all collections of an existing resource
  job = new CollectionListJob( Collection::root(), true );
  job->setResource( "akonadi_dummy_resource_1" );
  QVERIFY( job->exec() );

  Collection::List list = job->collections();
  QCOMPARE( list.count(), 5 );
  QVERIFY( findCol( list, "res1" ) );
  QVERIFY( findCol( list, "res1/foo" ) );
  QVERIFY( findCol( list, "res1/foo/bar" ) );
  QVERIFY( findCol( list, "res1/foo/bar/bla" ) );

  // limited listing of a resource
  job = new CollectionListJob( "res1/foo", true );
  job->setResource( "akonadi_dummy_resource_1" );
  QVERIFY( job->exec() );

  list = job->collections();
  QCOMPARE( list.count(), 3 );
  QVERIFY( findCol( list, "res1/foo/bar" ) );
  QVERIFY( findCol( list, "res1/foo/bar/bla" ) );
  QVERIFY( findCol( list, "res1/foo/bla" ) );
}

void CollectionJobTest::testIllegalCreateFolder( )
{
  // root
  CollectionCreateJob *job = new CollectionCreateJob( "/", this );
  QVERIFY( !job->exec() );

  // empty
  job = new CollectionCreateJob( "" , this );
  QVERIFY( !job->exec() );

  // search folder
  job = new CollectionCreateJob( "/Search/New Folder", this );
  QVERIFY( !job->exec() );

  // already existing folder
  job = new CollectionCreateJob( "res2/foo2", this );
  QVERIFY( !job->exec() );

  // Parent folder with \Noinferiors flag
  job = new CollectionCreateJob( "res2/foo2/bar", this );
  QVERIFY( !job->exec() );
}

void CollectionJobTest::testCreateDeleteFolder( )
{
  // simple new folder
  CollectionCreateJob *job = new CollectionCreateJob( "res3/new folder", this );
  QVERIFY( job->exec() );

  CollectionListJob *ljob = new CollectionListJob( "/res3", false, this );
  QVERIFY( ljob->exec() );
  QVERIFY( findCol( ljob->collections(), "res3/new folder" ) != 0 );

  CollectionDeleteJob *del = new CollectionDeleteJob( "res3/new folder", this );
  QVERIFY( del->exec() );

  ljob = new CollectionListJob( "res3", false, this );
  QVERIFY( ljob->exec() );
  QVERIFY( findCol( ljob->collections(), "res3/new folder" ) == 0 );

  // folder that already exists within another resource
  job = new CollectionCreateJob( "res3/foo", this );
  QVERIFY( job->exec() );

  ljob = new CollectionListJob( "/res3", false, this );
  QVERIFY( ljob->exec() );
  QVERIFY( findCol( ljob->collections(), "res3/foo" ) != 0 );

  del = new CollectionDeleteJob( "res3/foo", this );
  QVERIFY( del->exec() );

  ljob = new CollectionListJob( "res3", false, this );
  QVERIFY( ljob->exec() );
  QVERIFY( findCol( ljob->collections(), "res3/foo" ) == 0 );

  // folder with mime types
  job = new CollectionCreateJob( "res3/mail folder", this );
  QList<QByteArray> mimeTypes;
  mimeTypes << "inode/directory" << "message/rfc822";
  job->setContentTypes( mimeTypes );
  QVERIFY( job->exec() );
  delete job;

  CollectionStatusJob *status = new CollectionStatusJob( "res3/mail folder", this );
  QVERIFY( status->exec() );
  QList<QByteArray> mimeTypes2 = status->mimeTypes();
  delete status;
  QCOMPARE( mimeTypes2.count(), 2 );
  QCOMPARE( mimeTypes, mimeTypes2 );

  del = new CollectionDeleteJob( "res3/mail folder", this );
  QVERIFY( del->exec() );
  delete del;
}

void CollectionJobTest::testCreateDeleteFolderRecursive()
{
  // folder with missing parents
  CollectionCreateJob *job = new CollectionCreateJob( "res3/sub1/sub2/sub3", this );
  QVERIFY( job->exec() );

  CollectionListJob *ljob = new CollectionListJob( "/res3", true, this );
  QVERIFY( ljob->exec() );
  QVERIFY( findCol( ljob->collections(), "res3/sub1" ) != 0 );
  QVERIFY( findCol( ljob->collections(), "res3/sub1/sub2" ) != 0 );
  QVERIFY( findCol( ljob->collections(), "res3/sub1/sub2/sub3" ) != 0 );

  CollectionDeleteJob *del = new CollectionDeleteJob( "res3/sub1", this );
  QVERIFY( del->exec() );

  ljob = new CollectionListJob( "/res3", true, this );
  QVERIFY( ljob->exec() );
  QVERIFY( findCol( ljob->collections(), "res3/sub1" ) == 0 );
  QVERIFY( findCol( ljob->collections(), "res3/sub1/sub2" ) == 0 );
  QVERIFY( findCol( ljob->collections(), "res3/sub1/sub2/sub3" ) == 0 );
}

void CollectionJobTest::testStatus()
{
  // empty folder
  CollectionStatusJob *status = new CollectionStatusJob( "res1", this );
  QVERIFY( status->exec() );

  QList<CollectionAttribute*> attrs = status->attributes();
  QCOMPARE( attrs.count(), 1 );
  MessageCollectionAttribute *attr = dynamic_cast<MessageCollectionAttribute*>( attrs.first() );
  QVERIFY( attr != 0 );
  QCOMPARE( attr->count(), 0 );
  QCOMPARE( attr->unreadCount(), 0 );

  qDebug() << "mimeTypes: " << status->mimeTypes();
  QVERIFY( status->mimeTypes().isEmpty() );

  // folder with attributes and content
  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );

  attrs = status->attributes();
  QCOMPARE( attrs.count(), 1 );
  attr = dynamic_cast<MessageCollectionAttribute*>( attrs.first() );
  QVERIFY( attr != 0 );
  QCOMPARE( attr->count(), 3 );
  QCOMPARE( attr->unreadCount(), 0 );

  QList<QByteArray> mimeTypes = status->mimeTypes();
  QCOMPARE( mimeTypes.count(), 3 );
  QVERIFY( mimeTypes.contains( "text/calendar" ) );
  QVERIFY( mimeTypes.contains( "text/vcard" ) );
  QVERIFY( mimeTypes.contains( "message/rfc822" ) );
}

void CollectionJobTest::testModify()
{
  QList<QByteArray> reference;
  reference << "text/calendar" << "text/vcard" << "message/rfc822";

  // test noop modify
  CollectionModifyJob *mod = new CollectionModifyJob( "res1/foo", this );
  QVERIFY( mod->exec() );
  delete mod;

  CollectionStatusJob *status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );
  QCOMPARE( status->mimeTypes(), reference );
  delete status;

  // test clearing content types
  mod = new CollectionModifyJob( "res1/foo", this );
  mod->setContentTypes( QList<QByteArray>() );
  QVERIFY( mod->exec() );
  delete mod;

  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );
  QVERIFY( status->mimeTypes().isEmpty() );
  delete status;

  // test setting contnet types
  mod = new CollectionModifyJob( "res1/foo", this );
  mod->setContentTypes( reference );
  QVERIFY( mod->exec() );
  delete mod;

  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );
  QCOMPARE( status->mimeTypes(), reference );
  delete status;
}



#include "collectionjobtest.moc"
