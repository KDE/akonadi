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

#include "akonadi_export.h"

#include <kselectionproxymodel.h>

#include <QtGui/QItemSelectionModel>

namespace Akonadi
{

class SelectionProxyModelPrivate;

/**
 * @short A proxy model that provides data depending on the selection of a view.
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADI_EXPORT SelectionProxyModel : public KSelectionProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new selection proxy model.
     *
     * @param selectionModel The selection model to work on.
     * @param parent The parent object.
     */
    explicit SelectionProxyModel( QItemSelectionModel *selectionModel, QObject *parent = 0 );

    /**
     * Destroys the selection proxy model.
     */
    virtual ~SelectionProxyModel();

    /**
     * Sets the header @p set that shall be used by the proxy model.
     *
     * \s EntityTreeModel::HeaderGroup
     */
    void setHeaderSet( int set );

    /**
     * Returns the header set used by the proxy model.
     */
    int headerSet() const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE( SelectionProxyModel )
    SelectionProxyModelPrivate *d_ptr;
    //@endcond
};

}

#endif
