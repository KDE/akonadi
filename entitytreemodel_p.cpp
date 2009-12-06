/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "entitytreemodel_p.h"
#include "entitytreemodel.h"

#include "monitor_p.h" // For friend ref/deref

#include <KDE/KIconLoader>
#include <KDE/KUrl>

#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/session.h>

#include <kdebug.h>

using namespace Akonadi;

EntityTreeModelPrivate::EntityTreeModelPrivate( EntityTreeModel *parent )
  : q_ptr( parent ),
    m_collectionFetchStrategy( EntityTreeModel::FetchCollectionsRecursive ),
    m_itemPopulation( EntityTreeModel::ImmediatePopulation ),
    m_includeUnsubscribed( true ),
    m_includeStatistics( false ),
    m_showRootCollection( false ),
    m_showSystemEntities( false )
{
}

void EntityTreeModelPrivate::init( ChangeRecorder *monitor, Session *session )
{
  Q_Q( EntityTreeModel );
  m_monitor = monitor;
  m_session = session;

  m_monitor->setChangeRecordingEnabled( false );

  m_includeStatistics = true;
  m_monitor->fetchCollectionStatistics( true );
  m_monitor->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );

  m_mimeChecker.setWantedMimeTypes( m_monitor->mimeTypesMonitored() );

  q->connect( monitor, SIGNAL( mimeTypeMonitored( const QString&, bool ) ),
              SLOT( monitoredMimeTypeChanged( const QString&, bool ) ) );

  // monitor collection changes
  q->connect( monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionChanged( const Akonadi::Collection& ) ) );
  q->connect( monitor, SIGNAL( collectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) ) );
  q->connect( monitor, SIGNAL( collectionRemoved( const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionRemoved( const Akonadi::Collection& ) ) );
  q->connect( monitor,
           SIGNAL( collectionMoved( const Akonadi::Collection&, const Akonadi::Collection&, const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionMoved( const Akonadi::Collection&, const Akonadi::Collection&, const Akonadi::Collection& ) ) );

  if ( !monitor->itemFetchScope().isEmpty() ) {
    // Monitor item changes.
    q->connect( monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
            SLOT( monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
    q->connect( monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
            SLOT( monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
    q->connect( monitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
            SLOT( monitoredItemRemoved( const Akonadi::Item& ) ) );
    q->connect( monitor, SIGNAL( itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) ),
            SLOT( monitoredItemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) ) );

    q->connect( monitor, SIGNAL( itemLinked( const Akonadi::Item&, const Akonadi::Collection& ) ),
            SLOT( monitoredItemLinked( const Akonadi::Item&, const Akonadi::Collection& ) ) );
    q->connect( monitor, SIGNAL( itemUnlinked( const Akonadi::Item&, const Akonadi::Collection& ) ),
            SLOT( monitoredItemUnlinked( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  }
  q->connect( monitor, SIGNAL( collectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics& ) ),
           SLOT( monitoredCollectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics& ) ) );

  QList<Collection> list = monitor->collectionsMonitored();
  if ( list.size() == 1 )
    m_rootCollection = list.first();
  else
    m_rootCollection = Collection::root();

  m_rootCollectionDisplayName = QLatin1String( "[*]" );

  fillModel();
}

int EntityTreeModelPrivate::indexOf( const QList<Node*> &nodes, Entity::Id id ) const
{
  int i = 0;
  foreach ( const Node *node, nodes ) {
    if ( node->id == id )
      return i;
    i++;
  }

  return -1;
}

ItemFetchJob* EntityTreeModelPrivate::getItemFetchJob( const Collection &parent, const ItemFetchScope &scope ) const
{
  ItemFetchJob *itemJob = new Akonadi::ItemFetchJob( parent, m_session );
  itemJob->setFetchScope( scope );
  itemJob->fetchScope().setAncestorRetrieval( ItemFetchScope::All );
  return itemJob;
}

ItemFetchJob* EntityTreeModelPrivate::getItemFetchJob( const Item &item, const ItemFetchScope &scope ) const
{
  ItemFetchJob *itemJob = new Akonadi::ItemFetchJob( item, m_session );
  itemJob->setFetchScope( scope );
  return itemJob;
}

void EntityTreeModelPrivate::runItemFetchJob( ItemFetchJob *itemFetchJob, const Collection &parent ) const
{
  Q_Q( const EntityTreeModel );

  // TODO: This hack is probably not needed anymore. Remove it.
  // ### HACK: itemsReceivedFromJob needs to know which collection items were added to.
  // That is not provided by akonadi, so we attach it in a property.
  itemFetchJob->setProperty( ItemFetchCollectionId(), QVariant( parent.id() ) );

  q->connect( itemFetchJob, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ),
              q, SLOT( itemsFetched( const Akonadi::Item::List& ) ) );
  q->connect( itemFetchJob, SIGNAL( result( KJob* ) ),
              q, SLOT( fetchJobDone( KJob* ) ) );
}

void EntityTreeModelPrivate::fetchItems( const Collection &parent )
{
  // TODO: Use a more specific fetch scope to get only the envelope for mails etc.
  ItemFetchJob *itemJob = getItemFetchJob( parent, m_monitor->itemFetchScope() );

  runItemFetchJob( itemJob, parent );
}

void EntityTreeModelPrivate::fetchCollections( const Collection &collection, CollectionFetchJob::Type type )
{
  Q_Q( EntityTreeModel );
  CollectionFetchJob *job = new CollectionFetchJob( collection, type, m_session );
  job->fetchScope().setIncludeUnsubscribed( m_includeUnsubscribed );
  job->fetchScope().setIncludeStatistics( m_includeStatistics );
  job->fetchScope().setContentMimeTypes( m_monitor->mimeTypesMonitored() );
  job->fetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );
  q->connect( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ),
              q, SLOT( collectionsFetched( const Akonadi::Collection::List& ) ) );
  q->connect( job, SIGNAL( result( KJob* ) ),
              q, SLOT( fetchJobDone( KJob* ) ) );
}

bool EntityTreeModelPrivate::isHidden( const Entity &entity ) const
{
  if ( m_showSystemEntities )
    return false;

  if ( entity.id() == Collection::root().id() )
    return false;

  if ( entity.hasAttribute<EntityHiddenAttribute>() )
    return true;

  const Collection parent = entity.parentCollection();
  if ( parent.isValid() )
    return isHidden( parent );

  return false;
}

void EntityTreeModelPrivate::collectionsFetched( const Akonadi::Collection::List& collections )
{
  Q_Q( EntityTreeModel );

  QListIterator<Akonadi::Collection> it( collections );

  QHash<Collection::Id, Collection> collectionsToInsert;
  QHash<Collection::Id, QList<Collection::Id> > subTreesToInsert;
  QHash<Collection::Id, Collection> parents;

  while ( it.hasNext() ) {
    const Collection collection = it.next();

    if ( isHidden( collection ) )
      continue;

    if ( m_collections.contains( collection.id() ) ) {
      // This is probably the result of a parent of a previous collection already being in the model.
      // Replace the dummy collection with the real one and move on.
      m_collections[ collection.id() ] = collection;

      const QModelIndex collectionIndex = indexForCollection( collection );
      dataChanged( collectionIndex, collectionIndex );
      continue;
    }

    Collection parent = collection;
    Collection tmp;

    while ( !m_collections.contains( parent.parentCollection().id() ) ) {
      if ( !subTreesToInsert[ parent.parentCollection().id() ].contains( parent.parentCollection().id() ) ) {
        subTreesToInsert[ parent.parentCollection().id() ].append( parent.parentCollection().id() );
        collectionsToInsert.insert( parent.parentCollection().id(), parent.parentCollection() );
      }

      foreach( Collection::Id collectionId, subTreesToInsert.take( parent.id() ) ) {
        if ( !subTreesToInsert[ parent.parentCollection().id() ].contains( collectionId ) )
          subTreesToInsert[ parent.parentCollection().id() ].append( collectionId );
      }

      tmp = parent.parentCollection();
      parent = tmp;
    }

    if ( !subTreesToInsert[ parent.id() ].contains( collection.id() ) )
      subTreesToInsert[ parent.id() ].append( collection.id() );

    collectionsToInsert.insert( collection.id(), collection );
    if ( !parents.contains( parent.id() ) )
      parents.insert( parent.id(), parent.parentCollection() );
  }

  const int row = 0;

  QHashIterator<Collection::Id, QList<Collection::Id> > collectionIt( subTreesToInsert );
  while ( collectionIt.hasNext() ) {
    collectionIt.next();

    const Collection::Id topCollectionId = collectionIt.key();

    Q_ASSERT( !m_collections.contains( topCollectionId ) );

    Q_ASSERT( parents.contains( topCollectionId ) );
    const QModelIndex parentIndex = indexForCollection( parents.value( topCollectionId ) );

    q->beginInsertRows( parentIndex, row, row );
    Q_ASSERT( !collectionIt.value().isEmpty() );
    Q_ASSERT( m_collections.contains( parents.value( topCollectionId ).id() ) );

    foreach( Collection::Id collectionId, collectionIt.value() ) {
      const Collection collection = collectionsToInsert.take( collectionId );
      Q_ASSERT( collection.isValid() );

      m_collections.insert( collectionId, collection );

      Node *node = new Node;
      node->id = collectionId;
      node->parent = collection.parentCollection().id();
      node->type = Node::Collection;
      m_childEntities[ collection.parentCollection().id() ].prepend( node );
    }
    q->endInsertRows();

    if ( m_itemPopulation == EntityTreeModel::ImmediatePopulation )
      foreach( const Collection::Id &collectionId, collectionIt.value() )
        fetchItems( m_collections.value( collectionId ) );
  }
}

void EntityTreeModelPrivate::itemsFetched( const Akonadi::Item::List& items )
{
  Q_Q( EntityTreeModel );
  QObject *job = q->sender();

  Q_ASSERT( job );

  const Collection::Id collectionId = job->property( ItemFetchCollectionId() ).value<Collection::Id>();
  Item::List itemsToInsert;
  Item::List itemsToUpdate;

  const Collection collection = m_collections.value( collectionId );

  Q_ASSERT( collection.isValid() );

  const QList<Node*> collectionEntities = m_childEntities.value( collectionId );
  foreach ( const Item &item, items ) {

    if ( isHidden( item ) )
      continue;

    if ( indexOf( collectionEntities, item.id() ) != -1 ) {
      itemsToUpdate << item;
    } else {
      if ( m_mimeChecker.wantedMimeTypes().isEmpty() || m_mimeChecker.isWantedItem( item ) ) {
        itemsToInsert << item;
      }
    }
  }

  if ( itemsToInsert.size() > 0 ) {
    const int startRow = m_childEntities.value( collectionId ).size();

    Q_ASSERT( m_collections.contains( collectionId ) );

    const QModelIndex parentIndex = indexForCollection( m_collections.value( collectionId ) );
    q->beginInsertRows( parentIndex, startRow, startRow + items.size() - 1 );
    foreach ( const Item &item, items ) {
      const Item::Id itemId = item.id();
      m_items.insert( itemId, item );

      Node *node = new Node;
      node->id = itemId;
      node->parent = collectionId;
      node->type = Node::Item;

      m_childEntities[ collectionId ].append( node );
    }
    q->endInsertRows();
  }

  if ( itemsToUpdate.size() > 0 ) {
    foreach ( const Item &item, itemsToUpdate ) {
      m_items[ item.id() ].apply( item );
      foreach ( const QModelIndex &index, indexesForItem( item ) ) {
        dataChanged( index, index );
      }
    }
  }
}

void EntityTreeModelPrivate::monitoredMimeTypeChanged( const QString & mimeType, bool monitored )
{
  if ( monitored )
    m_mimeChecker.addWantedMimeType( mimeType );
  else
    m_mimeChecker.removeWantedMimeType( mimeType );
}

void EntityTreeModelPrivate::retrieveAncestors( const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  Collection parentCollection = collection.parentCollection();

  Q_ASSERT( parentCollection != Collection::root() );

  Collection temp;

  Collection::List ancestors;

  while ( !m_collections.contains( parentCollection.id() ) ) {
    // Put a temporary node in the tree later.
    ancestors.prepend( parentCollection );

    // Fetch the real ancestor
    CollectionFetchJob *job = new CollectionFetchJob( parentCollection, CollectionFetchJob::Base, m_session );
    job->fetchScope().setIncludeUnsubscribed( m_includeUnsubscribed );
    job->fetchScope().setIncludeStatistics( m_includeStatistics );
    q->connect( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ),
                q, SLOT( ancestorsFetched( const Akonadi::Collection::List& ) ) );
    q->connect( job, SIGNAL( result( KJob* ) ),
                q, SLOT( fetchJobDone( KJob* ) ) );

    temp = parentCollection.parentCollection();
    parentCollection = temp;
  }

  const QModelIndex parent = indexForCollection( parentCollection );

  // Still prepending all collections for now.
  int row = 0;

  // Although we insert several Collections here, we only need to notify though the model
  // about the top-level one. The rest will be found auotmatically by the view.
  q->beginInsertRows( parent, row, row );

  Collection::List::const_iterator it;
  const Collection::List::const_iterator begin = ancestors.constBegin();
  const Collection::List::const_iterator end = ancestors.constEnd();

  for ( it = begin; it != end; ++it ) {
    const Collection collection = *it;
    m_collections.insert( collection.id(), collection );

    Node *node = new Node;
    node->id = collection.id();
    node->parent = collection.parentCollection().id();
    node->type = Node::Collection;
    m_childEntities[ node->parent ].prepend( node );
  }

  q->endInsertRows();
}

