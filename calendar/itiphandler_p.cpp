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

#include "itiphandler_p.h"
#include "fetchjobcalendar.h"
#include <kcalcore/incidence.h>
#include <KMessageBox>
#include <KLocalizedString>

using namespace Akonadi;

ITIPHandler::Private::Private(ITIPHandlerComponentFactory *factory, ITIPHandler *qq) : m_calendarLoadError(false)
    , m_factory(factory ? factory : new ITIPHandlerComponentFactory(this))
    , m_scheduler(new MailScheduler(m_factory, qq))
    , m_method(KCalCore::iTIPNoMethod)
    , m_helper(new ITIPHandlerHelper(m_factory))
    , m_currentOperation(OperationNone)
    , m_uiDelegate(0)
    , m_showDialogsOnError(true)
    , q(qq)
{
    m_helper->setParent(this);
    connect(m_scheduler, SIGNAL(transactionFinished(Akonadi::Scheduler::Result,QString)),
            SLOT(onSchedulerFinished(Akonadi::Scheduler::Result,QString)));

    connect(m_helper, SIGNAL(finished(Akonadi::ITIPHandlerHelper::SendResult,QString)),
            SLOT(onHelperFinished(Akonadi::ITIPHandlerHelper::SendResult,QString)));

    connect(m_helper, SIGNAL(sendIncidenceModifiedMessageFinished(ITIPHandlerHelper::SendResult,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)),
            SLOT(onHelperModifyDialogClosed(ITIPHandlerHelper::SendResult,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)));
}

void ITIPHandler::Private::onSchedulerFinished(Akonadi::Scheduler::Result result,
        const QString &errorMessage)
{
    if (m_currentOperation == OperationNone) {
        kFatal() << "Operation can't be none!" << result << errorMessage;
        return;
    }

    if (m_currentOperation == OperationProcessiTIPMessage) {
        m_currentOperation = OperationNone;
        finishProcessiTIPMessage(result, errorMessage);
    } else if (m_currentOperation == OperationSendiTIPMessage) {
        m_currentOperation = OperationNone;
        finishSendiTIPMessage(result, errorMessage);
    } else if (m_currentOperation == OperationPublishInformation) {
        m_currentOperation = OperationNone;
        finishPublishInformation(result, errorMessage);
    } else {
        Q_ASSERT(false);
        kError() << "Unknown operation" << m_currentOperation;
    }
}

void ITIPHandler::Private::onHelperFinished(Akonadi::ITIPHandlerHelper::SendResult result,
        const QString &errorMessage)
{
    const bool success = result == ITIPHandlerHelper::ResultSuccess;

    if (m_currentOperation == OperationProcessiTIPMessage) {
        MailScheduler::Result result2 = success ? MailScheduler::ResultSuccess : MailScheduler::ResultGenericError;
        finishProcessiTIPMessage(result2, i18n("Error: %1", errorMessage));
    } else {
        emit q->iTipMessageSent(success ? ResultSuccess : ResultError,
                                success ? QString() : i18n("Error: %1", errorMessage));
    }
}

void ITIPHandler::Private::onCounterProposalDelegateFinished(bool success, const QString &errorMessage)
{
    Q_UNUSED(success);
    Q_UNUSED(errorMessage);
    // This will be used when we make editing counter proposals async.
}

void ITIPHandler::Private::onLoadFinished(bool success, const QString &errorMessage)
{
    if (m_currentOperation == OperationProcessiTIPMessage) {
        if (success) {

            // Harmless hack, processiTIPMessage() asserts that there's not current operation running
            // to prevent users from calling it twice.
            m_currentOperation = OperationNone;
            q->processiTIPMessage(m_queuedInvitation.receiver,
                                  m_queuedInvitation.iCal,
                                  m_queuedInvitation.action);
        } else {
            emit q->iTipMessageSent(ResultError, i18n("Error loading calendar: %1", errorMessage));
        }
    } else if (m_currentOperation ==  OperationSendiTIPMessage) {
        q->sendiTIPMessage(m_queuedInvitation.method,
                           m_queuedInvitation.incidence,
                           m_parentWidget);
    } else if (!success) {   //TODO
        m_calendarLoadError = true;
    }
}

void ITIPHandler::Private::finishProcessiTIPMessage(Akonadi::MailScheduler::Result result,
        const QString &errorMessage)
{
    // Handle when user cancels on the collection selection dialog
    if (result == MailScheduler::ResultUserCancelled) {
        emit q->iTipMessageProcessed(ResultCancelled, QString());
        return;
    }

    const bool success = result == MailScheduler::ResultSuccess;

    if (m_method == KCalCore::iTIPCounter) {
        // Here we're processing a counter-proposal that someone sent us and we're the organizer.
        // TODO: Shouldn't there be a test to see if we're the organizer?
        if (success) {
            // send update to all attendees
            Q_ASSERT(m_incidence);
            m_helper->setDialogParent(m_parentWidget);
            m_helper->sendIncidenceModifiedMessage(KCalCore::iTIPRequest,
                        KCalCore::Incidence::Ptr(m_incidence->clone()),
                        false);
            m_incidence.clear();
            return;
        } else {
            //fall through
        }
    }

    emit q->iTipMessageProcessed(success ? ResultSuccess : ResultError,
                                 success ? QString() : i18n("Error: %1", errorMessage));
}

