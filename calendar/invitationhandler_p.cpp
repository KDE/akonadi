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

#include "invitationhandler.h"
#include "kcalprefs.h"
#include "mailscheduler2.h"
#include "nepomukcalendar.h"

#include <KCalCore/Calendar>
#include <KCalCore/ICalFormat>
#include <KCalUtils/IncidenceFormatter>
#include <KCalUtils/Stringify>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>

using namespace CalendarSupport;

/// Private

namespace CalendarSupport {

GroupwareUiDelegate::~GroupwareUiDelegate()
{
}

struct Invitation {
  QString type;
  QString iCal;
  QString receiver;
};

struct InvitationHandler::Private
{
  /// Members
  FetchJobCalendar::Ptr mCalendar;
  InvitationHandler::Action mDefaultAction;
  QWidget *mParent;
  QHash<NepomukCalendar::Ptr,Invitation*> mInvitationByCalendar;

  /// Methods
  Private( const FetchJobCalendar::Ptr &calendar, QWidget *parent );

  InvitationHandler::SendResult sentInvitation( int messageBoxReturnCode,
                                                const KCalCore::Incidence::Ptr &incidence,
                                                KCalCore::iTIPMethod method );

  int askUserIfNeeded( const QString &question,
                       bool ignoreDefaultAction = true,
                       const KGuiItem &buttonYes = KGuiItem( i18n( "Send Email" ) ),
                       const KGuiItem &buttonNo = KGuiItem( i18n( "Do Not Send" ) ) ) const;

  /**
    We are the organizer. If there is more than one attendee, or if there is
    only one, and it's not the same as the organizer, ask the user to send
    mail.
  */
  bool weAreOrganizerOf( const KCalCore::Incidence::Ptr &incidence );

  /**
    Assumes that we are the organizer. If there is more than one attendee, or if
    there is only one, and it's not the same as the organizer, ask the user to send
    mail.
   */
  bool weNeedToSendMailFor( const KCalCore::Incidence::Ptr &incidence );
};

}

