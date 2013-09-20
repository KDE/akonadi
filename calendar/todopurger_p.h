/*
   Copyright (C) 2013 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef TODOPURGER_P_H
#define TODOPURGER_P_H

#include "todopurger.h"
#include "incidencechanger.h"

#include <QString>
#include <QObject>

namespace Akonadi {

class IncidenceChanger;

class TodoPurger::Private : public QObject {
    Q_OBJECT
public:
    Private(TodoPurger *q);
    IncidenceChanger *m_changer;
    QString m_lastError;
    CalendarBase::Ptr m_calendar;
    int m_currentChangeId;
    int m_ignoredItems;
    bool m_calendarOwnership; // If false it's not ours.

    void deleteTodos();
    bool treeIsDeletable(const KCalCore::Todo::Ptr &todo);

public Q_SLOTS:
    void onCalendarLoaded(bool success, const QString &message);
    void onItemsDeleted(int changeId, const QVector<Akonadi::Item::Id> &deletedItems, Akonadi::IncidenceChanger::ResultCode, const QString &message);
private:
    TodoPurger *const q;
};

}


#endif
