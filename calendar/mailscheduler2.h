/*
  Copyright (c) 2001,2004 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2010 Sérgio Martins <iamsergio@gmail.com>

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

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef CALENDARSUPPORT_MAILSCHEDULER2_H
#define CALENDARSUPPORT_MAILSCHEDULER2_H

#include "calendarsupport_export.h"
#include "scheduler.h"
#include "incidencechanger.h" // TODO: remove me
#include "fetchjobcalendar.h"

#include <KCalCore/Incidence>
#include <KCalCore/ScheduleMessage>

#include <Akonadi/Item>

//include commits done in mailscheduler1

namespace KCalCore {
  class ICalFormat;
}

namespace CalendarSupport {
  class Calendar;
  class IncidenceChanger;
/*
  This class implements the iTIP interface using the email interface specified
  as Mail.
*/
class CALENDARSUPPORT_EXPORT MailScheduler2 : public CalendarSupport::Scheduler
{
  public:
    // TODO: protect against invalid calendars
    MailScheduler2( const CalendarSupport::FetchJobCalendar::Ptr & );
    ~MailScheduler2();

    CallId publish( const KCalCore::IncidenceBase::Ptr &incidence,
                    const QString &recipients );

    CallId performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                               KCalCore::iTIPMethod method );

    CallId performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                               KCalCore::iTIPMethod method,
                               const QString &recipients );

    /** Returns the directory where the free-busy information is stored */
    virtual QString freeBusyDir() const;

    /** Accepts a counter proposal */
    CallId acceptCounterProposal( const KCalCore::Incidence::Ptr &incidence );

  private Q_SLOTS:

    void modifyFinished( int changeId, const Akonadi::Item &item,
                         IncidenceChanger::ResultCode changerResultCode, const QString &errorMessage );

    void createFinished( int changeId, const Akonadi::Item &item,
                         IncidenceChanger::ResultCode changerResultCode, const QString &errorMessage );


    //@cond private
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
