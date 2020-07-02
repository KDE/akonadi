/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionview.h"

#include "collection.h"
#include "controlgui.h"
#include "entitytreemodel.h"
#include "akonadiwidgets_debug.h"

#include <KLocalizedString>
#include <KXMLGUIFactory>
#include <kxmlguiwindow.h>

#include <QAction>
#include <QMimeData>
#include <QTimer>
#include <QUrlQuery>
#include <QApplication>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QMenu>
#include <QUrl>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN CollectionView::Private
{
public:
    explicit Private(CollectionView *parent)
        : mParent(parent)
    {
    }

    void init();
    void dragExpand();
    void itemClicked(const QModelIndex &index);
    void itemCurrentChanged(const QModelIndex &index);
    bool hasParent(const QModelIndex &idx, Collection::Id parentId) const;

    CollectionView *mParent = nullptr;
    QModelIndex dragOverIndex;
    QTimer dragExpandTimer;

    KXMLGUIClient *xmlGuiClient = nullptr;
};

void CollectionView::Private::init()
{
    mParent->header()->setSectionsClickable(true);
    mParent->header()->setStretchLastSection(false);

    mParent->setSortingEnabled(true);
    mParent->sortByColumn(0, Qt::AscendingOrder);
    mParent->setEditTriggers(QAbstractItemView::EditKeyPressed);
    mParent->setAcceptDrops(true);
    mParent->setDropIndicatorShown(true);
    mParent->setDragDropMode(DragDrop);
    mParent->setDragEnabled(true);

    dragExpandTimer.setSingleShot(true);
    mParent->connect(&dragExpandTimer, &QTimer::timeout, mParent, [this]() { dragExpand(); });

    mParent->connect(mParent, &QAbstractItemView::clicked, mParent, [this](const QModelIndex &mi) { itemClicked(mi); });

    ControlGui::widgetNeedsAkonadi(mParent);
}

bool CollectionView::Private::hasParent(const QModelIndex &idx, Collection::Id parentId) const
{
    QModelIndex idx2 = idx;
    while (idx2.isValid()) {
        if (mParent->model()->data(idx2, EntityTreeModel::CollectionIdRole).toLongLong() == parentId) {
            return true;
        }

        idx2 = idx2.parent();
    }
    return false;
}

void CollectionView::Private::dragExpand()
{
    mParent->setExpanded(dragOverIndex, true);
    dragOverIndex = QModelIndex();
}

void CollectionView::Private::itemClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (!collection.isValid()) {
        return;
    }

    Q_EMIT mParent->clicked(collection);
}

void CollectionView::Private::itemCurrentChanged(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (!collection.isValid()) {
        return;
    }

    Q_EMIT mParent->currentChanged(collection);
}

CollectionView::CollectionView(QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    d->init();
}

CollectionView::CollectionView(KXMLGUIClient *xmlGuiClient, QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    d->xmlGuiClient = xmlGuiClient;
    d->init();
}

CollectionView::~CollectionView()
{
    delete d;
}

void CollectionView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    header()->setStretchLastSection(true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &mi) { d->itemCurrentChanged(mi); });
}

void CollectionView::dragMoveEvent(QDragMoveEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (d->dragOverIndex != index) {
        d->dragExpandTimer.stop();
        if (index.isValid() && !isExpanded(index) && itemsExpandable()) {
            d->dragExpandTimer.start(QApplication::startDragTime());
            d->dragOverIndex = index;
        }
    }

    // Check if the collection under the cursor accepts this data type
    const QStringList supportedContentTypes = model()->data(index, EntityTreeModel::CollectionRole).value<Collection>().contentMimeTypes();
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData) {
        return;
    }
    const QList<QUrl> urls = mimeData->urls();
    for (const QUrl &url : urls) {

        const Collection collection = Collection::fromUrl(url);
        if (collection.isValid()) {
            if (!supportedContentTypes.contains(QLatin1String("inode/directory"))) {
                break;
            }

            // Check if we don't try to drop on one of the children
            if (d->hasParent(index, collection.id())) {
                break;
            }
        } else {
            const QList<QPair<QString, QString> > query = QUrlQuery(url).queryItems();
            const int numberOfQuery(query.count());
            for (int i = 0; i < numberOfQuery; ++i) {
                if (query.at(i).first == QLatin1String("type")) {
                    const QString type = query.at(i).second;
                    if (!supportedContentTypes.contains(type)) {
                        break;
                    }
                }
            }
        }

        QTreeView::dragMoveEvent(event);
        return;
    }

    event->setDropAction(Qt::IgnoreAction);
}

void CollectionView::dragLeaveEvent(QDragLeaveEvent *event)
{
    d->dragExpandTimer.stop();
    d->dragOverIndex = QModelIndex();
    QTreeView::dragLeaveEvent(event);
}

void CollectionView::dropEvent(QDropEvent *event)
{
    d->dragExpandTimer.stop();
    d->dragOverIndex = QModelIndex();

    // open a context menu offering different drop actions (move, copy and cancel)
    // TODO If possible, hide non available actions ...
    QMenu popup(this);
    QAction *moveDropAction = popup.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("&Move here"));
    QAction *copyDropAction = popup.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("&Copy here"));
    popup.addSeparator();
    popup.addAction(QIcon::fromTheme(QStringLiteral("process-stop")), i18n("Cancel"));

    QAction *activatedAction = popup.exec(QCursor::pos());
    if (activatedAction == moveDropAction) {
        event->setDropAction(Qt::MoveAction);
    } else if (activatedAction == copyDropAction) {
        event->setDropAction(Qt::CopyAction);
    } else {
        return;
    }

    QTreeView::dropEvent(event);
}

void CollectionView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!d->xmlGuiClient) {
        return;
    }
    QMenu *popup = static_cast<QMenu *>(d->xmlGuiClient->factory()->container(
                                            QStringLiteral("akonadi_collectionview_contextmenu"), d->xmlGuiClient));
    if (popup) {
        popup->exec(event->globalPos());
    }
}

void CollectionView::setXmlGuiClient(KXMLGUIClient *xmlGuiClient)
{
    d->xmlGuiClient = xmlGuiClient;
}

#include "moc_collectionview.cpp"
