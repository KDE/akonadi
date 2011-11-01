/*
  Copyright (c) 2011 Sérgio Martins <iamsergio@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef AKONADI_ETMCALENDAR_P_H
#define AKONADI_ETMCALENDAR_P_H

#include "etmcalendar.h"
#include "akonadicalendar_p.h"
#include "incidencechanger.h"
#include "collectionselection_p.h"

#include <QVector>
#include <QModelIndex>

class QAbstractItemModel;
class KCheckableProxyModel;

namespace Akonadi {

class EntityMimeTypeFilterModel;
  
class ETMCalendarPrivate : public AkonadiCalendarPrivate
{
  Q_OBJECT
  public:

    explicit ETMCalendarPrivate( ETMCalendar *qq );
    ~ETMCalendarPrivate();

    void init();
    void setupFilteredETM();
    void loadFromETM();

  public Q_SLOTS:
    Akonadi::Item::List itemsFromModel( const QAbstractItemModel *model,
                                        const QModelIndex &parentIndex = QModelIndex(),
                                        int start = 0,
                                        int end = -1 );

    Akonadi::Collection::List collectionsFromModel( const QAbstractItemModel *model,
                                                    const QModelIndex &parentIndex = QModelIndex(),
                                                    int start = 0,
                                                    int end = -1 );
    void clear();
    Akonadi::Item itemFromIndex( const QModelIndex &idx );
    Akonadi::Collection collectionFromIndex( const QModelIndex &index );
    void itemsAdded( const Akonadi::Item::List &items );
    void itemsRemoved( const Akonadi::Item::List &items );

    void onRowsInserted( const QModelIndex &index, int start, int end );
    void onRowsAboutToBeRemoved( const QModelIndex &index, int start, int end );
    void onDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );
    void onRowsMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                      const QModelIndex &destinationParent, int destinationRow );

    void onLayoutChangedInFilteredModel();
    void onModelResetInFilteredModel();
    void onDataChangedInFilteredModel( const QModelIndex &topLeft, const QModelIndex &bottomRight );
    void onRowsInsertedInFilteredModel( const QModelIndex &index, int start, int end );
    void onRowsAboutToBeRemovedInFilteredModel( const QModelIndex &index, int start, int end );

  public:
    QAbstractItemModel *mETM;
    Akonadi::EntityMimeTypeFilterModel *mFilteredETM;

    // akonadi id to collections
    QHash<Akonadi::Entity::Id, Akonadi::Collection> mCollectionMap;
    KCheckableProxyModel *mCheckableProxyModel;
    Akonadi::CollectionSelection *mCollectionSelection;
  private:
    ETMCalendar *const q;
};

}

#endif
