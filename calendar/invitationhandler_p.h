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

#ifndef AKONADI_CALENDAR_INVITATION_HANDLER_P_H
#define AKONADI_CALENDAR_INVITATION_HANDLER_P_H

#include "fetchjobcalendar.h"
#include "mailscheduler_p.h"
#include "invitationhandler.h"
#include "invitationhandlerhelper_p.h"

#include <kcalcore/schedulemessage.h>

#include <QObject>
#include <QString>

namespace Akonadi {
  
class InvitationHandler::Private : public QObject
{
  Q_OBJECT
public:
  Private( const FetchJobCalendar::Ptr &calendar, InvitationHandler *q );
  
  FetchJobCalendar::Ptr m_calendar;
  MailScheduler *m_scheduler;
  KCalCore::Incidence::Ptr m_incidence;
  KCalCore::iTIPMethod m_method;
  InvitationHandlerHelper *m_helper;
  InvitationHandler *q;
public Q_SLOTS:
  void onSchedulerFinished( Akonadi::MailScheduler::Result, const QString &errorMsg );
  void onHelperFinished( Akonadi::InvitationHandlerHelper::SendResult result,
                         const QString &errorMessage );
};
}
#endif
