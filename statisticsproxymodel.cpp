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

#include <akonadi/collectionquotaattribute.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/entitydisplayattribute.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <klocalizedstring.h>
#include <kio/global.h>

#include <QApplication>
#include <QPalette>
#include <KIcon>
using namespace Akonadi;

/**
 * @internal
 */
class StatisticsProxyModel::Private
{
  public:
    Private( StatisticsProxyModel *parent )
      : mParent( parent ), mToolTipEnabled( false ), mExtraColumnsEnabled( true )
    {
    }

    int sourceColumnCount( const QModelIndex &parent )
    {
      return mParent->sourceModel()->columnCount( mParent->mapToSource( parent ) );
    }

    void getCountRecursive( const QModelIndex &index, qint64 &totalSize ) const
    {
      Collection collection = qvariant_cast<Collection>( index.data( EntityTreeModel::CollectionRole ) );
      // Do not assert on invalid collections, since a collection may be deleted
      // in the meantime and deleted collections are invalid.
      if ( collection.isValid() ) {
        CollectionStatistics statistics = collection.statistics();
        totalSize += qMax( 0LL, statistics.size() );
        if ( index.model()->hasChildren( index ) ) {
          const int rowCount = index.model()->rowCount( index );
          for ( int row = 0; row < rowCount; row++ ) {
            static const int column = 0;
            getCountRecursive( index.model()->index( row, column, index ),  totalSize );
          }
        }
      }
    }

    QString toolTipForCollection( const QModelIndex &index, const Collection &collection )
    {
      QString bckColor = QApplication::palette().color( QPalette::ToolTipBase ).name();
      QString txtColor = QApplication::palette().color( QPalette::ToolTipText ).name();

      QString tip = QString::fromLatin1(
        "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">\n"
        );
      const QString textDirection =  ( QApplication::layoutDirection() == Qt::LeftToRight ) ? QLatin1String( "left" ) : QLatin1String( "right" );
      tip += QString::fromLatin1(
        "  <tr>\n"
        "    <td bgcolor=\"%1\" colspan=\"2\" align=\"%4\" valign=\"middle\">\n"
        "      <div style=\"color: %2; font-weight: bold;\">\n"
        "      %3\n"
        "      </div>\n"
        "    </td>\n"
        "  </tr>\n"
        ).arg( txtColor ).arg( bckColor ).arg( index.data( Qt::DisplayRole ).toString() ).arg( textDirection );

      tip += QString::fromLatin1(
        "  <tr>\n"
        "    <td align=\"%1\" valign=\"top\">\n"
        ).arg( textDirection );

      QString tipInfo;
      tipInfo += QString::fromLatin1(
        "      <strong>%1</strong>: %2<br>\n"
        "      <strong>%3</strong>: %4<br><br>\n"
        ).arg( i18n( "Total Messages" ) ).arg( collection.statistics().count() )
         .arg( i18n( "Unread Messages" ) ).arg( collection.statistics().unreadCount() );

      if ( collection.hasAttribute<CollectionQuotaAttribute>() ) {
        CollectionQuotaAttribute *quota = collection.attribute<CollectionQuotaAttribute>();
        if ( quota->currentValue() > -1 && quota->maximumValue() > 0 ) {
          qreal percentage = ( 100.0 * quota->currentValue() ) / quota->maximumValue();

          if ( qAbs( percentage ) >= 0.01 ) {
            QString percentStr = QString::number( percentage, 'f', 2 );
            tipInfo += QString::fromLatin1(
              "      <strong>%1</strong>: %2%<br>\n"
              ).arg( i18n( "Quota" ) ).arg( percentStr );
          }
        }
      }

      qint64 currentFolderSize( collection.statistics().size() );
      tipInfo += QString::fromLatin1(
        "      <strong>%1</strong>: %2<br>\n"
        ).arg( i18n( "Storage Size" ) ).arg( KIO::convertSize( (KIO::filesize_t)( currentFolderSize ) ) );

      qint64 totalSize = 0;
      getCountRecursive( index, totalSize );
      totalSize -= currentFolderSize;
      if (totalSize > 0 ) {
        tipInfo += QString::fromLatin1(
          "<strong>%1</strong>: %2<br>"
          ).arg( i18n("Subfolder Storage Size") ).arg( KIO::convertSize( (KIO::filesize_t)( totalSize ) ) );
      }

     QString iconName = CollectionUtils::defaultIconName( collection );
      if ( collection.hasAttribute<EntityDisplayAttribute>() &&
         !collection.attribute<EntityDisplayAttribute>()->iconName().isEmpty() ) {
         if ( !collection.attribute<EntityDisplayAttribute>()->activeIconName().isEmpty() && collection.statistics().unreadCount()> 0) {
           iconName = collection.attribute<EntityDisplayAttribute>()->activeIconName();
         }
         else
           iconName = collection.attribute<EntityDisplayAttribute>()->iconName();
      }

      int iconSizes[] = { 32, 22 };
      int icon_size_found = 32;

      QString iconPath;

      for ( int i = 0; i < 2; i++ ) {
        iconPath = KIconLoader::global()->iconPath( iconName, -iconSizes[ i ], true );
        if ( !iconPath.isEmpty() ) {
          icon_size_found = iconSizes[ i ];
          break;
        }
      }

      if ( iconPath.isEmpty() ) {
        iconPath = KIconLoader::global()->iconPath( QLatin1String( "folder" ), -32, false );
      }

      QString tipIcon = QString::fromLatin1(
        "      <table border=\"0\"><tr><td width=\"32\" height=\"32\" align=\"center\" valign=\"middle\">\n"
        "      <img src=\"%1\" width=\"%2\" height=\"32\">\n"
        "      </td></tr></table>\n"
        "    </td>\n"
        ).arg( iconPath ).arg( icon_size_found ) ;

      if ( QApplication::layoutDirection() == Qt::LeftToRight )
      {
        tip += tipInfo + QString::fromLatin1( "</td><td align=\"%3\" valign=\"top\">" ).arg( textDirection ) + tipIcon;
      }
      else
      {
        tip += tipIcon + QString::fromLatin1( "</td><td align=\"%3\" valign=\"top\">" ).arg( textDirection ) + tipInfo;
      }

      tip += QString::fromLatin1(
        "  </tr>" \
        "</table>"
        );

      return tip;
    }

