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

#include "specialcollections.h"

#include "specialcollections_p.h"
#include "specialcollectionattribute_p.h"

#include "akonadi/agentinstance.h"
#include "akonadi/agentmanager.h"
#include "akonadi/collectionmodifyjob.h"
#include "akonadi/monitor.h"

#include <KDebug>
#include <kcoreconfigskeleton.h>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSet>

using namespace Akonadi;

SpecialCollectionsPrivate::SpecialCollectionsPrivate( KCoreConfigSkeleton *settings, SpecialCollections *qq )
  : q( qq ),
    mSettings( settings ),
    mBatchMode( false )
{
  mMonitor = new Monitor( q );
  QObject::connect( mMonitor, SIGNAL( collectionRemoved( const Akonadi::Collection& ) ),
                    q, SLOT( collectionRemoved( const Akonadi::Collection& ) ) );
}

SpecialCollectionsPrivate::~SpecialCollectionsPrivate()
{
}

QString SpecialCollectionsPrivate::defaultResourceId() const
{
  const KConfigSkeletonItem *item = mSettings->findItem( QLatin1String( "DefaultResourceId" ) );
  Q_ASSERT( item );

  return item->property().toString();
}

void SpecialCollectionsPrivate::emitChanged( const QString &resourceId )
{
  if ( mBatchMode ) {
    mToEmitChangedFor.insert( resourceId );
  } else {
    kDebug() << "Emitting changed for" << resourceId;
    const AgentInstance agentInstance = AgentManager::self()->instance( resourceId ); 
    emit q->collectionsChanged( agentInstance );
    if ( resourceId == defaultResourceId() ) {
      kDebug() << "Emitting defaultFoldersChanged.";
      emit q->defaultCollectionsChanged();
    }
  }
}

void SpecialCollectionsPrivate::collectionRemoved( const Collection &collection )
{
  kDebug() << "Collection" << collection.id() << "resource" << collection.resource();
  if ( mFoldersForResource.contains( collection.resource() ) ) {

    // Retrieve the list of special folders for the resource the collection belongs to
    QHash<QByteArray, Collection> &folders = mFoldersForResource[ collection.resource() ];
    {
      QMutableHashIterator<QByteArray, Collection> it( folders );
      while ( it.hasNext() ) {
        it.next();
        if ( it.value() == collection ) {
          // The collection to be removed is a special folder
          it.remove();
          emitChanged( collection.resource() );
        }
      }
    }

    if ( folders.isEmpty() ) {
      // This resource has no more folders, so remove it completely.
      mFoldersForResource.remove( collection.resource() );
    }
  }
}

void SpecialCollectionsPrivate::beginBatchRegister()
{
  Q_ASSERT( !mBatchMode );
  mBatchMode = true;
  Q_ASSERT( mToEmitChangedFor.isEmpty() );
}

void SpecialCollectionsPrivate::endBatchRegister()
{
  Q_ASSERT( mBatchMode );
  mBatchMode = false;

  foreach ( const QString &resourceId, mToEmitChangedFor ) {
    emitChanged( resourceId );
  }

  mToEmitChangedFor.clear();
}

void SpecialCollectionsPrivate::forgetFoldersForResource( const QString &resourceId )
{
  if ( mFoldersForResource.contains( resourceId ) ) {
    const Collection::List folders = mFoldersForResource[ resourceId ].values();

    foreach ( const Collection &collection, folders ) {
      mMonitor->setCollectionMonitored( collection, false );
    }

    mFoldersForResource.remove( resourceId );
    emitChanged( resourceId );
  }
}

AgentInstance SpecialCollectionsPrivate::defaultResource() const
{
  const QString identifier = defaultResourceId();
  return AgentManager::self()->instance( identifier );
}


SpecialCollections::SpecialCollections( KCoreConfigSkeleton *settings, QObject *parent )
  : QObject( parent ),
    d( new SpecialCollectionsPrivate( settings, this ) )
{
}

SpecialCollections::~SpecialCollections()
{
  delete d;
}

bool SpecialCollections::hasCollection( const QByteArray &type, const AgentInstance &instance ) const
{
  kDebug() << "Type" << type << "resourceId" << instance.identifier();

  if ( !d->mFoldersForResource.contains( instance.identifier() ) ) {
    // We do not know any folders in this resource.
    return false;
  }

  return d->mFoldersForResource[ instance.identifier() ].contains( type );
}

Collection SpecialCollections::collection( const QByteArray &type, const AgentInstance &instance ) const
{
  if ( !d->mFoldersForResource.contains( instance.identifier() ) ) {
    // We do not know any folders in this resource.
    return Collection( -1 );
  }
  return d->mFoldersForResource[ instance.identifier() ][ type ];
}

bool SpecialCollections::registerCollection( const QByteArray &type, const Collection &collection )
{
  if ( !collection.isValid() ) {
    kWarning() << "Invalid collection.";
    return false;
  }

  const QString &resourceId = collection.resource();
  if ( resourceId.isEmpty() ) {
    kWarning() << "Collection has empty resourceId.";
    return false;
  }

  {
    Collection attributeCollection( collection );
    SpecialCollectionAttribute *attribute = attributeCollection.attribute<SpecialCollectionAttribute>( Collection::AddIfMissing );
    attribute->setCollectionType( type );

    CollectionModifyJob *job = new CollectionModifyJob( attributeCollection );
    job->start();
  }

  if ( !d->mFoldersForResource.contains( resourceId ) ) {
    // We do not know any folders in this resource yet.
    d->mFoldersForResource.insert( resourceId, QHash<QByteArray, Collection>() );
  }

  if ( !d->mFoldersForResource[ resourceId ].contains( type ) )
    d->mFoldersForResource[ resourceId ].insert( type, Collection() );

  if ( d->mFoldersForResource[ resourceId ][ type ] != collection ) {
    d->mMonitor->setCollectionMonitored( d->mFoldersForResource[ resourceId ][ type ], false );
    d->mMonitor->setCollectionMonitored( collection, true );
    d->mFoldersForResource[ resourceId ].insert( type, collection );
    d->emitChanged( resourceId );
  }

  kDebug() << "Registered collection" << collection.id() << "as folder of type" << type
    << "in resource" << resourceId;
  return true;
}

bool SpecialCollections::hasDefaultCollection( const QByteArray &type ) const
{
  return hasCollection( type, d->defaultResource() );
}

Collection SpecialCollections::defaultCollection( const QByteArray &type ) const
{
  return collection( type, d->defaultResource() );
}

#include "specialcollections.moc"
