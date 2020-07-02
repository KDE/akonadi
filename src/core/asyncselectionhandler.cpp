/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "asyncselectionhandler_p.h"
#include "models/entitytreemodel.h"
#include "akonadicore_debug.h"

using namespace Akonadi;

AsyncSelectionHandler::AsyncSelectionHandler(QAbstractItemModel *model, QObject *parent)
    : QObject(parent)
    , mModel(model)
{
    Q_ASSERT(mModel);

    connect(mModel, &QAbstractItemModel::rowsInserted, this, &AsyncSelectionHandler::rowsInserted);
}

AsyncSelectionHandler::~AsyncSelectionHandler()
{
}

bool AsyncSelectionHandler::scanSubTree(const QModelIndex &index, bool searchForItem)
{
    if (searchForItem) {
        const Item::Id id = index.data(EntityTreeModel::ItemIdRole).toLongLong();

        if (mItem.id() == id) {
            Q_EMIT itemAvailable(index);
            return true;
        }
    } else {
        const Collection::Id id = index.data(EntityTreeModel::CollectionIdRole).toLongLong();

        if (mCollection.id() == id) {
            Q_EMIT collectionAvailable(index);
            return true;
        }
    }

    for (int row = 0; row < mModel->rowCount(index); ++row) {
        const QModelIndex childIndex = mModel->index(row, 0, index);
        //This should not normally happen, but if it does we end up in an endless loop
        if (!childIndex.isValid()) {
            qCWarning(AKONADICORE_LOG) << "Invalid child detected: " << index.data().toString();
            Q_ASSERT(false);
            return false;
        }
        if (scanSubTree(childIndex, searchForItem)) {
            return true;
        }
    }

    return false;
}

void AsyncSelectionHandler::waitForCollection(const Collection &collection)
{
    mCollection = collection;

    scanSubTree(QModelIndex(), false);
}

void AsyncSelectionHandler::waitForItem(const Item &item)
{
    mItem = item;

    scanSubTree(QModelIndex(), true);
}

void AsyncSelectionHandler::rowsInserted(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i <= end; ++i) {
        scanSubTree(mModel->index(i, 0, parent), false);
        scanSubTree(mModel->index(i, 0, parent), true);
    }
}

#include "moc_asyncselectionhandler_p.cpp"
