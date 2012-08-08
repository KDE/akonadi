/*
  Copyright (c) 2009 KDAB
  Author: Frank Osterfeld <frank@kdab.net>

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

#include "collectionselection_p.h"
#include <akonadi/entitytreemodel.h>
#include <QItemSelectionModel>

using namespace Akonadi;

static Akonadi::Collection collectionFromIndex( const QModelIndex &index )
{
  return index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
}

static Akonadi::Collection::Id collectionIdFromIndex( const QModelIndex &index )
{
  return index.data( Akonadi::EntityTreeModel::CollectionIdRole ).value<Akonadi::Collection::Id>();
}

static Akonadi::Collection::List collectionsFromIndexes( const QModelIndexList &indexes )
{
  Akonadi::Collection::List l;
  Q_FOREACH( const QModelIndex &idx, indexes ) {
    l.push_back( collectionFromIndex( idx ) );
  }
  return l;
}

class CollectionSelection::Private
{
  public:
    explicit Private( QItemSelectionModel *model_ ) : model( model_ )
    {
    }

    QItemSelectionModel *model;
};

CollectionSelection::CollectionSelection( QItemSelectionModel *selectionModel, QObject *parent )
  : QObject( parent ), d( new Private ( selectionModel ) )
{
  connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
           this, SLOT(slotSelectionChanged(QItemSelection,QItemSelection)) );
}

CollectionSelection::~CollectionSelection()
{
  delete d;
}

QItemSelectionModel *CollectionSelection::model() const
{
  return d->model;
}

bool CollectionSelection::hasSelection() const
{
  return d->model->hasSelection();
}

bool CollectionSelection::contains( const Akonadi::Collection &c ) const
{
  return selectedCollectionIds().contains( c.id() );
}

bool CollectionSelection::contains( const Akonadi::Collection::Id &id ) const
{
  return selectedCollectionIds().contains( id );
}

Akonadi::Collection::List CollectionSelection::selectedCollections() const
{
  Akonadi::Collection::List selected;
  Q_FOREACH ( const QModelIndex &idx, d->model->selectedIndexes() ) {
    selected.append( collectionFromIndex( idx ) );
  }
  return selected;
}

QList<Akonadi::Collection::Id> CollectionSelection::selectedCollectionIds() const
{
  QList<Akonadi::Collection::Id> selected;
  Q_FOREACH ( const QModelIndex &idx, d->model->selectedIndexes() ) {
    selected.append( collectionIdFromIndex( idx ) );
  }
  return selected;
}

void CollectionSelection::slotSelectionChanged( const QItemSelection &selectedIndexes,
                                                const QItemSelection &deselIndexes )
{
  const Akonadi::Collection::List selected = collectionsFromIndexes( selectedIndexes.indexes() );
  const Akonadi::Collection::List deselected = collectionsFromIndexes( deselIndexes.indexes() );

  emit selectionChanged( selected, deselected );
  Q_FOREACH ( const Akonadi::Collection &c, deselected ) {
    emit collectionDeselected( c );
  }
  Q_FOREACH ( const Akonadi::Collection &c, selected ) {
    emit collectionSelected( c );
  }
}
