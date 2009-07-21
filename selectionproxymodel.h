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

#ifndef AKONADI_SELECTIONPROXYMODEL_H
#define AKONADI_SELECTIONPROXYMODEL_H

#include <QItemSelectionModel>

#include "akonadi_export.h"
#include <kselectionproxymodel.h>

namespace Akonadi
{

class SelectionProxyModelPrivate;

class AKONADI_EXPORT SelectionProxyModel : public KSelectionProxyModel
{
  Q_OBJECT
public:
  explicit SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent = 0 );

  virtual ~SelectionProxyModel();
  /**
   * Sets the header @p set that shall be used by the proxy.
   *
   * \s EntityTreeModel::HeaderGroup
   */
  void setHeaderSet( int set );

  /**
   * Returns the header set used by the proxy.
   */
  int headerSet() const;

  virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

private:
  Q_DECLARE_PRIVATE(SelectionProxyModel)
  //@cond PRIVATE
  SelectionProxyModelPrivate *d_ptr;
  //@endcond

};

}

#endif
