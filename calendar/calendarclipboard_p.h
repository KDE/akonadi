/*
   Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef _AKONADI_CALENDAR_CLIPBOARD_P_H
#define _AKONADI_CALENDAR_CLIPBOARD_P_H

#include "calendarclipboard.h"
#include "incidencechanger.h"
#include "calendarbase.h"
#include <kcalcore/incidence.h>
#include <QVector>
#include <QSet>

namespace KCalUtils {
class DndFactory;
}

namespace Akonadi {

class IncidenceChanger;

class CalendarClipboard::Private : public QObject {
    Q_OBJECT
public:
    Private(const Akonadi::CalendarBase::Ptr &,
            Akonadi::IncidenceChanger *changer,
            CalendarClipboard *qq);

    ~Private();

    /**
     * Returns all uids of incidenes having @p incidence has their parent (or grand parent, etc.)
     * @p incidence's uid is included in the list too.
     */
    void getIncidenceHierarchy(const KCalCore::Incidence::Ptr &incidence, QStringList &uids);

    /**
     * Copies all these incidences to clipboard. Deletes them.
     * This function assumes the caller already unparented all childs ( made them independent ).
     */
    void cut(const KCalCore::Incidence::List &incidences);

    /**
     * Overload.
     */
    void cut(const KCalCore::Incidence::Ptr &incidence);

    /**
     * All immediate childs of @p incidence are made independent.
     * Their RELATED-TO field is cleared.
     *
     * After it's done, signal makeChildsIndependentFinished() is emitted.
     */
    void makeChildsIndependent(const KCalCore::Incidence::Ptr &incidence);

public Q_SLOTS:
    void slotModifyFinished(int changeId, const Akonadi::Item &item,
                            Akonadi::IncidenceChanger::ResultCode resultCode,
                            const QString &errorMessage);

    void slotDeleteFinished(int changeId, const QVector<Akonadi::Item::Id> &ids,
                            Akonadi::IncidenceChanger::ResultCode result,
                            const QString &errorMessage);

public:

    Akonadi::CalendarBase::Ptr m_calendar;
    Akonadi::IncidenceChanger *m_changer;
    KCalUtils::DndFactory *m_dndfactory;
    bool m_abortCurrentOperation;
    QSet<int> m_pendingChangeIds;
    CalendarClipboard *const q;
};
}

#endif
