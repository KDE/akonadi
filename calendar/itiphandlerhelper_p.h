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

#ifndef AKONADI_CALENDAR_ITIP_HANDLERHELPER_P_H
#define AKONADI_CALENDAR_ITIP_HANDLERHELPER_P_H

#include "itiphandler.h"
#include "mailscheduler_p.h"

#include <kcalcore/incidence.h>
#include <kcalcore/schedulemessage.h>

#include <KGuiItem>
#include <KLocalizedString>

#include <QObject>

class QWidget;

namespace Akonadi {

enum Status {
    StatusNone,
    StatusSendingInvitation
};

/**
  This class handles sending of invitations to attendees when Incidences (e.g.
  events or todos) are created/modified/deleted.

  There are two scenarios:
  o "we" are the organizer, where "we" means any of the identities or mail
    addresses known to Kontact/PIM. If there are attendees, we need to mail
    them all, even if one or more of them are also "us". Otherwise there
    would be no way to invite a resource or our boss, other identities we
    also manage.
  o "we: are not the organizer, which means we changed the completion status
    of a todo, or we changed our attendee status from, say, tentative to
    accepted. In both cases we only mail the organizer. All other changes
    bring us out of sync with the organizer, so we won't mail, if the user
    insists on applying them.

  NOTE: Currently only events and todos are support, meaning Incidence::type()
        should either return "Event" or "Todo"
 */
class ITIPHandlerHelper : public QObject
{
    Q_OBJECT
public:
    explicit ITIPHandlerHelper(ITIPHandlerComponentFactory *factory, QWidget *parent = 0);
    ~ITIPHandlerHelper();

    enum SendResult {
        ResultCanceled,        /**< Sending was canceled by the user, meaning there are
                                  local changes of which other attendees are not aware. */
        ResultFailKeepUpdate,  /**< Sending failed, the changes to the incidence must be kept. */
        ResultFailAbortUpdate, /**< Sending failed, the changes to the incidence must be undone. */
        ResultNoSendingNeeded, /**< In some cases it is not needed to send an invitation
                                (e.g. when we are the only attendee) */
        ResultError,           /**< An unexpected error occurred */
        ResultSuccess,         /**< The invitation was sent to all attendees. */
        ResultPending          /**< The user has been asked about one detail, waiting for him to anwser it */
    };


    /**
      When an Incidence is created/modified/deleted the user can choose to send
      an ICal message to the other participants. By default the user will be asked
      if he wants to send a message to other participants. In some cases it is
      preferably though to not bother the user with this question. This method
      allows to change the default behavior. This method applies to the
      sendIncidence*Message() methods.
      @param action the action to set as default
     */
    void setDefaultAction(ITIPHandlerDialogDelegate::Action action);

    /**
      Before an invitation is sent the user is asked for confirmation by means of
      an dialog.
      @param parent The parent widget used for the dialogs.
     */
    void setDialogParent(QWidget *parent);

    /**
      Handles sending of invitations for newly created incidences. This method
      asserts that we (as in any of the identities or mail addresses known to
      Kontact/PIM) are the organizer.
      @param incidence The new incidence.
     */
    void sendIncidenceCreatedMessage(KCalCore::iTIPMethod method,
            const KCalCore::Incidence::Ptr &incidence);

    /**
       Checks if the incidence should really be modified.

       If the user is not the organizer of this incidence, he will be asked if he really
       wants to proceed.

       Only create the ItemModifyJob if this method returns true.

       @param incidence The modified incidence. It may not be null.
     */
    bool handleIncidenceAboutToBeModified(const KCalCore::Incidence::Ptr &incidence);

    /**
      Handles sending of invitations for modified incidences.
      @param incidence The modified incidence.
      @param attendeeStatusChanged if @c true and @p method is #iTIPRequest ask the user whether to send a status update as well
     */
    void sendIncidenceModifiedMessage(KCalCore::iTIPMethod method,
            const KCalCore::Incidence::Ptr &incidence,
            bool attendeeStatusChanged);

    /**
      Handles sending of ivitations for deleted incidences.
      @param incidence The deleted incidence.
     */
    void sendIncidenceDeletedMessage(KCalCore::iTIPMethod method,
            const KCalCore::Incidence::Ptr &incidence);

    /**
      Send counter proposal message.
      @param oldIncidence The original event provided in the invitations.
      @param newIncidence The new event as edited by the user.
    */
    ITIPHandlerHelper::SendResult sendCounterProposal(const KCalCore::Incidence::Ptr &oldIncidence,
            const KCalCore::Incidence::Ptr &newIncidence);

    // Frees calendar if it doesn't have jobs running
    void calendarJobFinished(bool success, const QString &errorString);

Q_SIGNALS:
    void finished(Akonadi::ITIPHandlerHelper::SendResult result,
                  const QString &errorMessage);

    void sendIncidenceDeletedMessageFinished(ITIPHandlerHelper::SendResult, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);
    void sendIncidenceModifiedMessageFinished(ITIPHandlerHelper::SendResult, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);
    void sendIncidenceCreatedMessageFinished(ITIPHandlerHelper::SendResult, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);

private Q_SLOTS:
    void onSchedulerFinished(Akonadi::Scheduler::Result result, const QString &errorMsg);

    void slotIncidenceDeletedDialogClosed(const int, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);
    void slotIncidenceModifiedDialogClosed(const int, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);
    void slotIncidenceCreatedDialogClosed(const int, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);

    void slotSchedulerFinishDialog(const int result, KCalCore::iTIPMethod method, const KCalCore::Incidence::Ptr &incidence);

private:
    ITIPHandlerHelper::SendResult sentInvitation(int messageBoxReturnCode,
            const KCalCore::Incidence::Ptr &incidence,
            KCalCore::iTIPMethod method);

    /**
      We are the organizer. If there is more than one attendee, or if there is
      only one, and it's not the same as the organizer, ask the user to send
      mail.
    */
    bool weAreOrganizerOf(const KCalCore::Incidence::Ptr &incidence);

    /**
      Assumes that we are the organizer. If there is more than one attendee, or if
      there is only one, and it's not the same as the organizer, ask the user to send
      mail.
     */
    bool weNeedToSendMailFor(const KCalCore::Incidence::Ptr &incidence);

    ITIPHandlerDialogDelegate::Action mDefaultAction;
    QWidget *mParent;
    ITIPHandlerComponentFactory *m_factory;
    MailScheduler *m_scheduler;
    Status m_status;
};

}

#endif
