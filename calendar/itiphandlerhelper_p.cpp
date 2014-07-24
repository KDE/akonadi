/*
  Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

  Copyright (C) 2010 Bertjan Broeksema <broeksema@kde.org>
  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

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

#include "itiphandlerhelper_p.h"
#include "calendarsettings.h"
#include "utils_p.h"

#include <kcalcore/calendar.h>
#include <kcalcore/icalformat.h>
#include <kcalutils/incidenceformatter.h>
#include <kcalutils/stringify.h>
#include <KDebug>
#include <KLocalizedString>
#include <KMessageBox>

#include <QWidget>

using namespace Akonadi;

QString proposalComment(const KCalCore::Incidence::Ptr &incidence)
{
    QString comment;

    // TODO: doesn't KCalUtils/IncidenceFormater already provide this?
    // if not, it should go there.

    switch (incidence->type()) {
    case KCalCore::IncidenceBase::TypeEvent:
    {
        const KDateTime dtEnd = incidence->dateTime(KCalCore::Incidence::RoleDisplayEnd);
        comment = i18n("Proposed new meeting time: %1 - %2",
                       KCalUtils::IncidenceFormatter::dateToString(incidence->dtStart()),
                       KCalUtils::IncidenceFormatter::dateToString(dtEnd));
    }
    break;
    case KCalCore::IncidenceBase::TypeTodo:
    {
        kWarning() << "NOT IMPLEMENTED: proposalComment called for to-do.";
    }
    break;
    default:
        kWarning() << "NOT IMPLEMENTED: proposalComment called for " << incidence->typeStr();
    }

    return comment;
}

ITIPHandlerDialogDelegate::ITIPHandlerDialogDelegate(const KCalCore::Incidence::Ptr &incidence, KCalCore::iTIPMethod method, QWidget *parent)
    : mParent(parent)
    , mIncidence(incidence)
    , mMethod(method)
{
}


int ITIPHandlerDialogDelegate::askUserIfNeeded(const QString &question, Action action,
                                  const KGuiItem &buttonYes, const KGuiItem &buttonNo) const
{
    Q_ASSERT_X(!question.isEmpty(), "ITIPHandlerHelper::askUser", "ask what?");

    switch (action) {
    case ActionSendMessage:
        return KMessageBox::Yes;
    case ActionDontSendMessage:
        return KMessageBox::No;
    default:
        return KMessageBox::questionYesNo(mParent, question, i18n("Group Scheduling Email"),
                                          buttonYes, buttonNo);
    }
}

void ITIPHandlerDialogDelegate::openDialogIncidenceCreated(Recipient recipient,
                                              const QString &question, Action action,
                                              const KGuiItem &buttonYes, const KGuiItem &buttonNo)
{
    Q_UNUSED(recipient);
    const int messageBoxReturnCode = askUserIfNeeded(question, action, buttonYes, buttonNo);
    emit dialogClosed(messageBoxReturnCode, mMethod, mIncidence);
}

void ITIPHandlerDialogDelegate::openDialogIncidenceModified(bool attendeeStatusChanged, Recipient recipient,
                                               const QString &question, Action action,
                                               const KGuiItem &buttonYes, const KGuiItem &buttonNo)
{
    Q_UNUSED(attendeeStatusChanged);
    Q_UNUSED(recipient);
    const int messageBoxReturnCode = askUserIfNeeded(question, action, buttonYes, buttonNo);
    emit dialogClosed(messageBoxReturnCode, mMethod, mIncidence);
}

void ITIPHandlerDialogDelegate::openDialogIncidenceDeleted(Recipient recipient,
                                              const QString &question, Action action,
                                              const KGuiItem &buttonYes, const KGuiItem &buttonNo)
{
    Q_UNUSED(recipient);
    const int messageBoxReturnCode = askUserIfNeeded(question, action, buttonYes, buttonNo);
    emit dialogClosed(messageBoxReturnCode, mMethod, mIncidence);
}

void ITIPHandlerDialogDelegate::openDialogSchedulerFinished(const QString &question, Action action,
                                               const KGuiItem &buttonYes, const KGuiItem &buttonNo)
{
    const int messageBoxReturnCode = askUserIfNeeded(question, action, buttonYes, buttonNo);
    emit dialogClosed(messageBoxReturnCode, mMethod, mIncidence);
}

ITIPHandlerHelper::SendResult
ITIPHandlerHelper::sentInvitation(int messageBoxReturnCode,
                                  const KCalCore::Incidence::Ptr &incidence,
                                  KCalCore::iTIPMethod method)
{
    // The value represented by messageBoxReturnCode is the answer on a question
    // which is a variant of: Do you want to send an email to the attendees?
    //
    // Where the email contains an invitation, modification notification or
    // deletion notification.

    if (messageBoxReturnCode == KMessageBox::Yes) {
        // We will be sending out a message here. Now make sure there is some summary
        // Yes, we do modify the incidence here, but we still keep the Ptr
        // semantics, because this change is only for sending and not stored int the
        // local calendar.
        KCalCore::Incidence::Ptr _incidence(incidence->clone());
        if (_incidence->summary().isEmpty()) {
            _incidence->setSummary(i18n("<placeholder>No summary given</placeholder>"));
        }

        // Send the mail
        m_status = StatusSendingInvitation;
        m_scheduler->performTransaction(_incidence, method);
        return ITIPHandlerHelper::ResultSuccess;

    } else if (messageBoxReturnCode == KMessageBox::No) {
        emit finished(ITIPHandlerHelper::ResultCanceled, QString());
        return ITIPHandlerHelper::ResultCanceled;
    } else {
        Q_ASSERT(false);   // TODO: Figure out if/how this can happen (by closing the dialog with x??)
        emit finished(ITIPHandlerHelper::ResultCanceled, QString());
        return ITIPHandlerHelper::ResultCanceled;
    }
}

bool ITIPHandlerHelper::weAreOrganizerOf(const KCalCore::Incidence::Ptr &incidence)
{
    const QString email = incidence->organizer()->email();
    return Akonadi::CalendarUtils::thatIsMe(email) || email.isEmpty()
           || email == QLatin1String("invalid@email.address");
}

bool ITIPHandlerHelper::weNeedToSendMailFor(const KCalCore::Incidence::Ptr &incidence)
{
    if (!weAreOrganizerOf(incidence)) {
        kError() << "We should be the organizer of this incidence."
                 << "; email= "       << incidence->organizer()->email()
                 << "; thatIsMe() = " << Akonadi::CalendarUtils::thatIsMe(incidence->organizer()->email());
        Q_ASSERT(false);
        return false;
    }

    if (incidence->attendees().isEmpty()) {
        return false;
    }

    // At least one attendee
    return
        incidence->attendees().count() > 1 ||
        incidence->attendees().first()->email() != incidence->organizer()->email();
}

ITIPHandlerHelper::ITIPHandlerHelper(ITIPHandlerComponentFactory *factory, QWidget *parent)
    : QObject(parent)
    , mDefaultAction(ITIPHandlerDialogDelegate::ActionAsk)
    , mParent(parent)
    , m_factory(factory ? factory : new ITIPHandlerComponentFactory(this))
    , m_scheduler(new MailScheduler(m_factory, this))
    , m_status(StatusNone)
{
    connect(m_scheduler, SIGNAL(transactionFinished(Akonadi::Scheduler::Result,QString)),
            SLOT(onSchedulerFinished(Akonadi::Scheduler::Result,QString)));
}

ITIPHandlerHelper::~ITIPHandlerHelper()
{
}

void ITIPHandlerHelper::setDialogParent(QWidget *parent)
{
    mParent = parent;
}

void ITIPHandlerHelper::setDefaultAction(ITIPHandlerDialogDelegate::Action action)
{
    mDefaultAction = action;
}

void ITIPHandlerHelper::sendIncidenceCreatedMessage(KCalCore::iTIPMethod method,
        const KCalCore::Incidence::Ptr &incidence)
{
    /// When we created the incidence, we *must* be the organizer.

    if (!weAreOrganizerOf(incidence)) {
        kWarning() << "Creating incidence which has another organizer! Will skip sending invitations."
                 << "; email= "       << incidence->organizer()->email()
                 << "; thatIsMe() = " << Akonadi::CalendarUtils::thatIsMe(incidence->organizer()->email());
        emit sendIncidenceCreatedMessageFinished(ITIPHandlerHelper::ResultFailAbortUpdate, method, incidence);
        emit finished(ITIPHandlerHelper::ResultFailAbortUpdate, QString());
        return;
    }

    if (!weNeedToSendMailFor(incidence)) {
        emit sendIncidenceCreatedMessageFinished(ITIPHandlerHelper::ResultNoSendingNeeded, method, incidence);
        emit finished(ITIPHandlerHelper::ResultNoSendingNeeded, QString());
        return;
    }

    QString question;
    if (incidence->type() == KCalCore::Incidence::TypeEvent) {
        question = i18n("The event \"%1\" includes other people.\n"
                        "Do you want to email the invitation to the attendees?",
                        incidence->summary());
    } else if (incidence->type() == KCalCore::Incidence::TypeTodo) {
        question = i18n("The todo \"%1\" includes other people.\n"
                        "Do you want to email the invitation to the attendees?",
                        incidence->summary());
    } else {
        question = i18n("This incidence includes other people. "
                        "Should an email be sent to the attendees?");
    }

    ITIPHandlerDialogDelegate *askDelegator = m_factory->createITIPHanderDialogDelegate(incidence, method, mParent);
    connect(askDelegator, SIGNAL(dialogClosed(int, KCalCore::iTIPMethod, KCalCore::Incidence::Ptr)),
            SLOT(slotIncidenceCreatedDialogClosed(int, KCalCore::iTIPMethod, KCalCore::Incidence::Ptr)));
    askDelegator->openDialogIncidenceCreated(ITIPHandlerDialogDelegate::Attendees, question, mDefaultAction);
}

void ITIPHandlerHelper::slotIncidenceCreatedDialogClosed(int messageBoxReturnCode, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence)
{
    ITIPHandlerHelper::SendResult status = sentInvitation(messageBoxReturnCode, incidence, method);
    emit sendIncidenceCreatedMessageFinished(status, method, incidence);
}

bool ITIPHandlerHelper::handleIncidenceAboutToBeModified(const KCalCore::Incidence::Ptr &incidence)
{
    Q_ASSERT(incidence);
    if (!weAreOrganizerOf(incidence)) {
        switch (incidence->type()) {
        case KCalCore::Incidence::TypeEvent:
        {
            const QString question =
                i18n("You are not the organizer of this event. Editing it will "
                     "bring your calendar out of sync with the organizer's calendar. "
                     "Do you really want to edit it?");
            const int messageBoxReturnCode = KMessageBox::warningYesNo(mParent, question);
            return messageBoxReturnCode != KMessageBox::No;
        }
        break;
        case KCalCore::Incidence::TypeJournal:
        case KCalCore::Incidence::TypeTodo:
            // Not sure why we handle to-dos differently regarding this
            return true;
            break;
        default:
            kError() << "Unknown incidence type: " << incidence->type() << incidence->typeStr();
            Q_ASSERT_X(false, "ITIPHandlerHelper::handleIncidenceAboutToBeModified()",
                       "Unknown incidence type");
            return false;
        }
    } else {
        return true;
    }
}

void ITIPHandlerHelper::sendIncidenceModifiedMessage(KCalCore::iTIPMethod method,
        const KCalCore::Incidence::Ptr &incidence,
        bool attendeeStatusChanged)
{
    ITIPHandlerDialogDelegate *askDelegator = m_factory->createITIPHanderDialogDelegate(incidence, method, mParent);

    connect(askDelegator, SIGNAL(dialogClosed(int,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)),
            SLOT(slotIncidenceModifiedDialogClosed(int,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)));
    // For a modified incidence, either we are the organizer or someone else.
    if (weAreOrganizerOf(incidence)) {
        if (weNeedToSendMailFor(incidence)) {
            const QString question = i18n("You changed the invitation \"%1\".\n"
                                          "Do you want to email the attendees an update message?",
                                          incidence->summary());

            askDelegator->openDialogIncidenceModified(attendeeStatusChanged, ITIPHandlerDialogDelegate::Attendees, question, mDefaultAction, KGuiItem(i18n("Send Update")));
            return;
        } else {
            emit sendIncidenceModifiedMessageFinished(ITIPHandlerHelper::ResultNoSendingNeeded, method, incidence);
            emit finished(ITIPHandlerHelper::ResultNoSendingNeeded, QString());
            delete askDelegator;
            return;
        }

    } else if (incidence->type() == KCalCore::Incidence::TypeTodo) {

        if (method == KCalCore::iTIPRequest) {
            // This is an update to be sent to the organizer
            method = KCalCore::iTIPReply;
        }

        const QString question = i18n("Do you want to send a status update to the "
                                      "organizer of this task?");
        askDelegator->openDialogIncidenceModified(attendeeStatusChanged, ITIPHandlerDialogDelegate::Organizer, question, mDefaultAction, KGuiItem(i18n("Send Update")));
        return;

    } else if (incidence->type() == KCalCore::Incidence::TypeEvent) {
        if (attendeeStatusChanged && method == KCalCore::iTIPRequest) {
            method = KCalCore::iTIPReply;
            const QString question =
                i18n("Your status as an attendee of this event changed. "
                     "Do you want to send a status update to the event organizer?");

                askDelegator->openDialogIncidenceModified(attendeeStatusChanged, ITIPHandlerDialogDelegate::Organizer, question, mDefaultAction, KGuiItem(i18n("Send Update")));
            return;
        } else {
            slotIncidenceModifiedDialogClosed(KMessageBox::Yes, method, incidence);
            delete askDelegator;
            return;
        }
    }
    Q_ASSERT(false);   // Shouldn't happen.
    emit sendIncidenceModifiedMessageFinished(ITIPHandlerHelper::ResultNoSendingNeeded, method, incidence);
    emit finished(ITIPHandlerHelper::ResultNoSendingNeeded, QString());
    delete askDelegator;
}

void ITIPHandlerHelper::slotIncidenceModifiedDialogClosed(int messageBoxReturnCode, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence)
{
    ITIPHandlerHelper::SendResult status = sentInvitation(messageBoxReturnCode, incidence, method);
    emit sendIncidenceModifiedMessageFinished(status, method, incidence);
}

void ITIPHandlerHelper::sendIncidenceDeletedMessage(KCalCore::iTIPMethod method,
        const KCalCore::Incidence::Ptr &incidence)
{
    Q_ASSERT(incidence);

    ITIPHandlerDialogDelegate *askDelegator = m_factory->createITIPHanderDialogDelegate(incidence, method, mParent);

    connect(askDelegator, SIGNAL(dialogClosed(int,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)),
            SLOT(slotIncidenceDeletedDialogClosed(int,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)));

    // For a modified incidence, either we are the organizer or someone else.
    if (weAreOrganizerOf(incidence)) {
        if (weNeedToSendMailFor(incidence)) {
            QString question;
            if (incidence->type() == KCalCore::Incidence::TypeEvent) {
                question = i18n("You removed the invitation \"%1\".\n"
                                "Do you want to email the attendees that the event is canceled?",
                                incidence->summary());
            } else if (incidence->type() == KCalCore::Incidence::TypeTodo) {
                question = i18n("You removed the invitation \"%1\".\n"
                                "Do you want to email the attendees that the todo is canceled?",
                                incidence->summary());
            }  else if (incidence->type() == KCalCore::Incidence::TypeJournal) {
                question = i18n("You removed the invitation \"%1\".\n"
                                "Do you want to email the attendees that the journal is canceled?",
                                incidence->summary());
            }
            askDelegator->openDialogIncidenceDeleted(ITIPHandlerDialogDelegate::Attendees, question, mDefaultAction);
            return;
        } else {
            emit sendIncidenceDeletedMessageFinished(ITIPHandlerHelper::ResultNoSendingNeeded, method, incidence);
            emit finished(ITIPHandlerHelper::ResultNoSendingNeeded, QString());
            delete askDelegator;
            return;
        }

    } else if (incidence->type() != KCalCore::Incidence::TypeEvent) {

        if (method == KCalCore::iTIPRequest) {
            // This is an update to be sent to the organizer
            method = KCalCore::iTIPReply;
        }

        const QString question = (incidence->type() == KCalCore::Incidence::TypeTodo) ?
                                 i18n("Do you want to send a status update to the "
                                      "organizer of this task?") :
                                 i18n("Do you want to send a status update to the "
                                      "organizer of this journal?");
        askDelegator->openDialogIncidenceDeleted(ITIPHandlerDialogDelegate::Organizer, question, mDefaultAction,
                                                  KGuiItem(i18nc("@action:button dialog positive answer","Send Update")));
        return;
    } else if (incidence->type() == KCalCore::Incidence::TypeEvent) {

        const QStringList myEmails = Akonadi::CalendarUtils::allEmails();
        bool incidenceAcceptedBefore = false;
        foreach(const QString &email, myEmails) {
            KCalCore::Attendee::Ptr me = incidence->attendeeByMail(email);
            if (me &&
                    (me->status() == KCalCore::Attendee::Accepted ||
                     me->status() == KCalCore::Attendee::Delegated)) {
                incidenceAcceptedBefore = true;
                break;
            }
        }

        if (incidenceAcceptedBefore) {
            QString question = i18n("You had previously accepted an invitation to this event. "
                                    "Do you want to send an updated response to the organizer "
                                    "declining the invitation?");
            askDelegator->openDialogIncidenceDeleted(ITIPHandlerDialogDelegate::Organizer, question, mDefaultAction,
                                                     KGuiItem(i18nc("@action:button dialog positive answer","Send Update")));
            return;
        } else {
            // We did not accept the event before and delete it from our calendar agian,
            // so there is no need to notify people.
            emit sendIncidenceDeletedMessageFinished(ITIPHandlerHelper::ResultNoSendingNeeded, method, incidence);
            emit finished(ITIPHandlerHelper::ResultNoSendingNeeded, QString());
            delete askDelegator;
            return;
        }
    }

    Q_ASSERT(false);   // Shouldn't happen.
    emit sendIncidenceDeletedMessageFinished(ITIPHandlerHelper::ResultNoSendingNeeded, method, incidence);
    emit finished(ITIPHandlerHelper::ResultNoSendingNeeded, QString());
    delete askDelegator;
}

void ITIPHandlerHelper::slotIncidenceDeletedDialogClosed(const int messageBoxReturnCode, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence)
{
    ITIPHandlerHelper::SendResult status = sentInvitation(messageBoxReturnCode, incidence, method);
    emit sendIncidenceDeletedMessageFinished(status, method, incidence);
}

ITIPHandlerHelper::SendResult
ITIPHandlerHelper::sendCounterProposal(const KCalCore::Incidence::Ptr &oldEvent,
                                       const KCalCore::Incidence::Ptr &newEvent)
{
    Q_ASSERT(oldEvent);
    Q_ASSERT(newEvent);

    if (!oldEvent || !newEvent || *oldEvent == *newEvent)
        return ITIPHandlerHelper::ResultNoSendingNeeded;

    if (CalendarSettings::self()->outlookCompatCounterProposals()) {
        KCalCore::Incidence::Ptr tmp(oldEvent->clone());
        tmp->setSummary(i18n("Counter proposal: %1", newEvent->summary()));
        tmp->setDescription(newEvent->description());
        tmp->addComment(proposalComment(newEvent));

        return sentInvitation(KMessageBox::Yes, tmp, KCalCore::iTIPReply);
    } else {
        return sentInvitation(KMessageBox::Yes, newEvent, KCalCore::iTIPCounter);
    }
}

void ITIPHandlerHelper::onSchedulerFinished(Akonadi::Scheduler::Result result,
        const QString &errorMsg)
{
    const bool success = result == MailScheduler::ResultSuccess;

    if (m_status == StatusSendingInvitation) {
        m_status = StatusNone;
        if (!success) {
            const QString question(i18n("Sending group scheduling email failed."));

            ITIPHandlerDialogDelegate *askDelegator = m_factory->createITIPHanderDialogDelegate(KCalCore::Incidence::Ptr(), KCalCore::iTIPNoMethod, mParent);

            connect(askDelegator, SIGNAL(dialogClosed(int,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)),
                    SLOT(slotSchedulerFinishDialog(int,KCalCore::iTIPMethod,KCalCore::Incidence::Ptr)));
            askDelegator->openDialogSchedulerFinished(question,  ITIPHandlerDialogDelegate::ActionAsk,
                                                      KGuiItem(i18nc("@action:button dialog positive answer to abort question","Abort Update")));
            return;
        } else {
            //fall through
        }
    }

    emit finished(success ? ResultSuccess : ResultError,
                  success ? QString() : i18n("Error: %1", errorMsg));
}

void ITIPHandlerHelper::slotSchedulerFinishDialog(const int result, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence)
{
    Q_UNUSED(method);
    Q_UNUSED(incidence);
    if (result == KMessageBox::Yes) {
        emit finished(ResultFailAbortUpdate, QString());
    } else {
        emit finished(ResultFailKeepUpdate, QString());
    }
}

