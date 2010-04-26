
#include <QtCore/QDebug>

#include <akonadi/item.h>
#include <akonadi/entitytreemodel.h>
#include <kabc/addressee.h>

#include "leafextensionproxymodel.h"

LeafExtensionProxyModel::LeafExtensionProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent )
{
}

LeafExtensionProxyModel::~LeafExtensionProxyModel()
{
}

QModelIndex LeafExtensionProxyModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( parent.isValid() ) {
    const QModelIndex sourceParent = mapToSource( parent );
    const QModelIndex sourceIndex = sourceModel()->index( row, column, sourceParent );
    if ( !sourceIndex.isValid() ) {
      int parentOffset = mParentIndexes.indexOf( parent );
      if ( parentOffset == -1 ) {
        mParentIndexes.append( parent );
        parentOffset = mParentIndexes.count() - 1;
      }

      qDebug() << "count" << mParentIndexes.count() << mOwnIndexes.count();
      const QModelIndex index = createIndex( row, column, parentOffset );
      mOwnIndexes.insert( index );
      return index;
    }
  }

  return QSortFilterProxyModel::index( row, column, parent );
}

QModelIndex LeafExtensionProxyModel::parent( const QModelIndex &index ) const
{
  if ( mOwnIndexes.contains( index ) )
    return mParentIndexes.at( index.internalId() );

  return QSortFilterProxyModel::parent( index );
}

int LeafExtensionProxyModel::rowCount( const QModelIndex &index ) const
{
  const QModelIndex sourceIndex = mapToSource( index );
  if ( sourceModel()->rowCount( sourceIndex ) == 0 ) {
    const Akonadi::Item item = sourceIndex.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee contact = item.payload<KABC::Addressee>();
      return contact.emails().count();
    }
    return 0;
  }

  return QSortFilterProxyModel::rowCount( index );
}

int LeafExtensionProxyModel::columnCount( const QModelIndex &index ) const
{
  return QSortFilterProxyModel::columnCount( index );
}

QVariant LeafExtensionProxyModel::data( const QModelIndex &index, int role ) const
{
  if ( mOwnIndexes.contains( index ) ) {
    if ( role == Qt::DisplayRole ) {
      const Akonadi::Item item = index.parent().data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();
      if ( item.hasPayload<KABC::Addressee>() ) {
        const KABC::Addressee contact = item.payload<KABC::Addressee>();
        return contact.emails().at( index.row() );
      } else {
        return QLatin1String( "unknown" );
      }
    }

    return QVariant();
  }

  return QSortFilterProxyModel::data( index, role );
}

Qt::ItemFlags LeafExtensionProxyModel::flags( const QModelIndex &index ) const
{
  if ( mOwnIndexes.contains( index ) )
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;

  return QSortFilterProxyModel::flags( index );
}

bool LeafExtensionProxyModel::setData( const QModelIndex &index, const QVariant &data, int role )
{
  if ( mOwnIndexes.contains( index ) )
    return false;

  return QSortFilterProxyModel::setData( index, data, role );
}

bool LeafExtensionProxyModel::hasChildren( const QModelIndex &parent ) const
{
  if ( mOwnIndexes.contains( parent ) )
    return false; // extensible in the future?

  const QModelIndex sourceParent = mapToSource( parent );
  if ( sourceModel()->rowCount( sourceParent ) == 0 )
    return true; // fake virtual children here for leaf nodes

  return QSortFilterProxyModel::hasChildren( parent );
}

QModelIndex LeafExtensionProxyModel::buddy( const QModelIndex &index ) const
{
  if ( mOwnIndexes.contains( index ) )
    return index;

  return QSortFilterProxyModel::buddy( index );
}

void LeafExtensionProxyModel::fetchMore( const QModelIndex &index )
{
  if ( mOwnIndexes.contains( index ) )
    return;

  QSortFilterProxyModel::fetchMore( index );
}

/*
void LeafExtensionProxyModel::setSourceModel( QAbstractItemModel *_sourceModel )
{
  if ( _sourceModel == sourceModel() )
    return;

  beginResetModel();
  if ( _sourceModel ) {
    disconnect(_sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            this, SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    disconnect(_sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            this, SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    disconnect(_sourceModel, SIGNAL(modelAboutToBeReset()),
            this, SLOT(sourceModelAboutToBeReset()));
    disconnect(_sourceModel, SIGNAL(modelReset()),
            this, SLOT(sourceModelReset()));
    disconnect(_sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex & )));
    disconnect(_sourceModel, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
            this, SLOT(sourceHeaderDataChanged(Qt::Orientation, int, int)));
    disconnect(_sourceModel, SIGNAL(layoutAboutToBeChanged()),
            this, SLOT(sourceLayoutAboutToBeChanged()));
    disconnect(_sourceModel, SIGNAL(layoutChanged()),
            this, SLOT(sourceLayoutChanged()));
    disconnect(_sourceModel, SIGNAL(destroyed()),
            this, SLOT(sourceModelDestroyed()));
  }
  QAbstractProxyModel::setSourceModel( _sourceModel );

  connect(_sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
          SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
  connect(_sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
  connect(_sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
  connect(_sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
  connect(_sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
          SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  connect(_sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
          SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  connect(_sourceModel, SIGNAL(modelAboutToBeReset()),
          SLOT(sourceModelAboutToBeReset()));
  connect(_sourceModel, SIGNAL(modelReset()),
          SLOT(sourceModelReset()));
  connect(_sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex & )));
  connect(_sourceModel, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
          SLOT(sourceHeaderDataChanged(Qt::Orientation, int, int)));
  connect(_sourceModel, SIGNAL(layoutAboutToBeChanged()),
          SLOT(sourceLayoutAboutToBeChanged()));
  connect(_sourceModel, SIGNAL(layoutChanged()),
          SLOT(sourceLayoutChanged()));
  connect(_sourceModel, SIGNAL(destroyed()),
          SLOT(sourceModelDestroyed()));

  endResetModel();
}

void LeafExtensionProxyModel::sourceRowsAboutToBeInserted( const QModelIndex &index, int start, int end )
{
  beginInsertRows( index, start, end );
}

void LeafExtensionProxyModel::sourceRowsInserted( const QModelIndex &index, int start, int end )
{
  endInsertRows();
}

void LeafExtensionProxyModel::sourceRowsAboutToBeRemoved( const QModelIndex &index, int start, int end )
{
  beginRemoveRows( index, start, end );
}

void LeafExtensionProxyModel::sourceRowsRemoved( const QModelIndex &index, int start, int end )
{
  endRemoveRows();
}

void LeafExtensionProxyModel::sourceRowsAboutToBeMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow )
{
  beginMoveRows( sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow );
}

void LeafExtensionProxyModel::sourceRowsMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow )
{
  endMoveRows();
}

void LeafExtensionProxyModel::sourceModelAboutToBeReset()
{
  beginResetModel();
}

void LeafExtensionProxyModel::sourceModelReset()
{
  endResetModel();
}

void LeafExtensionProxyModel::sourceDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
  emit dataChanged( topLeft, bottomRight );
}

void LeafExtensionProxyModel::sourceHeaderDataChanged( Qt::Orientation orientation, int first, int last )
{
  emit headerDataChanged( orientation, first, last );
}

void LeafExtensionProxyModel::sourceLayoutAboutToBeChanged()
{
  emit layoutAboutToBeChanged();
}

void LeafExtensionProxyModel::sourceLayoutChanged()
{
  emit layoutChanged();
}

void LeafExtensionProxyModel::sourceModelDestroyed()
{
}
*/

#include "leafextensionproxymodel.moc"
