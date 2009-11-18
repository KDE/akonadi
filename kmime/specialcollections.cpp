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
#include "specialcollectionssettings.h"

#include <QHash>
#include <QObject>
#include <QSet>

#include <KDebug>
#include <KGlobal>

#include "akonadi/agentinstance.h"
#include "akonadi/agentmanager.h"
#include "akonadi/collectionmodifyjob.h"
#include "akonadi/monitor.h"

using namespace Akonadi;

typedef SpecialCollectionsSettings Settings;

K_GLOBAL_STATIC( SpecialCollectionsPrivate, sInstance )

SpecialCollectionsPrivate::SpecialCollectionsPrivate()
  : instance( new SpecialCollections( this ) )
  , batchMode( false )
{
  monitor = new Monitor( instance );
  QObject::connect( monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
      instance, SLOT(collectionRemoved(Akonadi::Collection)) );
}

SpecialCollectionsPrivate::~SpecialCollectionsPrivate()
{
  delete instance;
}

void SpecialCollectionsPrivate::emitChanged( const QString &resourceId )
{
  if( batchMode ) {
    toEmitChangedFor.insert( resourceId );
  } else {
    kDebug() << "Emitting changed for" << resourceId;
    const AgentInstance agentInstance = AgentManager::self()->instance( resourceId ); 
    emit instance->collectionsChanged( agentInstance );
    if( resourceId == Settings::defaultResourceId() ) {
      kDebug() << "Emitting defaultFoldersChanged.";
      emit instance->defaultCollectionsChanged();
    }
  }
}

void SpecialCollectionsPrivate::collectionRemoved( const Collection &collection )
{
  kDebug() << "Collection" << collection.id() << "resource" << collection.resource();
  if ( foldersForResource.contains( collection.resource() ) ) {

    // Retrieve the list of special folders for the resource the collection belongs to
    QHash<SpecialCollections::Type, Collection> &folders = foldersForResource[ collection.resource() ];
    QMutableHashIterator<SpecialCollections::Type, Collection> it( folders );
    while ( it.hasNext() ) {
      it.next();
      if ( it.value() == collection ) {
        // The collection to be removed is a special folder
        it.remove();
        emitChanged( collection.resource() );
      }
    }

    if ( folders.isEmpty() ) {
      // This resource has no more folders, so remove it completely.
      foldersForResource.remove( collection.resource() );
    }
  }
}

void SpecialCollectionsPrivate::beginBatchRegister()
{
  Q_ASSERT( !batchMode );
  batchMode = true;
  Q_ASSERT( toEmitChangedFor.isEmpty() );
}

void SpecialCollectionsPrivate::endBatchRegister()
{
  Q_ASSERT( batchMode );
  batchMode = false;

  foreach( const QString &resourceId, toEmitChangedFor ) {
    emitChanged( resourceId );
  }

  toEmitChangedFor.clear();
}

void SpecialCollectionsPrivate::forgetFoldersForResource( const QString &resourceId )
{
  if ( foldersForResource.contains( resourceId ) ) {
    const Collection::List folders = foldersForResource[ resourceId ].values();

    foreach ( const Collection &collection, folders ) {
      monitor->setCollectionMonitored( collection, false );
    }

    foldersForResource.remove( resourceId );
    emitChanged( resourceId );
  }
}

AgentInstance SpecialCollectionsPrivate::defaultResource() const
{
  const QString identifier = Settings::defaultResourceId();
  return AgentManager::self()->instance( identifier );
}


SpecialCollections::SpecialCollections( SpecialCollectionsPrivate *dd )
  : QObject()
  , d( dd )
{
}

SpecialCollections *SpecialCollections::self()
{
  return sInstance->instance;
}

bool SpecialCollections::hasCollection( Type type, const AgentInstance &instance ) const
{
  kDebug() << "Type" << type << "resourceId" << instance.identifier();

  if( !d->foldersForResource.contains( instance.identifier() ) ) {
    // We do not know any folders in this resource.
    return false;
  }

  return d->foldersForResource[ instance.identifier() ].contains( type );
}

Collection SpecialCollections::collection( Type type, const AgentInstance &instance ) const
{
  Q_ASSERT( type > Invalid && type < LastType );
  if( !d->foldersForResource.contains( instance.identifier() ) ) {
    // We do not know any folders in this resource.
    return Collection( -1 );
  }
  return d->foldersForResource[ instance.identifier() ][ type ];
}

bool SpecialCollections::registerCollection( Type type, const Collection &collection )
{
  Q_ASSERT( type > Invalid  && type < LastType );

  if( !collection.isValid() ) {
    kWarning() << "Invalid collection.";
    return false;
  }

  const QString &resourceId = collection.resource();
  if( resourceId.isEmpty() ) {
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

  if( !d->foldersForResource.contains( resourceId ) ) {
    // We do not know any folders in this resource yet.
    d->foldersForResource.insert( resourceId, QHash<Type, Collection>() );
  }

  if ( d->foldersForResource[ resourceId ].contains( type ) ) {
    if ( d->foldersForResource[ resourceId ][ type ] != collection ) {
      d->monitor->setCollectionMonitored( d->foldersForResource[ resourceId ][ type ], false );
      d->monitor->setCollectionMonitored( collection, true );
      d->foldersForResource[ resourceId ].insert( type, collection );
      d->emitChanged( resourceId );
    }
  }
  kDebug() << "Registered collection" << collection.id() << "as folder of type" << type
    << "in resource" << resourceId;
  return true;
}

bool SpecialCollections::hasDefaultCollection( Type type ) const
{
  return hasCollection( type, d->defaultResource() );
}

Collection SpecialCollections::defaultCollection( Type type ) const
{
  return collection( type, d->defaultResource() );
}

#include "specialcollections.moc"
