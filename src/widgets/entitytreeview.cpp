/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>
    Copyright (c) 2012 Laurent Montel <montel@kde.org>

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

#include "entitytreeview.h"

#include "dragdropmanager_p.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QApplication>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QMenu>

#include "collection.h"
#include "control.h"
#include "item.h"
#include "entitytreemodel.h"

#include <qdebug.h>
#include <kxmlguiclient.h>
#include <KXMLGUIFactory>

#include "progressspinnerdelegate_p.h"

using namespace Akonadi;

/**
 * @internal
 */
class EntityTreeView::Private
{
public:
    Private(EntityTreeView *parent)
        : mParent(parent)
#ifndef QT_NO_DRAGANDDROP
        , mDragDropManager(new DragDropManager(mParent))
#endif
        , mXmlGuiClient(0)
        , mDefaultPopupMenu(QStringLiteral("akonadi_collectionview_contextmenu"))
    {
    }

    void init();
    void itemClicked(const QModelIndex &index);
    void itemDoubleClicked(const QModelIndex &index);
    void itemCurrentChanged(const QModelIndex &index);

    void slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    EntityTreeView *mParent;
    QBasicTimer mDragExpandTimer;
    DragDropManager *mDragDropManager;
    KXMLGUIClient *mXmlGuiClient;
    QString mDefaultPopupMenu;
};

void EntityTreeView::Private::init()
{
    Akonadi::DelegateAnimator *animator = new Akonadi::DelegateAnimator(mParent);
    Akonadi::ProgressSpinnerDelegate *customDelegate = new Akonadi::ProgressSpinnerDelegate(animator, mParent);
    mParent->setItemDelegate(customDelegate);

    mParent->header()->setClickable(true);
    mParent->header()->setStretchLastSection(false);
//   mParent->setRootIsDecorated( false );

    // QTreeView::autoExpandDelay has very strange behaviour. It toggles the collapse/expand state
    // of the item the cursor is currently over when a timer event fires.
    // The behaviour we want is to expand a collapsed row on drag-over, but not collapse it.
    // mDragExpandTimer is used to achieve this.
//   mParent->setAutoExpandDelay ( QApplication::startDragTime() );

    mParent->setSortingEnabled(true);
    mParent->sortByColumn(0, Qt::AscendingOrder);
    mParent->setEditTriggers(QAbstractItemView::EditKeyPressed);
    mParent->setAcceptDrops(true);
#ifndef QT_NO_DRAGANDDROP
    mParent->setDropIndicatorShown(true);
    mParent->setDragDropMode(DragDrop);
    mParent->setDragEnabled(true);
#endif

    mParent->connect(mParent, SIGNAL(clicked(QModelIndex)),
                     mParent, SLOT(itemClicked(QModelIndex)));
    mParent->connect(mParent, SIGNAL(doubleClicked(QModelIndex)),
                     mParent, SLOT(itemDoubleClicked(QModelIndex)));

    Control::widgetNeedsAkonadi(mParent);
}

void EntityTreeView::Private::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    const int column = 0;
    foreach (const QItemSelectionRange &range, selected) {
        const QModelIndex index = range.topLeft();

        if (index.column() > 0) {
            continue;
        }

        for (int row = index.row(); row <= range.bottomRight().row(); ++row) {
            // Don't use canFetchMore here. We need to bypass the check in
            // the EntityFilterModel when it shows only collections.
            mParent->model()->fetchMore(index.sibling(row, column));
        }
    }

    if (selected.size() == 1) {
        const QItemSelectionRange &range = selected.first();
        if (range.topLeft().row() == range.bottomRight().row()) {
            mParent->scrollTo(range.topLeft(), QTreeView::EnsureVisible);
        }
    }
}

void EntityTreeView::Private::itemClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex idx = index.sibling(index.row(), 0);

    const Collection collection = idx.model()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        emit mParent->clicked(collection);
    } else {
        const Item item = idx.model()->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            emit mParent->clicked(item);
        }
    }
}

void EntityTreeView::Private::itemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex idx = index.sibling(index.row(), 0);
    const Collection collection = idx.model()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        emit mParent->doubleClicked(collection);
    } else {
        const Item item = idx.model()->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            emit mParent->doubleClicked(item);
        }
    }
}

