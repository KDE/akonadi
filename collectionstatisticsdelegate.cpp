/*
    Copyright (c) 2008 Thomas McGuire <thomas.mcguire@gmx.net>

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
#include "collectionstatisticsdelegate.h"

#include "collectionstatisticsmodel.h"

#include <kcolorscheme.h>
#include <kdebug.h>

#include <QPainter>
#include <QTreeView>

using namespace Akonadi;

namespace Akonadi {

class CollectionStatisticsDelegatePrivate
{
  public:
    QModelIndex index;
    QTreeView *parent;
    bool drawUnreadAfterFolder;

    CollectionStatisticsDelegatePrivate( QTreeView *treeView )
        : parent( treeView ), drawUnreadAfterFolder( false )
    {
    }
};

}

CollectionStatisticsDelegate::CollectionStatisticsDelegate( QTreeView *parent )
  : QItemDelegate( parent ),
    d_ptr( new CollectionStatisticsDelegatePrivate( parent ) )
{
}

CollectionStatisticsDelegate::~CollectionStatisticsDelegate()
{
  delete d_ptr;
  d_ptr = 0;
}

void CollectionStatisticsDelegate::toggleUnreadAfterFolderName( bool enable )
{
  Q_D( CollectionStatisticsDelegate );
  d->drawUnreadAfterFolder = enable;
}

void CollectionStatisticsDelegate::paint( QPainter *painter,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index ) const
{
  d_ptr->index = index;
  QItemDelegate::paint( painter, option, index );
}

void CollectionStatisticsDelegate::drawDisplay( QPainter *painter,
                                                const QStyleOptionViewItem &option,
                                                const QRect &rect,
                                                const QString &text) const
{
  Q_D( const CollectionStatisticsDelegate );

  // When checking if the item is expanded, we need to check that for the first
  // column, as Qt only recogises the index as expanded for the first column
  QModelIndex firstColumn = d->index.model()->index( d->index.row(), 0,
                                                     d->index.parent() );
  bool expanded = d->parent->isExpanded( firstColumn );

  // Draw the unread count after the folder name (in parenthesis)
  if ( d->drawUnreadAfterFolder && d->index.column() == 0 ) {

    QVariant unreadCount = d->index.model()->data( d->index,
                           CollectionStatisticsModel::CollectionStatisticsUnreadRole );
    QVariant unreadRecursiveCount = d->index.model()->data( d->index,
                           CollectionStatisticsModel::CollectionStatisticsUnreadRecursiveRole );
    Q_ASSERT( unreadCount.type() == QVariant::LongLong );
    Q_ASSERT( unreadRecursiveCount.type() == QVariant::LongLong );

    // Construct the string which will appear after the foldername (with the
    // unread count)
    QString unread;
    QString unreadCountInChilds = QString::number( unreadRecursiveCount.toLongLong() -
                                                   unreadCount.toLongLong() );
    if ( expanded && unreadCount.toLongLong() > 0 )
      unread = QString( QLatin1String(" (%1)") ).arg( unreadCount.toLongLong() );
    else if ( !expanded ) {
      if ( unreadCount.toLongLong() != unreadRecursiveCount.toLongLong() )
        unread = QString( QLatin1String(" (%1 + %2)") ).arg( unreadCount.toString(),
                                                             unreadCountInChilds );
      else if ( unreadCount.toLongLong() > 0 )
        unread = QString( QLatin1String(" (%1)") ).arg( unreadCount.toString() );
    }

    painter->save();
    QItemDelegate::drawDisplay( painter, option, rect, QString() );

    if ( !unread.isEmpty() ) {
      QFont font = painter->font();
      font.setBold( true );
      painter->setFont( font );
    }

    // Squeeze the folder text if it is to big and calculate the rectangles
    // where the folder text and the unread count will be drawn to
    QString folderName = text;
    QFontMetrics fm( painter->fontMetrics() );
    int unreadWidth = fm.width( unread );
    if ( fm.width( folderName ) + unreadWidth > rect.width() ) {
      folderName = fm.elidedText( folderName, Qt::ElideRight,
                                  rect.width() - unreadWidth );
    }
    int folderWidth = fm.width( folderName );
    QRect folderRect = rect;
    QRect unreadRect = rect;
    folderRect.setRight( rect.left() + folderWidth );
    unreadRect.setLeft( folderRect.right() );

    // Draw folder name and unread count
    painter->drawText( folderRect, Qt::AlignLeft, folderName );
    KColorScheme::ColorSet cs = ( option.state & QStyle::State_Selected ) ?
                                 KColorScheme::Selection : KColorScheme::View;
    QColor unreadColor = KColorScheme( QPalette::Active, cs ).
                                   foreground( KColorScheme::LinkText ).color();
    painter->setPen( unreadColor );
    painter->drawText( unreadRect, Qt::AlignLeft, unread );
    painter->restore();
    return;
  }

  // For the unread/total column, paint the summed up count if the item
  // is collapsed
  if ( ( d->index.column() == 1 || d->index.column() == 2 ) ) {

    painter->save();

    int role;
    if ( d->index.column() == 1 ) {
      if ( !expanded )
        role = CollectionStatisticsModel::CollectionStatisticsUnreadRecursiveRole;
      else
        role = CollectionStatisticsModel::CollectionStatisticsUnreadRole;
    }
    else if ( d->index.column() == 2 ) {
      if ( !expanded )
        role = CollectionStatisticsModel::CollectionStatisticsTotalRecursiveRole;
      else
        role = CollectionStatisticsModel::CollectionStatisticsTotalRole;
    }

    QVariant sum = d->index.model()->data( d->index, role );
    Q_ASSERT( sum.type() == QVariant::LongLong );
    QStyleOptionViewItem opt = option;
    if ( d->index.column() == 1 && sum.toLongLong() > 0 ) {
      opt.font.setBold( true );
    }
    QString sumText;
    if ( sum.toLongLong() <= 0 )
      sumText = text;
    else
      sumText = sum.toString();

    QItemDelegate::drawDisplay( painter, opt, rect, sumText );
    painter->restore();
    return;
  }

  QItemDelegate::drawDisplay( painter, option, rect, text );
}
