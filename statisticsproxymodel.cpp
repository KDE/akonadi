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
#include <klocale.h>
#include <kio/global.h>

#include <QtGui/QApplication>
#include <QtGui/QPalette>
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

    QString toolTipForCollection( const QModelIndex &index, const Collection &collection )
    {
      QString bckColor = QApplication::palette().color( QPalette::ToolTipBase ).name();
      QString txtColor = QApplication::palette().color( QPalette::ToolTipText ).name();

      QString tip = QString::fromLatin1(
        "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">\n"
        );

      tip += QString::fromLatin1(
        "  <tr>\n"
        "    <td bgcolor=\"%1\" colspan=\"2\" align=\"left\" valign=\"middle\">\n"
        "      <div style=\"color: %2; font-weight: bold;\">\n"
        "      %3\n"
        "      </div>\n"
        "    </td>\n"
        "  </tr>\n"
        ).arg( txtColor ).arg( bckColor ).arg( index.data( Qt::DisplayRole ).toString() );


      tip += QString::fromLatin1(
        "  <tr>\n"
        "    <td align=\"left\" valign=\"top\">\n"
        );

      tip += QString::fromLatin1(
        "      <strong>%1</strong>: %2<br>\n"
        "      <strong>%3</strong>: %4<br><br>\n"
        ).arg( i18n("Total Messages") ).arg( collection.statistics().count() )
         .arg( i18n("Unread Messages") ).arg( collection.statistics().unreadCount() );

      if ( collection.hasAttribute<CollectionQuotaAttribute>() ) {
        CollectionQuotaAttribute *quota = collection.attribute<CollectionQuotaAttribute>();
        if ( quota->currentValue() > -1 && quota->maximumValue() > 0 ) {
          qreal percentage = ( 100.0 * quota->currentValue() ) / quota->maximumValue();

          if ( qAbs( percentage ) >= 0.01 ) {
            QString percentStr = QString::number( percentage, 'f', 2 );
            tip += QString::fromLatin1(
              "      <strong>%1</strong>: %2%<br>\n"
              ).arg( i18n( "Quota" ) ).arg( percentStr );
          }
        }
      }

      tip += QString::fromLatin1(
        "      <strong>%1</strong>: %2<br>\n"
        ).arg( i18n("Storage Size") ).arg( KIO::convertSize( (KIO::filesize_t)( collection.statistics().size() ) ) );


      QString iconName = CollectionUtils::defaultIconName( collection );
      if ( collection.hasAttribute<EntityDisplayAttribute>() &&
           !collection.attribute<EntityDisplayAttribute>()->iconName().isEmpty() ) {
        iconName = collection.attribute<EntityDisplayAttribute>()->iconName();
      }

      int iconSizes[] = { 32, 22 };
      QString iconPath;

      for ( int i = 0; i < 2; i++ ) {
        iconPath = KIconLoader::global()->iconPath( iconName, -iconSizes[ i ], true );
        if ( !iconPath.isEmpty() )
          break;
      }

      if ( iconPath.isEmpty() ) {
        iconPath = KIconLoader::global()->iconPath( QLatin1String("folder"), -32, false );
      }

      tip += QString::fromLatin1(
        "    </td>\n"
        "    <td align=\"right\" valign=\"top\">\n"
        "      <table border=\"0\"><tr><td width=\"32\" height=\"32\" align=\"center\" valign=\"middle\">\n"
        "      <img src=\"%1\">\n"
        "      </td></tr></table>\n"
        "    </td>\n"
        "  </tr>\n"
        ).arg( iconPath );

      tip += QString::fromLatin1(
        "</table>"
        );

      return tip;
    }

    StatisticsProxyModel *mParent;

    bool mToolTipEnabled;
    bool mExtraColumnsEnabled;
};

StatisticsProxyModel::StatisticsProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
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

#include "statisticsproxymodel.moc"

