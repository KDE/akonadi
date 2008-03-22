/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

// @cond PRIVATE

#include "collectionmodel_p.h"
#include "collectionmodel.h"

#include "collectionfetchjob.h"
#include "collectionstatusjob.h"
#include "monitor.h"
#include "session.h"

#include <kdebug.h>
#include <kjob.h>

#include <QtCore/QModelIndex>

using namespace Akonadi;


void CollectionModelPrivate::collectionRemoved( const Akonadi::Collection &collection )
{
  Q_Q( CollectionModel );
  QModelIndex colIndex = q->indexForId( collection.id() );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = q->parent( colIndex );
    // collection is still somewhere in the hierarchy
    q->removeRowFromModel( colIndex.row(), parentIndex );
  } else {
    if ( collections.contains( collection.id() ) ) {
      // collection is orphan, ie. the parent has been removed already
      collections.remove( collection.id() );
      childCollections.remove( collection.id() );
    }
  }
}

void CollectionModelPrivate::collectionChanged( const Akonadi::Collection &collection )
{
  Q_Q( CollectionModel );

  // What kind of change is it ?
  Collection::Id oldParentId = collections.value( collection.id() ).parent();
  Collection::Id newParentId = collection.parent();
  if ( newParentId !=  oldParentId && oldParentId >= 0 ) { // It's a move
    q->removeRowFromModel( q->indexForId( collections[ collection.id() ].id() ).row(), q->indexForId( oldParentId ) );
    Collection newParent;
    if ( newParentId == Collection::root().id() )
      newParent = Collection::root();
    else
      newParent = collections.value( newParentId );
    CollectionFetchJob *job = new CollectionFetchJob( newParent, CollectionFetchJob::Recursive, session );
    job->includeUnsubscribed( unsubscribed );
    q->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                q, SLOT(collectionsChanged(Akonadi::Collection::List)) );
    q->connect( job, SIGNAL( result( KJob* ) ),
                 q, SLOT( listDone( KJob* ) ) );

  }
  else { // It's a simple change
    CollectionFetchJob *job = new CollectionFetchJob( collection, CollectionFetchJob::Local, session );
    job->includeUnsubscribed( unsubscribed );
    q->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                q, SLOT(collectionsChanged(Akonadi::Collection::List)) );
    q->connect( job, SIGNAL( result( KJob* ) ),
                 q, SLOT( listDone( KJob* ) ) );
  }

}

void CollectionModelPrivate::updateDone( KJob *job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning( 5250 ) << "Job error:" << job->errorString();
  } else {
    CollectionStatusJob *csjob = static_cast<CollectionStatusJob*>( job );
    Collection result = csjob->collection();
    collectionStatusChanged( result.id(), csjob->status() );
  }
}

void CollectionModelPrivate::collectionStatusChanged( Collection::Id collection, const Akonadi::CollectionStatus & status )
{
  Q_Q( CollectionModel );

  if ( !collections.contains( collection ) )
    kWarning( 5250 ) << "Got status response for non-existing collection:" << collection;
  else {
    collections[ collection ].setStatus( status );

    Collection col = collections.value( collection );
    QModelIndex startIndex = q->indexForId( col.id() );
    QModelIndex endIndex = q->indexForId( col.id(), q->columnCount( q->parent( startIndex ) ) - 1 );
    emit q->dataChanged( startIndex, endIndex );

    // Unread total might have changed now.
    static int oldTotalUnread = -1;
    int totalUnread = 0;
    foreach ( const Collection& col, collections.values() ) {
      int colUnread = col.status().unreadCount();
      if ( colUnread > 0 )
          totalUnread += colUnread;
    }
    if ( oldTotalUnread != totalUnread ) {
        emit q->unreadCountChanged( totalUnread );
        oldTotalUnread = totalUnread;
    }
  }
}

void CollectionModelPrivate::listDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Job error: " << job->errorString() << endl;
  }
}

void CollectionModelPrivate::editDone( KJob * job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Edit failed: " << job->errorString();
  }
}

void CollectionModelPrivate::dropResult(KJob * job)
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Paste failed:" << job->errorString();
    // TODO: error handling
  }
}

void CollectionModelPrivate::collectionsChanged( const Collection::List &cols )
{
  Q_Q( CollectionModel );
  foreach( Collection col, cols ) {
    if ( collections.contains( col.id() ) ) {
      // collection already known
      col.setStatus( collections.value( col.id() ).status() );
      collections[ col.id() ] = col;
      QModelIndex startIndex = q->indexForId( col.id() );
      QModelIndex endIndex = q->indexForId( col.id(), q->columnCount( q->parent( startIndex ) ) - 1 );
      emit q->dataChanged( startIndex, endIndex );
      continue;
    }
    collections.insert( col.id(), col );
    QModelIndex parentIndex = q->indexForId( col.parent() );
    if ( parentIndex.isValid() || col.parent() == Collection::root().id() ) {
      q->beginInsertRows( parentIndex, q->rowCount( parentIndex ), q->rowCount( parentIndex ) );
      childCollections[ col.parent() ].append( col.id() );
      q->endInsertRows();
    } else {
      childCollections[ col.parent() ].append( col.id() );
    }

    updateSupportedMimeTypes( col );

    // start a status job for every collection to get message counts, etc.
    if ( fetchStatus && col.type() != Collection::VirtualParent ) {
      CollectionStatusJob* csjob = new CollectionStatusJob( col, session );
      q->connect( csjob, SIGNAL(result(KJob*)), q, SLOT(updateDone(KJob*)) );
    }
  }
}

void CollectionModelPrivate::init()
{
  Q_Q( CollectionModel );

  // start a list job
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, session );
  job->includeUnsubscribed( unsubscribed );
  q->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
              q, SLOT(collectionsChanged(Akonadi::Collection::List)) );
  q->connect( job, SIGNAL(result(KJob*)), q, SLOT(listDone(KJob*)) );

  // monitor collection changes
  q->connect( monitor, SIGNAL(collectionChanged(const Akonadi::Collection&)),
              q, SLOT(collectionChanged(const Akonadi::Collection&)) );
  q->connect( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
              q, SLOT(collectionChanged(Akonadi::Collection)) );
  q->connect( monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
              q, SLOT(collectionRemoved(Akonadi::Collection)) );
  q->connect( monitor, SIGNAL(collectionStatusChanged(Collection::Id,Akonadi::CollectionStatus)),
              q, SLOT(collectionStatusChanged(Collection::Id,Akonadi::CollectionStatus)) );
}