void EntityTreeModelPrivate::ancestorsFetched( const Akonadi::Collection::List& collectionList )
{
  Q_ASSERT( collectionList.size() == 1 );

  const Collection collection = collectionList.at( 0 );

  m_collections[ collection.id() ] = collection;

  const QModelIndex index = indexForCollection( collection );
  Q_ASSERT( index.isValid() );
  dataChanged( index, index );
}

void EntityTreeModelPrivate::insertCollection( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
  Q_ASSERT( collection.isValid() );
  Q_ASSERT( parent.isValid() );

  Q_Q( EntityTreeModel );
  // TODO: Use order attribute of parent if available
  // Otherwise prepend collections and append items. Currently this prepends all collections.

  // Or I can prepend and append for single signals, then 'change' the parent.

//   QList<qint64> childCols = m_childEntities.value( parent.id() );
//   int row = childCols.size();
//   int numChildCols = childCollections.value( parent.id() ).size();

  const int row = 0;
  const QModelIndex parentIndex = indexForCollection( parent );
  q->beginInsertRows( parentIndex, row, row );
  m_collections.insert( collection.id(), collection );

  Node *node = new Node;
  node->id = collection.id();
  node->parent = parent.id();
  node->type = Node::Collection;
  m_childEntities[ parent.id() ].prepend( node );
  q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredCollectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
  if ( isHidden( collection ) )
    return;

  // If the resource is removed while populating the model with it, we might still
  // get some monitor signals. These stale/out-of-order signals can't be completely eliminated
  // in the akonadi server due to implementation details, so we also handle such signals in the model silently
  // in all the monitored slots.
  // Stephen Kelly, 28, July 2009

  // This is currently temporarily blocked by a uninitialized value bug in the server.
//   if ( !m_collections.contains( parent.id() ) )
//   {
//     kWarning() << "Got a stale notification for a collection whose parent was already removed." << collection.id() << collection.remoteId();
//     return;
//   }

  // Some collection trees contain multiple mimetypes. Even though server side filtering ensures we
  // only get the ones we're interested in from the job, we have to filter on collections received through signals too.
  if ( !m_mimeChecker.wantedMimeTypes().isEmpty() && !m_mimeChecker.isWantedCollection( collection ) )
    return;

  if ( !m_collections.contains( parent.id() ) ) {
    // The collection we're interested in is contained in a collection we're not interested in.
    // We download the ancestors of the collection we're interested in to complete the tree.
    retrieveAncestors( collection );
    return;
  }

  insertCollection( collection, parent );

}