void ITIPHandler::Private::onHelperModifyDialogClosed(ITIPHandlerHelper::SendResult sendResult, KCalCore::iTIPMethod /*method*/, const KCalCore::Incidence::Ptr &incidence)
{
    if (sendResult == ITIPHandlerHelper::ResultNoSendingNeeded ||
        sendResult == ITIPHandlerHelper::ResultCanceled) {
            emit q->iTipMessageSent(ResultSuccess, QString());
    }
}


void ITIPHandler::Private::finishSendiTIPMessage(Akonadi::MailScheduler::Result result,
        const QString &errorMessage)
{
    if (result == Scheduler::ResultSuccess) {
        if (m_parentWidget) {
            KMessageBox::information(m_parentWidget,
                                     i18n("The groupware message for item '%1' "
                                          "was successfully sent.\nMethod: %2",
                                          m_queuedInvitation.incidence->summary(),
                                          KCalCore::ScheduleMessage::methodName(m_queuedInvitation.method)),
                                     i18n("Sending Free/Busy"),
                                     "FreeBusyPublishSuccess");
        }
        emit q->iTipMessageSent(ITIPHandler::ResultSuccess, QString());
    } else {
        const QString error = i18nc("Groupware message sending failed. "
                                    "%2 is request/reply/add/cancel/counter/etc.",
                                    "Unable to send the item '%1'.\nMethod: %2",
                                    m_queuedInvitation.incidence->summary(),
                                    KCalCore::ScheduleMessage::methodName(m_queuedInvitation.method));
        if (m_parentWidget) {
            KMessageBox::error(m_parentWidget, error);
        }
        kError() << "Groupware message sending failed." << error << errorMessage;
        emit q->iTipMessageSent(ITIPHandler::ResultError, error + errorMessage);
    }
}

void ITIPHandler::Private::finishPublishInformation(Akonadi::MailScheduler::Result result,
        const QString &errorMessage)
{
    if (result == Scheduler::ResultSuccess) {
        if (m_parentWidget) {
            KMessageBox::information(m_parentWidget,
                                     i18n("The item information was successfully sent."),
                                     i18n("Publishing"),
                                     "IncidencePublishSuccess");
        }
        emit q->informationPublished(ITIPHandler::ResultSuccess, QString());
    } else {
        const QString error = i18n("Unable to publish the item '%1'",
                                   m_queuedInvitation.incidence->summary());
        if (m_parentWidget) {
            KMessageBox::error(m_parentWidget, error);
        }
        kError() << "Publish failed." << error << errorMessage;
        emit q->informationPublished(ITIPHandler::ResultError, error + errorMessage);
    }
}

void ITIPHandler::Private::finishSendAsICalendar(Akonadi::MailScheduler::Result result,
        const QString &errorMessage)
{
    if (result == Scheduler::ResultSuccess) {
        if (m_parentWidget) {
            KMessageBox::information(m_parentWidget,
                                     i18n("The item information was successfully sent."),
                                     i18n("Forwarding"),
                                     "IncidenceForwardSuccess");
        }
        emit q->sentAsICalendar(ITIPHandler::ResultSuccess, QString());
    } else {
        if (m_parentWidget) {
            KMessageBox::error(m_parentWidget,
                               i18n("Unable to forward the item '%1'",
                                    m_queuedInvitation.incidence->summary()),
                               i18n("Forwarding Error"));
        }
        kError() << "Sent as iCalendar failed." << errorMessage;
        emit q->sentAsICalendar(ITIPHandler::ResultError, errorMessage);
    }

    sender()->deleteLater(); // Delete the mailer
}

CalendarBase::Ptr ITIPHandler::Private::calendar()
{
    if (!m_calendar) {
        FetchJobCalendar::Ptr fetchJobCalendar = FetchJobCalendar::Ptr(new FetchJobCalendar());
        connect(fetchJobCalendar.data(), SIGNAL(loadFinished(bool,QString)),
                SLOT(onLoadFinished(bool,QString)));

        m_calendar = fetchJobCalendar;
    }

    return m_calendar;
}

bool ITIPHandler::Private::isLoaded()
{
    FetchJobCalendar::Ptr fetchJobCalendar = calendar().dynamicCast<Akonadi::FetchJobCalendar>();
    if (fetchJobCalendar)
        return fetchJobCalendar->isLoaded();

    // If it's an ETMCalendar, set through setCalendar(), then it's already loaded, it's a requirement of setCalendar().
    // ETM doesn't have any way to check if it's already populated, so we have to require loaded calendars.
    return true;
}

