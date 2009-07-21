/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_FAVORITECOLLECTIONSMODEL_H
#define AKONADI_FAVORITECOLLECTIONSMODEL_H

#include "selectionproxymodel.h"

#include "akonadi_next_export.h"

#include <akonadi/collection.h>

namespace Akonadi {

class EntityTreeModel;

class AKONADI_NEXT_EXPORT FavoriteCollectionsModel : public SelectionProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new model listing a chosen set of favorite collections.
     *
     * @param parent The parent object.
     */
    explicit FavoriteCollectionsModel( EntityTreeModel *source, QObject *parent = 0 );

    /**
     * Destroys the favorite collections model.
     */
    virtual ~FavoriteCollectionsModel();

  public Q_SLOTS:
    void setCollections( const Collection::List &collections );
    void addCollection( const Collection &collection );
    void removeCollection( const Collection &collection );
    void setFavoriteLabel( const Collection &collection, const QString &label );

  public:
    Collection::List collections() const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  private:
    using SelectionProxyModel::setSourceModel;
    using SelectionProxyModel::setOmitChildren;
    using SelectionProxyModel::setOmitDescendants;
    using SelectionProxyModel::setStartWithChildTrees;
    using SelectionProxyModel::setIncludeAllSelected;

    Q_PRIVATE_SLOT( d, void clearAndUpdateSelection() )
    Q_PRIVATE_SLOT( d, void updateSelection() )

    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
