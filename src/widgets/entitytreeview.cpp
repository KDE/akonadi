/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>
    SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitytreeview.h"

#include "dragdropmanager_p.h"

#include <QApplication>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QMenu>
#include <QTimer>

#include "collection.h"
#include "controlgui.h"
#include "entitytreemodel.h"
#include "item.h"

#include <KXMLGUIClient>
#include <KXMLGUIFactory>

#include "progressspinnerdelegate_p.h"

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN EntityTreeView::Private
{
public:
    explicit Private(EntityTreeView *parent)
        : mParent(parent)
#ifndef QT_NO_DRAGANDDROP
        , mDragDropManager(new DragDropManager(mParent))
#endif
        , mDefaultPopupMenu(QStringLiteral("akonadi_collectionview_contextmenu"))
    {
    }

    void init();
    void itemClicked(const QModelIndex &index) const;
    void itemDoubleClicked(const QModelIndex &index) const;
    void itemCurrentChanged(const QModelIndex &index) const;

    void slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) const;

    EntityTreeView *const mParent;
    QBasicTimer mDragExpandTimer;
    DragDropManager *mDragDropManager = nullptr;
    KXMLGUIClient *mXmlGuiClient = nullptr;
    QString mDefaultPopupMenu;
};

void EntityTreeView::Private::init()
{
    auto animator = new Akonadi::DelegateAnimator(mParent);
    auto customDelegate = new Akonadi::ProgressSpinnerDelegate(animator, mParent);
    mParent->setItemDelegate(customDelegate);

    mParent->header()->setSectionsClickable(true);
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

    mParent->connect(mParent, &QAbstractItemView::clicked, mParent, [this](const auto &index) {
        itemClicked(index);
    });
    mParent->connect(mParent, &QAbstractItemView::doubleClicked, mParent, [this](const auto &index) {
        itemDoubleClicked(index);
    });

    ControlGui::widgetNeedsAkonadi(mParent);
}

void EntityTreeView::Private::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) const
{
    Q_UNUSED(deselected)
    const int column = 0;
    for (const QItemSelectionRange &range : selected) {
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

void EntityTreeView::Private::itemClicked(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex idx = index.sibling(index.row(), 0);

    const Collection collection = idx.model()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        Q_EMIT mParent->clicked(collection);
    } else {
        const Item item = idx.model()->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            Q_EMIT mParent->clicked(item);
        }
    }
}

void EntityTreeView::Private::itemDoubleClicked(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex idx = index.sibling(index.row(), 0);
    const Collection collection = idx.model()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        Q_EMIT mParent->doubleClicked(collection);
    } else {
        const Item item = idx.model()->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            Q_EMIT mParent->doubleClicked(item);
        }
    }
}

void EntityTreeView::Private::itemCurrentChanged(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex idx = index.sibling(index.row(), 0);
    const Collection collection = idx.model()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
    if (collection.isValid()) {
        Q_EMIT mParent->currentChanged(collection);
    } else {
        const Item item = idx.model()->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            Q_EMIT mParent->currentChanged(item);
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
        disconnect(selectionModel(), &QItemSelectionModel::currentChanged, this, nullptr);
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, nullptr);
    }

    QTreeView::setModel(model);
    header()->setStretchLastSection(true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const auto &index) {
        d->itemCurrentChanged(index);
    });
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const auto &oldSel, const auto &newSel) {
        d->slotSelectionChanged(oldSel, newSel);
    });
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
    d->mDragExpandTimer.start(QApplication::startDragTime(), this);

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

    if (index.isValid()) { // popup not over empty space
        // check whether the index under the cursor is a collection or item
        const Item item = model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        popupName = (item.isValid() ? QStringLiteral("akonadi_itemview_contextmenu") : QStringLiteral("akonadi_collectionview_contextmenu"));
    }

    auto popup = static_cast<QMenu *>(d->mXmlGuiClient->factory()->container(popupName, d->mXmlGuiClient));
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
