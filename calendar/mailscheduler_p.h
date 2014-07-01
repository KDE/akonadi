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
#ifndef AKONADI_CALENDAR_MAILSCHEDULER_P_H
#define AKONADI_CALENDAR_MAILSCHEDULER_P_H

#include "scheduler_p.h"
#include "mailclient_p.h"

#include <kcalcore/incidence.h>
#include <akonadi/item.h>

namespace KCalCore {
class ICalFormat;
}

namespace KPIMIdentities {
class IdentityManager;
}

namespace Akonadi {
/*
  This class implements the iTIP interface using the email interface specified
  as Mail.
*/
class MailScheduler : public Akonadi::Scheduler
{
    Q_OBJECT
public:

    /**
     * @param calendar Must be a valid and loaded calendar.
     */
    explicit MailScheduler(ITIPHandlerComponentFactory *factory, QObject *parent = 0);
    ~MailScheduler();

    /** reimp */
    void publish(const KCalCore::IncidenceBase::Ptr &incidence,
                 const QString &recipients);

    /** reimp */
    void performTransaction(const KCalCore::IncidenceBase::Ptr &incidence,
                            KCalCore::iTIPMethod method);

    /** reimp */
    void performTransaction(const KCalCore::IncidenceBase::Ptr &incidence,
                            KCalCore::iTIPMethod method,
                            const QString &recipients);


    /** Returns the directory where the free-busy information is stored */
    /** reimp*/ QString freeBusyDir() const;

    /**
     * Accepts a counter proposal.
     * @param incidence A non-null incidence.
     * @param calendar A loaded calendar. Try not to use an ETMCalendar here, due to it's
     *                 async loading.
     */
    void acceptCounterProposal(const KCalCore::Incidence::Ptr &incidence,
                               const Akonadi::CalendarBase::Ptr &calendar);

private Q_SLOTS:
    /**
     * @brief onMailerFinished Handles the result of the MailClient operation.
     * @param result Error code.
     * @param errorMsg Error message if @p result is not success.
     */
    void onMailerFinished(Akonadi::MailClient::Result result, const QString &errorMsg);

private:
    //@cond PRIVATE
    Q_DISABLE_COPY(MailScheduler)
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
