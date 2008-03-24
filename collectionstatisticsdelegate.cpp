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

#if QT_VERSION >= 0x040400

#include "collectionstatisticsmodel.h"

#include <kcolorscheme.h>
#include <kdebug.h>

#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QStyleOptionViewItemV4>
#include <QTreeView>

using namespace Akonadi;

namespace Akonadi {

class CollectionStatisticsDelegatePrivate
{
  public:
    QTreeView *parent;
    bool drawUnreadAfterFolder;

    CollectionStatisticsDelegatePrivate( QTreeView *treeView )
        : parent( treeView ), drawUnreadAfterFolder( false )
    {
    }
};

}

CollectionStatisticsDelegate::CollectionStatisticsDelegate( QTreeView *parent )
  : QStyledItemDelegate( parent ),
    d_ptr( new CollectionStatisticsDelegatePrivate( parent ) )
{
}

CollectionStatisticsDelegate::~CollectionStatisticsDelegate()
{
  delete d_ptr;
}

void CollectionStatisticsDelegate::toggleUnreadAfterFolderName( bool enable )
{
  Q_D( CollectionStatisticsDelegate );
  d->drawUnreadAfterFolder = enable;
}

void CollectionStatisticsDelegate::initStyleOption( QStyleOptionViewItem *option,
                                                    const QModelIndex &index ) const
{
  QStyleOptionViewItemV4 *noTextOption =
      qstyleoption_cast<QStyleOptionViewItemV4 *>( option );
  QStyledItemDelegate::initStyleOption( noTextOption, index );
  noTextOption->text = QString();
}

void CollectionStatisticsDelegate::paint( QPainter *painter,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index ) const
{
  Q_D( const CollectionStatisticsDelegate );

  // First, paint the basic, but without the text. We remove the text
  // in initStyleOption(), which gets called by QStyledItemDelegate::paint().
  QStyledItemDelegate::paint( painter, option, index );

  // No, we retrieve the correct style option by calling intiStyleOption from
  // the superclass.
  QStyleOptionViewItemV4 option4 = option;
  QStyledItemDelegate::initStyleOption( &option4, index );
  QString text = option4.text;

  // Now calculate the rectangle for the text
  QStyle *s = d->parent->style();
  const QWidget *widget = option4.widget;
  QRect textRect = s->subElementRect( QStyle::SE_ItemViewItemText, &option4, widget );

   // When checking if the item is expanded, we need to check that for the first
  // column, as Qt only recogises the index as expanded for the first column
  QModelIndex firstColumn = index.model()->index( index.row(), 0, index.parent() );
  bool expanded = d->parent->isExpanded( firstColumn );

  // Draw the unread count after the folder name (in parenthesis)
  if ( d->drawUnreadAfterFolder && index.column() == 0 ) {

    QVariant unreadCount = index.model()->data( index,
                           CollectionStatisticsModel::UnreadRole );
    QVariant unreadRecursiveCount = index.model()->data( index,
                           CollectionStatisticsModel::RecursiveUnreadRole );
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
    if ( fm.width( folderName ) + unreadWidth > textRect.width() ) {
      folderName = fm.elidedText( folderName, Qt::ElideRight,
                                  textRect.width() - unreadWidth );
    }
    int folderWidth = fm.width( folderName );
    QRect folderRect = textRect;
    QRect unreadRect = textRect;
    folderRect.setRight( textRect.left() + folderWidth );
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
  if ( ( index.column() == 1 || index.column() == 2 ) ) {

    painter->save();

    int role = 0;
    if ( index.column() == 1 ) {
      if ( !expanded )
        role = CollectionStatisticsModel::RecursiveUnreadRole;
      else
        role = CollectionStatisticsModel::UnreadRole;
    }
    else if ( index.column() == 2 ) {
      if ( !expanded )
        role = CollectionStatisticsModel::RecursiveTotalRole;
      else
        role = CollectionStatisticsModel::TotalRole;
    }

    QVariant sum = index.model()->data( index, role );
    Q_ASSERT( sum.type() == QVariant::LongLong );
    QStyleOptionViewItem opt = option;
    if ( index.column() == 1 && sum.toLongLong() > 0 ) {
      QFont font = painter->font();
      font.setBold( true );
      painter->setFont( font );
    }
    QString sumText;
    if ( sum.toLongLong() <= 0 )
      sumText = text;
    else
      sumText = sum.toString();

    painter->drawText( textRect, Qt::AlignRight, sumText );
    painter->restore();
    return;
  }

  painter->drawText( textRect, option4.displayAlignment, text );
}

#endif
