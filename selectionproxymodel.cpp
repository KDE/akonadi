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
  SelectionProxyModelPrivate(SelectionProxyModel *model)
    : m_headerSet(EntityTreeModel::EntityTreeHeaders),
    q_ptr(model)
  {

  }
private:

  int m_headerSet;

  Q_DECLARE_PUBLIC(SelectionProxyModel)
  SelectionProxyModel *q_ptr;

};

}

SelectionProxyModel::SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
  : KSelectionProxyModel(selectionModel, parent), d_ptr(new SelectionProxyModelPrivate(this))
{

}

SelectionProxyModel::~SelectionProxyModel()
{
  delete d_ptr;
}

QVariant SelectionProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_D(const SelectionProxyModel);
  int adjustedRole;

  adjustedRole = role + ( Akonadi::EntityTreeModel::TerminalUserRole * d->m_headerSet );
  return sourceModel()->headerData(section, orientation, adjustedRole);
}

int SelectionProxyModel::headerSet() const
{
  Q_D(const SelectionProxyModel);
  return d->m_headerSet;
}

void SelectionProxyModel::setHeaderSet(int set)
{
  Q_D(SelectionProxyModel);
  d->m_headerSet = set;
}
