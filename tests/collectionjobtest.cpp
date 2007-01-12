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
#include "collectionrenamejob.h"
#include "collectionstatusjob.h"
#include "control.h"
#include "messagecollectionattribute.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi;

QTEST_KDEMAIN( CollectionJobTest, NoGUI )

void CollectionJobTest::initTestCase()
{
  Control::start();
}

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

template <typename T> static T* extractAttribute( QList<CollectionAttribute*> attrs )
{
  T dummy;
  foreach ( CollectionAttribute* attr, attrs ) {
    if ( attr->type() == dummy.type() )
      return dynamic_cast<T*>( attr );
  }
  return 0;
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
  CollectionContentTypeAttribute *attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  compareLists( attr->contentTypes(), mimeTypes );
  delete status;

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
  QCOMPARE( attrs.count(), 2 );
  MessageCollectionAttribute *mcattr = extractAttribute<MessageCollectionAttribute>( attrs );
  QVERIFY( mcattr != 0 );
  QCOMPARE( mcattr->count(), 0 );
  QCOMPARE( mcattr->unreadCount(), 0 );

  CollectionContentTypeAttribute *ctattr = extractAttribute<CollectionContentTypeAttribute>( attrs );
  QVERIFY( ctattr != 0 );
  QVERIFY( ctattr->contentTypes().isEmpty() );

  // folder with attributes and content
  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );

  attrs = status->attributes();
  QCOMPARE( attrs.count(), 2 );
  mcattr = extractAttribute<MessageCollectionAttribute>( attrs );
  QVERIFY( mcattr != 0 );
  QCOMPARE( mcattr->count(), 3 );
  QCOMPARE( mcattr->unreadCount(), 0 );

  ctattr = extractAttribute<CollectionContentTypeAttribute>( attrs );
  QVERIFY( ctattr != 0 );
  QList<QByteArray> mimeTypes = ctattr->contentTypes();
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
  CollectionContentTypeAttribute *attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  compareLists( attr->contentTypes(), reference );
  delete status;

  // test clearing content types
  mod = new CollectionModifyJob( "res1/foo", this );
  mod->setContentTypes( QList<QByteArray>() );
  QVERIFY( mod->exec() );
  delete mod;

  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );
  attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  QVERIFY( attr->contentTypes().isEmpty() );
  delete status;

  // test setting contnet types
  mod = new CollectionModifyJob( "res1/foo", this );
  mod->setContentTypes( reference );
  QVERIFY( mod->exec() );
  delete mod;

  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );
  attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  compareLists( attr->contentTypes(), reference );
  delete status;
}

void CollectionJobTest::testRename()
{
  CollectionRenameJob *rename = new CollectionRenameJob( "res1", "res1 (renamed)", this );
  QVERIFY( rename->exec() );
  delete rename;

  CollectionListJob *ljob = new CollectionListJob( "res1 (renamed)", true );
  QVERIFY( ljob->exec() );
  Collection::List list = ljob->collections();
  delete ljob;

  QCOMPARE( list.count(), 4 );
  QVERIFY( findCol( list, "res1 (renamed)/foo" ) != 0 );
  QVERIFY( findCol( list, "res1 (renamed)/foo/bar" ) != 0 );
  QVERIFY( findCol( list, "res1 (renamed)/foo/bla" ) != 0 );
  QVERIFY( findCol( list, "res1 (renamed)/foo/bar/bla" ) != 0 );

  // cleanup
  rename = new CollectionRenameJob( "res1 (renamed)", "res1", this );
  QVERIFY( rename->exec() );
  delete rename;
}

void CollectionJobTest::testIllegalRename()
{
  // non-existing collection
  CollectionRenameJob *rename = new CollectionRenameJob( "i dont exist", "i dont exist either", this );
  QVERIFY( !rename->exec() );
  delete rename;

  // already existing target
  rename = new CollectionRenameJob( "res1", "res2", this );
  QVERIFY( !rename->exec() );
  delete rename;

  // root being source or target
  rename = new CollectionRenameJob( Collection::root(), "some name", this );
  QVERIFY( !rename->exec() );
  delete rename;

  rename = new CollectionRenameJob( "res1", Collection::root(), this );
  QVERIFY( !rename->exec() );
  delete rename;
}

void CollectionJobTest::testUtf8CollectionName()
{
  QString folderName = QString::fromUtf8( "res3/ä" );

  // create collection
  CollectionCreateJob *create = new CollectionCreateJob( folderName, this );
  QVERIFY( create->exec() );
  delete create;

  // list parent
  CollectionListJob *list = new CollectionListJob( "res3", true, this );
  QVERIFY( list->exec() );
  QCOMPARE( list->collections().count(), 1 );
  Collection *col = list->collections().first();
  QCOMPARE( col->path(), folderName );
  delete list;

  // modify collection
  CollectionModifyJob *modify = new CollectionModifyJob( folderName, this );
  QList<QByteArray> contentTypes;
  contentTypes << "message/rfc822";
  modify->setContentTypes( contentTypes );
  QVERIFY( modify->exec() );
  delete modify;

  // collection status
  CollectionStatusJob *status = new CollectionStatusJob( folderName, this );
  QVERIFY( status->exec() );
  CollectionContentTypeAttribute *ccta = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( ccta );
  compareLists( ccta->contentTypes(), contentTypes );

  // delete collection
  CollectionDeleteJob *del = new CollectionDeleteJob( folderName, this );
  QVERIFY( del->exec() );
  delete del;
}

#include "collectionjobtest.moc"
