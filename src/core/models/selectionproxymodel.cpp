/*
  Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "selectionproxymodel.h"

#include "entitytreemodel.h"

using namespace Akonadi;

namespace Akonadi
{

class SelectionProxyModelPrivate
{
public:
    SelectionProxyModelPrivate(SelectionProxyModel *selectionProxyModel)
        : q_ptr(selectionProxyModel)
    {
        Q_Q(SelectionProxyModel);
        foreach (const QModelIndex &rootIndex, q->sourceRootIndexes()) {
            rootIndexAdded(rootIndex);
        }
    }
    ~SelectionProxyModelPrivate()
    {
        Q_Q(SelectionProxyModel);
        foreach (const QModelIndex &idx, q->sourceRootIndexes()) {
            rootIndexAboutToBeRemoved(idx);
        }
    }

    /**
      Increases the refcount of the Collection in @p newRootIndex
    */
    void rootIndexAdded(const QModelIndex &newRootIndex)
    {
        Q_Q(SelectionProxyModel);
        // newRootIndex is already in the sourceModel.
        q->sourceModel()->setData(newRootIndex, QVariant(), EntityTreeModel::CollectionRefRole);
        q->sourceModel()->fetchMore(newRootIndex);
    }

    /**
      Decreases the refcount of the Collection in @p removedRootIndex
    */
    void rootIndexAboutToBeRemoved(const QModelIndex &removedRootIndex)
    {
        Q_Q(SelectionProxyModel);
        q->sourceModel()->setData(removedRootIndex, QVariant(), EntityTreeModel::CollectionDerefRole);
    }

    Q_DECLARE_PUBLIC(SelectionProxyModel)
    SelectionProxyModel *q_ptr;
};

}

SelectionProxyModel::SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
    : KSelectionProxyModel(selectionModel, parent)
    , d_ptr(new SelectionProxyModelPrivate(this))
{
    connect(this, SIGNAL(rootIndexAdded(QModelIndex)), SLOT(rootIndexAdded(QModelIndex)));
    connect(this, SIGNAL(rootIndexAboutToBeRemoved(QModelIndex)), SLOT(rootIndexAboutToBeRemoved(QModelIndex)));
}

SelectionProxyModel::~SelectionProxyModel()
{
    delete d_ptr;
}

#include "moc_selectionproxymodel.cpp"
