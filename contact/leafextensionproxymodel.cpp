/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#include "leafextensionproxymodel_p.h"

#include <QtCore/QSet>

using namespace Akonadi;

class LeafExtensionProxyModel::Private
{
  public:
    Private( LeafExtensionProxyModel *qq )
      : q( qq ), mUniqueKeyCounter( 0 )
    {
    }

    void sourceRowsInserted( const QModelIndex&, int, int );
    void sourceRowsRemoved( const QModelIndex&, int, int );

    LeafExtensionProxyModel *q;
    QMap<qint64, QModelIndex> mParentIndexes;
    QSet<QModelIndex> mOwnIndexes;
    qint64 mUniqueKeyCounter;
};

void LeafExtensionProxyModel::Private::sourceRowsInserted( const QModelIndex &parentIndex, int start, int end )
{
  // iterate over all of our stored parent indexes
  QMutableMapIterator<qint64, QModelIndex> it( mParentIndexes );
  while ( it.hasNext() ) {
    it.next();
    if ( it.value().parent() == parentIndex ) {
      if ( it.value().row() >= start ) {
        const QModelIndex newIndex = q->QSortFilterProxyModel::index( it.value().row() + ( end - start ) + 1, it.value().column(), parentIndex );
        it.setValue( newIndex );
      }
    }
  }
}

void LeafExtensionProxyModel::Private::sourceRowsRemoved( const QModelIndex &parentIndex, int start, int end )
{
  // iterate over all of our stored parent indexes
  QMutableMapIterator<qint64, QModelIndex> it( mParentIndexes );
  while ( it.hasNext() ) {
    it.next();
    if ( it.value().parent() == parentIndex ) {
      if ( it.value().row() >= start && it.value().row() <= end ) {
        it.remove();
      } else if ( it.value().row() > end ) {
        const QModelIndex newIndex = q->index( it.value().row() - ( end - start ) - 1, it.value().column(), parentIndex );
        it.setValue( newIndex );
      }
    }
  }
}

LeafExtensionProxyModel::LeafExtensionProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ), d( new Private( this ) )
{
}

LeafExtensionProxyModel::~LeafExtensionProxyModel()
{
  delete d;
}

QModelIndex LeafExtensionProxyModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( row < 0 || column < 0 ) {
    return QModelIndex();
  }

  if ( parent.isValid() ) {
    const QModelIndex sourceParent = mapToSource( parent );
    const QModelIndex sourceIndex = sourceModel()->index( row, column, sourceParent );
    if ( !sourceIndex.isValid() ) {

      qint64 key = -1;
      QMapIterator<qint64, QModelIndex> it( d->mParentIndexes );
      while ( it.hasNext() ) {
        it.next();
        if ( it.value() == parent ) {
          key = it.key();
          break;
        }
      }

      if ( key == -1 ) {
        key = ++( d->mUniqueKeyCounter );
        d->mParentIndexes.insert( key, parent );
      }

      const QModelIndex index = createIndex( row, column, static_cast<quint32>( key ) );
      d->mOwnIndexes.insert( index );

      return index;
    }
  }

  return QSortFilterProxyModel::index( row, column, parent );
}

QModelIndex LeafExtensionProxyModel::parent( const QModelIndex &index ) const
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return d->mParentIndexes.value( index.internalId() );
  }

  return QSortFilterProxyModel::parent( index );
}

int LeafExtensionProxyModel::rowCount( const QModelIndex &index ) const
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return 0;
  }

  const QModelIndex sourceIndex = mapToSource( index );
  if ( sourceModel()->rowCount( sourceIndex ) == 0 ) {
    return leafRowCount( index );
  }

  return QSortFilterProxyModel::rowCount( index );
}

int LeafExtensionProxyModel::columnCount( const QModelIndex &index ) const
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return 1;
  }

  return QSortFilterProxyModel::columnCount( index );
}

QVariant LeafExtensionProxyModel::data( const QModelIndex &index, int role ) const
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return leafData( index.parent(), index.row(), index.column(), role );
  }

  return QSortFilterProxyModel::data( index, role );
}

Qt::ItemFlags LeafExtensionProxyModel::flags( const QModelIndex &index ) const
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return Qt::ItemFlags( Qt::ItemIsEnabled|Qt::ItemIsSelectable );
  }

  return QSortFilterProxyModel::flags( index );
}

bool LeafExtensionProxyModel::setData( const QModelIndex &index, const QVariant &data, int role )
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return false;
  }

  return QSortFilterProxyModel::setData( index, data, role );
}

bool LeafExtensionProxyModel::hasChildren( const QModelIndex &parent ) const
{
  if ( d->mOwnIndexes.contains( parent ) ) {
    return false; // extensible in the future?
  }

  const QModelIndex sourceParent = mapToSource( parent );
  if ( sourceModel() && sourceModel()->rowCount( sourceParent ) == 0 ) {
    return ( leafRowCount( parent ) != 0 );
  }

  return QSortFilterProxyModel::hasChildren( parent );
}

QModelIndex LeafExtensionProxyModel::buddy( const QModelIndex &index ) const
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return index;
  }

  return QSortFilterProxyModel::buddy( index );
}

void LeafExtensionProxyModel::fetchMore( const QModelIndex &index )
{
  if ( d->mOwnIndexes.contains( index ) ) {
    return;
  }

  QSortFilterProxyModel::fetchMore( index );
}

void LeafExtensionProxyModel::setSourceModel( QAbstractItemModel *_sourceModel )
{
  if ( _sourceModel == sourceModel() ) {
    return;
  }

  beginResetModel();

  disconnect( this, SIGNAL(rowsInserted(QModelIndex,int,int)),
              this, SLOT(sourceRowsInserted(QModelIndex,int,int)) );
  disconnect( this, SIGNAL(rowsRemoved(QModelIndex,int,int)),
              this, SLOT(sourceRowsRemoved(QModelIndex,int,int)) );

  QSortFilterProxyModel::setSourceModel( _sourceModel );

  connect( this, SIGNAL(rowsInserted(QModelIndex,int,int)),
           this, SLOT(sourceRowsInserted(QModelIndex,int,int)) );
  connect( this, SIGNAL(rowsRemoved(QModelIndex,int,int)),
           this, SLOT(sourceRowsRemoved(QModelIndex,int,int)) );

  endResetModel();
}

#include "moc_leafextensionproxymodel_p.cpp"
