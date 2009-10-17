/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "asyncselectionhandler_p.h"

#include <akonadi/entitytreemodel.h>

using namespace Akonadi;

AsyncSelectionHandler::AsyncSelectionHandler( QAbstractItemModel *model, QObject *parent )
  : QObject( parent ), mModel( model )
{
  Q_ASSERT( mModel );

  connect( mModel, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( rowsInserted( const QModelIndex&, int, int ) ) );
}

AsyncSelectionHandler::~AsyncSelectionHandler()
{
}

void AsyncSelectionHandler::waitForCollection( const Collection &collection )
{
  mCollection = collection;

  for ( int i = 0; i < mModel->rowCount(); ++i ) {
    const QModelIndex index = mModel->index( i, 0 );
    const Collection::Id id = index.data( EntityTreeModel::CollectionIdRole ).toLongLong();
    if ( collection.id() == id ) {
      emit collectionAvailable( index );
      break;
    }
  }
}

void AsyncSelectionHandler::waitForItem( const Item &item )
{
  mItem = item;

  for ( int i = 0; i < mModel->rowCount(); ++i ) {
    const QModelIndex index = mModel->index( i, 0 );
    const Item::Id id = index.data( EntityTreeModel::ItemIdRole ).toLongLong();
    if ( item.id() == id ) {
      emit itemAvailable( index );
      break;
    }
  }
}

void AsyncSelectionHandler::rowsInserted( const QModelIndex &parent, int start, int end )
{
  for ( int i = start; i <= end; ++i ) {
    const QModelIndex index = mModel->index( i, 0, parent );

    if ( mCollection.isValid() ) {
      const Collection::Id id = index.data( EntityTreeModel::CollectionIdRole ).toLongLong();
      if ( mCollection.id() == id ) {
        emit collectionAvailable( index );
      }
    }

    if ( mItem.isValid() ) {
      const Item::Id id = index.data( EntityTreeModel::CollectionIdRole ).toLongLong();
      if ( mItem.id() == id )
        emit itemAvailable( index );
    }
  }
}

#include "asyncselectionhandler_p.moc"
