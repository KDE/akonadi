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

#include "akonadi_export.h"

#include <akonadi/selectionproxymodel.h>

#include <akonadi/collection.h>

namespace Akonadi {

class EntityTreeModel;

/**
 * @short A model that lists a set of favorite collections.
 *
 * @author Kevin Ottens <ervin@kde.org>
 * @since 4.4
 */
class AKONADI_EXPORT FavoriteCollectionsModel : public Akonadi::SelectionProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new model listing a chosen set of favorite collections.
     *
     * @param parent The parent object.
     */
    explicit FavoriteCollectionsModel( QAbstractItemModel *source, QObject *parent = 0 );

    /**
     * Destroys the favorite collections model.
     */
    virtual ~FavoriteCollectionsModel();

    /**
     * Returns the list of favorite collections.
     */
    Collection::List collections() const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  public Q_SLOTS:
    void setCollections( const Collection::List &collections );
    void addCollection( const Collection &collection );
    void removeCollection( const Collection &collection );
    void setFavoriteLabel( const Collection &collection, const QString &label );

  private:
    using KSelectionProxyModel::setSourceModel;

    Q_PRIVATE_SLOT( d, void clearAndUpdateSelection() )
    Q_PRIVATE_SLOT( d, void updateSelection() )

    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
