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

#include "localfolderstest.h"

#include "../../collectionpathresolver_p.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QFile>
#include <QSignalSpy>

#include <KDebug>
#include <KStandardDirs>

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/control.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/kmime/localfolderattribute.h>
#include <akonadi/kmime/localfolders.h>
#include "../localfolderstesting.h"
#include "../localfoldershelperjobs_p.h"

using namespace Akonadi;

static Collection res1;

void LocalFoldersTest::initTestCase()
{
  QVERIFY( Control::start() );
  QTest::qWait( 1000 );

  CollectionPathResolver *resolver = new CollectionPathResolver( "res1", this );
  QVERIFY( resolver->exec() );
  res1 = Collection( resolver->collection() );

  CollectionFetchJob *fjob = new CollectionFetchJob( res1, CollectionFetchJob::Base, this );
  AKVERIFYEXEC( fjob );
  Q_ASSERT( fjob->collections().count() == 1 );
  res1 = fjob->collections().first();
  QVERIFY( !res1.resource().isEmpty() );
}

void LocalFoldersTest::testLock()
{
  const QString dbusName = QString::fromLatin1( "org.kde.pim.LocalFolders" );

  // Initially not locked.
  QVERIFY( !QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusName ) );

  // Get the lock.
  {
    GetLockJob *ljob = new GetLockJob( this );
    AKVERIFYEXEC( ljob );
    QVERIFY( QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusName ) );
  }

  // Getting the lock again should fail.
  {
    GetLockJob *ljob = new GetLockJob( this );
    QVERIFY( !ljob->exec() );
  }

  // Release the lock.
  QVERIFY( QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusName ) );
  releaseLock();
  QVERIFY( !QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusName ) );
}

void LocalFoldersTest::testInitialState()
{
  LocalFolders *lf = LocalFolders::self();
  LocalFoldersTesting *lft = LocalFoldersTesting::_t_self();
  Q_ASSERT( lf );
  Q_ASSERT( lft );

  // No one has created the default resource.
  QVERIFY( lf->defaultResourceId().isEmpty() );

  // LF does not have a default Outbox folder (or any other).
  QCOMPARE( lf->hasDefaultFolder( LocalFolders::Outbox ), false );
  QVERIFY( !lf->defaultFolder( LocalFolders::Outbox ).isValid() );

  // LF treats unknown resources correctly.
  const QString resourceId = QString::fromLatin1( "this_resource_does_not_exist" );
  QCOMPARE( lf->hasFolder( LocalFolders::Outbox, resourceId ), false );
  QVERIFY( !lf->folder( LocalFolders::Outbox, resourceId ).isValid() );
}

void LocalFoldersTest::testRegistrationErrors()
{
  LocalFolders *lf = LocalFolders::self();

  // A valid collection that can be registered.
  Collection outbox;
  outbox.setName( QLatin1String( "my_outbox" ) );
  outbox.setParentCollection( res1 );
  outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
  outbox.setResource( res1.resource() );

  // Needs to be valid.
  {
    Collection col( outbox );
    col.setId( -1 );
    QVERIFY( !lf->registerFolder( col ) );
  }

  // Needs to have a resourceId.
  {
    Collection col( outbox );
    col.setResource( QString() );
    QVERIFY( !lf->registerFolder( col ) );
  }

  // Needs to have a LocalFolderAttribute.
  {
    Collection col( outbox );
    col.removeAttribute<LocalFolderAttribute>();
    QVERIFY( !lf->registerFolder( col ) );
  }
}

