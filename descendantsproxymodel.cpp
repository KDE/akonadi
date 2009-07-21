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


#include "descendantsproxymodel.h"
#include "entitytreemodel.h"

using namespace Akonadi;

namespace Akonadi
{

class DescendantsProxyModelPrivate
{
public:
  DescendantsProxyModelPrivate(DescendantsProxyModel *model)
    : m_headerSet(EntityTreeModel::EntityTreeHeaders),
      q_ptr(model)
  {

  }
private:

  int m_headerSet;
  
  Q_DECLARE_PUBLIC(DescendantsProxyModel)
  DescendantsProxyModel *q_ptr;

};

}

DescendantsProxyModel::DescendantsProxyModel(QObject *parent)
  : KDescendantsProxyModel(parent), d_ptr(new DescendantsProxyModelPrivate(this))
{

}

DescendantsProxyModel::~DescendantsProxyModel()
{
  delete d_ptr;
}

QVariant DescendantsProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_D(const DescendantsProxyModel);
  int adjustedRole;

  adjustedRole = role + ( Akonadi::EntityTreeModel::TerminalUserRole * d->m_headerSet );
  return sourceModel()->headerData(section, orientation, adjustedRole);
}

int DescendantsProxyModel::headerSet() const
{
  Q_D(const DescendantsProxyModel);
  return d->m_headerSet;
}

void DescendantsProxyModel::setHeaderSet(int set)
{
  Q_D(DescendantsProxyModel);
  d->m_headerSet = set;
}

bool DescendantsProxyModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_ASSERT(sourceModel());
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
}

QMimeData* DescendantsProxyModel::mimeData( const QModelIndexList & indexes ) const
{
  Q_ASSERT(sourceModel());
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList DescendantsProxyModel::mimeTypes() const
{
  Q_ASSERT(sourceModel());
  return sourceModel()->mimeTypes();
}