void EntityTreeView::Private::itemCurrentChanged(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex idx = index.sibling(index.row(), 0);
    const Collection collection = idx.model()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        emit mParent->currentChanged(collection);
    } else {
        const Item item = idx.model()->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            emit mParent->currentChanged(item);
        }
    }
}

EntityTreeView::EntityTreeView(QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    d->init();
}

EntityTreeView::EntityTreeView(KXMLGUIClient *xmlGuiClient, QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    d->mXmlGuiClient = xmlGuiClient;
    d->init();
}

EntityTreeView::~EntityTreeView()
{
    delete d->mDragDropManager;
    delete d;
}

void EntityTreeView::setModel(QAbstractItemModel *model)
{
    if (selectionModel()) {
        disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                   this, SLOT(itemCurrentChanged(QModelIndex)));

        disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                   this, SLOT(slotSelectionChanged(QItemSelection,QItemSelection)));
    }

    QTreeView::setModel(model);
    header()->setStretchLastSection(true);

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(itemCurrentChanged(QModelIndex)));

    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(slotSelectionChanged(QItemSelection,QItemSelection)));
}

void EntityTreeView::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == d->mDragExpandTimer.timerId()) {
        const QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
        if (state() == QAbstractItemView::DraggingState && viewport()->rect().contains(pos)) {
            setExpanded(indexAt(pos), true);
        }
    }

    QTreeView::timerEvent(event);
}

#ifndef QT_NO_DRAGANDDROP
void EntityTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    d->mDragExpandTimer.start(QApplication::startDragTime() , this);

    if (d->mDragDropManager->dropAllowed(event)) {
        // All urls are supported. process the event.
        QTreeView::dragMoveEvent(event);
        return;
    }

    event->setDropAction(Qt::IgnoreAction);
}

void EntityTreeView::dropEvent(QDropEvent *event)
{
    d->mDragExpandTimer.stop();
    bool menuCanceled = false;
    if (d->mDragDropManager->processDropEvent(event, menuCanceled, (dropIndicatorPosition() == QAbstractItemView::OnItem))) {
        QTreeView::dropEvent(event);
    }
}
#endif

#ifndef QT_NO_CONTEXTMENU
void EntityTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!d->mXmlGuiClient || !model()) {
        return;
    }

    const QModelIndex index = indexAt(event->pos());
    QString popupName = d->mDefaultPopupMenu;

    if (index.isValid()) {                                  // popup not over empty space
        // check whether the index under the cursor is a collection or item
        const Item item = model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        popupName = (item.isValid() ? QStringLiteral("akonadi_itemview_contextmenu") :
                     QStringLiteral("akonadi_collectionview_contextmenu"));
    }

    QMenu *popup = static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(popupName, d->mXmlGuiClient));
    if (popup) {
        popup->exec(event->globalPos());
    }
}
#endif

void EntityTreeView::setXmlGuiClient(KXMLGUIClient *xmlGuiClient)
{
    d->mXmlGuiClient = xmlGuiClient;
}

KXMLGUIClient *EntityTreeView::xmlGuiClient() const
{
    return d->mXmlGuiClient;
}

#ifndef QT_NO_DRAGANDDROP
void EntityTreeView::startDrag(Qt::DropActions supportedActions)
{
    d->mDragDropManager->startDrag(supportedActions);
}
#endif

void EntityTreeView::setDropActionMenuEnabled(bool enabled)
{
#ifndef QT_NO_DRAGANDDROP
    d->mDragDropManager->setShowDropActionMenu(enabled);
#endif
}

bool EntityTreeView::isDropActionMenuEnabled() const
{
#ifndef QT_NO_DRAGANDDROP
    return d->mDragDropManager->showDropActionMenu();
#else
    return false;
#endif
}

void EntityTreeView::setManualSortingActive(bool active)
{
#ifndef QT_NO_DRAGANDDROP
    d->mDragDropManager->setManualSortingActive(active);
#endif
}

bool EntityTreeView::isManualSortingActive() const
{
#ifndef QT_NO_DRAGANDDROP
    return d->mDragDropManager->isManualSortingActive();
#else
    return false;
#endif
}

void EntityTreeView::setDefaultPopupMenu(const QString &name)
{
    d->mDefaultPopupMenu = name;
}

#include "moc_entitytreeview.cpp"