void LocalFoldersTest::testDefaultFolderRegistration()
{
  LocalFolders *lf = LocalFolders::self();
  LocalFoldersTesting *lft = LocalFoldersTesting::_t_self();
  Q_ASSERT( lf );
  Q_ASSERT( lft );
  QSignalSpy spy( lf, SIGNAL(foldersChanged(QString)) );
  QSignalSpy defSpy( lf, SIGNAL(defaultFoldersChanged()) );

  // No one has created the default resource.
  QVERIFY( lf->defaultResourceId().isEmpty() );

  // Set the default resource. LF should still have no folders.
  lft->_t_setDefaultResourceId( res1.resource() );
  QCOMPARE( lf->defaultResourceId(), res1.resource() );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );

  // Manually create an Outbox collection.
  Collection outbox;
  outbox.setName( QLatin1String( "my_outbox" ) );
  outbox.setParentCollection( res1 );
  outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
  CollectionCreateJob *cjob = new CollectionCreateJob( outbox, this );
  AKVERIFYEXEC( cjob );
  outbox = cjob->collection();

  // LF should still have no folders, since the Outbox wasn't registered.
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 0 );
  QCOMPARE( defSpy.count(), 0 );

  // Register the collection. LF should now know it.
  bool ok = lf->registerFolder( outbox );
  QVERIFY( ok );
  QVERIFY( lf->hasDefaultFolder( LocalFolders::Outbox ) );
  QCOMPARE( lf->defaultFolder( LocalFolders::Outbox ), outbox );
  QCOMPARE( lft->_t_knownResourceCount(), 1 );
  QCOMPARE( lft->_t_knownFolderCount(), 1 );
  QCOMPARE( spy.count(), 1 );
  QCOMPARE( defSpy.count(), 1 );

  // Forget all folders in the default resource.
  lft->_t_forgetFoldersForResource( lf->defaultResourceId() );
  QCOMPARE( lf->hasDefaultFolder( LocalFolders::Outbox ), false );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 2 );
  QCOMPARE( defSpy.count(), 2 );

  // Delete the collection.
  CollectionDeleteJob *djob = new CollectionDeleteJob( outbox, this );
  AKVERIFYEXEC( djob );
}

void LocalFoldersTest::testCustomFolderRegistration()
{
  LocalFolders *lf = LocalFolders::self();
  LocalFoldersTesting *lft = LocalFoldersTesting::_t_self();
  Q_ASSERT( lf );
  Q_ASSERT( lft );
  QSignalSpy spy( lf, SIGNAL(foldersChanged(QString)) );
  QSignalSpy defSpy( lf, SIGNAL(defaultFoldersChanged()) );

  // Set a fake default resource, so that res1.resource() isn't seen as the default one.
  // LF should have no folders.
  lft->_t_setDefaultResourceId( "dummy" );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );

  // Manually create an Outbox collection.
  Collection outbox;
  outbox.setName( QLatin1String( "my_outbox" ) );
  outbox.setParentCollection( res1 );
  outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
  CollectionCreateJob *cjob = new CollectionCreateJob( outbox, this );
  AKVERIFYEXEC( cjob );
  outbox = cjob->collection();

  // LF should still have no folders, since the Outbox wasn't registered.
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 0 );
  QCOMPARE( defSpy.count(), 0 );

  // Register the collection. LF should now know it.
  bool ok = lf->registerFolder( outbox );
  QVERIFY( ok );
  QVERIFY( !lf->hasDefaultFolder( LocalFolders::Outbox ) );
  QVERIFY( lf->hasFolder( LocalFolders::Outbox, outbox.resource() ) );
  QCOMPARE( lf->folder( LocalFolders::Outbox, outbox.resource() ), outbox );
  QCOMPARE( lft->_t_knownResourceCount(), 1 );
  QCOMPARE( lft->_t_knownFolderCount(), 1 );
  QCOMPARE( spy.count(), 1 );
  QCOMPARE( defSpy.count(), 0 );

  // Forget all folders in this resource.
  lft->_t_forgetFoldersForResource( outbox.resource() );
  QCOMPARE( lf->hasDefaultFolder( LocalFolders::Outbox ), false );
  QCOMPARE( lf->hasFolder( LocalFolders::Outbox, outbox.resource() ), false );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 2 );
  QCOMPARE( defSpy.count(), 0 );

  // Delete the collection.
  CollectionDeleteJob *djob = new CollectionDeleteJob( outbox, this );
  AKVERIFYEXEC( djob );
}

