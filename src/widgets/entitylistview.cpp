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
#include <QDragMoveEvent>
#include <QMenu>

#include <qdebug.h>
#include <kxmlguiclient.h>
#include <KXMLGUIFactory>

#include "collection.h"
#include "control.h"
#include "item.h"
#include "entitytreemodel.h"
#include "progressspinnerdelegate_p.h"

using namespace Akonadi;

/**
 * @internal
 */
class EntityListView::Private
{
public:
    Private(EntityListView *parent)
        : mParent(parent)
#ifndef QT_NO_DRAGANDDROP
        , mDragDropManager(new DragDropManager(mParent))
#endif
        , mXmlGuiClient(0)
    {
    }

    void init();
    void itemClicked(const QModelIndex &index);
    void itemDoubleClicked(const QModelIndex &index);
    void itemCurrentChanged(const QModelIndex &index);

    EntityListView *mParent;
    DragDropManager *mDragDropManager;
    KXMLGUIClient *mXmlGuiClient;
};

void EntityListView::Private::init()
{
    mParent->setEditTriggers(QAbstractItemView::EditKeyPressed);
    mParent->setAcceptDrops(true);
#ifndef QT_NO_DRAGANDDROP
    mParent->setDropIndicatorShown(true);
    mParent->setDragDropMode(DragDrop);
    mParent->setDragEnabled(true);
#endif
    mParent->connect(mParent, SIGNAL(clicked(QModelIndex)), mParent, SLOT(itemClicked(QModelIndex)));
    mParent->connect(mParent, SIGNAL(doubleClicked(QModelIndex)), mParent, SLOT(itemDoubleClicked(QModelIndex)));

    DelegateAnimator *animator = new DelegateAnimator(mParent);
    ProgressSpinnerDelegate *customDelegate = new ProgressSpinnerDelegate(animator, mParent);
    mParent->setItemDelegate(customDelegate);

    Control::widgetNeedsAkonadi(mParent);
}

void EntityListView::Private::itemClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        emit mParent->clicked(collection);
    } else {
        const Item item = index.model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            emit mParent->clicked(item);
        }
    }
}

void EntityListView::Private::itemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        emit mParent->doubleClicked(collection);
    } else {
        const Item item = index.model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            emit mParent->doubleClicked(item);
        }
    }
}

void EntityListView::Private::itemCurrentChanged(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        emit mParent->currentChanged(collection);
    } else {
        const Item item = index.model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            emit mParent->currentChanged(item);
        }
    }
}

EntityListView::EntityListView(QWidget *parent)
    : QListView(parent)
    , d(new Private(this))
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    d->init();
}

EntityListView::EntityListView(KXMLGUIClient *xmlGuiClient, QWidget *parent)
    : QListView(parent)
    , d(new Private(this))
{
    d->mXmlGuiClient = xmlGuiClient;
    d->init();
}

EntityListView::~EntityListView()
{
    delete d->mDragDropManager;
    delete d;
}

void EntityListView::setModel(QAbstractItemModel *model)
{
    if (selectionModel()) {
        disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                   this, SLOT(itemCurrentChanged(QModelIndex)));
    }

    QListView::setModel(model);

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(itemCurrentChanged(QModelIndex)));
}

#ifndef QT_NO_DRAGANDDROP
void EntityListView::dragMoveEvent(QDragMoveEvent *event)
{
    if (d->mDragDropManager->dropAllowed(event) ||
        qobject_cast<Akonadi::FavoriteCollectionsModel *>(model())) {
        // All urls are supported. process the event.
        QListView::dragMoveEvent(event);
        return;
    }

    event->setDropAction(Qt::IgnoreAction);
}

void EntityListView::dropEvent(QDropEvent *event)
{
    bool menuCanceled = false;
    if (d->mDragDropManager->processDropEvent(event, menuCanceled) &&
        !menuCanceled) {
        if (menuCanceled) {
            return;
        }
        QListView::dropEvent(event);
    } else if (qobject_cast<Akonadi::FavoriteCollectionsModel *>(model()) &&
               !menuCanceled) {
        QListView::dropEvent(event);
    }
}
#endif

#ifndef QT_NO_CONTEXTMENU
void EntityListView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!d->mXmlGuiClient) {
        return;
    }

    const QModelIndex index = indexAt(event->pos());

    QMenu *popup = 0;

    // check if the index under the cursor is a collection or item
    const Collection collection = model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        popup = static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(
                                         QStringLiteral("akonadi_favoriteview_contextmenu"), d->mXmlGuiClient));
    } else {
        popup = static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(
                                         QStringLiteral("akonadi_favoriteview_emptyselection_contextmenu"), d->mXmlGuiClient));
    }

    if (popup) {
        popup->exec(event->globalPos());
    }
}
#endif

void EntityListView::setXmlGuiClient(KXMLGUIClient *xmlGuiClient)
{
    d->mXmlGuiClient = xmlGuiClient;
}

KXMLGUIClient *EntityListView::xmlGuiClient() const
{
    return d->mXmlGuiClient;
}

#ifndef QT_NO_DRAGANDDROP
void EntityListView::startDrag(Qt::DropActions supportedActions)
{
    d->mDragDropManager->startDrag(supportedActions);
}
#endif

void EntityListView::setDropActionMenuEnabled(bool enabled)
{
#ifndef QT_NO_DRAGANDDROP
    d->mDragDropManager->setShowDropActionMenu(enabled);
#endif
}

bool EntityListView::isDropActionMenuEnabled() const
{
#ifndef QT_NO_DRAGANDDROP
    return d->mDragDropManager->showDropActionMenu();
#else
    return false;
#endif
}

#include "moc_entitylistview.cpp"