void EntityTreeModelPrivate::monitoredCollectionRemoved( const Akonadi::Collection& collection )
{
  if ( isHidden( collection ) )
    return;

  Collection::Id parentId = collection.parentCollection().id();

  if ( parentId < 0 ) parentId = -1;

  if ( !m_collections.contains( parentId ) )
    return;

  Q_Q( EntityTreeModel );

  // This may be a signal for a collection we've already removed by removing its ancestor.
  if ( !m_collections.contains( collection.id() ) ) {
    kWarning() << "Got a stale notification for a collection which was already removed." << collection.id() << collection.remoteId();
    return;
  }


  Q_ASSERT( m_childEntities.contains( parentId ) );

  const int row = indexOf( m_childEntities.value( parentId ), collection.id() );

  Q_ASSERT( row >= 0 );

  Q_ASSERT( m_collections.contains( parentId ) );

  const QModelIndex parentIndex = indexForCollection( m_collections.value( parentId ) );

  q->beginRemoveRows( parentIndex, row, row );

  // Delete all descendant collections and items.
  removeChildEntities( collection.id() );

  // Remove deleted collection from its parent.
  m_childEntities[ parentId ].removeAt( row );

  q->endRemoveRows();
}

void EntityTreeModelPrivate::removeChildEntities( Collection::Id collectionId )
{
  QList<Node*>::const_iterator it;
  QList<Node*> childList = m_childEntities.value( collectionId );
  const QList<Node*>::const_iterator begin = childList.constBegin();
  const QList<Node*>::const_iterator end = childList.constEnd();
  for ( it = begin; it != end; ++it ) {
    if ( Node::Item == (*it)->type )
      m_items.remove( (*it)->id );
    else {
      removeChildEntities( (*it)->id );
      m_collections.remove( (*it)->id );
    }
  }

  m_childEntities.remove( collectionId );
}