void LocalFoldersTest::testCollectionDelete()
{
  LocalFolders *lf = LocalFolders::self();
  LocalFoldersTesting *lft = LocalFoldersTesting::_t_self();
  Q_ASSERT( lf );
  Q_ASSERT( lft );
  QSignalSpy spy( lf, SIGNAL(foldersChanged(QString)) );
  QSignalSpy defSpy( lf, SIGNAL(defaultFoldersChanged()) );

  // Set the default resource. LF should have no folders.
  lft->_t_setDefaultResourceId( res1.resource() );
  QCOMPARE( lf->defaultResourceId(), res1.resource() );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );

  // Manually create an Outbox collection.
  Collection outbox;
  outbox.setName( QLatin1String( "my_outbox" ) );
  outbox.setParentCollection( res1 );
  outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
  CollectionCreateJob *cjob = new CollectionCreateJob( outbox, this );
  AKVERIFYEXEC( cjob );
  outbox = cjob->collection();

  // LF should still have no folders, since the Outbox wasn't registered.
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 0 );
  QCOMPARE( defSpy.count(), 0 );

  // Register the collection. LF should now know it.
  bool ok = lf->registerFolder( outbox );
  QVERIFY( ok );
  QVERIFY( lf->hasDefaultFolder( LocalFolders::Outbox ) );
  QCOMPARE( lf->defaultFolder( LocalFolders::Outbox ), outbox );
  QCOMPARE( lft->_t_knownResourceCount(), 1 );
  QCOMPARE( lft->_t_knownFolderCount(), 1 );
  QCOMPARE( spy.count(), 1 );
  QCOMPARE( defSpy.count(), 1 );

  // Delete the collection. LF should watch for that.
  CollectionDeleteJob *djob = new CollectionDeleteJob( outbox, this );
  AKVERIFYEXEC( djob );
  QTest::qWait( 100 );
  QVERIFY( !lf->hasDefaultFolder( LocalFolders::Outbox ) );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 2 );
  QCOMPARE( defSpy.count(), 2 );
}

void LocalFoldersTest::testBatchRegister()
{
  LocalFolders *lf = LocalFolders::self();
  LocalFoldersTesting *lft = LocalFoldersTesting::_t_self();
  Q_ASSERT( lf );
  Q_ASSERT( lft );
  QSignalSpy spy( lf, SIGNAL(foldersChanged(QString)) );
  QSignalSpy defSpy( lf, SIGNAL(defaultFoldersChanged()) );

  // Set the default resource. LF should have no folders.
  lft->_t_setDefaultResourceId( res1.resource() );
  QCOMPARE( lf->defaultResourceId(), res1.resource() );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );

  // Manually create an Outbox collection.
  Collection outbox;
  outbox.setName( QLatin1String( "my_outbox" ) );
  outbox.setParentCollection( res1 );
  outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
  CollectionCreateJob *cjob = new CollectionCreateJob( outbox, this );
  AKVERIFYEXEC( cjob );
  outbox = cjob->collection();

  // Manually create a Drafts collection.
  Collection drafts;
  drafts.setName( QLatin1String( "my_drafts" ) );
  drafts.setParentCollection( res1 );
  drafts.addAttribute( new LocalFolderAttribute( LocalFolders::Drafts ) );
  cjob = new CollectionCreateJob( drafts, this );
  AKVERIFYEXEC( cjob );
  drafts = cjob->collection();

  // LF should still have no folders, since the folders were not registered.
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 0 );
  QCOMPARE( defSpy.count(), 0 );

  // Register the folders in batch mode.
  lft->_t_beginBatchRegister();
  bool ok = lf->registerFolder( outbox );
  QVERIFY( ok );
  QVERIFY( lf->hasDefaultFolder( LocalFolders::Outbox ) );
  QCOMPARE( lf->defaultFolder( LocalFolders::Outbox ), outbox );
  QCOMPARE( lft->_t_knownResourceCount(), 1 );
  QCOMPARE( lft->_t_knownFolderCount(), 1 );
  QCOMPARE( spy.count(), 0 );
  QCOMPARE( defSpy.count(), 0 );
  ok = lf->registerFolder( drafts );
  QVERIFY( ok );
  QVERIFY( lf->hasDefaultFolder( LocalFolders::Drafts ) );
  QCOMPARE( lf->defaultFolder( LocalFolders::Drafts ), drafts );
  QCOMPARE( lft->_t_knownResourceCount(), 1 );
  QCOMPARE( lft->_t_knownFolderCount(), 2 );
  QCOMPARE( spy.count(), 0 );
  QCOMPARE( defSpy.count(), 0 );
  lft->_t_endBatchRegister();
  QCOMPARE( lft->_t_knownResourceCount(), 1 );
  QCOMPARE( lft->_t_knownFolderCount(), 2 );
  QCOMPARE( spy.count(), 1 );
  QCOMPARE( defSpy.count(), 1 );

  // Forget all folders in the default resource.
  lft->_t_forgetFoldersForResource( lf->defaultResourceId() );
  QCOMPARE( lf->hasDefaultFolder( LocalFolders::Outbox ), false );
  QCOMPARE( lf->hasDefaultFolder( LocalFolders::Drafts ), false );
  QCOMPARE( lft->_t_knownResourceCount(), 0 );
  QCOMPARE( lft->_t_knownFolderCount(), 0 );
  QCOMPARE( spy.count(), 2 );
  QCOMPARE( defSpy.count(), 2 );

  // Delete the collections.
  CollectionDeleteJob *djob = new CollectionDeleteJob( outbox, this );
  AKVERIFYEXEC( djob );
  djob = new CollectionDeleteJob( drafts, this );
  AKVERIFYEXEC( djob );
}

