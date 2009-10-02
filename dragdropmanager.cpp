/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "dragdropmanager_p.h"

#include <QtGui/QDropEvent>
#include <QtGui/QMenu>

#include <KDE/KUrl>
#include <KDE/KIcon>
#include <KDE/KLocale>

#include "akonadi/collection.h"
#include "akonadi/entitytreemodel.h"

using namespace Akonadi;

DragDropManager::DragDropManager( QAbstractItemView *view )
    : m_view( view )
{

}

Collection DragDropManager::currentDropTarget(QDropEvent* event) const
{
  const QModelIndex index = m_view->indexAt( event->pos() );
  Collection col = m_view->model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( !col.isValid() ) {
    const Item item = m_view->model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      col = m_view->model()->data( index.parent(), EntityTreeModel::CollectionRole ).value<Collection>();
  }
  return col;
}

bool DragDropManager::dropAllowed( QDragMoveEvent * event )
{
  // Check if the collection under the cursor accepts this data type
  const Collection col = currentDropTarget( event );
  if ( col.isValid() )
  {
    QStringList supportedContentTypes = col.contentMimeTypes();
    const QMimeData *data = event->mimeData();
    KUrl::List urls = KUrl::List::fromMimeData( data );
    foreach( const KUrl &url, urls ) {
      const Collection collection = Collection::fromUrl( url );
      if ( collection.isValid() ) {
        if ( !supportedContentTypes.contains( Collection::mimeType() ) )
          break;

        // Check if we don't try to drop on one of the children
        if ( hasAncestor( m_view->indexAt( event->pos() ), collection.id() ) )
          break;
      } else { // This is an item.
        QString type = url.queryItems()[ QString::fromLatin1( "type" )];
        if ( !supportedContentTypes.contains( type ) )
          break;
      }
      return true;
    }
  }
  return false;
}

bool DragDropManager::hasAncestor( const QModelIndex& idx, Collection::Id parentId )
{
  QModelIndex idx2 = idx;
  while ( idx2.isValid() ) {
    if ( m_view->model()->data( idx2, EntityTreeModel::CollectionIdRole ).toLongLong() == parentId )
      return true;

    idx2 = idx2.parent();
  }
  return false;
}

bool DragDropManager::processDropEvent( QDropEvent *event )
{
  const Collection target = currentDropTarget( event );
  if ( !target.isValid() )
    return false;

  QMenu popup( m_view );
  int actionCount = 0;
  Qt::DropAction defaultAction;
  QAction* moveDropAction = 0;
  // TODO check if the source supports moving

  if ( (target.rights() & (Collection::CanCreateCollection | Collection::CanCreateItem))
        && (event->possibleActions() & Qt::MoveAction) ) {
    moveDropAction = popup.addAction( KIcon( QString::fromLatin1( "edit-rename" ) ), i18n( "&Move here" ) );
    ++actionCount;
    defaultAction = Qt::MoveAction;
  }
  QAction* copyDropAction = 0;
  if ( (target.rights() & (Collection::CanCreateCollection | Collection::CanCreateItem))
        && (event->possibleActions() & Qt::CopyAction) ) {
    copyDropAction = popup.addAction( KIcon( QString::fromLatin1( "edit-copy" ) ), i18n( "&Copy here" ) );
    ++actionCount;
    defaultAction = Qt::CopyAction;
  }
  QAction* linkAction = 0;
  if ( (target.rights() & Collection::CanLinkItem) && (event->possibleActions() & Qt::LinkAction) ) {
    linkAction = popup.addAction( KIcon( QLatin1String( "edit-link" ) ), i18n( "&Link here" ) );
    ++actionCount;
    defaultAction = Qt::LinkAction;
  }

  if ( actionCount == 0 ) {
    kDebug() << "Cannot drop here:" << event->possibleActions() << m_view->model()->supportedDragActions() << m_view->model()->supportedDropActions();
    return false;
  }

  if ( actionCount == 1 ) {
    kDebug() << "Selecting drop action" << defaultAction << ", there are no other possibilities";
    event->setDropAction( defaultAction );
    return true;
  }

  popup.addSeparator();
  popup.addAction( KIcon( QString::fromLatin1( "process-stop" ) ), i18n( "Cancel" ) );

  QAction *activatedAction = popup.exec( QCursor::pos() );

  if ( !activatedAction ) {
    return false;
  } else if ( activatedAction == moveDropAction ) {
    event->setDropAction( Qt::MoveAction );
  } else if ( activatedAction == copyDropAction ) {
    event->setDropAction( Qt::CopyAction );
  } else if ( activatedAction == linkAction ) {
    event->setDropAction( Qt::LinkAction );
  } else {
    return false;
  }
  return true;
}

void DragDropManager::startDrag( Qt::DropActions _supportedActions )
{
  QModelIndexList indexes;
  bool sourceDeletable = true;
  foreach ( const QModelIndex &index, m_view->selectionModel()->selectedRows() ) {
    if ( !m_view->model()->flags( index ) & Qt::ItemIsDragEnabled )
      continue;

    if ( sourceDeletable ) {
      Collection source = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( !source.isValid() ) {
        // index points to an item
        source = index.data( EntityTreeModel::ParentCollectionRole ).value<Collection>();
        sourceDeletable = source.rights() & Collection::CanDeleteItem;
      } else {
        // index points to a collection
        sourceDeletable = source.rights() & Collection::CanDeleteCollection;
      }
    }

    indexes.append( index );
  }

  if ( indexes.isEmpty() )
    return;

  QMimeData *mimeData = m_view->model()->mimeData( indexes );
  if ( !mimeData )
    return;

  QDrag *drag = new QDrag( m_view );
  drag->setMimeData( mimeData );
  if ( indexes.size() > 1 ) {
    drag->setPixmap( KIcon( QLatin1String( "document-multiple" ) ).pixmap( QSize( 22, 22 ) ) );
  } else {
    QPixmap pixmap = indexes.first().data( Qt::DecorationRole ).value<QIcon>().pixmap( QSize( 22, 22 ) );
    if ( pixmap.isNull() )
      pixmap = KIcon( QLatin1String( "text-plain" ) ).pixmap( QSize( 22, 22 ) );
    drag->setPixmap( pixmap );
  }

  Qt::DropActions supportedActions( _supportedActions );
  if ( !sourceDeletable )
    supportedActions &= ~Qt::MoveAction;
  drag->exec( supportedActions, Qt::CopyAction );
}




