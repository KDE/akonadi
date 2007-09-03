/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include <QtGui/QHeaderView>

#include "itemview.h"

#include "itemmodel.h"

using namespace Akonadi;

class ItemView::Private
{
  public:
    Private( ItemView *parent )
      : mParent( parent )
    {
    }

    void itemActivated( const QModelIndex& );
    void itemCurrentChanged( const QModelIndex& );

  private:
    ItemView *mParent;
};

void ItemView::Private::itemActivated( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const int currentItem = index.sibling(index.row(),ItemModel::Id).data(ItemModel::IdRole).toInt();
  if ( currentItem <= 0 )
    return;

  const QString remoteId = index.sibling(index.row(),ItemModel::RemoteId).data(ItemModel::IdRole).toString();

  emit mParent->activated( DataReference( currentItem, remoteId ) );
}

void ItemView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const int currentItem = index.sibling(index.row(),ItemModel::Id).data(ItemModel::IdRole).toInt();
  if ( currentItem <= 0 )
    return;

  const QString remoteId = index.sibling(index.row(),ItemModel::RemoteId).data(ItemModel::IdRole).toString();

  emit mParent->currentChanged( DataReference( currentItem, remoteId ) );
}

ItemView::ItemView( QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  setRootIsDecorated( false );

  header()->setClickable( true );
  header()->setStretchLastSection( true );

  connect( this, SIGNAL( activated( const QModelIndex& ) ),
           this, SLOT( itemActivated( const QModelIndex& ) ) );
}

ItemView::~ItemView()
{
  delete d;
}

void ItemView::setModel( QAbstractItemModel * model )
{
  QTreeView::setModel( model );

  connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( itemCurrentChanged( const QModelIndex& ) ) );
}

#include "itemview.moc"
