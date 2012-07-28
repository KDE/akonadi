/*
  Copyright (c) 2001,2004 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2010,2012 Sérgio Martins <iamsergio@gmail.com>

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
#ifndef AKONADI_CALENDAR_MAILSCHEDULER_H
#define AKONADI_CALENDAR_MAILSCHEDULER_H

#include "scheduler_p.h"
#include "fetchjobcalendar.h"

#include <KCalCore/Incidence>

#include <Akonadi/Item>

//TODO: include commits done in mailscheduler1

namespace KCalCore {
  class ICalFormat;
}

namespace KPIMIdentities {
  class IdentityManager;
}

namespace Akonadi {
  class Calendar;
/*
  This class implements the iTIP interface using the email interface specified
  as Mail.
*/
class MailScheduler : public Akonadi::Scheduler
{
  public:
    // TODO: protect against invalid calendars
    MailScheduler( const Akonadi::FetchJobCalendar::Ptr &,
                   bool bcc,
                   const QString &mailTransport,
                   QObject *parent = 0 );
    ~MailScheduler();

    /** reimp */
    void publish( const KCalCore::IncidenceBase::Ptr &incidence,
                  const QString &recipients );

    /** reimp */
    void performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                             KCalCore::iTIPMethod method );

    /** reimp */
    void performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                             KCalCore::iTIPMethod method,
                             const QString &recipients );

    /** Returns the directory where the free-busy information is stored */
    /** reimp*/ QString freeBusyDir() const;

    /** Accepts a counter proposal */
    void acceptCounterProposal( const KCalCore::Incidence::Ptr &incidence );

private:
    //@cond PRIVATE
    Q_DISABLE_COPY( MailScheduler )
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
