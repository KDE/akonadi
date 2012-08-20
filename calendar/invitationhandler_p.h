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
#include <QPointer>

namespace Akonadi {

struct Invitation {
  QString receiver;
  QString iCal;
  QString action;
  KCalCore::iTIPMethod method;
  KCalCore::Incidence::Ptr incidence;
};

/**
 * Our API has two methods, one to process received invitations and another one to send them.
 * These operations are async and we don't want them to be called before the other has finished.
 * This enum is just to Q_ASSERT that.
 */
enum Operation {
  OperationNone,
  OperationProcessiTIPMessage,
  OperationSendiTIPMessage,
  OperationPublishInformation
};

class InvitationHandler::Private : public QObject
{
  Q_OBJECT
public:
  Private( InvitationHandler *q );

  void finishProcessiTIPMessage( Akonadi::MailScheduler::Result, const QString &errorMessage );
  void finishSendiTIPMessage( Akonadi::MailScheduler::Result, const QString &errorMessage );
  void finishPublishInformation( Akonadi::MailScheduler::Result, const QString &errorMessage );

  Invitation m_queuedInvitation;
  bool m_calendarLoadError;
  FetchJobCalendar::Ptr m_calendar;
  MailScheduler *m_scheduler;
  KCalCore::Incidence::Ptr m_incidence;
  KCalCore::iTIPMethod m_method;
  InvitationHandlerHelper *m_helper;
  Operation m_currentOperation;
  QPointer<QWidget> m_parentWidget; // To be used for KMessageBoxes
  InvitationHandler *q;

public Q_SLOTS:
  void onLoadFinished( bool success, const QString &errorMessage );
  void onSchedulerFinished( Akonadi::MailScheduler::Result, const QString &errorMessage );
  void onHelperFinished( Akonadi::InvitationHandlerHelper::SendResult result,
                         const QString &errorMessage );
};

}
#endif
