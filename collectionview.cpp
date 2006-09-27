/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <klocale.h>

#include <QtCore/QDebug>
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

  // temporary for testing
  connect( this, SIGNAL(doubleClicked(QModelIndex)), SLOT(createCollection(QModelIndex)) );
}

CollectionView::~ CollectionView( )
{
  delete d;
  d = 0;
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
  QModelIndex index =sourceIndex( parent );
  if ( !d->model->canCreateCollection( index ) )
    return;
  QString name = QInputDialog::getText( this, i18n("New Folder"), i18n("Name") );
  if ( name.isEmpty() )
    return;
  if ( !d->model->createCollection( index, name ) )
    qWarning() << "Collection creation failed immediately!";
}

void CollectionView::dragMoveEvent(QDragMoveEvent * event)
{
  QModelIndex index = sourceIndex( indexAt( event->pos() ) );
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

#include "collectionview.moc"
