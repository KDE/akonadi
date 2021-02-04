/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitylistview.h"

#include "dragdropmanager_p.h"

#include <QDragMoveEvent>
#include <QMenu>

#include "akonadiwidgets_debug.h"
#include <KXMLGUIClient>
#include <KXMLGUIFactory>

#include "collection.h"
#include "controlgui.h"
#include "entitytreemodel.h"
#include "item.h"
#include "progressspinnerdelegate_p.h"

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN EntityListView::Private
{
public:
    explicit Private(EntityListView *parent)
        : mParent(parent)
#ifndef QT_NO_DRAGANDDROP
        , mDragDropManager(new DragDropManager(mParent))
#endif
    {
    }

    void init();
    void itemClicked(const QModelIndex &index) const;
    void itemDoubleClicked(const QModelIndex &index) const;
    void itemCurrentChanged(const QModelIndex &index) const;

    EntityListView *const mParent;
    DragDropManager *mDragDropManager = nullptr;
    KXMLGUIClient *mXmlGuiClient = nullptr;
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
    mParent->connect(mParent, &QAbstractItemView::clicked, mParent, [this](const auto &index) {
        itemClicked(index);
    });
    mParent->connect(mParent, &QAbstractItemView::doubleClicked, mParent, [this](const auto &index) {
        itemDoubleClicked(index);
    });

    auto animator = new DelegateAnimator(mParent);
    auto customDelegate = new ProgressSpinnerDelegate(animator, mParent);
    mParent->setItemDelegate(customDelegate);

    ControlGui::widgetNeedsAkonadi(mParent);
}

void EntityListView::Private::itemClicked(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        Q_EMIT mParent->clicked(collection);
    } else {
        const Item item = index.model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            Q_EMIT mParent->clicked(item);
        }
    }
}

void EntityListView::Private::itemDoubleClicked(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        Q_EMIT mParent->doubleClicked(collection);
    } else {
        const Item item = index.model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            Q_EMIT mParent->doubleClicked(item);
        }
    }
}

void EntityListView::Private::itemCurrentChanged(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        Q_EMIT mParent->currentChanged(collection);
    } else {
        const Item item = index.model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            Q_EMIT mParent->currentChanged(item);
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
        disconnect(selectionModel(), &QItemSelectionModel::currentChanged, this, nullptr);
    }

    QListView::setModel(model);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &index) {
        d->itemCurrentChanged(index);
    });
}

#ifndef QT_NO_DRAGANDDROP
void EntityListView::dragMoveEvent(QDragMoveEvent *event)
{
    if (d->mDragDropManager->dropAllowed(event)) {
        // All urls are supported. process the event.
        QListView::dragMoveEvent(event);
        return;
    }

    event->setDropAction(Qt::IgnoreAction);
}

void EntityListView::dropEvent(QDropEvent *event)
{
    bool menuCanceled = false;
    if (d->mDragDropManager->processDropEvent(event, menuCanceled) && !menuCanceled) {
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

    QMenu *popup = nullptr;

    // check if the index under the cursor is a collection or item
    const Collection collection = model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        popup = static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(QStringLiteral("akonadi_favoriteview_contextmenu"), d->mXmlGuiClient));
    } else {
        popup =
            static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(QStringLiteral("akonadi_favoriteview_emptyselection_contextmenu"), d->mXmlGuiClient));
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
