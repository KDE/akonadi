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

#include "statisticstooltipproxymodel.h"

#include "entitytreemodel.h"
#include "collectionutils_p.h"

#include <akonadi/collectionstatistics.h>
#include <akonadi/entitydisplayattribute.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kio/global.h>

#include <QtGui/QApplication>
#include <QtGui/QPalette>

using namespace Akonadi;

/**
 * @internal
 */
class StatisticsToolTipProxyModel::Private
{
  public:
    Private( StatisticsToolTipProxyModel *parent )
      : mParent( parent )
    {
    }

    StatisticsToolTipProxyModel *mParent;
};

StatisticsToolTipProxyModel::StatisticsToolTipProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
  setSupportedDragActions( Qt::CopyAction | Qt::MoveAction );
}

StatisticsToolTipProxyModel::~StatisticsToolTipProxyModel()
{
  delete d;
}

QVariant StatisticsToolTipProxyModel::data( const QModelIndex & index, int role) const
{
  if ( role == Qt::ToolTipRole ) {
    const QModelIndex sourceIndex = mapToSource( index );
    Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    if ( collection.isValid() && collection.statistics().count()>0 ) {

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
        ).arg( txtColor ).arg( bckColor ).arg( data( index, Qt::DisplayRole ).toString() );


      tip += QString::fromLatin1(
        "  <tr>\n"
        "    <td align=\"left\" valign=\"top\">\n"
        );

      tip += QString::fromLatin1(
        "      <strong>%1</strong>: %2<br>\n"
        "      <strong>%3</strong>: %4<br><br>\n"
        ).arg( i18n("Total Messages") ).arg( collection.statistics().count() )
         .arg( i18n("Unread Messages") ).arg( collection.statistics().unreadCount() );

      //TODO: Quota missing
      //tip += QString::fromLatin1(
      //  "      <strong>%1</strong>: %2<br>\n"
      //  ).arg( i18n( "Quota" ) ).arg( "0" );

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
        iconPath = KIconLoader::global()->iconPath( "folder", -32, false );
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
  }

  return QAbstractProxyModel::data( index, role );
}

bool StatisticsToolTipProxyModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_ASSERT(sourceModel());
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
}

QMimeData* StatisticsToolTipProxyModel::mimeData( const QModelIndexList & indexes ) const
{
  Q_ASSERT(sourceModel());
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList StatisticsToolTipProxyModel::mimeTypes() const
{
  Q_ASSERT(sourceModel());
  return sourceModel()->mimeTypes();
}

#include "statisticstooltipproxymodel.moc"

