/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "collectiondialog.h"

#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/collectionview.h>

#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class CollectionDialog::Private
{
  public:
    CollectionDialog *q;
    CollectionModel *collectionModel;
    CollectionFilterProxyModel *filterModel;
    CollectionView *collectionView;

    explicit Private(CollectionDialog *q);
    void slotSelectionChanged();
};

CollectionDialog::Private::Private(CollectionDialog *q)
  : q(q),
    collectionModel( 0 ),
    filterModel( 0 ),
    collectionView( 0 )
{
}

void CollectionDialog::Private::slotSelectionChanged()
{
  q->enableButton(KDialog::Ok, collectionView->selectionModel()->selectedIndexes().count() > 0);
}

CollectionDialog::CollectionDialog( QWidget *parent )
  : KDialog( parent ),
    d( new Private(this) )
{
  QWidget *widget = mainWidget();
  QVBoxLayout *layout = new QVBoxLayout( widget );

  d->collectionModel = new CollectionModel( this );

  d->filterModel = new CollectionFilterProxyModel( this );
  d->filterModel->setDynamicSortFilter( true );
  d->filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
  d->filterModel->setSourceModel( d->collectionModel );

  d->collectionView = new CollectionView( widget );
  d->collectionView->setModel( d->filterModel );
  layout->addWidget( d->collectionView );

  connect(d->collectionView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotSelectionChanged()));
  enableButton(KDialog::Ok, false);
}

CollectionDialog::~CollectionDialog()
{
  delete d;
}

Akonadi::Collection CollectionDialog::selectedCollection() const
{
  if ( selectionMode() == QAbstractItemView::SingleSelection ) {
    const QModelIndex index = d->collectionView->currentIndex();
    if ( index.isValid() )
      return index.model()->data( index, CollectionModel::CollectionRole ).value<Collection>();
  }

  return Collection();
}

Akonadi::Collection::List CollectionDialog::selectedCollections() const
{
  Collection::List collections;
  const QItemSelectionModel *selectionModel = d->collectionView->selectionModel();
  const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
  foreach ( const QModelIndex &index, selectedIndexes ) {
    if ( index.isValid() ) {
      const Collection collection = index.model()->data( index, CollectionModel::CollectionRole ).value<Collection>();
      if ( collection.isValid() )
        collections.append( collection );
    }
  }

  return collections;
}

void CollectionDialog::setMimeTypeFilter( const QStringList &mimeTypes )
{
  d->filterModel->clearFilters();
  d->filterModel->addMimeTypeFilters( mimeTypes );
}

QStringList CollectionDialog::mimeTypeFilter() const
{
  return d->filterModel->mimeTypeFilters();
}

void CollectionDialog::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  d->collectionView->setSelectionMode( mode );
}

QAbstractItemView::SelectionMode CollectionDialog::selectionMode() const
{
  return d->collectionView->selectionMode();
}

#include "collectiondialog.moc"
