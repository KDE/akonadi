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

#ifndef AKONADI_DESCENDANTSPROXYMODEL_H
#define AKONADI_DESCENDANTSPROXYMODEL_H

#include "akonadi_export.h"

#include <kdescendantsproxymodel.h>

namespace Akonadi
{
class DescendantsProxyModelPrivate;

/**
 * @short A proxy model that flattens a tree-based model to a list model.
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADI_EXPORT DescendantsProxyModel : public KDescendantsProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new descendants proxy model.
     *
     * @param parent The parent object.
     */
    DescendantsProxyModel( QObject *parent = 0 );

    /**
     * Destroys the descendants proxy model.
     */
    virtual ~DescendantsProxyModel();

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

    // QAbstractProxyModel does not proxy all methods...
    virtual bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QMimeData* mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes() const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(DescendantsProxyModel)
    DescendantsProxyModelPrivate *d_ptr;
    //@endcond
};

}

#endif