void EntityTreeModelPrivate::monitoredCollectionMoved( const Akonadi::Collection& collection,
                                                       const Akonadi::Collection& sourceCollection,
                                                       const Akonadi::Collection& destCollection )
{
  if ( isHidden( collection ) )
    return;

  if ( isHidden( sourceCollection ) ) {
    if ( isHidden( destCollection ) )
      return;

    monitoredCollectionAdded( collection, destCollection );
    return;
  } else if ( isHidden( destCollection ) ) {
    monitoredCollectionRemoved( collection );
    return;
  }

  if ( !m_collections.contains( collection.id() ) ) {
    kWarning() << "Got a stale notification for a collection which was already removed." << collection.id() << collection.remoteId();
    return;
  }

  Q_Q( EntityTreeModel );

  const QModelIndex srcParentIndex = indexForCollection( sourceCollection );
  const QModelIndex destParentIndex = indexForCollection( destCollection );

  Q_ASSERT( collection.parentCollection() == destCollection );

  const int srcRow = indexOf( m_childEntities.value( sourceCollection.id() ), collection.id() );
  const int destRow = 0; // Prepend collections

  if ( !q->beginMoveRows( srcParentIndex, srcRow, srcRow, destParentIndex, destRow ) ) {
    kWarning() << "Invalid move";
    return;
  }

  Node *node = m_childEntities[ sourceCollection.id() ].takeAt( srcRow );
  // collection has the correct parentCollection etc. We need to set it on the
  // internal data structure to not corrupt things.
  m_collections.insert( collection.id(), collection );
  node->parent = destCollection.id();
  m_childEntities[ destCollection.id() ].prepend( node );
  q->endMoveRows();
}

