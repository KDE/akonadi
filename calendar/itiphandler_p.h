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

#ifndef AKONADI_CALENDAR_ITIP_HANDLER_P_H
#define AKONADI_CALENDAR_ITIP_HANDLER_P_H

#include "calendarbase.h"
#include "mailscheduler_p.h"
#include "itiphandler.h"
#include "itiphandlerhelper_p.h"

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
    OperationPublishInformation,
    OperationSendAsICalendar
};

class ITIPHandler::Private : public QObject
{
    Q_OBJECT
public:
    Private(ITIPHandlerComponentFactory *factory, ITIPHandler *q);

    void finishProcessiTIPMessage(Akonadi::MailScheduler::Result, const QString &errorMessage);
    void finishSendiTIPMessage(Akonadi::MailScheduler::Result, const QString &errorMessage);
    void finishPublishInformation(Akonadi::MailScheduler::Result, const QString &errorMessage);

    /**
     * Returns the calendar.
     * Creates a new one, if none is set.
     */
    CalendarBase::Ptr calendar();
    bool isLoaded(); // don't make const

    Invitation m_queuedInvitation;
    bool m_calendarLoadError;
    CalendarBase::Ptr m_calendar;
    ITIPHandlerComponentFactory *m_factory;
    MailScheduler *m_scheduler;
    KCalCore::Incidence::Ptr m_incidence;
    KCalCore::iTIPMethod m_method; // METHOD field of ical rfc of incoming invitation
    ITIPHandlerHelper *m_helper;
    Operation m_currentOperation;
    QPointer<QWidget> m_parentWidget; // To be used for KMessageBoxes
    GroupwareUiDelegate *m_uiDelegate;
    bool m_showDialogsOnError;
    ITIPHandler *const q;

private Q_SLOTS:
    void finishSendAsICalendar(Akonadi::MailScheduler::Result, const QString &errorMessage);
    void onLoadFinished(bool success, const QString &errorMessage);
    void onSchedulerFinished(Akonadi::Scheduler::Result, const QString &errorMessage);
    void onHelperFinished(Akonadi::ITIPHandlerHelper::SendResult result,
                          const QString &errorMessage);

    void onCounterProposalDelegateFinished(bool success, const QString &errorMessage);
};

}
#endif
