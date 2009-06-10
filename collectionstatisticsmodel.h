/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONSTATISTICSMODEL_H
#define AKONADI_COLLECTIONSTATISTICSMODEL_H

#include "akonadi_export.h"
#include <akonadi/collectionmodel.h>

namespace Akonadi {

class CollectionStatisticsModelPrivate;

/**
 * @short A model that provides statistics for collections.
 *
 * This model extends the CollectionModel by providing additional
 * information about the collections, e.g. the number of items
 * in a collection, the number of read/unread items, or the total size
 * of the collection.
 *
 * Example:
 *
 * @code
 *
 * QTreeView *view = new QTreeView( this );
 *
 * Akonadi::CollectionStatisticsModel *model = new Akonadi::CollectionStatisticsModel( view );
 * view->setModel( model );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CollectionStatisticsModel : public CollectionModel
{
  Q_OBJECT

  public:

    /**
     * Describes the roles for the statistics collection model.
     */
    enum Roles {
      UnreadRole = CollectionModel::UserRole + 1, ///< The number of unread items in this collection.
      TotalRole,                                  ///< The number of items in this collection.
      StatisticsRole,                             ///< A statistics object of this collection.
      RecursiveUnreadRole,                        ///< The number of unread items in this collection and its children.
      RecursiveTotalRole,                         ///< The number of items in this collection and its children.
      RecursiveStatisticsRole,                    ///< A statistics object of this collection and its children.
      SizeRole,                                  ///< The total size of the items in this collection.
      RecursiveSizeRole,                         ///< The total size of the items in this collection and its children.
      UserRole = CollectionModel::UserRole + 42   ///< Role for user extensions.
    };

    /**
     * Creates a new collection statistics model.
     * @param parent The parent object.
     */
    explicit CollectionStatisticsModel( QObject *parent = 0 );

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  private:
    Q_DECLARE_PRIVATE( CollectionStatisticsModel )
};

}

#endif