void EntityTreeModelPrivate::monitoredCollectionChanged( const Akonadi::Collection &collection )
{
  if ( isHidden( collection ) )
    return;

  if ( !m_collections.contains( collection.id() ) ) {
    kWarning() << "Got a stale notification for a collection which was already removed." << collection.id() << collection.remoteId();
    return;
  }

  m_collections[ collection.id() ] = collection;

  const QModelIndex index = indexForCollection( collection );
  Q_ASSERT( index.isValid() );
  dataChanged( index, index );
}

void EntityTreeModelPrivate::monitoredCollectionStatisticsChanged( Akonadi::Collection::Id id,
                                                                   const Akonadi::CollectionStatistics &statistics )
{
  if ( !m_collections.contains( id ) ) {
    kWarning() << "Got statistics response for non-existing collection:" << id;
  } else {
    m_collections[ id ].setStatistics( statistics );

    const QModelIndex index = indexForCollection( m_collections[ id ] );
    dataChanged( index, index );
  }
}

void EntityTreeModelPrivate::monitoredItemAdded( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  if ( isHidden( item ) )
    return;

  if ( !m_collections.contains( collection.id() ) ) {
    kWarning() << "Got a stale notification for an item whose collection was already removed." << item.id() << item.remoteId();
    return;
  }

  Q_ASSERT( m_collections.contains( collection.id() ) );

  if ( !m_mimeChecker.wantedMimeTypes().isEmpty() && !m_mimeChecker.isWantedItem( item ) )
    return;

  const int row = m_childEntities.value( collection.id() ).size();

  const QModelIndex parentIndex = indexForCollection( m_collections.value( collection.id() ) );

  q->beginInsertRows( parentIndex, row, row );
  m_items.insert( item.id(), item );
  Node *node = new Node;
  node->id = item.id();
  node->parent = collection.id();
  node->type = Node::Item;
  m_childEntities[ collection.id() ].append( node );
  q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemRemoved( const Akonadi::Item &item )
{
  Q_Q( EntityTreeModel );

  if ( isHidden( item ) )
    return;

  const Collection::List parents = getParentCollections( item );
  if ( parents.isEmpty() )
    return;

  if ( !m_items.contains( item.id() ) ) {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }

  // TODO: Iterate over all (virtual) collections.
  const Collection collection = parents.first();

  Q_ASSERT( m_collections.contains( collection.id() ) );

  const int row = indexOf( m_childEntities.value( collection.id() ), item.id() );

  const QModelIndex parentIndex = indexForCollection( m_collections.value( collection.id() ) );

  q->beginRemoveRows( parentIndex, row, row );
  m_items.remove( item.id() );
  m_childEntities[ collection.id() ].removeAt( row );
  q->endRemoveRows();
}

void EntityTreeModelPrivate::monitoredItemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( isHidden( item ) )
    return;

  if ( !m_items.contains( item.id() ) ) {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }

  m_items[ item.id() ].apply( item );

  const QModelIndexList indexes = indexesForItem( item );
  foreach ( const QModelIndex &index, indexes ) {
    if ( !index.isValid() )
      kWarning() << "item has invalid index:" << item.id() << item.remoteId();
    else
     dataChanged( index, index );
  }
}

void EntityTreeModelPrivate::monitoredItemMoved( const Akonadi::Item& item,
                                                 const Akonadi::Collection& sourceCollection,
                                                 const Akonadi::Collection& destCollection )
{
  Q_Q( EntityTreeModel );

  if ( isHidden( item ) )
    return;

  if ( isHidden( sourceCollection ) ) {
    if ( isHidden( destCollection ) )
      return;

    monitoredItemAdded( item, destCollection );
    return;
  } else if ( isHidden( destCollection ) ) {
    monitoredItemRemoved( item );
    return;
  }

  if ( !m_items.contains( item.id() ) ) {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }

  Q_ASSERT( m_collections.contains( sourceCollection.id() ) );
  Q_ASSERT( m_collections.contains( destCollection.id() ) );

  const QModelIndex srcIndex = indexForCollection( sourceCollection );
  const QModelIndex destIndex = indexForCollection( destCollection );

  // Where should it go? Always append items and prepend collections and reorganize them with separate reactions to Attributes?

  const Item::Id itemId = item.id();

  const int srcRow = indexOf( m_childEntities.value( sourceCollection.id() ), itemId );
  const int destRow = q->rowCount( destIndex );

  if ( !q->beginMoveRows( srcIndex, srcRow, srcRow, destIndex, destRow ) ) {
    kWarning() << "Invalid move";
    return;
  }

  Node *node = m_childEntities[ sourceCollection.id() ].takeAt( srcRow );
  m_items.insert( item.id(), item );
  node->parent = destCollection.id();
  m_childEntities[ destCollection.id() ].append( node );
  q->endMoveRows();
}

