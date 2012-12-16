/*
  Copyright (c) 2009 KDAB
  Author: Frank Osterfeld <osterfeld@kde.org>

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

#ifndef AKONADI_CALENDARMODEL_P_H
#define AKONADI_CALENDARMODEL_P_H

#include <akonadi/entitytreemodel.h>

namespace Akonadi {

class CalendarModel : public Akonadi::EntityTreeModel
{
  Q_OBJECT
public:
  enum ItemColumn {
    Summary=0,
    Type,
    DateTimeStart,
    DateTimeEnd,
    DateTimeDue,
    PrimaryDate,
    Priority,
    PercentComplete,
    ItemColumnCount
  };

  enum CollectionColumn {
    CollectionTitle=0,
    CollectionColumnCount
  };

  enum Role {
    SortRole=Akonadi::EntityTreeModel::UserRole,
    RecursRole
  };

  explicit CalendarModel( Akonadi::ChangeRecorder *monitor, QObject *parent = 0 );
  ~CalendarModel();

  /* reimp */
  QVariant entityData( const Akonadi::Item &item, int column, int role=Qt::DisplayRole ) const;

  /* reimp */
  QVariant entityData( const Akonadi::Collection &collection, int column,
                        int role=Qt::DisplayRole ) const;

  /* reimp */
  int entityColumnCount( EntityTreeModel::HeaderGroup headerSet ) const;

  /* reimp */
  QVariant entityHeaderData( int section, Qt::Orientation orientation, int role,
                              EntityTreeModel::HeaderGroup headerSet ) const;

private:
  class Private;
  Private *const d;
};

}

#endif

