/*
  SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selectionproxymodel.h"

#include "entitytreemodel.h"

using namespace Akonadi;

namespace Akonadi
{

class SelectionProxyModelPrivate
{
public:
    explicit SelectionProxyModelPrivate(SelectionProxyModel *selectionProxyModel)
        : q_ptr(selectionProxyModel)
    {
        Q_Q(SelectionProxyModel);
        const auto indexes = q->sourceRootIndexes();
        for (const auto &rootIndex : indexes) {
            rootIndexAdded(rootIndex);
        }
    }
    ~SelectionProxyModelPrivate()
    {
        Q_Q(SelectionProxyModel);
        const auto indexes = q->sourceRootIndexes();
        for (const auto &rootIndex : indexes) {
            rootIndexAboutToBeRemoved(rootIndex);
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

} // namespace Akonadi

SelectionProxyModel::SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
    : KSelectionProxyModel(selectionModel, parent)
    , d_ptr(new SelectionProxyModelPrivate(this))
{
    connect(this, SIGNAL(rootIndexAdded(QModelIndex)), SLOT(rootIndexAdded(QModelIndex))); // clazy:exclude=old-style-connect
    connect(this, SIGNAL(rootIndexAboutToBeRemoved(QModelIndex)), SLOT(rootIndexAboutToBeRemoved(QModelIndex))); // clazy:exclude=old-style-connect
}

SelectionProxyModel::~SelectionProxyModel()
{
    delete d_ptr;
}

#include "moc_selectionproxymodel.cpp"