void EntityTreeModelPrivate::monitoredItemLinked( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  if ( isHidden( item ) )
    return;

  if ( !m_items.contains( item.id() ) ) {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }
  Q_ASSERT( m_collections.contains( collection.id() ) );

  if ( !m_mimeChecker.wantedMimeTypes().isEmpty() && !m_mimeChecker.isWantedItem( item ) )
    return;

  const int row = m_childEntities.value( collection.id() ).size();

  const QModelIndex parentIndex = indexForCollection( m_collections.value( collection.id() ) );

  q->beginInsertRows( parentIndex, row, row );
  Node *node = new Node;
  node->id = item.id();
  node->parent = collection.id();
  node->type = Node::Item;
  m_childEntities[ collection.id() ].append( node );
  q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemUnlinked( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  if ( isHidden( item ) )
    return;

  if ( !m_items.contains( item.id() ) ) {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }

  Q_ASSERT( m_collections.contains( collection.id() ) );

  const int row = indexOf( m_childEntities.value( collection.id() ), item.id() );

  const QModelIndex parentIndex = indexForCollection( m_collections.value( collection.id() ) );

  q->beginRemoveRows( parentIndex, row, row );
  m_childEntities[ collection.id() ].removeAt( row );
  q->endRemoveRows();
}

void EntityTreeModelPrivate::fetchJobDone( KJob *job )
{
  if ( job->error() )
    kWarning() << "Job error: " << job->errorString() << endl;
}

void EntityTreeModelPrivate::pasteJobDone( KJob *job )
{
  if ( job->error() )
    kWarning() << "Job error: " << job->errorString() << endl;
}

void EntityTreeModelPrivate::updateJobDone( KJob *job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning() << "Job error:" << job->errorString();
  } else {

    ItemModifyJob *modifyJob = qobject_cast<ItemModifyJob *>( job );
    if ( !modifyJob )
      return;

    const Item item = modifyJob->item();

    Q_ASSERT( item.isValid() );

    m_items[ item.id() ].apply( item );
    const QModelIndexList list = indexesForItem( item );

    foreach ( const QModelIndex &index, list )
      dataChanged( index, index );

    // TODO: Is this trying to do the job of collectionstatisticschanged?
//     CollectionStatisticsJob *csjob = static_cast<CollectionStatisticsJob*>( job );
//     Collection result = csjob->collection();
//     collectionStatisticsChanged( result.id(), csjob->statistics() );
  }
}

void EntityTreeModelPrivate::rootCollectionFetched( const Collection::List &list )
{
  Q_ASSERT( list.size() == 1 );
  m_rootCollection = list.first();
  startFirstListJob();
}

void EntityTreeModelPrivate::startFirstListJob()
{
  Q_Q( EntityTreeModel );

  if ( m_collections.size() > 0 )
    return;

  // Even if the root collection is the invalid collection, we still need to start
  // the first list job with Collection::root.
  if ( m_showRootCollection ) {
    // Notify the outside that we're putting collection::root into the model.
    q->beginInsertRows( QModelIndex(), 0, 0 );
    m_collections.insert( m_rootCollection.id(), m_rootCollection );
    m_rootNode = new Node;
    m_rootNode->id = m_rootCollection.id();
    m_rootNode->parent = -1;
    m_rootNode->type = Node::Collection;
    m_childEntities[ -1 ].append( m_rootNode );
    q->endInsertRows();
  } else {
    // Otherwise store it silently because it's not part of the usable model.
    m_rootNode = new Node;
    m_rootNode->id = m_rootCollection.id();
    m_rootNode->parent = -1;
    m_rootNode->type = Node::Collection;
    m_collections.insert( m_rootCollection.id(), m_rootCollection );
  }

  // Includes recursive trees. Lower levels are fetched in the onRowsInserted slot if
  // necessary.
  // HACK: fix this for recursive listing if we filter on mimetypes that only exit deeper
  // in the hierarchy
  if ( ( m_collectionFetchStrategy == EntityTreeModel::FetchFirstLevelChildCollections )
    /*|| ( m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsRecursive )*/ ) {
    fetchCollections( m_rootCollection, CollectionFetchJob::FirstLevel );
  }

  if ( m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsRecursive )
    fetchCollections( m_rootCollection, CollectionFetchJob::Recursive );
  // If the root collection is not collection::root, then it could have items, and they will need to be
  // retrieved now.

  if ( m_itemPopulation != EntityTreeModel::NoItemPopulation ) {
    if ( m_rootCollection != Collection::root() )
      fetchItems( m_rootCollection );
  }

  // Resources which are explicitly monitored won't have appeared yet if their mimetype didn't match.
  // We fetch the top level collections and examine them for whether to add them.
  // This fetches virtual collections into the tree.
  if ( !m_monitor->resourcesMonitored().isEmpty() )
    fetchTopLevelCollections();
}