    void proxyDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void sourceLayoutAboutToBeChanged();
    void sourceLayoutChanged();

    QVector<QModelIndex> m_nonPersistent;
    QVector<QModelIndex> m_nonPersistentFirstColumn;
    QVector<QPersistentModelIndex> m_persistent;
    QVector<QPersistentModelIndex> m_persistentFirstColumn;

    StatisticsProxyModel *mParent;

    bool mToolTipEnabled;
    bool mExtraColumnsEnabled;
};

void StatisticsProxyModel::Private::proxyDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  if ( mExtraColumnsEnabled )
  {
    // Ugly hack.
    // The proper solution is a KExtraColumnsProxyModel, but this will do for now.
    QModelIndex parent = topLeft.parent();
    int parentColumnCount = mParent->columnCount( parent );
    QModelIndex extraTopLeft = mParent->index( topLeft.row(), parentColumnCount - 1 - 3 , parent );
    QModelIndex extraBottomRight = mParent->index( bottomRight.row(), parentColumnCount -1, parent );
    mParent->disconnect( mParent, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                         mParent, SLOT(proxyDataChanged(QModelIndex,QModelIndex)) );
    emit mParent->dataChanged( extraTopLeft, extraBottomRight );

    // We get this signal when the statistics of a row changes.
    // However, we need to emit data changed for the statistics of all ancestor rows too
    // so that recursive totals can be updated.
    while ( parent.isValid() )
    {
      emit mParent->dataChanged( parent.sibling( parent.row(), parentColumnCount - 1 - 3 ),
                                 parent.sibling( parent.row(), parentColumnCount - 1 ) );
      parent = parent.parent();
      parentColumnCount = mParent->columnCount( parent );
    }
    mParent->connect( mParent, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                      SLOT(proxyDataChanged(QModelIndex,QModelIndex)) );
  }
}

