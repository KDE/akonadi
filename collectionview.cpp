/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collectionview.h"

#include "collection.h"
#include "collectionmodel.h"

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kxmlguifactory.h>
#include <kxmlguiwindow.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QSortFilterProxyModel>

using namespace Akonadi;

class CollectionView::Private
{
  public:
    Private( CollectionView *parent )
      : mParent( parent ),
        xmlGuiWindow( 0 )
    {
    }

    void dragExpand();
    void itemClicked( const QModelIndex& );
    void itemCurrentChanged( const QModelIndex& );
    bool hasParent( const QModelIndex& idx, int parentId );

    CollectionView *mParent;
    QSortFilterProxyModel *filterModel;
    QModelIndex dragOverIndex;
    QTimer dragExpandTimer;

    KXmlGuiWindow *xmlGuiWindow;
};

bool CollectionView::Private::hasParent( const QModelIndex& idx, int parentId )
{
  QModelIndex idx2 = idx;
  while( idx2.isValid() ) {
    if ( mParent->model()->data( idx2, CollectionModel::CollectionIdRole).toInt() == parentId )
      return true;

    idx2 = idx2.parent();
  }
  return false;
}

void CollectionView::Private::dragExpand()
{
  mParent->setExpanded( dragOverIndex, true );
  dragOverIndex = QModelIndex();
}

void CollectionView::Private::itemClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  int currentCollection = index.model()->data( index, CollectionModel::CollectionIdRole ).toInt();
  if ( currentCollection <= 0 )
    return;

  emit mParent->clicked( Collection( currentCollection ) );
}

void CollectionView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  int currentCollection = index.model()->data( index, CollectionModel::CollectionIdRole ).toInt();
  if ( currentCollection <= 0 )
    return;

  emit mParent->currentChanged( Collection( currentCollection ) );
}

CollectionView::CollectionView( KXmlGuiWindow *xmlGuiWindow, QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  d->xmlGuiWindow = xmlGuiWindow;
  d->filterModel = new QSortFilterProxyModel( this );
  d->filterModel->setDynamicSortFilter( true );
  d->filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );

  header()->setClickable( true );
  header()->setStretchLastSection( false );

  setSortingEnabled( true );
  setEditTriggers( QAbstractItemView::EditKeyPressed );
  setAcceptDrops( true );
  setDropIndicatorShown( true );
  setDragDropMode( DragDrop );
  setDragEnabled( true );

  d->dragExpandTimer.setSingleShot( true );
  connect( &d->dragExpandTimer, SIGNAL(timeout()), SLOT(dragExpand()) );

  connect( this, SIGNAL( clicked( const QModelIndex& ) ),
           this, SLOT( itemClicked( const QModelIndex& ) ) );
}

CollectionView::~CollectionView()
{
  delete d;
}

void CollectionView::setModel( QAbstractItemModel * model )
{
  d->filterModel->setSourceModel( model );
  QTreeView::setModel( d->filterModel );
  header()->setResizeMode( 0, QHeaderView::Stretch );

  connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( itemCurrentChanged( const QModelIndex& ) ) );
}

void CollectionView::dragMoveEvent(QDragMoveEvent * event)
{
  QModelIndex index = indexAt( event->pos() );
  if ( d->dragOverIndex != index ) {
    d->dragExpandTimer.stop();
    if ( index.isValid() && !isExpanded( index ) && itemsExpandable() ) {
      d->dragExpandTimer.start( QApplication::startDragTime() );
      d->dragOverIndex = index;
    }
  }

  // Check if the collection under the cursor accepts this data type
  QStringList supportedContentTypes = model()->data( index, CollectionModel::CollectionContentTypesRole ).toStringList();
  const QMimeData *data = event->mimeData();
  KUrl::List urls = KUrl::List::fromMimeData( data );
  foreach( KUrl url, urls ) {

    if ( Collection::urlIsValid( url ) )
    {
      if ( !supportedContentTypes.contains( QString::fromLatin1( "inode/directory" ) ) )
        break;

      // Check if we don't try to drop on one of the children
      Collection col = Collection::fromUrl( url );
      if ( d->hasParent( index, col.id() ) )
        break;
    }
    else
    {
      QString type = url.queryItems()[ QString::fromLatin1("type") ];
      if ( !supportedContentTypes.contains( type ) )
        break;
    }

    QTreeView::dragMoveEvent( event );
    return;
  }

  event->setDropAction( Qt::IgnoreAction );
  return;
}

void CollectionView::dragLeaveEvent(QDragLeaveEvent * event)
{
  d->dragExpandTimer.stop();
  d->dragOverIndex = QModelIndex();
  QTreeView::dragLeaveEvent( event );
}


void CollectionView::dropEvent(QDropEvent * event)
{
  d->dragExpandTimer.stop();
  d->dragOverIndex = QModelIndex();

  // open a context menu offering different drop actions (move, copy and cancel)
  // TODO If possible, hide non available actions ...
  QMenu popup( this );
  QAction* moveDropAction = popup.addAction( KIcon( QString::fromLatin1("goto-page") ), i18n("&Move here") );
  QAction* copyDropAction = popup.addAction( KIcon( QString::fromLatin1("edit-copy") ), i18n("&Copy here") );
  popup.addSeparator();
  popup.addAction( KIcon( QString::fromLatin1("process-stop") ), i18n("Cancel"));

  QAction *activatedAction = popup.exec( QCursor::pos() );
  if (activatedAction == moveDropAction) {
    event->setDropAction( Qt::MoveAction );
  }
  else if (activatedAction == copyDropAction) {
    event->setDropAction( Qt::CopyAction );
  }
  else return;

  QTreeView::dropEvent( event );
}

void CollectionView::contextMenuEvent(QContextMenuEvent * event)
{
  if ( !d->xmlGuiWindow )
    return;
  QMenu *popup = static_cast<QMenu*>( d->xmlGuiWindow->guiFactory()->container(
                                      QLatin1String("akonadi_collectionview_contextmenu"), d->xmlGuiWindow ) );
  if ( popup )
    popup->exec( event->globalPos() );
}

#include "collectionview.moc"