void LocalFoldersTest::testResourceScanErrors()
{
  // Job fails if no resourceId is given.
  ResourceScanJob *resjob = new ResourceScanJob( QString(), this );
  QVERIFY( !resjob->exec() );
}

void LocalFoldersTest::testResourceScan()
{
  // Verify that res1 has no collections.
  {
    CollectionFetchJob *fjob = new CollectionFetchJob( res1, CollectionFetchJob::Recursive, this );
    AKVERIFYEXEC( fjob );
    QCOMPARE( fjob->collections().count(), 0 );
  }

  // Manually create an Outbox collection.
  Collection outbox;
  {
    outbox.setName( QLatin1String( "my_outbox" ) );
    outbox.setParentCollection( res1 );
    outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
    CollectionCreateJob *cjob = new CollectionCreateJob( outbox, this );
    AKVERIFYEXEC( cjob );
    outbox = cjob->collection();
  }

  // Manually create a Drafts collection.
  Collection drafts;
  {
    drafts.setName( QLatin1String( "my_drafts" ) );
    drafts.setParentCollection( res1 );
    drafts.addAttribute( new LocalFolderAttribute( LocalFolders::Drafts ) );
    CollectionCreateJob *cjob = new CollectionCreateJob( drafts, this );
    AKVERIFYEXEC( cjob );
    drafts = cjob->collection();
  }

  // Manually create a non-LocalFolder collection.
  Collection intruder;
  {
    intruder.setName( QLatin1String( "intruder" ) );
    intruder.setParentCollection( res1 );
    CollectionCreateJob *cjob = new CollectionCreateJob( intruder, this );
    AKVERIFYEXEC( cjob );
    intruder = cjob->collection();
  }

  // Verify that the collections have been created properly.
  {
    CollectionFetchJob *fjob = new CollectionFetchJob( res1, CollectionFetchJob::Recursive, this );
    AKVERIFYEXEC( fjob );
    QCOMPARE( fjob->collections().count(), 3 );
  }

  // Test that ResourceScanJob finds everything correctly.
  ResourceScanJob *resjob = new ResourceScanJob( res1.resource(), this );
  AKVERIFYEXEC( resjob );
  QCOMPARE( resjob->rootResourceCollection(), res1 );
  const Collection::List folders = resjob->localFolders();
  QCOMPARE( folders.count(), 2 );
  QVERIFY( folders.contains( outbox ) );
  QVERIFY( folders.contains( drafts ) );

  // Clean up after ourselves.
  CollectionDeleteJob *djob = new CollectionDeleteJob( outbox, this );
  AKVERIFYEXEC( djob );
  djob = new CollectionDeleteJob( drafts, this );
  AKVERIFYEXEC( djob );
  djob = new CollectionDeleteJob( intruder, this );
  AKVERIFYEXEC( djob );
}

