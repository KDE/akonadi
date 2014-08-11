/*
  Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

  Copyright (C) 2010 Bertjan Broeksema <broeksema@kde.org>
  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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

#include "itiphandler.h"
#include "itiphandler_p.h"
#include "itiphandlerhelper_p.h"
#include "calendarsettings.h"
#include "publishdialog.h"
#include "utils_p.h"
#include "mailclient_p.h"

#include <kcalcore/icalformat.h>
#include <kcalcore/incidence.h>
#include <kcalcore/schedulemessage.h>
#include <kcalcore/attendee.h>
#include <kcalutils/stringify.h>

#include <kpimidentities/identitymanager.h>
#include <mailtransport/messagequeuejob.h>
#include <mailtransport/transportmanager.h>

#include <KMessageBox>
#include <KLocalizedString>

using namespace Akonadi;

// async emittion
static void emitiTipMessageProcessed(ITIPHandler *handler,
                                     ITIPHandler::Result resultCode,
                                     const QString &errorString)
{
    QMetaObject::invokeMethod(handler, "iTipMessageProcessed", Qt::QueuedConnection,
                              Q_ARG(Akonadi::ITIPHandler::Result, resultCode),
                              Q_ARG(QString, errorString));
}

GroupwareUiDelegate::~GroupwareUiDelegate()
{
}

ITIPHandlerComponentFactory::ITIPHandlerComponentFactory(QObject *parent)
  : QObject(parent)
{
}

ITIPHandlerComponentFactory::~ITIPHandlerComponentFactory()
{
}

MailTransport::MessageQueueJob *ITIPHandlerComponentFactory::createMessageQueueJob(const KCalCore::IncidenceBase::Ptr &incidence, const KPIMIdentities::Identity &identity, QObject *parent)
{
    Q_UNUSED(incidence);
    Q_UNUSED(identity);
    return new MailTransport::MessageQueueJob(parent);
}

ITIPHandlerDialogDelegate *ITIPHandlerComponentFactory::createITIPHanderDialogDelegate(const KCalCore::Incidence::Ptr &incidence, KCalCore::iTIPMethod method, QWidget *parent)
{
    return new ITIPHandlerDialogDelegate(incidence, method, parent);
}

ITIPHandler::ITIPHandler(QObject *parent) : QObject(parent)
    , d(new Private(/*factory=*/0, this))
{
    qRegisterMetaType<Akonadi::ITIPHandler::Result>("Akonadi::ITIPHandler::Result");
}


ITIPHandler::ITIPHandler(ITIPHandlerComponentFactory *factory, QObject *parent) : QObject(parent)
    , d(new Private(factory, this))
{
    qRegisterMetaType<Akonadi::ITIPHandler::Result>("Akonadi::ITIPHandler::Result");
}

ITIPHandler::~ITIPHandler()
{
    delete d;
}

