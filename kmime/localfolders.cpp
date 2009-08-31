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

#include "localfolders.h"

#include "localfolders_p.h"
#include "localfolderattribute.h"
#include "localfolderssettings.h"

#include <QHash>
#include <QObject>
#include <QSet>

#include <KDebug>
#include <KGlobal>

#include <Akonadi/Monitor>

using namespace Akonadi;

typedef LocalFoldersSettings Settings;

K_GLOBAL_STATIC( LocalFoldersPrivate, sInstance )

LocalFoldersPrivate::LocalFoldersPrivate()
  : instance( new LocalFolders( this ) )
  , batchMode( false )
{
  for( int type = 0; type < LocalFolders::LastDefaultType; type++ ) {
    emptyFolderList.append( Collection( -1 ) );
  }

  monitor = new Monitor( instance );
  QObject::connect( monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
      instance, SLOT(collectionRemoved(Akonadi::Collection)) );
}

LocalFoldersPrivate::~LocalFoldersPrivate()
{
  delete instance;
}

void LocalFoldersPrivate::emitChanged( const QString &resourceId )
{
  if( batchMode ) {
    toEmitChangedFor.insert( resourceId );
  } else {
    kDebug() << "Emitting changed for" << resourceId;
    emit instance->foldersChanged( resourceId );
    if( resourceId == Settings::defaultResourceId() ) {
      kDebug() << "Emitting defaultFoldersChanged.";
      emit instance->defaultFoldersChanged();
    }
  }
}

void LocalFoldersPrivate::collectionRemoved( const Collection &col )
{
  kDebug() << "Collection" << col.id() << "resource" << col.resource();
  if( foldersForResource.contains( col.resource() ) ) {
    Collection::List &list = foldersForResource[ col.resource() ];
    int others = 0;
    for( int i = 0; i < LocalFolders::LastDefaultType; i++ ) {
      if( list[i] == col ) {
        list[i] = Collection( -1 );
        emitChanged( col.resource() );
      } else if( list[i].isValid() ) {
        others++;
      }
    }
    if( others == 0 ) {
      // This resource has no more folders, so remove it completely.
      foldersForResource.remove( col.resource() );
    }
  }
}



LocalFolders::LocalFolders( LocalFoldersPrivate *dd )
  : QObject()
  , d( dd )
{
}

LocalFolders *LocalFolders::self()
{
  return sInstance->instance;
}

bool LocalFolders::hasFolder( int type, const QString &resourceId ) const
{
  kDebug() << "Type" << type << "resourceId" << resourceId;

  Q_ASSERT( type >= 0 && type < LastDefaultType );
  if( !d->foldersForResource.contains( resourceId ) ) {
    // We do not know any folders in this resource.
    return false;
  }
  return d->foldersForResource[ resourceId ][ type ].isValid();
}

Collection LocalFolders::folder( int type, const QString &resourceId ) const
{
  Q_ASSERT( type >= 0 && type < LastDefaultType );
  if( !d->foldersForResource.contains( resourceId ) ) {
    // We do not know any folders in this resource.
    return Collection( -1 );
  }
  return d->foldersForResource[ resourceId ][ type ];
}

bool LocalFolders::registerFolder( const Collection &collection )
{
  if( !collection.isValid() ) {
    kWarning() << "Invalid collection.";
    return false;
  }

  const QString &resourceId = collection.resource();
  if( resourceId.isEmpty() ) {
    kWarning() << "Collection has empty resourceId.";
    return false;
  }

  if( !collection.hasAttribute<LocalFolderAttribute>() ) {
    kWarning() << "Collection has no LocalFolderAttribute.";
    return false;
  }
  const LocalFolderAttribute *attr = collection.attribute<LocalFolderAttribute>();
  const int type = attr->folderType();
  Q_ASSERT( type >= 0 && type < LastDefaultType );

  if( !d->foldersForResource.contains( resourceId ) ) {
    // We do not know any folders in this resource yet.
    Q_ASSERT( d->emptyFolderList.count() == LastDefaultType );
    d->foldersForResource[ resourceId ] = d->emptyFolderList;
  }

  if( d->foldersForResource[ resourceId ][ type ] != collection ) {
    d->monitor->setCollectionMonitored( d->foldersForResource[ resourceId ][ type ], false );
    d->monitor->setCollectionMonitored( collection, true );
    d->foldersForResource[ resourceId ][ type ] = collection;
    d->emitChanged( resourceId );
  }
  kDebug() << "Registered collection" << collection.id() << "as folder of type" << type
    << "in resource" << resourceId;
  return true;
}

QString LocalFolders::defaultResourceId() const
{
  return Settings::defaultResourceId();
}

bool LocalFolders::hasDefaultFolder( int type ) const
{
  return hasFolder( type, defaultResourceId() );
}

Collection LocalFolders::defaultFolder( int type ) const
{
  return folder( type, defaultResourceId() );
}

void LocalFolders::forgetFoldersForResource( const QString &resourceId )
{
  if( d->foldersForResource.contains( resourceId ) ) {
    foreach( const Collection &col, d->foldersForResource[ resourceId ] ) {
      d->monitor->setCollectionMonitored( col, false );
    }
    d->foldersForResource.remove( resourceId );
    d->emitChanged( resourceId );
  }
}

void LocalFolders::beginBatchRegister()
{
  Q_ASSERT( !d->batchMode );
  d->batchMode = true;
  Q_ASSERT( d->toEmitChangedFor.isEmpty() );
}

void LocalFolders::endBatchRegister()
{
  Q_ASSERT( d->batchMode );
  d->batchMode = false;
  
  foreach( const QString &resourceId, d->toEmitChangedFor ) {
    d->emitChanged( resourceId );
  }
  d->toEmitChangedFor.clear();
}

#include "localfolders.moc"