void LocalFoldersTest::testDefaultResourceJob()
{
  // Initially the defaut maildir does not exist.
  QVERIFY( !QFile::exists( KGlobal::dirs()->localxdgdatadir() + nameForType( LocalFolders::Root ) ) );

  // Run the job.
  Collection maildirRoot;
  QString resourceId;
  {
    DefaultResourceJob *resjob = new DefaultResourceJob( this );
    AKVERIFYEXEC( resjob );
    resourceId = resjob->resourceId();
    const Collection::List folders = resjob->localFolders();
    QCOMPARE( folders.count(), 1 );
    maildirRoot = folders.first();
    QVERIFY( maildirRoot.hasAttribute<LocalFolderAttribute>() );
    QCOMPARE( maildirRoot.attribute<LocalFolderAttribute>()->folderType(), int( LocalFolders::Root ) );
  }

  // The maildir should exist now.
  QVERIFY( QFile::exists( KGlobal::dirs()->localxdgdatadir() + nameForType( LocalFolders::Root ) ) );

  // Create a LocalFolder in the default resource.
  Collection outbox;
  {
    outbox.setName( QLatin1String( "outbox" ) );
    outbox.setParentCollection( maildirRoot );
    outbox.addAttribute( new LocalFolderAttribute( LocalFolders::Outbox ) );
    CollectionCreateJob *cjob = new CollectionCreateJob( outbox, this );
    AKVERIFYEXEC( cjob );
    outbox = cjob->collection();

    // Wait for the maildir resource to actually create the folder on disk...
    // (needed in testRecoverDefaultResource())
    QTest::qWait( 500 );
  }

  // Run the job again.
  {
    DefaultResourceJob *resjob = new DefaultResourceJob( this );
    AKVERIFYEXEC( resjob );
    QCOMPARE( resourceId, resjob->resourceId() ); // Did not mistakenly create another resource.
    const Collection::List folders = resjob->localFolders();
    QCOMPARE( folders.count(), 2 );
    QVERIFY( folders.contains( maildirRoot ) );
    QVERIFY( folders.contains( outbox ) );
  }
}

void LocalFoldersTest::testRecoverDefaultResource()
{
  // The maildirs should exist (created in testDefaultResourceJob).
  const QString xdgPath = KGlobal::dirs()->localxdgdatadir();
  const QString rootPath = xdgPath + nameForType( LocalFolders::Root );
  const QString outboxPath = xdgPath + QString::fromLatin1( ".%1.directory/%2" ) \
                                       .arg( nameForType( LocalFolders::Root ) )
                                       .arg( nameForType( LocalFolders::Outbox ) );
  QVERIFY( QFile::exists( rootPath ) );
  QVERIFY( QFile::exists( outboxPath ) );

  // Kill the default resource.
  const QString oldResourceId = LocalFolders::self()->defaultResourceId();
  const AgentInstance resource = AgentManager::self()->instance( oldResourceId );
  QVERIFY( resource.isValid() );
  AgentManager::self()->removeInstance( resource );
  QTest::qWait( 100 );

  // Run the DefaultResourceJob now. It should recover the Root and Outbox folders
  // created in testDefaultResourceJob.
  {
    DefaultResourceJob *resjob = new DefaultResourceJob( this );
    AKVERIFYEXEC( resjob );
    QVERIFY( resjob->resourceId() != oldResourceId ); // Created another resource.
    Collection::List folders = resjob->localFolders();
    QCOMPARE( folders.count(), 2 );

    // Reorder the folders.
    if( folders.first().parentCollection() != Collection::root() ) {
      folders.swap( 0, 1 );
    }

    // The first folder should be the Root.
    {
      Collection col = folders[0];
      QCOMPARE( col.name(), nameForType( LocalFolders::Root ) );
      QVERIFY( col.hasAttribute<LocalFolderAttribute>() );
      QCOMPARE( col.attribute<LocalFolderAttribute>()->folderType(), int( LocalFolders::Root ) );
    }

    // The second folder should be the Outbox.
    {
      Collection col = folders[1];
      QCOMPARE( col.name(), nameForType( LocalFolders::Outbox ) );
      QVERIFY( col.hasAttribute<LocalFolderAttribute>() );
      QCOMPARE( col.attribute<LocalFolderAttribute>()->folderType(), int( LocalFolders::Outbox ) );
    }
  }
}

QTEST_AKONADIMAIN( LocalFoldersTest, NoGUI )

#include "localfolderstest.moc"