void StatisticsProxyModel::Private::sourceLayoutAboutToBeChanged()
{
  QModelIndexList persistent = mParent->persistentIndexList();
  const int columnCount = mParent->sourceModel()->columnCount();
  foreach( const QModelIndex &idx, persistent ) {
    if ( idx.column() >= columnCount ) {
      m_nonPersistent.push_back( idx );
      m_persistent.push_back( idx );
      const QModelIndex firstColumn = idx.sibling( 0, idx.column() );
      m_nonPersistentFirstColumn.push_back( firstColumn );
      m_persistentFirstColumn.push_back( firstColumn );
    }
  }
}

void StatisticsProxyModel::Private::sourceLayoutChanged()
{
  QModelIndexList oldList;
  QModelIndexList newList;

  const int columnCount = mParent->sourceModel()->columnCount();

  for ( int i = 0; i < m_persistent.size(); ++i ) {
    const QModelIndex persistentIdx = m_persistent.at( i );
    const QModelIndex nonPersistentIdx = m_nonPersistent.at( i );
    if ( m_persistentFirstColumn.at( i ) != m_nonPersistentFirstColumn.at( i ) && persistentIdx.column() >= columnCount ) {
      oldList.append( nonPersistentIdx );
      newList.append( persistentIdx );
    }
  }
  mParent->changePersistentIndexList( oldList, newList );
}

void StatisticsProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
  // Order is important here. sourceLayoutChanged must be called *before* any downstreams react
  // to the layoutChanged so that it can have the QPersistentModelIndexes uptodate in time.
  disconnect(this, SIGNAL(layoutChanged()), this, SLOT(sourceLayoutChanged()));
  connect(this, SIGNAL(layoutChanged()), SLOT(sourceLayoutChanged()));
  QSortFilterProxyModel::setSourceModel(sourceModel);
  // This one should come *after* any downstream handlers of layoutAboutToBeChanged.
  // The connectNotify stuff below ensures that it remains the last one.
  disconnect(this, SIGNAL(layoutAboutToBeChanged()), this, SLOT(sourceLayoutAboutToBeChanged()));
  connect(this, SIGNAL(layoutAboutToBeChanged()), SLOT(sourceLayoutAboutToBeChanged()));
}

void StatisticsProxyModel::connectNotify(const char *signal)
{
  static bool ignore = false;
  if (ignore || QLatin1String(signal) == SIGNAL(layoutAboutToBeChanged()))
    return QSortFilterProxyModel::connectNotify(signal);
  ignore = true;
  disconnect(this, SIGNAL(layoutAboutToBeChanged()), this, SLOT(sourceLayoutAboutToBeChanged()));
  connect(this, SIGNAL(layoutAboutToBeChanged()), SLOT(sourceLayoutAboutToBeChanged()));
  ignore = false;
  QSortFilterProxyModel::connectNotify(signal);
}

StatisticsProxyModel::StatisticsProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
  connect( this, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
           SLOT(proxyDataChanged(QModelIndex,QModelIndex)) );
}

StatisticsProxyModel::~StatisticsProxyModel()
{
  delete d;
}

void StatisticsProxyModel::setToolTipEnabled( bool enable )
{
  d->mToolTipEnabled = enable;
}

bool StatisticsProxyModel::isToolTipEnabled() const
{
  return d->mToolTipEnabled;
}

void StatisticsProxyModel::setExtraColumnsEnabled( bool enable )
{
  d->mExtraColumnsEnabled = enable;
}

bool StatisticsProxyModel::isExtraColumnsEnabled() const
{
  return d->mExtraColumnsEnabled;
}

QModelIndex Akonadi::StatisticsProxyModel::index( int row, int column, const QModelIndex & parent ) const
{
    if (!hasIndex(row, column, parent))
      return QModelIndex();

    int sourceColumn = column;

    if ( column>=d->sourceColumnCount( parent ) ) {
      sourceColumn = 0;
    }

    QModelIndex i = QSortFilterProxyModel::index( row, sourceColumn, parent );
    return createIndex( i.row(), column, i.internalPointer() );
}

