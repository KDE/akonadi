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

#include "collectionmodel.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QMenu>
#include <QtGui/QSortFilterProxyModel>

using namespace Akonadi;

class CollectionView::Private
{
  public:
    Private( CollectionView *parent )
      : mParent( parent )
    {
    }

    void dragExpand();
    void createCollection();
    void createResult( KJob* );
    void deleteCollection();
    void deleteResult( KJob* );
    void updateActions( const QModelIndex& );
    void itemActivated( const QModelIndex& );
    void itemCurrentChanged( const QModelIndex& );
    bool hasParent( const QModelIndex& idx, int parentId );

    CollectionView *mParent;
    QSortFilterProxyModel *filterModel;
    QModelIndex dragOverIndex;
    QTimer dragExpandTimer;

    KAction *newCollectionAction;
    KAction *deleteCollectionAction;
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
  kDebug() ;
  mParent->setExpanded( dragOverIndex, true );
  dragOverIndex = QModelIndex();
}

void CollectionView::Private::createCollection()
{
  QModelIndex index = mParent->currentIndex();
  if ( !index.data( CollectionModel::ChildCreatableRole ).toBool() )
    return;
  QString name = QInputDialog::getText( mParent, i18nc( "@title:window", "New Folder"), i18nc( "@label:textbox, name of a thing", "Name") );
  if ( name.isEmpty() )
    return;
  int parentId = index.data( CollectionModel::CollectionIdRole ).toInt();
  if ( parentId <= 0 )
    return;

  CollectionCreateJob *job = new CollectionCreateJob( Collection( parentId ), name );
  mParent->connect( job, SIGNAL(result(KJob*)), mParent, SLOT(createResult(KJob*)) );
}

void CollectionView::Private::createResult(KJob * job)
{
  if ( job->error() )
    KMessageBox::error( mParent, i18n("Could not create folder: %1", job->errorString()), i18n("Folder creation failed") );
}

void CollectionView::Private::deleteCollection()
{
  QModelIndex index = mParent->currentIndex();
  if ( !index.isValid() )
    return;
  int colId = index.data( CollectionModel::CollectionIdRole ).toInt();
  if ( colId <= 0 )
    return;

  CollectionDeleteJob *job = new CollectionDeleteJob( Collection( colId ) );
  mParent->connect( job, SIGNAL(result(KJob*)), mParent, SLOT(deleteResult(KJob*)) );
}

void CollectionView::Private::deleteResult(KJob * job)
{
  if ( job->error() )
    KMessageBox::error( mParent, i18n("Could not delete folder: %1", job->errorString()), i18n("Folder deletion failed") );
}

void CollectionView::Private::updateActions( const QModelIndex &current )
{
  if ( !current.isValid() ) {
    newCollectionAction->setEnabled( false );
    deleteCollectionAction->setEnabled( false );
    return;
  }

  newCollectionAction->setEnabled( current.data( CollectionModel::ChildCreatableRole ).toBool() );

  if ( current.parent().isValid() )
    deleteCollectionAction->setEnabled( true );
  else
    deleteCollectionAction->setEnabled( false );
}

void CollectionView::Private::itemActivated( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  int currentCollection = index.model()->data( index, CollectionModel::CollectionIdRole ).toInt();
  if ( currentCollection <= 0 )
    return;

  emit mParent->activated( Collection( currentCollection ) );
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

CollectionView::CollectionView( QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{
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

  d->newCollectionAction = new KAction( KIcon(QLatin1String("folder-new")), i18n("New Folder..."), this );
  connect( d->newCollectionAction, SIGNAL(triggered()), SLOT(createCollection()) );
  d->deleteCollectionAction = new KAction( KIcon(QLatin1String("edit-delete")), i18n("Delete Folder"), this );
  connect( d->deleteCollectionAction, SIGNAL(triggered()), SLOT(deleteCollection()) );

  connect( this, SIGNAL( activated( const QModelIndex& ) ),
           this, SLOT( itemActivated( const QModelIndex& ) ) );
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
           this, SLOT( updateActions( const QModelIndex& ) ) );
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
  d->updateActions( indexAt( event->pos() ) );
  QList<QAction*> actions;
  actions << d->newCollectionAction << d->deleteCollectionAction;
  QMenu::exec( actions, event->globalPos() );
  d->updateActions( currentIndex() );
}

#include "collectionview.moc"
