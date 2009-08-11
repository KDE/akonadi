/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "customfoldertest.h"

#include <KDebug>
#include <KProcess>

#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/control.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/kmime/localfolders.h>

using namespace Akonadi;


void CustomFolderTest::initTestCase()
{
  QVERIFY( Control::start() );
  QTest::qWait( 1000 );
}

void CustomFolderTest::testDefaultFolders()
{
  LocalFolders::self()->fetch();
  QTest::kWaitForSignal( LocalFolders::self(), SIGNAL(foldersReady()) );
  for( int type = LocalFolders::Root; type < LocalFolders::LastDefaultType; type++ ) {
    QVERIFY( LocalFolders::self()->folder( type ).isValid() );

    // TODO verify icon, name, rights.
    // TODO verify functions like outbox().
  }
}

void CustomFolderTest::testCustomFoldersDontRebuild()
{
  QVERIFY( LocalFolders::self()->isReady() );
  Collection a, b;

  // Create a custom folder in root.
  {
    const Collection root = LocalFolders::self()->root();
    QVERIFY( root.isValid() );

    a.setParent( root.id() );
    a.setName( "a" );
    CollectionCreateJob *cjob = new CollectionCreateJob( a );
    AKVERIFYEXEC( cjob );
    a = cjob->collection();
    QVERIFY( LocalFolders::self()->isReady() );
  }

  // Create a custom folder in a default folder (inbox).
  {
    const Collection inbox = LocalFolders::self()->inbox();
    QVERIFY( inbox.isValid() );

    b.setParent( inbox.id() );
    b.setName( "b" );
    CollectionCreateJob *cjob = new CollectionCreateJob( b );
    AKVERIFYEXEC( cjob );
    b = cjob->collection();
    QVERIFY( LocalFolders::self()->isReady() );
  }

  QTest::qWait( 500 );

  // Delete those, and make sure LocalFolders does not rebuild.
  {
    QVERIFY( LocalFolders::self()->isReady() );
    CollectionDeleteJob *djob = new CollectionDeleteJob( a );
    AKVERIFYEXEC( djob );
    QVERIFY( LocalFolders::self()->isReady() );
    djob = new CollectionDeleteJob( b );
    AKVERIFYEXEC( djob );
    QVERIFY( LocalFolders::self()->isReady() );

    for( int i = 0; i < 200; i++ ) {
      QVERIFY( LocalFolders::self()->isReady() );
      QTest::qWait( 10 );
    }
  }

  // Check that a fetch() does not trigger a rebuild.
  {
    LocalFolders::self()->fetch();
    QVERIFY( LocalFolders::self()->isReady() );
  }
}

// NOTE: This was written when I added LocalFolders::customFolders().
// I removed that later because I'm not sure how it could be useful.
#if 0
void CustomFolderTest::testCustomFoldersInRoot()
{
  QSet<Collection> mine;
  QVERIFY( LocalFolders::self()->isReady() );
  const Collection root = LocalFolders::self()->root();
  QVERIFY( root.isValid() );

  // No custom folders yet.
  QVERIFY( LocalFolders::self()->customFolders().isEmpty() );

  // Create hierarchy of custom folders under root:
  // (a/(a1,a2/a2i), b/b1)
  {
    Collection a;
    a.setParent( root.id() );
    a.setName( "a" );
    CollectionCreateJob *cjob = new CollectionCreateJob( a );
    AKVERIFYEXEC( cjob );
    a = cjob->collection();
    mine.insert( a );

    Collection a1;
    a1.setParent( a.id() );
    a1.setName( "a1" );
    cjob = new CollectionCreateJob( a1 );
    AKVERIFYEXEC( cjob );
    a1 = cjob->collection();
    mine.insert( a1 );

    Collection a2;
    a2.setParent( a.id() );
    a2.setName( "a2" );
    cjob = new CollectionCreateJob( a2 );
    AKVERIFYEXEC( cjob );
    a2 = cjob->collection();
    mine.insert( a2 );

    Collection a2i;
    a2i.setParent( a2.id() );
    a2i.setName( "a2i" );
    cjob = new CollectionCreateJob( a2i );
    AKVERIFYEXEC( cjob );
    a2i = cjob->collection();
    mine.insert( a2i );

    Collection b;
    b.setParent( root.id() );
    b.setName( "b" );
    cjob = new CollectionCreateJob( b );
    AKVERIFYEXEC( cjob );
    b = cjob->collection();
    mine.insert( b );

    Collection b1;
    b1.setParent( root.id() );
    b1.setName( "b1" );
    cjob = new CollectionCreateJob( b1 );
    AKVERIFYEXEC( cjob );
    b1 = cjob->collection();
    mine.insert( b1 );
  }

  // Check that LocalFolders is aware of everything.
  QTest::qWait( 500 );
  QSet<Collection> theirs = LocalFolders::self()->customFolders();
  QCOMPARE( mine, theirs );

  // Remove the custom folders and make sure LocalFolders does not rebuild.
  foreach( const Collection &col, mine ) {
    QVERIFY( LocalFolders::self()->isReady() );
    CollectionDeleteJob *djob = new CollectionDeleteJob( col );
    AKVERIFYEXEC( djob );

    for( int i = 0; i < 50; i++ ) {
      QVERIFY( LocalFolders::self()->isReady() );
      QTest::qWait( 10 );
    }

    mine.remove( col );
    theirs = LocalFolders::self()->customFolders();
    QCOMPARE( mine, theirs );
  }

  // Check that a fetch() does not trigger a rebuild.
  LocalFolders::self()->fetch();
  QVERIFY( LocalFolders::self()->isReady() );

  // No custom folders after deletion.
  QVERIFY( LocalFolders::self()->customFolders().isEmpty() );
}

void CustomFolderTest::testCustomFoldersInDefaultFolders()
{
  QVERIFY( LocalFolders::self()->isReady() );
  const Collection inbox = LocalFolders::self()->inbox();
  QVERIFY( inbox.isValid() );

  // No custom folders yet.
  QVERIFY( LocalFolders::self()->customFolders().isEmpty() );

  // Create a custom folder under inbox.
  Collection col;
  col.setParent( inbox.id() );
  col.setName( "col" );
  CollectionCreateJob *cjob = new CollectionCreateJob( col );
  AKVERIFYEXEC( cjob );
  col = cjob->collection();

  // Verify that LocalFolders is aware of it.
  QTest::qWait( 500 );
  QSet<Collection> theirs = LocalFolders::self()->customFolders();
  QCOMPARE( theirs.count(), 1 );
  QVERIFY( theirs.contains( col ) );

  // Delete it and make sure LocalFolders does not rebuild.
  QVERIFY( LocalFolders::self()->isReady() );
  CollectionDeleteJob *djob = new CollectionDeleteJob( col );
  AKVERIFYEXEC( djob );
  for( int i = 0; i < 50; i++ ) {
    QVERIFY( LocalFolders::self()->isReady() );
    QTest::qWait( 10 );
  }

  // Check that a fetch() does not trigger a rebuild.
  LocalFolders::self()->fetch();
  QVERIFY( LocalFolders::self()->isReady() );

  // No custom folders after deletion.
  QVERIFY( LocalFolders::self()->customFolders().isEmpty() );
}
#endif


QTEST_AKONADIMAIN( CustomFolderTest, NoGUI )

#include "customfoldertest.moc"
