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

// #include <KDE/SelectionProxyModel>

#include <kselectionproxymodel.h>

#include "akonadi_export.h"

namespace Akonadi
{

class SelectionProxyModelPrivate;

/**
  @brief A Proxy Model used to reference count selected Akonadi::Collection in a view

  This model extends KSelectionProxyModel implement reference counting on the Collections
  in an EntityTreeModel. This should only be used if the EntityTreeModel uses LazyPopulation.

  By selecting a Collection, its reference count will be increased. A Collection in the
  EntityTreeModel which has a reference count of zero will ignore all signals from Monitor
  about items changed, inserted, removed etc, which can be expensive operations.

  @since 4.4
*/
class AKONADI_EXPORT SelectionProxyModel : public KSelectionProxyModel
{
  Q_OBJECT
public:
  /**
    Constructor
  */
  explicit SelectionProxyModel( QItemSelectionModel *selectionModel, QObject *parent = 0 );

private:
  Q_PRIVATE_SLOT( d_func(), void rootIndexAdded( const QModelIndex & ) )
  Q_PRIVATE_SLOT( d_func(), void rootIndexAboutToBeRemoved( const QModelIndex & ) )

  Q_DECLARE_PRIVATE( SelectionProxyModel );
  SelectionProxyModelPrivate *d_ptr;
};

}

#endif
