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

#include "specialcollectionsrequestjob.h"

#include "specialcollectionattribute_p.h"
#include "specialcollections.h"
#include "specialcollections_p.h"
#include "specialcollectionshelperjobs_p.h"
#include "specialcollectionssettings.h"

#include <KDebug>

#include "akonadi/agentmanager.h"
#include "akonadi/collectioncreatejob.h"
#include "akonadi/entitydisplayattribute.h"

using namespace Akonadi;

typedef SpecialCollectionsSettings Settings;

/**
  @internal
*/
class Akonadi::SpecialCollectionsRequestJobPrivate
{
  public:
    SpecialCollectionsRequestJobPrivate( SpecialCollectionsRequestJob *qq );

    bool isEverythingReady();
    void lockResult( KJob *job ); // slot
    void releaseLock(); // slot
    void nextResource();
    void resourceScanResult( KJob *job ); // slot
    void createRequestedFolders( ResourceScanJob *resjob, QList<bool> &requestedFolders );
    void collectionCreateResult( KJob *job ); // slot

    SpecialCollectionsRequestJob *q;
    QList<bool> falseBoolList;
    int pendingCreateJobs;

    SpecialCollections::Type mRequestedType;
    AgentInstance mRequestedResource;

    // Input:
    QList<bool> defaultFolders;
    bool requestingDefaultFolders;
    QHash< QString, QList<bool> > foldersForResource;

    // Output:
    QStringList toForget;
    QList< QPair<Collection, SpecialCollections::Type> > toRegister;
};



SpecialCollectionsRequestJobPrivate::SpecialCollectionsRequestJobPrivate( SpecialCollectionsRequestJob *qq )
  : q( qq )
  , pendingCreateJobs( 0 )
  , requestingDefaultFolders( false )
{
  for( int type = SpecialCollections::Root; type < SpecialCollections::LastType; type++ ) {
    falseBoolList.append( false );
  }

  defaultFolders = falseBoolList;
}

bool SpecialCollectionsRequestJobPrivate::isEverythingReady()
{
  SpecialCollections *lf = SpecialCollections::self();

  if( requestingDefaultFolders ) {
    for( int i = SpecialCollections::Root; i < SpecialCollections::LastType; i++ ) {
      if( defaultFolders[i] && !lf->hasDefaultCollection( (SpecialCollections::Type)i ) ) {
        return false;
      }
    }
  }

  foreach( const QString &resourceId, foldersForResource.keys() ) {
    const QList<bool> &requested = foldersForResource[ resourceId ];
    for( int i = SpecialCollections::Root; i < SpecialCollections::LastType; i++ ) {
      if ( requested[i] && !lf->hasCollection( (SpecialCollections::Type)i,
                                                AgentManager::self()->instance( resourceId ) ) ) {
        return false;
      }
    }
  }

  kDebug() << "All requested folders already known.";
  return true;
}

void SpecialCollectionsRequestJobPrivate::lockResult( KJob *job )
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

void SpecialCollectionsRequestJobPrivate::releaseLock()
{
  const bool ok = Akonadi::releaseLock();
  if( !ok ) {
    kWarning() << "WTF, can't release lock.";
  }
}

void SpecialCollectionsRequestJobPrivate::nextResource()
{
  if( foldersForResource.isEmpty() ) {
    kDebug() << "All done! Comitting.";

    SpecialCollections::self()->d->beginBatchRegister();

    // Forget everything we knew before about these resources.
    foreach( const QString &resourceId, toForget ) {
      SpecialCollections::self()->d->forgetFoldersForResource( resourceId );
    }

    // Register all the collections that we fetched / created.
    typedef QPair<Collection, SpecialCollections::Type> RegisterPair;
    foreach( const RegisterPair &pair, toRegister ) {
      const bool ok = SpecialCollections::self()->registerCollection( pair.second, pair.first );
      Q_ASSERT( ok );
      Q_UNUSED( ok );
    }

    SpecialCollections::self()->d->endBatchRegister();

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

void SpecialCollectionsRequestJobPrivate::resourceScanResult( KJob *job )
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
    //toForget.append( SpecialCollections::self()->defaultResourceId() );
    createRequestedFolders( resjob, defaultFolders );
  } else {
    // This is not the default resource.
    QList<bool> requestedFolders = foldersForResource[ resourceId ];
    foldersForResource.remove( resourceId );
    createRequestedFolders( resjob, requestedFolders );
  }
}