void EntityTreeModelPrivate::fetchTopLevelCollections() const
{
  Q_Q( const EntityTreeModel );
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::FirstLevel, m_session );
  q->connect( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ),
              q, SLOT( topLevelCollectionsFetched( const Akonadi::Collection::List& ) ) );
  q->connect( job, SIGNAL( result( KJob* ) ),
              q, SLOT( fetchJobDone( KJob* ) ) );
}

void EntityTreeModelPrivate::topLevelCollectionsFetched( const Akonadi::Collection::List& list )
{
  Q_Q( EntityTreeModel );
  foreach( const Collection &collection, list ) {
    // These collections have been explicitly shown in the Monitor,
    // but hidden trumps that for now. This may change in the future if we figure out a use for it.
    if ( isHidden( collection ) )
      continue;

    if ( m_monitor->resourcesMonitored().contains( collection.resource().toUtf8() ) && !m_collections.contains( collection.id() ) ) {
      const QModelIndex parentIndex = indexForCollection( collection.parentCollection() );
      // Prepending new collections.
      const int row  = 0;
      q->beginInsertRows( parentIndex, row, row );

      m_collections.insert( collection.id(), collection );
      Node *node = new Node;
      node->id = collection.id();
      node->parent = collection.parentCollection().id();
      node->type = Node::Collection;
      m_childEntities[ collection.parentCollection().id() ].prepend( node );

      q->endInsertRows();

      CollectionFetchJob *job = new CollectionFetchJob( collection, CollectionFetchJob::FirstLevel, m_session );
      job->fetchScope().setIncludeUnsubscribed( m_includeUnsubscribed );
      job->fetchScope().setIncludeStatistics( m_includeStatistics );
      job->fetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );
      q->connect( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ),
                  q, SLOT( collectionsFetched( const Akonadi::Collection::List& ) ) );
      q->connect( job, SIGNAL( result( KJob* ) ),
                  q, SLOT( fetchJobDone( KJob* ) ) );
    }
  }
}

Collection EntityTreeModelPrivate::getParentCollection( Entity::Id id ) const
{
  QHashIterator<Collection::Id, QList<Node*> > iter( m_childEntities );
  while ( iter.hasNext() ) {
    iter.next();
    if ( indexOf( iter.value(), id ) != -1 ) {
      return m_collections.value( iter.key() );
    }
  }

  return Collection();
}

Collection::List EntityTreeModelPrivate::getParentCollections( const Item &item ) const
{
  Collection::List list;
  QHashIterator<Collection::Id, QList<Node*> > iter( m_childEntities );
  while ( iter.hasNext() ) {
    iter.next();
    if ( indexOf( iter.value(), item.id() ) != -1 ) {
      list << m_collections.value( iter.key() );
    }
  }

  return list;
}

Collection EntityTreeModelPrivate::getParentCollection( const Collection &collection ) const
{
  return m_collections.value( collection.parentCollection().id() );
}

Entity::Id EntityTreeModelPrivate::childAt( Collection::Id id, int position, bool *ok ) const
{
  const QList<Node*> list = m_childEntities.value( id );
  if ( list.size() <= position ) {
    *ok = false;
    return 0;
  }

  *ok = true;
  return list.at( position )->id;
}


int EntityTreeModelPrivate::indexOf( Collection::Id parent, Collection::Id collectionId ) const
{
  return indexOf( m_childEntities.value( parent ), collectionId );
}

Item EntityTreeModelPrivate::getItem( Item::Id id ) const
{
  if ( id > 0 )
    id *= -1;

  return m_items.value( id );
}

void EntityTreeModelPrivate::ref( Collection::Id id )
{
  m_monitor->d_ptr->ref( id );
}

bool EntityTreeModelPrivate::shouldPurge( Collection::Id id )
{
  if ( m_monitor->d_ptr->refCountMap.contains( id ) )
    return false;

  if ( m_monitor->d_ptr->m_buffer.isBuffered( id ) )
    return false;

  static const int MAXITEMS = 10000;

  if ( m_items.size() < MAXITEMS )
    return false;

  return true;
}

