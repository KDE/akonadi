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
 * Supports columns for everything in the CollectionStatistics for
 * the collections.
 * Additionally, it provides special Roles to access those statistics via
 * data().
 * With this model, automatic fetching of the CollectionStatistic is enabled
 * by default.
*/
class AKONADI_EXPORT CollectionStatisticsModel : public CollectionModel
{
  Q_OBJECT

  public:

    /**
     * Custom roles for the message collection model
     */
    enum CollectionStatisticsRole {

      /// Get the number of unread items in this collection
      CollectionStatisticsUnreadRole = CollectionViewUserRole,

      /// Get the number of items in this collection
      CollectionStatisticsTotalRole,

      /// Get the number of unread items in this collection and its children
      CollectionStatisticsUnreadRecursiveRole,

      /// Get the number of items in this collection and its children
      CollectionStatisticsTotalRecursiveRole,

      CollectionStatisticsUserRole = Qt::UserRole + 64
    };

    /**
      Create a new message collection model.
      @param parent The parent object.
    */
    explicit CollectionStatisticsModel( QObject *parent = 0 );

    /**
      Reimplemented from CollectionModel.
    */
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from CollectionModel.
    */
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  private:
    Q_DECLARE_PRIVATE( CollectionStatisticsModel )
};

}

#endif
