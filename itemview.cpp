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

#include "control.h"
#include "itemmodel.h"

#include <KXMLGUIFactory>
#include <KXmlGuiWindow>

#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

using namespace Akonadi;

/**
 * @internal
 */
class ItemView::Private
{
  public:
    Private( ItemView *parent ) :
      xmlGuiClient( 0 ),
      mParent( parent )
    {
    }

    void init();
    void itemActivated( const QModelIndex& );
    void itemCurrentChanged( const QModelIndex& );
    void itemClicked( const QModelIndex& );
    void itemDoubleClicked( const QModelIndex& );

    Item itemForIndex( const QModelIndex& );

    KXMLGUIClient *xmlGuiClient;

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
  mParent->connect( mParent, SIGNAL( clicked( const QModelIndex& ) ),
                    mParent, SLOT( itemClicked( const QModelIndex& ) ) );
  mParent->connect( mParent, SIGNAL( doubleClicked( const QModelIndex& ) ),
                    mParent, SLOT( itemDoubleClicked( const QModelIndex& ) ) );

  Control::widgetNeedsAkonadi( mParent );
}

Item ItemView::Private::itemForIndex( const QModelIndex &index )
{
  if ( !index.isValid() )
    return Item();

  const Item::Id currentItem = index.sibling( index.row(), ItemModel::Id ).data( ItemModel::IdRole ).toLongLong();
  if ( currentItem <= 0 )
    return Item();

  const QString remoteId = index.sibling( index.row(), ItemModel::RemoteId ).data( ItemModel::IdRole ).toString();
  const QString mimeType = index.sibling( index.row(), ItemModel::MimeType ).data( ItemModel::MimeTypeRole ).toString();

  Item item( currentItem );
  item.setRemoteId( remoteId );
  item.setMimeType( mimeType );

  return item;
}

void ItemView::Private::itemActivated( const QModelIndex &index )
{
  const Item item = itemForIndex( index );

  if ( !item.isValid() )
    return;

  emit mParent->activated( item );
}

void ItemView::Private::itemCurrentChanged( const QModelIndex &index )
{
  const Item item = itemForIndex( index );

  if ( !item.isValid() )
    return;

  emit mParent->currentChanged( item );
}

void ItemView::Private::itemClicked( const QModelIndex &index )
{
  const Item item = itemForIndex( index );

  if ( !item.isValid() )
    return;

  emit mParent->clicked( item );
}

void ItemView::Private::itemDoubleClicked( const QModelIndex &index )
{
  const Item item = itemForIndex( index );

  if ( !item.isValid() )
    return;

  emit mParent->doubleClicked( item );
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
  d->xmlGuiClient = static_cast<KXMLGUIClient*>( xmlGuiWindow );
  d->init();
}

ItemView::ItemView(KXMLGUIClient * xmlGuiClient, QWidget * parent) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  d->xmlGuiClient = xmlGuiClient;
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
  if ( !d->xmlGuiClient )
    return;
  QMenu *popup = static_cast<QMenu*>( d->xmlGuiClient->factory()->container(
                                      QLatin1String("akonadi_itemview_contextmenu"), d->xmlGuiClient ) );
  if ( popup )
    popup->exec( event->globalPos() );
}

void ItemView::setXmlGuiWindow(KXmlGuiWindow * xmlGuiWindow)
{
  d->xmlGuiClient = static_cast<KXMLGUIClient*>( xmlGuiWindow );
}

void ItemView::setXmlGuiClient(KXMLGUIClient * xmlGuiClient)
{
  d->xmlGuiClient = xmlGuiClient;
}

#include "itemview.moc"
