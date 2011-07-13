/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#include "entitylistview.h"

#include "dragdropmanager_p.h"
#include "favoritecollectionsmodel.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

#include <KAction>
#include <KLocale>
#include <KMessageBox>
#include <KUrl>
#include <KXMLGUIFactory>

#include <kdebug.h>
#include <kxmlguiclient.h>

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/item.h>
#include <akonadi/entitytreemodel.h>

#include <progressspinnerdelegate_p.h>

using namespace Akonadi;

/**
 * @internal
 */
class EntityListView::Private
{
public:
  Private( EntityListView *parent )
      : mParent( parent ), mDragDropManager( new DragDropManager( mParent ) ), mXmlGuiClient( 0 )
  {
  }

  void init();
  void itemClicked( const QModelIndex& );
  void itemDoubleClicked( const QModelIndex& );
  void itemCurrentChanged( const QModelIndex& );

  EntityListView *mParent;
  DragDropManager *mDragDropManager;
  KXMLGUIClient *mXmlGuiClient;
};

void EntityListView::Private::init()
{
  mParent->setEditTriggers( QAbstractItemView::EditKeyPressed );
  mParent->setAcceptDrops( true );
  mParent->setDropIndicatorShown( true );
  mParent->setDragDropMode( DragDrop );
  mParent->setDragEnabled( true );

  mParent->connect( mParent, SIGNAL( clicked( const QModelIndex& ) ),
                    mParent, SLOT( itemClicked( const QModelIndex& ) ) );
  mParent->connect( mParent, SIGNAL( doubleClicked( const QModelIndex& ) ),
                    mParent, SLOT( itemDoubleClicked( const QModelIndex& ) ) );

  DelegateAnimator *animator = new DelegateAnimator(mParent);
  ProgressSpinnerDelegate *customDelegate = new ProgressSpinnerDelegate(animator, mParent);
  mParent->setItemDelegate(customDelegate);

  Control::widgetNeedsAkonadi( mParent );
}

void EntityListView::Private::itemClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->clicked( collection );
  } else {
    const Item item = index.model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      emit mParent->clicked( item );
  }
}

void EntityListView::Private::itemDoubleClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->doubleClicked( collection );
  } else {
    const Item item = index.model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      emit mParent->doubleClicked( item );
  }
}

void EntityListView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->currentChanged( collection );
  } else {
    const Item item = index.model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      emit mParent->currentChanged( item );
  }
}

EntityListView::EntityListView( QWidget * parent )
  : QListView( parent ),
    d( new Private( this ) )
{
  setSelectionMode( QAbstractItemView::SingleSelection );
  d->init();
}

EntityListView::EntityListView( KXMLGUIClient *xmlGuiClient, QWidget * parent )
  : QListView( parent ),
    d( new Private( this ) )
{
  d->mXmlGuiClient = xmlGuiClient;
  d->init();
}

EntityListView::~EntityListView()
{
  delete d->mDragDropManager;
  delete d;
}

void EntityListView::setModel( QAbstractItemModel * model )
{
  if ( selectionModel() ) {
    disconnect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( itemCurrentChanged( const QModelIndex& ) ) );
  }

  QListView::setModel( model );

  connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           SLOT( itemCurrentChanged( const QModelIndex& ) ) );
}

void EntityListView::dragMoveEvent( QDragMoveEvent * event )
{
  if ( d->mDragDropManager->dropAllowed( event ) || qobject_cast<Akonadi::FavoriteCollectionsModel*>( model() ) ) {
    // All urls are supported. process the event.
    QListView::dragMoveEvent( event );
    return;
  }

  event->setDropAction( Qt::IgnoreAction );
}

void EntityListView::dropEvent( QDropEvent * event )
{
  if ( d->mDragDropManager->processDropEvent( event ) || qobject_cast<Akonadi::FavoriteCollectionsModel*>( model() ) ) {
    QListView::dropEvent( event );
  }
}

void EntityListView::contextMenuEvent( QContextMenuEvent * event )
{
  if ( !d->mXmlGuiClient )
    return;

  const QModelIndex index = indexAt( event->pos() );

  QMenu *popup = 0;

  // check if the index under the cursor is a collection or item
  const Collection collection = model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    popup = static_cast<QMenu*>( d->mXmlGuiClient->factory()->container(
                                 QLatin1String( "akonadi_favoriteview_contextmenu" ), d->mXmlGuiClient ) );
  } else {
    popup = static_cast<QMenu*>( d->mXmlGuiClient->factory()->container(
                                   QLatin1String( "akonadi_favoriteview_emptyselection_contextmenu" ), d->mXmlGuiClient) );
  }

  if ( popup )
    popup->exec( event->globalPos() );
}

void EntityListView::setXmlGuiClient( KXMLGUIClient *xmlGuiClient )
{
  d->mXmlGuiClient = xmlGuiClient;
}

void EntityListView::startDrag( Qt::DropActions supportedActions )
{
  d->mDragDropManager->startDrag( supportedActions );
}

#include "entitylistview.moc"
