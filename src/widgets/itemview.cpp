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

#include "controlgui.h"
#include "entitytreemodel.h"

#include <KXMLGUIFactory>
#include <KXMLGUIClient>
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
    Private(ItemView *parent)
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
    ItemView *mParent = nullptr;
};

void ItemView::Private::init()
{
    mParent->setRootIsDecorated(false);

    mParent->header()->setSectionsClickable(true);
    mParent->header()->setStretchLastSection(true);

    mParent->connect(mParent, SIGNAL(activated(QModelIndex)), mParent, SLOT(itemActivated(QModelIndex)));
    mParent->connect(mParent, SIGNAL(clicked(QModelIndex)), mParent, SLOT(itemClicked(QModelIndex)));
    mParent->connect(mParent, QOverload<const QModelIndex &>::of(&QAbstractItemView::doubleClicked), mParent, [this](const QModelIndex &index) { itemDoubleClicked(index); });

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
    QTreeView::setModel(model);

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(itemCurrentChanged(QModelIndex)));
}

void ItemView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!d->xmlGuiClient) {
        return;
    }
    QMenu *popup = static_cast<QMenu *>(d->xmlGuiClient->factory()->container(
                                            QStringLiteral("akonadi_itemview_contextmenu"), d->xmlGuiClient));
    if (popup) {
        popup->exec(event->globalPos());
    }
}

void ItemView::setXmlGuiClient(KXMLGUIClient *xmlGuiClient)
{
    d->xmlGuiClient = xmlGuiClient;
}

#include "moc_itemview.cpp"