void SpecialCollectionsRequestJobPrivate::createRequestedFolders( ResourceScanJob *resjob,
                                          QList<bool> &requestedFolders )
{
  Q_ASSERT( requestedFolders.count() == SpecialCollections::LastType );

  // Remove from the request list the folders which already exist.
  foreach( const Collection &col, resjob->localFolders() ) {
    Q_ASSERT( col.hasAttribute<SpecialCollectionAttribute>() );
    const SpecialCollectionAttribute *attr = col.attribute<SpecialCollectionAttribute>();
    const int type = (int)attr->collectionType();
    toRegister.append( qMakePair( col, (SpecialCollections::Type)type ) );
    requestedFolders[ type ] = false;
  }
  toForget.append( resjob->resourceId() );

  // Folders left in the request list must be created.
  Q_ASSERT( pendingCreateJobs == 0 );
  Q_ASSERT( resjob->rootResourceCollection().isValid() );
  for( int type = SpecialCollections::Root; type < SpecialCollections::LastType; type++ ) {
    if( requestedFolders[ type ] ) {
      Collection col;
      col.setParentCollection( resjob->rootResourceCollection() );
      col.setName( nameForType( (SpecialCollections::Type)type ) );
      setCollectionAttributes( col, (SpecialCollections::Type)type );
      CollectionCreateJob *cjob = new CollectionCreateJob( col, q );
      cjob->setProperty( "type", type );
      QObject::connect( cjob, SIGNAL(result(KJob*)), q, SLOT(collectionCreateResult(KJob*)) );
      pendingCreateJobs++;
    }
  }

  if( pendingCreateJobs == 0 ) {
    nextResource();
  }
}

void SpecialCollectionsRequestJobPrivate::collectionCreateResult( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed CollectionCreateJob." << job->errorString();
    return;
  }

  Q_ASSERT( dynamic_cast<CollectionCreateJob*>( job ) );
  CollectionCreateJob *cjob = static_cast<CollectionCreateJob*>( job );
  const Collection col = cjob->collection();
  toRegister.append( qMakePair( col, (SpecialCollections::Type)cjob->property( "type" ).toInt() ) );

  Q_ASSERT( pendingCreateJobs > 0 );
  pendingCreateJobs--;
  kDebug() << "pendingCreateJobs now" << pendingCreateJobs;
  if( pendingCreateJobs == 0 ) {
    nextResource();
  }
}



SpecialCollectionsRequestJob::SpecialCollectionsRequestJob( QObject *parent )
  : TransactionSequence( parent )
  , d( new SpecialCollectionsRequestJobPrivate( this ) )
{
}

SpecialCollectionsRequestJob::~SpecialCollectionsRequestJob()
{
  delete d;
}

void SpecialCollectionsRequestJob::requestDefaultCollection( SpecialCollections::Type type )
{
  Q_ASSERT( type > SpecialCollections::Invalid && type < SpecialCollections::LastType );
  d->defaultFolders[ type ] = true;
  d->requestingDefaultFolders = true;
  d->mRequestedType = type;
}

void SpecialCollectionsRequestJob::requestCollection( SpecialCollections::Type type, const AgentInstance &instance )
{
  Q_ASSERT( type > SpecialCollections::Invalid && type < SpecialCollections::LastType );
  if( !d->foldersForResource.contains( instance.identifier() ) ) {
    // This resource was previously unknown.
    d->foldersForResource[ instance.identifier() ] = d->falseBoolList;
  }
  d->foldersForResource[ instance.identifier() ][ type ] = true;

  d->mRequestedType = type;
  d->mRequestedResource = instance;
}

Collection SpecialCollectionsRequestJob::collection() const
{
  if ( d->mRequestedResource.isValid() )
    return SpecialCollections::self()->collection( d->mRequestedType, d->mRequestedResource );
  else
    return SpecialCollections::self()->defaultCollection( d->mRequestedType );
}

void SpecialCollectionsRequestJob::doStart()
{
  if( d->isEverythingReady() ) {
    emitResult();
  } else {
    GetLockJob *ljob = new GetLockJob( this );
    connect( ljob, SIGNAL(result(KJob*)), this, SLOT(lockResult(KJob*)) );
    ljob->start();
  }
}

void SpecialCollectionsRequestJob::slotResult( KJob *job )
{
  if( job->error() ) {
    // If we failed, let others try.
    releaseLock();
  }

  TransactionSequence::slotResult( job );
}

#include "specialcollectionsrequestjob.moc"
