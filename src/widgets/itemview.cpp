/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemview.h"

#include "controlgui.h"
#include "entitytreemodel.h"

#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN ItemView::Private
{
public:
    explicit Private(ItemView *parent)
        : mParent(parent)
    {
    }

    void init();
    void itemActivated(const QModelIndex &index);
    void itemCurrentChanged(const QModelIndex &index);
    void itemClicked(const QModelIndex &index);
    void itemDoubleClicked(const QModelIndex &index);

    Item itemForIndex(const QModelIndex &index);

    KXMLGUIClient *xmlGuiClient = nullptr;

private:
    ItemView *const mParent;
};

void ItemView::Private::init()
{
    mParent->setRootIsDecorated(false);

    mParent->header()->setSectionsClickable(true);
    mParent->header()->setStretchLastSection(true);

    mParent->connect(mParent, &QAbstractItemView::activated, mParent, [this](const auto &index) {
        itemActivated(index);
    });
    mParent->connect(mParent, &QAbstractItemView::clicked, mParent, [this](const auto &index) {
        itemClicked(index);
    });
    mParent->connect(mParent, &QAbstractItemView::doubleClicked, [this](const auto &index) {
        itemDoubleClicked(index);
    });

    ControlGui::widgetNeedsAkonadi(mParent);
}

Item ItemView::Private::itemForIndex(const QModelIndex &index)
{
    if (!index.isValid()) {
        return Item();
    }

    return mParent->model()->data(index, EntityTreeModel::ItemRole).value<Item>();
}

void ItemView::Private::itemActivated(const QModelIndex &index)
{
    const Item item = itemForIndex(index);

    if (!item.isValid()) {
        return;
    }

    Q_EMIT mParent->activated(item);
}

void ItemView::Private::itemCurrentChanged(const QModelIndex &index)
{
    const Item item = itemForIndex(index);

    if (!item.isValid()) {
        return;
    }

    Q_EMIT mParent->currentChanged(item);
}

void ItemView::Private::itemClicked(const QModelIndex &index)
{
    const Item item = itemForIndex(index);

    if (!item.isValid()) {
        return;
    }

    Q_EMIT mParent->clicked(item);
}

void ItemView::Private::itemDoubleClicked(const QModelIndex &index)
{
    const Item item = itemForIndex(index);

    if (!item.isValid()) {
        return;
    }

    Q_EMIT mParent->doubleClicked(item);
}

ItemView::ItemView(QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    d->init();
}

ItemView::ItemView(KXMLGUIClient *xmlGuiClient, QWidget *parent)
    : QTreeView(parent)
    , d(new Private(this))
{
    d->xmlGuiClient = xmlGuiClient;
    d->init();
}

ItemView::~ItemView()
{
    delete d;
}

void ItemView::setModel(QAbstractItemModel *model)
{
    if (selectionModel()) {
        disconnect(selectionModel(), &QItemSelectionModel::currentChanged, this, nullptr);
    }

    QTreeView::setModel(model);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const auto &index) {
        d->itemCurrentChanged(index);
    });
}

void ItemView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!d->xmlGuiClient) {
        return;
    }
    QMenu *popup = static_cast<QMenu *>(d->xmlGuiClient->factory()->container(QStringLiteral("akonadi_itemview_contextmenu"), d->xmlGuiClient));
    if (popup) {
        popup->exec(event->globalPos());
    }
}

void ItemView::setXmlGuiClient(KXMLGUIClient *xmlGuiClient)
{
    d->xmlGuiClient = xmlGuiClient;
}

#include "moc_itemview.cpp"
