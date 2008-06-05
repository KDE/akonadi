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


#include "itemview.h"

#include "itemmodel.h"

#include <KXMLGUIFactory>
#include <KXmlGuiWindow>

#include <QtGui/QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>

using namespace Akonadi;

/**
 * @internal
 */
class ItemView::Private
{
  public:
    Private( ItemView *parent ) :
      xmlGuiWindow( 0 ),
      mParent( parent )
    {
    }

    void init();
    void itemActivated( const QModelIndex& );
    void itemCurrentChanged( const QModelIndex& );

    KXmlGuiWindow *xmlGuiWindow;

  private:
    ItemView *mParent;
};

void ItemView::Private::init()
{
  mParent->setRootIsDecorated( false );

  mParent->header()->setClickable( true );
  mParent->header()->setStretchLastSection( true );

  mParent->connect( mParent, SIGNAL( activated( const QModelIndex& ) ),
                    mParent, SLOT( itemActivated( const QModelIndex& ) ) );
}

void ItemView::Private::itemActivated( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Item::Id currentItem = index.sibling(index.row(),ItemModel::Id).data(ItemModel::IdRole).toLongLong();
  if ( currentItem <= 0 )
    return;

  const QString remoteId = index.sibling(index.row(),ItemModel::RemoteId).data(ItemModel::IdRole).toString();

  Item item( currentItem );
  item.setRemoteId( remoteId );

  emit mParent->activated( item );
}

void ItemView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Item::Id currentItem = index.sibling(index.row(),ItemModel::Id).data(ItemModel::IdRole).toLongLong();
  if ( currentItem <= 0 )
    return;

  const QString remoteId = index.sibling(index.row(),ItemModel::RemoteId).data(ItemModel::IdRole).toString();

  Item item( currentItem );
  item.setRemoteId( remoteId );

  emit mParent->currentChanged( item );
}

ItemView::ItemView( QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  d->init();
}

ItemView::ItemView(KXmlGuiWindow * xmlGuiWindow, QWidget * parent) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  d->xmlGuiWindow = xmlGuiWindow;
  d->init();
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

void ItemView::contextMenuEvent(QContextMenuEvent * event)
{
  if ( !d->xmlGuiWindow )
    return;
  QMenu *popup = static_cast<QMenu*>( d->xmlGuiWindow->guiFactory()->container(
                                      QLatin1String("akonadi_itemview_contextmenu"), d->xmlGuiWindow ) );
  if ( popup )
    popup->exec( event->globalPos() );
}

void ItemView::setXmlGuiWindow(KXmlGuiWindow * xmlGuiWindow)
{
  d->xmlGuiWindow = xmlGuiWindow;
}

#include "itemview.moc"