QVariant StatisticsProxyModel::data( const QModelIndex & index, int role) const
{
  if (!sourceModel())
    return QVariant();
  if ( role == Qt::DisplayRole && index.column()>=d->sourceColumnCount( index.parent() ) ) {
    const QModelIndex sourceIndex = mapToSource( index.sibling( index.row(), 0 ) );
    Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    if ( collection.isValid() && collection.statistics().count()>=0 ) {
      if ( index.column() == d->sourceColumnCount( QModelIndex() )+2 ) {
        return KIO::convertSize( (KIO::filesize_t)( collection.statistics().size() ) );
      } else if ( index.column() == d->sourceColumnCount( QModelIndex() )+1 ) {
        return collection.statistics().count();
      } else if ( index.column() == d->sourceColumnCount( QModelIndex() ) ) {
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

  } else if ( role == Qt::TextAlignmentRole && index.column()>=d->sourceColumnCount( index.parent() ) ) {
    return Qt::AlignRight;

  } else if ( role == Qt::ToolTipRole && d->mToolTipEnabled ) {
    const QModelIndex sourceIndex = mapToSource( index.sibling( index.row(), 0 ) );
    Collection collection
      = sourceModel()->data( sourceIndex,
                             EntityTreeModel::CollectionRole ).value<Collection>();

    if ( collection.isValid() && collection.statistics().count()>0 ) {
      return d->toolTipForCollection( index, collection );
    }

  } else if ( role == Qt::DecorationRole && index.column() == 0 ) {
    const QModelIndex sourceIndex = mapToSource( index.sibling( index.row(), 0 ) );
    Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    if ( collection.isValid() )
      return KIcon(  CollectionUtils::displayIconName( collection ) );
    else
      return QVariant();
  }

  return QAbstractProxyModel::data( index, role );
}

QVariant StatisticsProxyModel::headerData( int section, Qt::Orientation orientation, int role) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    if ( section == d->sourceColumnCount( QModelIndex() ) + 2 ) {
      return i18nc( "collection size", "Size" );
    } else if ( section == d->sourceColumnCount( QModelIndex() ) + 1 ) {
      return i18nc( "number of entities in the collection", "Total" );
    } else if ( section == d->sourceColumnCount( QModelIndex() ) ) {
      return i18nc( "number of unread entities in the collection", "Unread" );
    }
  }

  return QSortFilterProxyModel::headerData( section, orientation, role );
}

Qt::ItemFlags StatisticsProxyModel::flags( const QModelIndex & index ) const
{
  if ( index.column()>=d->sourceColumnCount( index.parent() ) ) {
    return QSortFilterProxyModel::flags( index.sibling( index.row(), 0 ) )
         & ( Qt::ItemIsSelectable | Qt::ItemIsDragEnabled // Allowed flags
           | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled );
  }

  return QSortFilterProxyModel::flags( index );
}

int StatisticsProxyModel::columnCount( const QModelIndex & parent ) const
{
  if ( sourceModel()==0 ) {
    return 0;
  } else {
    return d->sourceColumnCount( parent )
         + ( d->mExtraColumnsEnabled ? 3 : 0 );
  }
}

QModelIndexList StatisticsProxyModel::match( const QModelIndex& start, int role, const QVariant& value,
                                             int hits, Qt::MatchFlags flags ) const
{
  if ( role < Qt::UserRole )
    return QSortFilterProxyModel::match( start, role, value, hits, flags );

  QModelIndexList list;
  QModelIndex proxyIndex;
  foreach ( const QModelIndex &idx, sourceModel()->match( mapToSource( start ), role, value, hits, flags ) ) {
    proxyIndex = mapFromSource( idx );
    if ( proxyIndex.isValid() )
      list << proxyIndex;
  }

  return list;
}

#include "moc_statisticsproxymodel.cpp"

