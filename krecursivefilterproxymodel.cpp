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

#include "krecursivefilterproxymodel.h"

#include <kdebug.h>

class RecursiveFilterProxyModelPrivate
{
  Q_DECLARE_PUBLIC(KRecursiveFilterProxyModel)
  KRecursiveFilterProxyModel *q_ptr;
public:
  RecursiveFilterProxyModelPrivate(KRecursiveFilterProxyModel *model)
    : q_ptr(model),
      ignoreRemove(false),
      invalidate(false)
  {
    qRegisterMetaType<QModelIndex>("QModelIndex");
  }

  void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right);
  void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
  void sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
  void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
  void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);

  bool ignoreRemove;
  bool invalidate;

};

void RecursiveFilterProxyModelPrivate::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right)
{
  Q_Q(KRecursiveFilterProxyModel);
  q->invalidate();
  return;

  QModelIndex parent = source_top_left.parent();
  int start = -1;
  int end = -1;
  for (int row = source_top_left.row(); row <= source_bottom_right.row(); ++row)
  {
    if (q->filterAcceptsRow(row, parent))
    {
      end = row;
      if (start == -1)
        start = row;
    } else {
      if (start != 1)
      {
        // Tell the QSFPM that the matching rows were inserted so that the filtering logic is processed.
        sourceRowsInserted(parent, start, end);
        start = -1;
        end = -1;
      }
    }
  }
  if (start == -1)
    return;

  // Tell the QSFPM that the matching rows were inserted so that the filtering logic is processed.
  sourceRowsInserted(parent, start, end);
}


void RecursiveFilterProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
{
  return;
}

void RecursiveFilterProxyModelPrivate::sourceRowsInserted(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);
  q->invalidate();
  return;

  if (!source_parent.isValid() || q->acceptRow(source_parent.row(), source_parent.parent()))
  {
    // If the parent is already in the model, we can just pass on the signal.
    QMetaObject::invokeMethod(q, "_q_sourceRowsInserted", Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
    return;
  }

  bool requireRow = false;
  for (int row = start; row <= end; ++row)
  {
    if (q->filterAcceptsRow(row, source_parent))
    {
      requireRow = true;
      break;
    }
  }
  if (!requireRow)
    // The row doesn't have descendants that match the filter. Filter it out.
    return;

  QModelIndex sourceAscendant = source_parent;
  int lastRow;
  // We got a matching descendant, so find the right place to insert the row.
  while(!q->acceptRow(lastRow, sourceAscendant) && sourceAscendant.isValid())
  {
    lastRow = sourceAscendant.row();
    sourceAscendant = sourceAscendant.parent();
  }

  QMetaObject::invokeMethod(q, "_q_sourceRowsInserted", Qt::DirectConnection,
      Q_ARG(QModelIndex, sourceAscendant),
      Q_ARG(int, lastRow),
      Q_ARG(int, lastRow));

  return;
}

void RecursiveFilterProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  return;

  bool accepted = false;
  for (int row = start; row < end; ++row)
  {
    if (q->filterAcceptsRow(row, source_parent))
    {
      accepted = true;
      break;
    }
  }
  if (!accepted)
  {
    ignoreRemove = true;
    return; // All removed rows are already filtered out. We don't care about the signal.
  }

  if (q->acceptRow(source_parent.row(), source_parent.parent()))
  {
    QMetaObject::invokeMethod(q, "_q_sourceRowsAboutToBeRemoved", Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
    return;
  }

  // Unfortunately we need to invalidate the model because although we could invoke the method that a parent was removed,
  // it was not actually removed, so we would get breakage.
  ignoreRemove = true;
  invalidate = true;
  return;
}

void RecursiveFilterProxyModelPrivate::sourceRowsRemoved(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);
  q->invalidate();
  return;

  if (!ignoreRemove)
    QMetaObject::invokeMethod(q, "_q_sourceRowsRemoved", Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));

  ignoreRemove = false;

  if (invalidate)
    q->invalidate();

  invalidate = false;
}

KRecursiveFilterProxyModel::KRecursiveFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent), d_ptr(new RecursiveFilterProxyModelPrivate(this))
{

}

KRecursiveFilterProxyModel::~KRecursiveFilterProxyModel()
{

}

bool KRecursiveFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
  QVariant da = sourceModel()->index(sourceRow, 0, sourceParent).data();

  if (acceptRow(sourceRow, sourceParent))
  {
//     kDebug() << "Direct accepted" << da;
    return true;
  }

  QModelIndex source_index = sourceModel()->index(sourceRow, 0, sourceParent);
  bool accepted = false;
  for (int row = 0 ; row < sourceModel()->rowCount(source_index); ++row)
    if (filterAcceptsRow(row, source_index))
      accepted = true; // Need to do this in a loop so that all siblings in a parent get processed, not just the first.

//   kDebug() << "#### Indirect accepted" << da;
  return accepted;
}

bool KRecursiveFilterProxyModel::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void KRecursiveFilterProxyModel::setSourceModel(QAbstractItemModel* model)
{
  Q_D(RecursiveFilterProxyModel);

  // Disconnect in the QSortFilterProxyModel. These methods will be invoked manually
  disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

  disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

  // Standard disconnect.
  disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

  disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

  QSortFilterProxyModel::setSourceModel(model);

  // Slots for manual invoking of QSortFilterProxyModel methods.
  connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

  connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
}

#include "krecursivefilterproxymodel.moc"