void ITIPHandler::processiTIPMessage(const QString &receiver,
                                     const QString &iCal,
                                     const QString &action)
{
    kDebug() << "processiTIPMessage called with receiver=" << receiver
             << "; action=" << action;

    if (d->m_currentOperation != OperationNone) {
        d->m_currentOperation = OperationNone;
        kFatal() << "There can't be an operation in progress!" << d->m_currentOperation;
        return;
    }

    d->m_currentOperation = OperationProcessiTIPMessage;

    if (!d->isLoaded()) {
        d->m_queuedInvitation.receiver = receiver;
        d->m_queuedInvitation.iCal     = iCal;
        d->m_queuedInvitation.action   = action;
        return;
    }

    if (d->m_calendarLoadError) {
        d->m_currentOperation = OperationNone;
        kError() << "Error loading calendar";
        emitiTipMessageProcessed(this, ResultError, i18n("Error loading calendar."));
        return;
    }

    KCalCore::ICalFormat format;
    KCalCore::ScheduleMessage::Ptr message = format.parseScheduleMessage(d->calendar(), iCal);

    if (!message) {
        const QString errorMessage = format.exception() ? i18n("Error message: %1", KCalUtils::Stringify::errorMessage(*format.exception()))
                                     : i18n("Unknown error while parsing iCal invitation");

        kError() << "Error parsing" << errorMessage;

        if (d->m_showDialogsOnError) {
            KMessageBox::detailedError(0, // mParent, TODO
                                       i18n("Error while processing an invitation or update."),
                                       errorMessage);
        }

        d->m_currentOperation = OperationNone;
        emitiTipMessageProcessed(this, ResultError, errorMessage);

        return;
    }

    d->m_method = static_cast<KCalCore::iTIPMethod>(message->method());

    KCalCore::ScheduleMessage::Status status = message->status();
    d->m_incidence = message->event().dynamicCast<KCalCore::Incidence>();
    if (!d->m_incidence) {
        kError() << "Invalid incidence";
        d->m_currentOperation = OperationNone;
        emitiTipMessageProcessed(this, ResultError, i18n("Invalid incidence"));
        return;
    }

    if (action.startsWith(QLatin1String("accepted")) ||
            action.startsWith(QLatin1String("tentative")) ||
            action.startsWith(QLatin1String("delegated")) ||
            action.startsWith(QLatin1String("counter"))) {
        // Find myself and set my status. This can't be done in the scheduler,
        // since this does not know the choice I made in the KMail bpf
        const KCalCore::Attendee::List attendees = d->m_incidence->attendees();
        foreach(KCalCore::Attendee::Ptr attendee, attendees) {
            if (attendee->email() == receiver) {
                if (action.startsWith(QLatin1String("accepted"))) {
                    attendee->setStatus(KCalCore::Attendee::Accepted);
                } else if (action.startsWith(QLatin1String("tentative"))) {
                    attendee->setStatus(KCalCore::Attendee::Tentative);
                } else if (CalendarSettings::self()->outlookCompatCounterProposals() &&
                           action.startsWith(QLatin1String("counter"))) {
                    attendee->setStatus(KCalCore::Attendee::Tentative);
                } else if (action.startsWith(QLatin1String("delegated"))) {
                    attendee->setStatus(KCalCore::Attendee::Delegated);
                }
                break;
            }
        }
        if (CalendarSettings::self()->outlookCompatCounterProposals() ||
                !action.startsWith(QLatin1String("counter"))) {
            d->m_scheduler->acceptTransaction(d->m_incidence, d->calendar(), d->m_method, status, receiver);
            return; // signal emitted in onSchedulerFinished().
        }
        //TODO: what happens here? we must emit a signal
    } else if (action.startsWith(QLatin1String("cancel"))) {
        // Delete the old incidence, if one is present
        KCalCore::Incidence::Ptr existingIncidence = d->calendar()->incidenceFromSchedulingID(d->m_incidence->uid());
        if (existingIncidence) {
            d->m_scheduler->acceptTransaction(d->m_incidence, d->calendar(), KCalCore::iTIPCancel, status, receiver);
            return; // signal emitted in onSchedulerFinished().
        } else {
            // We don't have the incidence, nothing to cancel
            kWarning() << "Couldn't find the incidence to delete.\n"
                       << "You deleted it previously or didn't even accept the invitation it in the first place.\n"
                       << "; uid=" << d->m_incidence->uid()
                       << "; identifier=" << d->m_incidence->instanceIdentifier()
                       << "; summary=" << d->m_incidence->summary();

            kDebug() << "\n Here's what we do have with such a summary:";
            KCalCore::Incidence::List knownIncidences = calendar()->incidences();
            foreach(const KCalCore::Incidence::Ptr &knownIncidence, knownIncidences) {
                if (knownIncidence->summary() == d->m_incidence->summary()) {
                    kDebug() << "\nFound: uid=" << knownIncidence->uid()
                             << "; identifier=" << knownIncidence->instanceIdentifier()
                             << "; schedulingId" << knownIncidence->schedulingID();
                }
            }

            emitiTipMessageProcessed(this, ResultSuccess, QString());
        }
    } else if (action.startsWith(QLatin1String("reply"))) {
        if (d->m_method != KCalCore::iTIPCounter) {
            d->m_scheduler->acceptTransaction(d->m_incidence, d->calendar(), d->m_method, status, QString());
        } else {
            d->m_scheduler->acceptCounterProposal(d->m_incidence, d->calendar());
        }
        return; // signal emitted in onSchedulerFinished().
    } else {
        kError() << "Unknown incoming action" << action;

        d->m_currentOperation = OperationNone;
        emitiTipMessageProcessed(this, ResultError, i18n("Invalid action: %1", action));
    }

    if (action.startsWith(QLatin1String("counter"))) {
        if (d->m_uiDelegate) {
            Akonadi::Item item;
            item.setMimeType(d->m_incidence->mimeType());
            item.setPayload(KCalCore::Incidence::Ptr(d->m_incidence->clone()));

            // TODO_KDE5: This blocks because m_uiDelegate is not a QObject and can't emit a finished()
            // signal. Make async in kde5
            d->m_uiDelegate->requestIncidenceEditor(item);
            KCalCore::Incidence::Ptr newIncidence;
            if (item.hasPayload<KCalCore::Incidence::Ptr>()) {
                newIncidence = item.payload<KCalCore::Incidence::Ptr>();
            }

            if (*newIncidence == *d->m_incidence) {
                emitiTipMessageProcessed(this, ResultCancelled, QString());
            } else {
                ITIPHandlerHelper::SendResult result = d->m_helper->sendCounterProposal(d->m_incidence, newIncidence);
                if (result != ITIPHandlerHelper::ResultSuccess) {
                    // It gives success in all paths, this never happens
                    emitiTipMessageProcessed(this, ResultError, i18n("Error sending counter proposal"));
                    Q_ASSERT(false);
                }
            }
        } else {
            // This should never happen
            kWarning() << "No UI delegate is set";
            emitiTipMessageProcessed(this, ResultError, i18n("Could not start editor to edit counter proposal"));
        }
    }
}