QString proposalComment( const KCalCore::Incidence::Ptr &incidence )
{
  QString comment;

  // TODO: doesn't KCalUtils/IncidenceFormater already provide this?
  // if not, it should go there.

  switch ( incidence->type() ) {
  case KCalCore::IncidenceBase::TypeEvent:
    {
      const KDateTime dtEnd = incidence.dynamicCast<const KCalCore::Event>()->dtEnd();
      comment = i18n( "Proposed new meeting time: %1 - %2",
                      KCalUtils::IncidenceFormatter::dateToString( incidence->dtStart() ),
                      KCalUtils::IncidenceFormatter::dateToString( dtEnd ) );
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

InvitationHandler::Private::Private( const FetchJobCalendar::Ptr &calendar, QWidget *parent )
  : mCalendar( calendar ), mDefaultAction( InvitationHandler::ActionAsk ), mParent( parent )
{ }

int InvitationHandler::Private::askUserIfNeeded( const QString &question,
                                                 bool ignoreDefaultAction,
                                                 const KGuiItem &buttonYes,
                                                 const KGuiItem &buttonNo ) const
{
  Q_ASSERT_X( !question.isEmpty(), "InvitationHandler::askUser", "ask what?" );

  if ( ignoreDefaultAction || mDefaultAction == InvitationHandler::ActionAsk )
    return KMessageBox::questionYesNo( mParent, question, i18n( "Group Scheduling Email" ), buttonYes, buttonNo );

  switch ( mDefaultAction ) {
  case InvitationHandler::ActionSendMessage:
    return KMessageBox::Yes;
  case InvitationHandler::ActionDontSendMessage:
    return KMessageBox::No;
  default:
    Q_ASSERT( false );
    return 0;
  }
}

InvitationHandler::SendResult
InvitationHandler::Private::sentInvitation( int messageBoxReturnCode,
                                            const KCalCore::Incidence::Ptr &incidence,
                                            KCalCore::iTIPMethod method )
{
  // The value represented by messageBoxReturnCode is the answer on a question
  // which is a variant of: Do you want to send an email to the attendees?
  //
  // Where the email contains an invitation, modification notification or
  // deletion notification.

  if ( messageBoxReturnCode == KMessageBox::Yes ) {
    // We will be sending out a message here. Now make sure there is some summary
    // Yes, we do modify the incidence here, but we still keep the Ptr
    // semantics, because this change is only for sending and not stored int the
    // local calendar.
    KCalCore::Incidence::Ptr _incidence( incidence->clone() );
    if ( _incidence->summary().isEmpty() ) {
      _incidence->setSummary( i18n( "<placeholder>No summary given</placeholder>" ) );
    }

    // Send the mail
    MailScheduler2 scheduler( mCalendar );
    if ( scheduler.performTransaction( _incidence, method ) ) {
      return InvitationHandler::ResultSuccess;
    }

    const QString question( i18n( "Sending group scheduling email failed." ) );
    messageBoxReturnCode = askUserIfNeeded( question, true, KGuiItem( i18n( "Abort Update" ) ) );
    if ( messageBoxReturnCode == KMessageBox::Yes ) {
      return InvitationHandler::ResultFailAbortUpdate;
    } else {
      return InvitationHandler::ResultFailKeepUpdate;
    }

  } else if ( messageBoxReturnCode == KMessageBox::No ) {
    return InvitationHandler::ResultCanceled;
  } else {
    Q_ASSERT( false ); // TODO: Figure out if/how this can happen (by closing the dialog with x??)
    return InvitationHandler::ResultCanceled;
  }
}

bool InvitationHandler::Private::weAreOrganizerOf( const KCalCore::Incidence::Ptr &incidence )
{
  const QString email = incidence->organizer()->email();
  return KCalPrefs::instance()->thatIsMe( email ) ||
         email.isEmpty() ||
         email == QLatin1String( "invalid@email.address" );
}

bool InvitationHandler::Private::weNeedToSendMailFor( const KCalCore::Incidence::Ptr &incidence )
{
  if ( !weAreOrganizerOf( incidence ) ) {
    kError() << "We should be the organizer of ths incidence."
             << "; email= "       << incidence->organizer()->email()
             << "; thatIsMe() = " << KCalPrefs::instance()->thatIsMe( incidence->organizer()->email() );
    Q_ASSERT( false );
    return false;
  }

  if ( incidence->attendees().isEmpty() ) {
    return false;
  }

  // At least one attendee
  return
    incidence->attendees().count() > 1 ||
    incidence->attendees().first()->email() != incidence->organizer()->email();
}

/// InvitationSender

InvitationHandler::InvitationHandler( const FetchJobCalendar::Ptr &calendar, QWidget *widget )
  : d ( new InvitationHandler::Private( calendar, widget ) )
{ }

InvitationHandler::~InvitationHandler()
{
  delete d;
}

void InvitationHandler::setDialogParent( QWidget *parent )
{
  d->mParent = parent;
}


void InvitationHandler::handleInvitation( const QString &receiver,
                                          const QString &iCal,
                                          const QString &type )
{
  NepomukCalendar::Ptr calendar = NepomukCalendar::create();
  connect( calendar.data(), SIGNAL(loadFinished(bool,QString)),
           SLOT(finishHandlingInvitation()) );

  Invitation *invitation = new Invitation();
  invitation->receiver = receiver;
  invitation->iCal = iCal;
  invitation->type = type;
  d->mInvitationByCalendar.insert( calendar, invitation );
}

void InvitationHandler::finishHandlingInvitation()
{
  QWeakPointer<NepomukCalendar> weakPtr = qobject_cast<NepomukCalendar*>( sender() )->weakPointer();
  NepomukCalendar::Ptr calendar( weakPtr.toStrongRef() );

  connect( calendar.data(), SIGNAL(addFinished(bool,QString) ),SLOT(calendarJobFinished(bool,QString) ), Qt::QueuedConnection );
  connect( calendar.data(), SIGNAL(deleteFinished(bool,QString) ),SLOT(calendarJobFinished(bool,QString) ), Qt::QueuedConnection );
  connect( calendar.data(), SIGNAL(changeFinished(bool,QString) ),SLOT(calendarJobFinished(bool,QString) ), Qt::QueuedConnection );

  Invitation *invitation = d->mInvitationByCalendar.value( calendar );

  kDebug() << "Calendar has " << calendar->incidences().count();

  const QString iCal = invitation->iCal;
  const QString receiver = invitation->receiver;
  const QString action = invitation->type;

  KCalCore::ICalFormat mFormat;
  KCalCore::ScheduleMessage::Ptr message = mFormat.parseScheduleMessage( d->mCalendar, iCal );

  if ( !message ) {
    QString errorMessage = i18n( "Unknown error while parsing iCal invitation" );
    if ( mFormat.exception() ) {
      errorMessage = i18n( "Error message: %1",
                           KCalUtils::Stringify::errorMessage( *mFormat.exception() ) );
    }

    kDebug() << "Error parsing" << errorMessage;
    KMessageBox::detailedError( d->mParent,
                                i18n( "Error while processing an invitation or update." ),
                                errorMessage );
    delete d->mInvitationByCalendar.take( calendar );
    emit handleInvitationFinished( false/*error*/, errorMessage );
    return;
  }

  KCalCore::iTIPMethod method = static_cast<KCalCore::iTIPMethod>( message->method() );
  KCalCore::ScheduleMessage::Status status = message->status();
  KCalCore::Incidence::Ptr incidence = message->event().dynamicCast<KCalCore::Incidence>();
  if ( !incidence ) {
    delete d->mInvitationByCalendar.take( calendar );
    emit handleInvitationFinished( false/*error*/,
                                   QLatin1String( "Invalid incidence" ) );
    return;
  }

  MailScheduler2 scheduler( d->mCalendar );
  if ( action.startsWith( QLatin1String( "accepted" ) ) ||
       action.startsWith( QLatin1String( "tentative" ) ) ||
       action.startsWith( QLatin1String( "delegated" ) ) ||
       action.startsWith( QLatin1String( "counter" ) ) ) {
    // Find myself and set my status. This can't be done in the scheduler,
    // since this does not know the choice I made in the KMail bpf
    const KCalCore::Attendee::List attendees = incidence->attendees();
    foreach ( KCalCore::Attendee::Ptr attendee, attendees ) {
      if ( attendee->email() == receiver ) {
        if ( action.startsWith( QLatin1String( "accepted" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Accepted );
        } else if ( action.startsWith( QLatin1String( "tentative" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Tentative );
        } else if ( KCalPrefs::instance()->outlookCompatCounterProposals() &&
                    action.startsWith( QLatin1String( "counter" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Tentative );
        } else if ( action.startsWith( QLatin1String( "delegated" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Delegated );
        }
        break;
      }
    }
    if ( KCalPrefs::instance()->outlookCompatCounterProposals() ||
         !action.startsWith( QLatin1String( "counter" ) ) ) {
      scheduler.acceptTransaction( incidence, method, status, receiver );
    }
  } else if ( action.startsWith( QLatin1String( "cancel" ) ) ) {
    // Delete the old incidence, if one is present
    scheduler.acceptTransaction( incidence, KCalCore::iTIPCancel, status, receiver );
  } else if ( action.startsWith( QLatin1String( "reply" ) ) ) {
    if ( method != KCalCore::iTIPCounter ) {
      scheduler.acceptTransaction( incidence, method, status, QString() );
    } else {
      scheduler.acceptCounterProposal( incidence );
      // send update to all attendees
      sendIncidenceModifiedMessage( KCalCore::iTIPRequest,
                                    KCalCore::Incidence::Ptr( incidence->clone() ), false );
    }
  } else {
    kError() << "Unknown incoming action" << action;
  }

  if ( action.startsWith( QLatin1String( "counter" ) ) ) {
    emit editorRequested( KCalCore::Incidence::Ptr( incidence->clone() ) );
  }

  if ( !calendar->jobsInProgress() ) {
    delete d->mInvitationByCalendar.take( calendar );
    emit handleInvitationFinished( true/*success*/, QString() );
  } // else it will be finished in calendarJobFinished() slot.
}

void InvitationHandler::setDefaultAction( Action action )
{
  d->mDefaultAction = action;
}

InvitationHandler::SendResult
InvitationHandler::sendIncidenceCreatedMessage( KCalCore::iTIPMethod method,
                                                const KCalCore::Incidence::Ptr &incidence )
{
  /// When we created the incidence, we *must* be the organizer.

  if ( !d->weAreOrganizerOf( incidence ) ) {
    kError() << "We should be the organizer of ths incidence!"
             << "; email= "       << incidence->organizer()->email()
             << "; thatIsMe() = " << KCalPrefs::instance()->thatIsMe( incidence->organizer()->email() );
    Q_ASSERT( false );
    return InvitationHandler::ResultFailAbortUpdate;
  }

  if ( !d->weNeedToSendMailFor( incidence ) ) {
    return InvitationHandler::ResultNoSendingNeeded;
  }

  QString question;
  if ( incidence->type() == KCalCore::Incidence::TypeEvent ) {
    question = i18n( "The event \"%1\" includes other people.\n"
                     "Do you want to email the invitation to the attendees?",
                     incidence->summary() );
  } else if ( incidence->type() == KCalCore::Incidence::TypeTodo ) {
    question = i18n( "The todo \"%1\" includes other people.\n"
                     "Do you want to email the invitation to the attendees?",
                     incidence->summary() );
  } else {
    question = i18n( "This incidence includes other people. "
                     "Should an email be sent to the attendees?" );
  }

  const int messageBoxReturnCode = d->askUserIfNeeded( question, false );
  return d->sentInvitation( messageBoxReturnCode, incidence, method );
}

bool InvitationHandler::handleIncidenceAboutToBeModified( const KCalCore::Incidence::Ptr &incidence )
{
  Q_ASSERT( incidence );
  if ( !d->weAreOrganizerOf( incidence ) ) {
    switch( incidence->type() ) {
      case KCalCore::Incidence::TypeEvent:
      {
        const QString question =
          i18n( "You are not the organizer of this event. Editing it will "
                "bring your calendar out of sync with the organizer's calendar. "
                "Do you really want to edit it?" );
        const int messageBoxReturnCode = KMessageBox::warningYesNo( d->mParent, question );
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
        Q_ASSERT_X( false, "InvitationHandler::handleIncidenceAboutToBeModified()",
                           "Unknown incidence type" );
        return false;
    }
  } else {
    return true;
  }
}

InvitationHandler::SendResult
InvitationHandler::sendIncidenceModifiedMessage( KCalCore::iTIPMethod method,
                                                 const KCalCore::Incidence::Ptr &incidence,
                                                 bool attendeeStatusChanged )
{
  // For a modified incidence, either we are the organizer or someone else.
  if ( d->weAreOrganizerOf( incidence ) ) {
    if ( d->weNeedToSendMailFor( incidence ) ) {
      const QString question = i18n( "You changed the invitation \"%1\".\n"
                                     "Do you want to email the attendees an update message?",
                               incidence->summary() );

      const int messageBoxReturnCode = d->askUserIfNeeded( question, false, KGuiItem( i18n( "Send Update" ) ) );
      return d->sentInvitation( messageBoxReturnCode, incidence, method );

    } else {
      return ResultNoSendingNeeded;
    }

  } else if ( incidence->type() == KCalCore::Incidence::TypeTodo ) {

    if ( method == KCalCore::iTIPRequest ) {
      // This is an update to be sent to the organizer
      method = KCalCore::iTIPReply;
    }

    const QString question = i18n( "Do you want to send a status update to the "
                                   "organizer of this task?" );
    const int messageBoxReturnCode = d->askUserIfNeeded( question, false, KGuiItem( i18n( "Send Update" ) ) );
    return d->sentInvitation( messageBoxReturnCode, incidence, method );

  } else if ( incidence->type() == KCalCore::Incidence::TypeEvent ) {
    if ( attendeeStatusChanged && method == KCalCore::iTIPRequest ) {
      method = KCalCore::iTIPReply;
      const QString question =
        i18n( "Your status as an attendee of this event changed. "
              "Do you want to send a status update to the event organizer?" );
      const int messageBoxReturnCode = d->askUserIfNeeded( question, false, KGuiItem( i18n( "Send Update" ) ) );
      return d->sentInvitation( messageBoxReturnCode, incidence, method );
    } else {
      return d->sentInvitation( KMessageBox::Yes, incidence, method );
    }
  }
  Q_ASSERT( false ); // Shouldn't happen.
  return ResultNoSendingNeeded;
}

InvitationHandler::SendResult
InvitationHandler::sendIncidenceDeletedMessage( KCalCore::iTIPMethod method,
                                                const KCalCore::Incidence::Ptr &incidence )
{
  Q_ASSERT( incidence );
  // For a modified incidence, either we are the organizer or someone else.
  if ( d->weAreOrganizerOf( incidence ) ) {
    if ( d->weNeedToSendMailFor( incidence ) ) {
      QString question;
      if ( incidence->type() == KCalCore::Incidence::TypeEvent ) {
        question = i18n( "You removed the invitation \"%1\".\n"
                         "Do you want to email the attendees that the event is canceled?",
                         incidence->summary() );
      } else if ( incidence->type() == KCalCore::Incidence::TypeTodo ) {
        question = i18n( "You removed the invitation \"%1\".\n"
                         "Do you want to email the attendees that the todo is canceled?",
                         incidence->summary() );
      }  else if ( incidence->type() == KCalCore::Incidence::TypeJournal ) {
        question = i18n( "You removed the invitation \"%1\".\n"
                         "Do you want to email the attendees that the journal is canceled?",
                         incidence->summary() );
      }

      const int messageBoxReturnCode = d->askUserIfNeeded( question, false );
      return d->sentInvitation( messageBoxReturnCode, incidence, method );
    } else {
      return ResultNoSendingNeeded;
    }

  } else if ( incidence->type() != KCalCore::Incidence::TypeEvent  ) {

    if ( method == KCalCore::iTIPRequest ) {
      // This is an update to be sent to the organizer
      method = KCalCore::iTIPReply;
    }

    const QString question = ( incidence->type() == KCalCore::Incidence::TypeTodo ) ?
                                     i18n( "Do you want to send a status update to the "
                                           "organizer of this task?" ) :
                                     i18n( "Do you want to send a status update to the "
                                           "organizer of this journal?" );

    const int messageBoxReturnCode = d->askUserIfNeeded( question, false, KGuiItem( i18n( "Send Update" ) ) );
    return d->sentInvitation( messageBoxReturnCode, incidence, method );
  } else if ( incidence->type() == KCalCore::Incidence::TypeEvent ) {

    const QStringList myEmails = KCalPrefs::instance()->allEmails();
    bool incidenceAcceptedBefore = false;
    foreach ( const QString &email, myEmails ) {
      KCalCore::Attendee::Ptr me = incidence->attendeeByMail( email );
      if ( me &&
           ( me->status() == KCalCore::Attendee::Accepted ||
             me->status() == KCalCore::Attendee::Delegated ) ) {
        incidenceAcceptedBefore = true;
        break;
      }
    }

    if ( incidenceAcceptedBefore ) {
      QString question = i18n( "You had previously accepted an invitation to this event. "
                               "Do you want to send an updated response to the organizer "
                               "declining the invitation?" );
      int messageBoxReturnCode = d->askUserIfNeeded( question, false, KGuiItem( i18n( "Send Update" ) ) );
      return d->sentInvitation( messageBoxReturnCode, incidence, method );
    } else {
      // We did not accept the event before and delete it from our calendar agian,
      // so there is no need to notify people.
      return InvitationHandler::ResultNoSendingNeeded;
    }
  }

  Q_ASSERT( false ); // Shouldn't happen.
  return ResultNoSendingNeeded;
}

InvitationHandler::SendResult
InvitationHandler::sendCounterProposal( const KCalCore::Incidence::Ptr &oldEvent,
                                        const KCalCore::Incidence::Ptr &newEvent ) const
{
  if ( !oldEvent || !newEvent || *oldEvent == *newEvent ||
       !KCalPrefs::instance()->mUseGroupwareCommunication ) {
    return InvitationHandler::ResultNoSendingNeeded;
  }

  if ( KCalPrefs::instance()->outlookCompatCounterProposals() ) {
    KCalCore::Incidence::Ptr tmp( oldEvent->clone() );
    tmp->setSummary( i18n( "Counter proposal: %1", newEvent->summary() ) );
    tmp->setDescription( newEvent->description() );
    tmp->addComment( proposalComment( newEvent ) );

    // TODO: Shouldn't we ask here?
    return d->sentInvitation( KMessageBox::Yes, tmp, KCalCore::iTIPReply );
  } else {
    return d->sentInvitation( KMessageBox::Yes, newEvent, KCalCore::iTIPCounter );
  }
}

void InvitationHandler::calendarJobFinished( bool success, const QString &errorString )
{
  QWeakPointer<NepomukCalendar> weakPtr = qobject_cast<NepomukCalendar*>( sender() )->weakPointer();
  NepomukCalendar::Ptr calendar( weakPtr.toStrongRef() );

  if ( !calendar->jobsInProgress() ) {
    delete d->mInvitationByCalendar.take( calendar );
    emit handleInvitationFinished( success, errorString );
  }
}

#include "invitationhandler.moc"