void EntityTreeModelPrivate::deref( Collection::Id id )
{
  const Collection::Id bumpedId = m_monitor->d_ptr->deref( id );

  if ( bumpedId < 0 )
    return;

  if ( shouldPurge( bumpedId ) )
    purgeItems( bumpedId );
}

QList<Node*>::iterator EntityTreeModelPrivate::skipCollections( QList<Node*>::iterator it, QList<Node*>::iterator end, int * pos )
{
  for ( ; it != end; ++it ) {
    if ( ( *it )->type == Node::Item )
      break;

    ++( *pos );
  }

  return it;
}

QList<Node*>::iterator EntityTreeModelPrivate::removeItems( QList<Node*>::iterator it, QList<Node*>::iterator end, int *pos, const Collection &collection )
{
  Q_Q( EntityTreeModel );

  QList<Node *>::iterator startIt = it;

  int start = *pos;
  for ( ; it != end; ++it ) {
    if ( ( *it )->type != Node::Item )
      break;

    ++(*pos);
  }

  const QModelIndex parentIndex = indexForCollection( collection );

  q->beginRemoveRows( parentIndex, start, (*pos) - 1 );
  m_childEntities[ collection.id() ].erase( startIt, it );
  q->endRemoveRows();

  return it;
}

void EntityTreeModelPrivate::purgeItems( Collection::Id id )
{
  QList<Node*> &childEntities = m_childEntities[ id ];

  const Collection collection = m_collections.value( id );
  Q_ASSERT( collection.isValid() );

  QList<Node*>::iterator begin = childEntities.begin();
  const QList<Node*>::iterator end = childEntities.end();

  int pos = 0;
  while ( (begin = skipCollections( begin, end, &pos )) != end )
    begin = removeItems( begin, end, &pos, collection );
}

void EntityTreeModelPrivate::dataChanged( const QModelIndex &top, const QModelIndex &bottom )
{
  Q_Q( EntityTreeModel );

  QModelIndex rightIndex;

  Node* node = reinterpret_cast<Node*>( bottom.internalPointer() );

  if ( !node )
    return;

  if ( node->type == Node::Collection )
    rightIndex = bottom.sibling( bottom.row(), q->entityColumnCount( EntityTreeModel::CollectionTreeHeaders ) - 1 );
  if ( node->type == Node::Item )
    rightIndex = bottom.sibling( bottom.row(), q->entityColumnCount( EntityTreeModel::ItemListHeaders ) - 1 );

  emit q->dataChanged( top, rightIndex );
}

QModelIndex EntityTreeModelPrivate::indexForCollection( const Collection &collection ) const
{
  Q_Q( const EntityTreeModel );

  // The id of the parent of Collection::root is not guaranteed to be -1 as assumed by startFirstListJob,
  // we ensure that we use -1 for the invalid Collection.
  const Collection::Id parentId = collection.parentCollection().isValid() ? collection.parentCollection().id() : -1;

  const int row = indexOf( m_childEntities.value( parentId ), collection.id() );

  if ( row < 0 )
    return QModelIndex();

  Node *node = m_childEntities.value( parentId ).at( row );

  return q->createIndex( row, 0, reinterpret_cast<void*>( node ) );
}

QModelIndexList EntityTreeModelPrivate::indexesForItem( const Item &item ) const
{
  Q_Q( const EntityTreeModel );
  QModelIndexList indexes;

  const Collection::List collections = getParentCollections( item );
  const qint64 id = item.id();

  foreach ( const Collection &collection, collections ) {
    const int row = indexOf( m_childEntities.value( collection.id() ), id );

    Node *node = m_childEntities.value( collection.id() ).at( row );

    indexes << q->createIndex( row, 0, reinterpret_cast<void*>( node ) );
  }

  return indexes;
}

void EntityTreeModelPrivate::beginResetModel()
{
  Q_Q( EntityTreeModel );
  q->beginResetModel();
}

void EntityTreeModelPrivate::endResetModel()
{
  Q_Q( EntityTreeModel );
  m_collections.clear();
  m_items.clear();
  m_childEntities.clear();
  q->endResetModel();
  fillModel();
}

void EntityTreeModelPrivate::fillModel()
{
  Q_Q( EntityTreeModel );
  if ( m_rootCollection == Collection::root() )
  {
    QTimer::singleShot( 0, q, SLOT( startFirstListJob() ) );
  } else {
    CollectionFetchJob *rootFetchJob = new CollectionFetchJob( m_rootCollection, CollectionFetchJob::Base, m_session );
    q->connect( rootFetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(rootCollectionFetched(Akonadi::Collection::List)) );
    q->connect( rootFetchJob, SIGNAL(result(KJob *)), SLOT(fetchJobDone(KJob *)) );
  }
}

