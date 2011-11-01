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

#ifndef CALENDARSUPPORT_AKONADICALENDAR_P_H
#define CALENDARSUPPORT_AKONADICALENDAR_P_H

#include "akonadicalendar.h"
#include "incidencechanger.h"

#include <QVector>

class KJob;

namespace Akonadi {

class AkonadiCalendarPrivate : public QObject
{
  Q_OBJECT
  public:

    explicit AkonadiCalendarPrivate( AkonadiCalendar *qq );
    ~AkonadiCalendarPrivate();

    void internalInsert( const Akonadi::Item &item );
    void internalRemove( const Akonadi::Item &item );

  public Q_SLOTS:
    void slotDeleteFinished( int changeId,
                             const QVector<Akonadi::Item::Id> &,
                             Akonadi::IncidenceChanger::ResultCode,
                             const QString &errorMessage );

    void slotCreateFinished( int changeId,
                             const Akonadi::Item &,
                             Akonadi::IncidenceChanger::ResultCode,
                             const QString &errorMessage );
    
  public:
    QHash<QString,Akonadi::Item::Id> mItemIdByUid;
    QHash<Akonadi::Item::Id, Akonadi::Item> mItemById;
    Akonadi::IncidenceChanger *mIncidenceChanger;
    QHash<QString,QStringList> mParentUidToChildrenUid;
    QWeakPointer<AkonadiCalendar> mWeakPointer;

  private:
    AkonadiCalendar *const q;
};

}

#endif
