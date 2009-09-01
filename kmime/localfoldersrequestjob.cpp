/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "localfoldersrequestjob.h"

#include "localfolderattribute.h"
#include "localfolders.h"
#include "localfoldershelperjobs_p.h"
#include "localfolderssettings.h"

#include <KDebug>

#include "akonadi/collectioncreatejob.h"
#include "akonadi/entitydisplayattribute.h"

using namespace Akonadi;

typedef LocalFoldersSettings Settings;

/**
  @internal
*/
class Akonadi::LocalFoldersRequestJobPrivate
{
  public:
    LocalFoldersRequestJobPrivate( LocalFoldersRequestJob *qq );

    bool isEverythingReady();
    void lockResult( KJob *job ); // slot
    void releaseLock(); // slot
    void nextResource();
    void resourceScanResult( KJob *job ); // slot
    void createRequestedFolders( ResourceScanJob *resjob, QList<bool> &requestedFolders );
    void collectionCreateResult( KJob *job ); // slot

    LocalFoldersRequestJob *q;
    QList<bool> falseBoolList;
    int pendingCreateJobs;

    // Input:
    QList<bool> defaultFolders;
    bool requestingDefaultFolders;
    QHash< QString, QList<bool> > foldersForResource;

    // Output:
    QStringList toForget;
    QList<Collection> toRegister;
};



LocalFoldersRequestJobPrivate::LocalFoldersRequestJobPrivate( LocalFoldersRequestJob *qq )
  : q( qq )
  , pendingCreateJobs( 0 )
  , requestingDefaultFolders( false )
{
  for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
    falseBoolList.append( false );
  }

  defaultFolders = falseBoolList;
}

bool LocalFoldersRequestJobPrivate::isEverythingReady()
{
  LocalFolders *lf = LocalFolders::self();

  if( requestingDefaultFolders ) {
    for( int i = 0; i < LocalFolders::LastDefaultType; i++ ) {
      if( defaultFolders[i] && !lf->hasDefaultFolder( i ) ) {
        return false;
      }
    }
  }

  foreach( const QString &resourceId, foldersForResource.keys() ) {
    const QList<bool> &requested = foldersForResource[ resourceId ];
    for( int i = 0; i < LocalFolders::LastDefaultType; i++ ) {
      if( requested[i] && !lf->hasFolder( i, resourceId ) ) {
        return false;
      }
    }
  }

  kDebug() << "All requested folders already known.";
  return true;
}

void LocalFoldersRequestJobPrivate::lockResult( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed to get lock:" << job->errorString();
    return;
  }

  if( requestingDefaultFolders ) {
    // If default folders are requested, deal with that first.
    DefaultResourceJob *resjob = new DefaultResourceJob( q );
    QObject::connect( resjob, SIGNAL(result(KJob*)), q, SLOT(resourceScanResult(KJob*)) );
  } else {
    // If no default folders are requested, go straight to the next step.
    nextResource();
  }
}

void LocalFoldersRequestJobPrivate::releaseLock()
{
  const bool ok = Akonadi::releaseLock();
  if( !ok ) {
    kWarning() << "WTF, can't release lock.";
  }
}

void LocalFoldersRequestJobPrivate::nextResource()
{
  if( foldersForResource.isEmpty() ) {
    kDebug() << "All done! Comitting.";

    LocalFolders::self()->beginBatchRegister();

    // Forget everything we knew before about these resources.
    foreach( const QString &resourceId, toForget ) {
      LocalFolders::self()->forgetFoldersForResource( resourceId );
    }

    // Register all the collections that we fetched / created.
    foreach( const Collection &col, toRegister ) {
      const bool ok = LocalFolders::self()->registerFolder( col );
      Q_ASSERT( ok );
    }

    LocalFolders::self()->endBatchRegister();

    // We are done!
    q->commit();

    // Release the lock once the transaction has been committed.
    QObject::connect( q, SIGNAL(result(KJob*)), q, SLOT(releaseLock()) );
  } else {
    const QString resourceId = foldersForResource.keys().first();
    kDebug() << "A resource is done," << foldersForResource.count()
             << "more to do. Now doing resource" << resourceId;
    ResourceScanJob *resjob = new ResourceScanJob( resourceId, q );
    QObject::connect( resjob, SIGNAL(result(KJob*)), q, SLOT(resourceScanResult(KJob*)) );
  }
}

