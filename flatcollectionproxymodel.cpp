/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "flatcollectionproxymodel.h"

#include <kdebug.h>

using namespace Akonadi;

class FlatCollectionProxyModel::Private
{
  public:
    Private( FlatCollectionProxyModel* parent ) : q( parent ) {}

    // ### SLOW!
    static int totalCount( QAbstractItemModel *model, const QModelIndex &index )
    {
      Q_ASSERT( model );
      const int rows = model->rowCount( index );
      int count = rows;
      for ( int i = 0; i < rows; ++i ) {
        QModelIndex current = model->index( i, 0, index );
        if ( current.isValid() )
          count += totalCount( model, current );
      }
      return count;
    }

    // ### EVEN SLOWER, and even called multiple times in a row!
    static QModelIndex findSourceIndex( QAbstractItemModel *model, int _row, int col, const QModelIndex &parent = QModelIndex() )
    {
      int row = _row;
      Q_ASSERT( model );
      for ( int i = 0; i <= row; ++i ) {
        QModelIndex current = model->index( i, col, parent );
        if ( i == row )
          return current;
        const int childCount = totalCount( model, current );
        if ( childCount >= ( row - i ) )
          return findSourceIndex( model, row - i - 1, col, current );
        else
          row -= childCount;
      }
      Q_ASSERT( false );
      return QModelIndex();
    }

    void sourceDataChanged( const QModelIndex &sourceTopLeft, const QModelIndex &sourceBottomRight )
    {
      if ( sourceTopLeft != sourceBottomRight ) {
        emit q->dataChanged( q->mapFromSource( sourceTopLeft ), q->mapFromSource( sourceBottomRight ) );
      } else {
        const QModelIndex proxyIndex = q->mapFromSource( sourceTopLeft );
        emit q->dataChanged( proxyIndex, proxyIndex );
      }
    }

    FlatCollectionProxyModel *q;
};

FlatCollectionProxyModel::FlatCollectionProxyModel(QObject * parent) :
    QAbstractProxyModel( parent ),
    d( new Private( this ) )
{
}

FlatCollectionProxyModel::~FlatCollectionProxyModel()
{
  delete d;
}

void FlatCollectionProxyModel::setSourceModel(QAbstractItemModel * sourceModel)
{
  QAbstractProxyModel::setSourceModel( sourceModel );
  connect( sourceModel, SIGNAL(modelReset()), SIGNAL(modelReset()) );
  connect( sourceModel, SIGNAL(layoutChanged()), SIGNAL(layoutChanged()) );
  connect( sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
           SLOT(sourceDataChanged(QModelIndex,QModelIndex)) );
}

int FlatCollectionProxyModel::rowCount(const QModelIndex & parent) const
{
  if ( parent.isValid() )
    return 0;
  return d->totalCount( sourceModel(), mapToSource( parent ) );
}

int FlatCollectionProxyModel::columnCount(const QModelIndex & parent) const
{
  return sourceModel()->columnCount( mapToSource( parent ) );
}

QModelIndex FlatCollectionProxyModel::index(int row, int column, const QModelIndex & parent) const
{
  Q_UNUSED( parent );
  return createIndex( row, column );
}

QModelIndex FlatCollectionProxyModel::parent(const QModelIndex & index) const
{
  Q_UNUSED( index );
  return QModelIndex();
}

QModelIndex FlatCollectionProxyModel::mapToSource(const QModelIndex & proxyIndex) const
{
  if ( !proxyIndex.isValid() )
    return QModelIndex();
  return d->findSourceIndex( sourceModel(), proxyIndex.row(), proxyIndex.column() );
}

QModelIndex FlatCollectionProxyModel::mapFromSource(const QModelIndex & sourceIndex) const
{
  if ( !sourceIndex.isValid() )
    return QModelIndex();
  int row = 0;
  QModelIndex current = sourceIndex;
  while ( current.parent() != QModelIndex() ) {
    row += current.row() + 1;
    for ( int i = current.row() - 1; i >= 0; --i )
      row += d->totalCount( sourceModel(), sourceModel()->index( i, 0, current.parent() ) );
    current = current.parent();
  }
  row += current.row();
  for ( int i = current.row() - 1; i >= 0; --i )
    row += d->totalCount( sourceModel(), sourceModel()->index( i, 0 ) );
  return createIndex( row, sourceIndex.column() );
}

QVariant FlatCollectionProxyModel::data(const QModelIndex & index, int role) const
{
  if ( role == Qt::DisplayRole ) {
    QModelIndex sourceIndex = mapToSource( index );
    QString name = sourceIndex.data( role ).toString();
    sourceIndex = sourceIndex.parent();
    while ( sourceIndex.isValid() ) {
      name.prepend( sourceIndex.data( role ).toString() + QLatin1String( "/" ) );
      sourceIndex = sourceIndex.parent();
    }
    return name;
  }
  return QAbstractProxyModel::data( index, role );
}

#include "flatcollectionproxymodel.moc"
