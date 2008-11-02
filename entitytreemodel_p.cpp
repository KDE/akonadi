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



#include <QtCore/QAbstractItemModel>
#include <QHash>
#include <QStringList>
#include <QTimer>

#include <KUrl>

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionmodel_p.h"
// #include "entityaboveattribute.h"
#include "entitydisplayattribute.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "monitor.h"

#include <kdebug.h>

using namespace Akonadi;

EntityTreeModelPrivate::EntityTreeModelPrivate( EntityTreeModel *parent )
    : CollectionModelPrivate( parent )
{
  itemMonitor = new Monitor();
  itemMonitor->setCollectionMonitored( Collection::root() );
  itemMonitor->fetchCollection( true );
//   itemMonitor->itemFetchScope().fetchAttribute<EntityAboveAttribute>();
  itemMonitor->itemFetchScope().fetchAttribute< EntityDisplayAttribute >();
}


void EntityTreeModelPrivate::onRowsInserted( const QModelIndex & parent, int start, int end )
{
  Q_Q( EntityTreeModel );

  // This slot can be notified of several new collections in the model.
  // Iterate to add items for each one.
  for ( int iCount = start; iCount <= end; iCount++ ) {
    QModelIndex i = q->CollectionModel::index( iCount, 0, parent );

    if ( !i.isValid() )
      continue; // This is not a collection, so it doesn't have any items.

    QVariant datav = q->data( i, CollectionModel::CollectionRole );

    if ( datav == QVariant() ) {
      // This is not a collection, so it doesn't have any items.
      continue;
    }

    Akonadi::Collection col = qvariant_cast< Akonadi::Collection > ( datav );

    if ( !mimetypeMatches( col.contentMimeTypes() ) ) {
      // New collection shouldn't have its items added to the model.
      continue;
    }

    Akonadi::ItemFetchJob *itemJob = new Akonadi::ItemFetchJob( col );
    itemJob->fetchScope().fetchFullPayload();
    itemJob->fetchScope().fetchAttribute< EntityDisplayAttribute >();
//     itemJob->fetchScope().fetchAttribute<EntityAboveAttribute>();

    itemJob->setProperty( ItemFetchCollectionId(), QVariant( col.id() ) );

    q->connect( itemJob, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
                q, SLOT( itemsAdded( Akonadi::Item::List ) ) );
    q->connect( itemJob, SIGNAL( result( KJob* ) ),
                q, SLOT( listDone( KJob* ) ) );
  }
}


bool EntityTreeModelPrivate::mimetypeMatches( const QStringList &mimetypes )
{
  QStringList::const_iterator constIterator;
  bool found = false;

  for ( constIterator = mimetypes.constBegin(); constIterator != mimetypes.constEnd(); ++constIterator ) {
    if ( m_mimeTypeFilter.contains(( *constIterator ) ) ) {
      found = true;
      break;
    }
  }
  return found;
}

void EntityTreeModelPrivate::itemChanged( const Akonadi::Item &item, const QSet< QByteArray >& )
{
  Q_Q( EntityTreeModel );
  // No need to check if this is an item.
  if ( m_items.contains( item.id() ) ) {
    QModelIndex i = indexForItem( item );
    emit q->dataChanged( i, i );
  }
}

QModelIndex EntityTreeModelPrivate::indexForItem( Item item )
{
  Q_Q( EntityTreeModel );

  Item::Id id = item.id();

  QHashIterator< Collection::Id, QList< Item::Id > > iter( m_itemsInCollection );
  while ( iter.hasNext() ) {
    iter.next();
    if ( iter.value().contains( id ) ) {    // value is the QList<Item::Id>.
      QModelIndex parentIndex = indexForId( iter.key() );    // key is the Collection::Id.
      Collection::Id parentId = parentIndex.internalId();
      int collectionCount = childCollections.value( parentId ).size();
      int row = collectionCount + m_itemsInCollection[ parentId ].indexOf( id );
      return q->index( row, 0, parentIndex );
    }
  }

  return QModelIndex();
}

void EntityTreeModelPrivate::itemMoved( const Akonadi::Item &item, const Akonadi::Collection& colSrc, const Akonadi::Collection& colDst )
{
  kDebug() << "item.remoteId=" << item.remoteId() << "sourceCollectionId=" << colSrc.remoteId()
  << "destCollectionId=" << colDst.remoteId();

  // TODO: Implement this.

}

void EntityTreeModelPrivate::itemsAdded( const Akonadi::Item::List &list )
{
  Q_Q( EntityTreeModel );
  QObject *job = q->sender();
  if (job)
  {
    Collection::Id colId = job->property( ItemFetchCollectionId() ).value<Collection::Id>();

    Item::List itemsToInsert;

    foreach( Item item, list ) {
      if ( mimetypeMatches( QStringList() << item.mimeType() ) ) {
        itemsToInsert << item;
      }
    }

    if ( itemsToInsert.size() > 0 ) {
      QModelIndex parentIndex = indexForId( colId );
  // //         beginInsertRows( parent, d->childEntitiesCount( parent ), d->childEntitiesCount( parent ) + itemsToInsert.size() - 1 );
      foreach( Item item, itemsToInsert ) {
        Item::Id itemId = item.id();
        q->beginInsertRows( parentIndex, childEntitiesCount( parentIndex ), childEntitiesCount( parentIndex ) );
        m_items.insert( itemId, item );
        m_itemsInCollection[ colId ].append( itemId );
        q->endInsertRows();
      }
      //       endInsertRows();
    }
  }
}

void EntityTreeModelPrivate::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& col )
{
  Q_Q( EntityTreeModel );

  QModelIndex parentIndex = indexForId( col.id() );
  q->beginInsertRows( parentIndex, childEntitiesCount( parentIndex ), childEntitiesCount( parentIndex ) );
  m_items.insert( item.id(), item );
  m_itemsInCollection[ col.id()].append( item.id() );
  q->endInsertRows();

}

void EntityTreeModelPrivate::itemRemoved( const Akonadi::Item &item )
{
  Q_Q( EntityTreeModel );
  QModelIndex itemIndex = indexForItem( item );
  QModelIndex parent = itemIndex.parent();
  int row = m_itemsInCollection.value( parent.internalId() ).indexOf( item.id() );

  q->beginRemoveRows( itemIndex.parent(), row, row );

  m_items.remove( item.id() );

  QList<Item::Id> list = m_itemsInCollection.value( parent.internalId() );
  list.removeAt( row );
  m_itemsInCollection.insert( parent.internalId(), list );

  q->endRemoveRows();
}

void EntityTreeModelPrivate::listDone( KJob * job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Job error: " << job->errorString() << endl;
  }
}


int EntityTreeModelPrivate::childEntitiesCount( const QModelIndex & parent ) const
{
  return childCollections.value( parent.internalId() ).size() + m_itemsInCollection.value( parent.internalId() ).size();
}