void LocalFoldersRequestJobPrivate::resourceScanResult( KJob *job )
{
  Q_ASSERT( dynamic_cast<ResourceScanJob*>( job ) );
  ResourceScanJob *resjob = static_cast<ResourceScanJob*>( job );
  const QString resourceId = resjob->resourceId();
  kDebug() << "resourceId" << resourceId;

  if( job->error() ) {
    kWarning() << "Failed to request resource" << resourceId << ":" << job->errorString();
    return;
  }

  if( dynamic_cast<DefaultResourceJob*>( job ) ) {
    // This is the default resource.
    Q_ASSERT( resourceId == Settings::defaultResourceId() );
    //toForget.append( LocalFolders::self()->defaultResourceId() );
    createRequestedFolders( resjob, defaultFolders );
  } else {
    // This is not the default resource.
    QList<bool> requestedFolders = foldersForResource[ resourceId ];
    foldersForResource.remove( resourceId );
    createRequestedFolders( resjob, requestedFolders );
  }
}

void LocalFoldersRequestJobPrivate::createRequestedFolders( ResourceScanJob *resjob,
                                          QList<bool> &requestedFolders )
{
  Q_ASSERT( requestedFolders.count() == LocalFolders::LastDefaultType );

  // Remove from the request list the folders which already exist.
  foreach( const Collection &col, resjob->localFolders() ) {
    toRegister.append( col );
    Q_ASSERT( col.hasAttribute<LocalFolderAttribute>() );
    const LocalFolderAttribute *attr = col.attribute<LocalFolderAttribute>();
    const int type = attr->folderType();
    requestedFolders[ type ] = false;
  }
  toForget.append( resjob->resourceId() );

  // Folders left in the request list must be created.
  Q_ASSERT( pendingCreateJobs == 0 );
  Q_ASSERT( resjob->rootResourceCollection().isValid() );
  for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
    if( requestedFolders[ type ] ) {
      Collection col;
      col.setParentCollection( resjob->rootResourceCollection() );
      col.setName( nameForType( type ) );
      setCollectionAttributes( col, type );
      CollectionCreateJob *cjob = new CollectionCreateJob( col, q );
      QObject::connect( cjob, SIGNAL(result(KJob*)), q, SLOT(collectionCreateResult(KJob*)) );
      pendingCreateJobs++;
    }
  }

  if( pendingCreateJobs == 0 ) {
    nextResource();
  }
}

void LocalFoldersRequestJobPrivate::collectionCreateResult( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed CollectionCreateJob." << job->errorString();
    return;
  }

  Q_ASSERT( dynamic_cast<CollectionCreateJob*>( job ) );
  CollectionCreateJob *cjob = static_cast<CollectionCreateJob*>( job );
  const Collection col = cjob->collection();
  toRegister.append( col );

  Q_ASSERT( pendingCreateJobs > 0 );
  pendingCreateJobs--;
  kDebug() << "pendingCreateJobs now" << pendingCreateJobs;
  if( pendingCreateJobs == 0 ) {
    nextResource();
  }
}



LocalFoldersRequestJob::LocalFoldersRequestJob( QObject *parent )
  : TransactionSequence( parent )
  , d( new LocalFoldersRequestJobPrivate( this ) )
{
}

LocalFoldersRequestJob::~LocalFoldersRequestJob()
{
  delete d;
}

void LocalFoldersRequestJob::requestDefaultFolder( int type )
{
  Q_ASSERT( type >= 0 && type < LocalFolders::LastDefaultType );
  d->defaultFolders[ type ] = true;
  d->requestingDefaultFolders = true;
}

void LocalFoldersRequestJob::requestFolder( int type, const QString &resourceId )
{
  Q_ASSERT( type >= 0 && type < LocalFolders::LastDefaultType );
  if( !d->foldersForResource.contains( resourceId ) ) {
    // This resource was previously unknown.
    d->foldersForResource[ resourceId ] = d->falseBoolList;
  }
  d->foldersForResource[ resourceId ][ type ] = true;
}

void LocalFoldersRequestJob::doStart()
{
  if( d->isEverythingReady() ) {
    emitResult();
  } else {
    GetLockJob *ljob = new GetLockJob( this );
    connect( ljob, SIGNAL(result(KJob*)), this, SLOT(lockResult(KJob*)) );
    ljob->start();
  }
}

void LocalFoldersRequestJob::slotResult( KJob *job )
{
  if( job->error() ) {
    // If we failed, let others try.
    releaseLock();
  }

  TransactionSequence::slotResult( job );
}

#include "localfoldersrequestjob.moc"
