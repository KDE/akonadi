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

#include "entitytreemodel.h"
#include "entitytreemodel_p.h"


#include <QtCore/QAbstractItemModel>
#include <QtCore/QHash>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionmodel_p.h"
// #include "entityaboveattribute.h"
#include "entitydisplayattribute.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "monitor.h"
#include "pastehelper.h"

#include <KIcon>
#include <KUrl>

#include "kdebug.h"

using namespace Akonadi;

EntityTreeModel::EntityTreeModel( const QStringList &mimetypes, QObject *parent )
  : CollectionModel( new EntityTreeModelPrivate( this ), parent )
{
  Q_D( EntityTreeModel );
  d->m_mimeTypeFilter = mimetypes;

  connect( this, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
           SLOT( onRowsInserted( const QModelIndex &, int, int ) ) );

  connect( d->itemMonitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           this, SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  connect( d->itemMonitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
           this, SLOT( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
  connect( d->itemMonitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
           this, SLOT( itemRemoved( const Akonadi::Item& ) ) );

}

EntityTreeModel::~EntityTreeModel()
{

}

QVariant EntityTreeModel::data( const QModelIndex & index, int role ) const
{
  Q_D( const EntityTreeModel );
  if ( isItem( index ) ) {
    switch ( role ) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      if ( d->m_items.value( index.internalId() ).hasAttribute<EntityDisplayAttribute>() &&
           ! d->m_items.value( index.internalId() ).attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
        return d->m_items.value( index.internalId() ).attribute<EntityDisplayAttribute>()->displayName();
      return d->m_items.value( index.internalId() ).remoteId();
      break;

    case Qt::DecorationRole:
      if ( d->m_items.value( index.internalId() ).hasAttribute<EntityDisplayAttribute>() &&
           ! d->m_items.value( index.internalId() ).attribute<EntityDisplayAttribute>()->iconName().isEmpty() )
        return d->m_items.value( index.internalId() ).attribute<EntityDisplayAttribute>()->icon();
      break;

    case ItemRole:
      return QVariant::fromValue( d->m_items.value( index.internalId() ) );
      break;

    case ItemIdRole:
      return d->m_items.value( index.internalId() ).id();
      break;

    case MimeTypeRole:
      return d->m_items.value( index.internalId() ).mimeType();
      break;

    case RemoteIdRole:
      return d->m_items.value( index.internalId() ).remoteId();
      break;

//     case EntityAboveRole:
//       if (  d->m_items.value(index.internalId()).hasAttribute<EntityAboveAttribute>() &&
//         !d->m_items.value(index.internalId()).attribute<EntityAboveAttribute>()->entityAbove().isEmpty() )
//         return d->m_items.value(index.internalId()).attribute<EntityAboveAttribute>()->entityAbove();
      break;

    default:
      break;
    }
  }

  if ( isCollection( index ) ) {
    switch ( role ) {
    case MimeTypeRole:
      return d->collections.value( index.internalId() ).mimeType();
      break;

    case RemoteIdRole:
      return d->collections.value( index.internalId() ).remoteId();

//     case EntityAboveRole:
//       if (  d->collections.value(index.internalId()).hasAttribute<EntityAboveAttribute>() &&
//         ! d->collections.value(index.internalId()).attribute<EntityAboveAttribute>()->entityAbove().isEmpty() )
//         return d->collections.value(index.internalId()).attribute<EntityAboveAttribute>()->entityAbove();
      break;

      break;
    default:
      break;
    }

  }

  return CollectionModel::data( index, role );
}


Qt::ItemFlags EntityTreeModel::flags( const QModelIndex & index ) const
{
  Q_D( const EntityTreeModel );
  if ( isCollection( index ) ) {
    return CollectionModel::flags( index );
  }

  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

  flags = flags | Qt::ItemIsDragEnabled;

  Item item;
  if ( index.isValid() ) {
    item = d->m_items.value( index.internalId() );
    Q_ASSERT( item.isValid() );
  } else
    return flags | Qt::ItemIsDropEnabled; // HACK Workaround for a probable bug in Qt

  const Collection col( index.parent().internalId() );
  if ( col.isValid() ) {
    if ( col.rights() & ( Collection::CanChangeItem  |
                          Collection::CanCreateItem |
                          Collection::CanDeleteItem |
                          Collection::CanChangeCollection ) ) {
      if ( index.column() == 0 )
        flags = flags | Qt::ItemIsEditable;

      flags = flags | Qt::ItemIsDropEnabled;
    }
  }

  return flags;
}


bool EntityTreeModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  // TODO: Implement this!


  if ( isCollection( parent ) ) {
    // TODO: I might not be able to use this convenience.
//     CollectionModel::dropMimeData(data, action, row, column, parent);
  }

  const Collection col( parent.internalId() );

  KJob* job = PasteHelper::paste( data, col, action != Qt::MoveAction );
  // TODO: error handling
  return job;
}

QModelIndex EntityTreeModel::index( int row, int column, const QModelIndex & parent ) const
{

  Q_D( const EntityTreeModel );

  if ( row < d->childCollections.value( parent.internalId() ).size() ) {
    // This will return an invalid index where necessary.
    return CollectionModel::index( row, column, parent );
  }

  // Because we already know that index is not a model, skip over all collection indexes.
  // For example, if a parent collection has 5 child collections and 4 child items, the
  // 7th row of that parent is the 3rd item in the internal list of items (recalling that
  // row is zero indexed). offsetRow would be 3 in that case.
  const int offsetRow = row - d->childCollections.value( parent.internalId() ).size();

  const QList<Item::Id> list = d->m_itemsInCollection[ parent.internalId()];

  if ( offsetRow < 0 || offsetRow >= list.size() )
    return QModelIndex();
  if ( !d->m_items.contains( list.at( offsetRow ) ) )
    return QModelIndex();

  return createIndex( row, column, reinterpret_cast<void*>( list.at( offsetRow ) ) );
}

QModelIndex EntityTreeModel::parent( const QModelIndex & index ) const
{
  Q_D( const EntityTreeModel );

//   if ( !d->m_items.contains( index.internalId() ) )
  if ( isCollection( index ) ) {
    const QModelIndex idx = CollectionModel::parent( index );
    return idx;
  }

  const Item item = d->m_items.value( index.internalId() );
  if ( !item.isValid() )
    return QModelIndex();

  QHashIterator< Collection::Id, QList<Item::Id> > iter( d->m_itemsInCollection );
  while ( iter.hasNext() ) {
    iter.next();
    if ( iter.value().contains( item.id() ) ) {
      return d->indexForId( iter.key() );
    }
  }

  return CollectionModel::parent( index );
}

int EntityTreeModel::rowCount( const QModelIndex & parent ) const
{
  Q_D( const EntityTreeModel );
  return d->childEntitiesCount( parent );
}

QMimeData *EntityTreeModel::mimeData( const QModelIndexList &indexes ) const
{
  QMimeData *data = new QMimeData();
  KUrl::List urls;
  foreach( const QModelIndex &index, indexes ) {
    if ( index.column() != 0 )
      continue;

    if ( isCollection( index ) ) {
      urls << Collection( index.internalId() ).url();
    }

    if ( isItem( index ) ) {
      urls << Item( index.internalId() ).url();
    }
  }
  urls.populateMimeData( data );

  return data;
}

bool EntityTreeModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  Q_D( EntityTreeModel );
  if ( index.column() == 0 && role == Qt::EditRole ) {
    if ( isCollection( index ) ) {
      // rename collection
      Collection col = d->collections.value( index.internalId() );
      if ( !col.isValid() || value.toString().isEmpty() )
        return false;
      col.setName( value.toString() );
      CollectionModifyJob *job = new CollectionModifyJob( col );
      connect( job, SIGNAL( result( KJob* ) ), SLOT( editDone( KJob* ) ) );
      return true;
    }
    if ( isItem( index ) ) {
      Item item = d->m_items.value( index.internalId() );
      if ( !item.isValid() || value.toString().isEmpty() )
        return false;

//       if ( item.hasAttribute< EntityDisplayAttribute >() )
//       {
      EntityDisplayAttribute *displayAttribute = item.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
      displayAttribute->setDisplayName( value.toString() );
//       }

      ItemModifyJob *job = new ItemModifyJob( item );
      connect( job, SIGNAL( result( KJob* ) ), SLOT( editDone( KJob* ) ) );
      return true;
    }
  }

  return QAbstractItemModel::setData( index, value, role );
}



// Item.id() is unique to an item, Collection.id() is unique to a collection.
// However, a collection and an item can have the same id.
bool EntityTreeModel::isItem( const QModelIndex &index ) const
{
  Q_D( const EntityTreeModel );

  if ( d->m_items.contains( index.internalId() ) ) {
    if ( !d->collections.contains( index.internalId() ) ) {
      return true;
    }

    // The id is common to a collection and an item.
    // Assume the index is a collection and proove myself wrong.
    const Collection::Id parentId = d->collections.value( index.internalId() ).parent();
    if ( d->collections.contains( parentId ) ) {
      const int row = index.row();
      const int childColsCount = d->childCollections.value( parentId ).size();
      const int childItemsCount = d->m_itemsInCollection.value( parentId ).size();
      if (( row >= childColsCount ) && ( row < ( childColsCount + childItemsCount ) ) ) {
        // I don't have enough collections for this to be a row, and it is within the bounds of being
        // an item.
        return true;
      }
    }
  }

  return false;
}

bool EntityTreeModel::isCollection( const QModelIndex &index ) const
{
  Q_D( const EntityTreeModel );

  if ( d->collections.contains( index.internalId() ) ) {
    if ( !d->m_items.contains( index.internalId() ) ) {
      return true;
    }

    // The id is common to a collection and an item.
    // Assume the index is a collection and proove myself right.
    const Collection::Id parentId = d->collections.value( index.internalId() ).parent();
    if ( d->collections.contains( parentId ) ) {
      const int row = index.row();
      const int childColsCount = d->childCollections.value( parentId ).size();
      if ( row < childColsCount ) {
        return true;
      }
    }
  }

  return false;
}

#include "entitytreemodel.moc"
