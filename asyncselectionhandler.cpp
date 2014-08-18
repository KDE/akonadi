/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "asyncselectionhandler_p.h"

#include <akonadi/entitytreemodel.h>

using namespace Akonadi;

AsyncSelectionHandler::AsyncSelectionHandler(QAbstractItemModel *model, QObject *parent)
    : QObject(parent)
    , mModel(model)
{
    Q_ASSERT(mModel);

    connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(rowsInserted(QModelIndex,int,int)));
}

AsyncSelectionHandler::~AsyncSelectionHandler()
{
}

bool AsyncSelectionHandler::scanSubTree(const QModelIndex &index, bool searchForItem)
{
    if (searchForItem) {
        const Item::Id id = index.data(EntityTreeModel::ItemIdRole).toLongLong();

        if (mItem.id() == id) {
            emit itemAvailable(index);
            return true;
        }
    } else {
        const Collection::Id id = index.data(EntityTreeModel::CollectionIdRole).toLongLong();

        if (mCollection.id() == id) {
            emit collectionAvailable(index);
            return true;
        }
    }

    for (int row = 0; row < mModel->rowCount(index); ++row) {
        const QModelIndex childIndex = mModel->index(row, 0, index);
        //This should not normally happen, but if it does we end up in an endless loop
        if (!childIndex.isValid()) {
            kWarning() << "Invalid child detected: " << index.data().toString();
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