void ITIPHandler::sendiTIPMessage(KCalCore::iTIPMethod method,
                                  const KCalCore::Incidence::Ptr &incidence,
                                  QWidget *parentWidget)
{
    if (!incidence) {
        Q_ASSERT(false);
        kError() << "Invalid incidence";
        return;
    }

    d->m_queuedInvitation.method    = method;
    d->m_queuedInvitation.incidence = incidence;
    d->m_parentWidget = parentWidget;

    if (!d->isLoaded()) {
        // This method will be called again once the calendar is loaded.
        return;
    }

    Q_ASSERT(d->m_currentOperation == OperationNone);
    if (d->m_currentOperation != OperationNone) {
        kError() << "There can't be an operation in progress!" << d->m_currentOperation;
        return;
    }

    if (incidence->attendeeCount() == 0 && method != KCalCore::iTIPPublish) {
        if (d->m_showDialogsOnError) {
            KMessageBox::information(parentWidget,
                                     i18n("The item '%1' has no attendees. "
                                          "Therefore no groupware message will be sent.",
                                          incidence->summary()),
                                     i18n("Message Not Sent"),
                                     QLatin1String("ScheduleNoAttendees"));
        }

        return;
    }

    d->m_currentOperation = OperationSendiTIPMessage;

    KCalCore::Incidence *incidenceCopy = incidence->clone();
    incidenceCopy->registerObserver(0);
    incidenceCopy->clearAttendees();

    d->m_scheduler->performTransaction(incidence, method);
}

