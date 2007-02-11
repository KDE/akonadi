/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectionmodel.h"
#include "collectionview.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QSortFilterProxyModel>

using namespace Akonadi;

class CollectionView::Private
{
  public:
    QSortFilterProxyModel *filterModel;
    CollectionModel *model;
    QModelIndex dragOverIndex;
    QTimer dragExpandTimer;
};

CollectionView::CollectionView( QWidget * parent ) :
    QTreeView( parent ),
    d( new Private() )
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
  setDragDropMode( DropOnly );

  d->dragExpandTimer.setSingleShot( true );
  connect( &d->dragExpandTimer, SIGNAL(timeout()), SLOT(dragExpand()) );

  // temporary for testing
  connect( this, SIGNAL(doubleClicked(QModelIndex)), SLOT(createCollection(QModelIndex)) );
}

CollectionView::~ CollectionView( )
{
  delete d;
}

void CollectionView::setModel( QAbstractItemModel * model )
{
  d->model = static_cast<CollectionModel*>( model );
  d->filterModel->setSourceModel( model );
  QTreeView::setModel( d->filterModel );
  header()->setResizeMode( 0, QHeaderView::Stretch );
}

void CollectionView::createCollection( const QModelIndex & parent )
{
  if ( !model()->data( parent, CollectionModel::ChildCreatableRole ).toBool() )
    return;
  QString name = QInputDialog::getText( this, i18n("New Folder"), i18n("Name") );
  if ( name.isEmpty() )
    return;
  QModelIndex index = sourceIndex( parent );
  if ( !d->model->createCollection( index, name ) )
    qWarning() << "Collection creation failed immediately!";
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

  index = sourceIndex( indexAt( event->pos() ) );
  QStringList mimeTypes = event->mimeData()->formats();
  if ( !d->model->supportsContentType( index, mimeTypes ) ) {
    event->setDropAction( Qt::IgnoreAction );
    return;
  }
  QTreeView::dragMoveEvent( event );
}

QModelIndex CollectionView::sourceIndex(const QModelIndex & index)
{
  if ( index.model() == d->filterModel )
    return d->filterModel->mapToSource( index );
  return index;
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
  QTreeView::dropEvent( event );
}

void CollectionView::dragExpand()
{
  kDebug() << k_funcinfo << endl;
  setExpanded( d->dragOverIndex, true );
  d->dragOverIndex = QModelIndex();
}

#include "collectionview.moc"
