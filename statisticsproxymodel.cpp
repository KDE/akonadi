/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>


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

#include "statisticsproxymodel.h"

#include "entitytreemodel.h"
#include "collectionutils_p.h"

#include <akonadi/collectionstatistics.h>
#include <akonadi/entitydisplayattribute.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kio/global.h>

#include <QtGui/QApplication>
#include <QtGui/QPalette>

using namespace Akonadi;

/**
 * @internal
 */
class StatisticsProxyModel::Private
{
  public:
    Private( StatisticsProxyModel *parent )
      : mParent( parent )
    {
    }

    StatisticsProxyModel *mParent;
};

StatisticsProxyModel::StatisticsProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
  setSupportedDragActions( Qt::CopyAction | Qt::MoveAction );
}

StatisticsProxyModel::~StatisticsProxyModel()
{
  delete d;
}

QModelIndex Akonadi::StatisticsProxyModel::index( int row, int column, const QModelIndex & parent ) const
{
    int sourceColumn = column;

    if ( column>=columnCount( parent)-3 ) {
      sourceColumn = 0;
    }

    QModelIndex i = QSortFilterProxyModel::index( row, sourceColumn, parent );
    return createIndex( i.row(), column, i.internalPointer() );
}

QVariant StatisticsProxyModel::data( const QModelIndex & index, int role) const
{
  if ( role == Qt::DisplayRole && index.column()>=columnCount( index.parent() )-3 ) {
    const QModelIndex sourceIndex = mapToSource( index.sibling( index.row(), 0 ) );
    Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    if ( collection.isValid() && collection.statistics().count()>=0 ) {
      if ( index.column() == columnCount( QModelIndex() )-1 ) {
        return KIO::convertSize( (KIO::filesize_t)( collection.statistics().size() ) );
      } else if ( index.column() == columnCount( QModelIndex() )-2 ) {
        return collection.statistics().count();
      } else if ( index.column() == columnCount( QModelIndex() )-3 ) {
        if ( collection.statistics().unreadCount() > 0 ) {
          return collection.statistics().unreadCount();
        } else {
          return QString();
        }
      } else {
        kWarning() << "We shouldn't get there for a column which is not total, unread or size.";
        return QVariant();
      }
    }
  } else if ( role == Qt::TextAlignmentRole && index.column()>=columnCount( index.parent() )-3 ) {
    return Qt::AlignRight;
  }

  return QAbstractProxyModel::data( index, role );
}

QVariant StatisticsProxyModel::headerData( int section, Qt::Orientation orientation, int role) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    if ( section == columnCount( QModelIndex() )-1 ) {
      return i18n( "Size" );
    } else if ( section == columnCount( QModelIndex() )-2 ) {
      return i18n( "Total" );
    } else if ( section == columnCount( QModelIndex() )-3 ) {
      return i18n( "Unread" );
    }
  }

  return QSortFilterProxyModel::headerData( section, orientation, role );
}

Qt::ItemFlags StatisticsProxyModel::flags( const QModelIndex & index ) const
{
  if ( index.column()>=columnCount( index.parent() )-3 ) {
    return QSortFilterProxyModel::flags( index.sibling( index.row(), 0 ) )
         & ( Qt::ItemIsSelectable | Qt::ItemIsDragEnabled // Allowed flags
           | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled );
  }

  return QSortFilterProxyModel::flags( index );
}

int StatisticsProxyModel::columnCount( const QModelIndex & parent ) const
{
  return sourceModel()->columnCount( mapToSource( parent ) ) + 3;
}

bool StatisticsProxyModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_ASSERT(sourceModel());
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
}

QMimeData* StatisticsProxyModel::mimeData( const QModelIndexList & indexes ) const
{
  Q_ASSERT(sourceModel());
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList StatisticsProxyModel::mimeTypes() const
{
  Q_ASSERT(sourceModel());
  return sourceModel()->mimeTypes();
}

#include "statisticsproxymodel.moc"