void ITIPHandler::publishInformation(const KCalCore::Incidence::Ptr &incidence,
                                     QWidget *parentWidget)
{
    Q_ASSERT(incidence);
    if (!incidence) {
        kError() << "Invalid incidence. Aborting.";
        return;
    }

    Q_ASSERT(d->m_currentOperation == OperationNone);
    if (d->m_currentOperation != OperationNone) {
        kError() << "There can't be an operation in progress!" << d->m_currentOperation;
        return;
    }

    d->m_queuedInvitation.incidence = incidence;
    d->m_parentWidget = parentWidget;

    d->m_currentOperation = OperationPublishInformation;

    QPointer<Akonadi::PublishDialog> publishdlg = new Akonadi::PublishDialog();
    if (incidence->attendeeCount() > 0) {
        KCalCore::Attendee::List attendees = incidence->attendees();
        KCalCore::Attendee::List::ConstIterator it;
        KCalCore::Attendee::List::ConstIterator end(attendees.constEnd());
        for (it = attendees.constBegin(); it != end; ++it) {
            publishdlg->addAttendee(*it);
        }
    }
    if (publishdlg->exec() == QDialog::Accepted && publishdlg)
        d->m_scheduler->publish(incidence, publishdlg->addresses());
    else
        emit informationPublished(ResultSuccess, QString());   // Canceled.
    delete publishdlg;
}

void ITIPHandler::sendAsICalendar(const KCalCore::Incidence::Ptr &originalIncidence,
                                  QWidget *parentWidget)
{
    Q_UNUSED(parentWidget);
    Q_ASSERT(originalIncidence);
    if (!originalIncidence) {
        kError() << "Invalid incidence";
        return;
    }

    // Clone so we can change organizer and recurid
    KCalCore::Incidence::Ptr incidence = KCalCore::Incidence::Ptr(originalIncidence->clone());

    KPIMIdentities::IdentityManager identityManager;

    QPointer<Akonadi::PublishDialog> publishdlg = new Akonadi::PublishDialog;
    if (publishdlg->exec() == QDialog::Accepted && publishdlg) {
        const QString recipients = publishdlg->addresses();
        if (incidence->organizer()->isEmpty()) {
            incidence->setOrganizer(KCalCore::Person::Ptr(
                                        new KCalCore::Person(Akonadi::CalendarUtils::fullName(),
                                                Akonadi::CalendarUtils::email())));
        }

        if (incidence->hasRecurrenceId()) {
            // For an individual occurrence, recur id doesn't make sense, since we're not sending the whole recurrence series.
            incidence->setRecurrenceId(KDateTime());
        }

        KCalCore::ICalFormat format;
        const QString from = Akonadi::CalendarUtils::email();
        const bool bccMe = Akonadi::CalendarSettings::self()->bcc();
        const QString messageText = format.createScheduleMessage(incidence, KCalCore::iTIPRequest);
        MailClient *mailer = new MailClient(d->m_factory);
        d->m_queuedInvitation.incidence = incidence;
        connect(mailer, SIGNAL(finished(Akonadi::MailClient::Result,QString)),
                d, SLOT(finishSendAsICalendar(Akonadi::MailScheduler::Result,QString)));

        mailer->mailTo(incidence, identityManager.identityForAddress(from), from, bccMe,
                       recipients, messageText,
                       MailTransport::TransportManager::self()->defaultTransportName());
    }
}

void ITIPHandler::setGroupwareUiDelegate(GroupwareUiDelegate *delegate)
{
    d->m_uiDelegate = delegate;
}

void ITIPHandler::setCalendar(const Akonadi::CalendarBase::Ptr &calendar)
{
    if (d->m_calendar != calendar) {
        d->m_calendar = calendar;
    }
}

void ITIPHandler::setShowDialogsOnError(bool enable)
{
    d->m_showDialogsOnError = enable;
}

Akonadi::CalendarBase::Ptr ITIPHandler::calendar() const
{
    return d->m_calendar;
}

