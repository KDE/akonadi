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

#include "invitationhandler_p.h"
#include "fetchjobcalendar.h"

#include <KCalCore/Calendar>
#include <KCalCore/ICalFormat>
#include <KCalUtils/IncidenceFormatter>
#include <KCalUtils/Stringify>
#include <KPIMIdentities/IdentityManager>
#include <KPIMIdentities/Identity>
#include <KMime/HeaderParsing>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KEMailSettings>

using namespace Akonadi;
using namespace KPIMIdentities;

namespace Akonadi {

GroupwareUiDelegate::~GroupwareUiDelegate()
{
}

struct Invitation {
  QString type;
  QString iCal;
  QString receiver;
};

enum Status {
    StatusNone,
    StatusAcceptingCounterProposal,
    StatusSendingInvitation
};

class InvitationHandler::Private
{
public:
  Private( const FetchJobCalendar::Ptr &calendar, QWidget *parent );
  bool thatIsMe( const QString &_email ) const;
  QStringList allEmails() const;

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

  FetchJobCalendar::Ptr mCalendar;
  InvitationHandler::Action mDefaultAction;
  QWidget *mParent;
  bool mOutlookCompatCounterProposals;
  KPIMIdentities::IdentityManager *m_identityManager;
  MailScheduler *m_scheduler;
  Status m_status;
  KCalCore::Incidence::Ptr m_incidence;
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
      const KDateTime dtEnd = incidence->dateTime( KCalCore::Incidence::RoleDisplayEnd );
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
  : mCalendar( calendar )
  , mDefaultAction( InvitationHandler::ActionAsk )
  , mParent( parent )
  , mOutlookCompatCounterProposals( false )
  , m_identityManager( new IdentityManager( /*ro=*/ true, parent ) )
  , m_scheduler( new MailScheduler( calendar, parent ) )
  , m_status( StatusNone )
{
    m_scheduler->setBccMe( false );
}

QStringList InvitationHandler::Private::allEmails() const
{
  // Grab emails from the email identities
  // Warning, this list could contain duplicates.
  return m_identityManager->allEmails();
}

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

bool InvitationHandler::Private::thatIsMe( const QString &_email ) const
{
  // Method copied from kdepim/calendarsupport/kcalprefs.cpp


  // NOTE: this method is called for every created agenda view item,
  // so we need to keep performance in mind

  /* identityManager()->thatIsMe() is quite expensive since it does parsing of
     _email in a way which is unnecessarily complex for what we can have here,
     so we do that ourselves. This makes sense since this

  if ( Akonadi::identityManager()->thatIsMe( _email ) ) {
    return true;
  }
  */

  // in case email contains a full name, strip it out.
  // the below is the simpler but slower version of the following code:
  // const QString email = KPIM::getEmailAddress( _email );
  const QByteArray tmp = _email.toUtf8();
  const char *cursor = tmp.constData();
  const char *end = tmp.data() + tmp.length();
  KMime::Types::Mailbox mbox;
  KMime::HeaderParsing::parseMailbox( cursor, end, mbox );
  const QString email = mbox.addrSpec().asString();

  KEMailSettings emailSettings;
  const QString myEmail = emailSettings.getSetting( KEMailSettings::EmailAddress );

  if ( myEmail == email ) {
    return true;
  }

  IdentityManager::ConstIterator it;
  for ( it = m_identityManager->begin();
        it != m_identityManager->end(); ++it ) {
    if ( (*it).matchesEmailAddress( email ) ) {
      return true;
    }
  }

  // TODO: Remove the additional e-mails stuff from korganizer and test e-mail aliases
  return false;
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
    m_status = StatusSendingInvitation;
    m_scheduler->performTransaction( _incidence, method );
    return InvitationHandler::ResultSuccess;

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
  return thatIsMe( email ) || email.isEmpty() || email == QLatin1String( "invalid@email.address" );
}

bool InvitationHandler::Private::weNeedToSendMailFor( const KCalCore::Incidence::Ptr &incidence )
{
  if ( !weAreOrganizerOf( incidence ) ) {
    kError() << "We should be the organizer of ths incidence."
             << "; email= "       << incidence->organizer()->email()
             << "; thatIsMe() = " << thatIsMe( incidence->organizer()->email() );
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

InvitationHandler::InvitationHandler( const FetchJobCalendar::Ptr &calendar, QWidget *widget )
  : d ( new InvitationHandler::Private( calendar, widget ) )
{
  connect( d->m_scheduler, SIGNAL(transactionFinished(Akonadi::MailScheduler::Result,QString)),
           SLOT(onSchedulerFinished(Akonadi::MailScheduler::Result,QString)) );
}

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
                                          const QString &action )
{
  KCalCore::ICalFormat mFormat;
  KCalCore::ScheduleMessage::Ptr message = mFormat.parseScheduleMessage( d->mCalendar, iCal );

  if ( !message ) {
    const QString errorMessage = mFormat.exception() ? i18n( "Error message: %1", KCalUtils::Stringify::errorMessage( *mFormat.exception() ) )
                                                     : i18n( "Unknown error while parsing iCal invitation" );

    kError() << "Error parsing" << errorMessage;
    KMessageBox::detailedError( d->mParent,
                                i18n( "Error while processing an invitation or update." ),
                                errorMessage );
    emit finished( ResultError, errorMessage );
    return;
  }

  KCalCore::iTIPMethod method = static_cast<KCalCore::iTIPMethod>( message->method() );
  KCalCore::ScheduleMessage::Status status = message->status();
  KCalCore::Incidence::Ptr incidence = message->event().dynamicCast<KCalCore::Incidence>();
  if ( !incidence ) {
    emit finished( ResultError, QLatin1String( "Invalid incidence" ) );
    return;
  }

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
        } else if ( d->mOutlookCompatCounterProposals &&
                    action.startsWith( QLatin1String( "counter" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Tentative );
        } else if ( action.startsWith( QLatin1String( "delegated" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Delegated );
        }
        break;
      }
    }
    if ( d->mOutlookCompatCounterProposals || !action.startsWith( QLatin1String( "counter" ) ) ) {
      d->m_scheduler->acceptTransaction( incidence, method, status, receiver );
      return; // signal emitted in onSchedulerFinished().
    }
  } else if ( action.startsWith( QLatin1String( "cancel" ) ) ) {
    // Delete the old incidence, if one is present
    d->m_scheduler->acceptTransaction( incidence, KCalCore::iTIPCancel, status, receiver );
    return; // signal emitted in onSchedulerFinished().
  } else if ( action.startsWith( QLatin1String( "reply" ) ) ) {
    if ( method != KCalCore::iTIPCounter ) {
      d->m_scheduler->acceptTransaction( incidence, method, status, QString() );
    } else {
      d->m_status = StatusAcceptingCounterProposal;
      d->m_incidence = incidence; // so we can access it in the slot
      d->m_scheduler->acceptCounterProposal( incidence );
    }
    return; // signal emitted in onSchedulerFinished().
  } else {
    kError() << "Unknown incoming action" << action;
    emit finished( ResultError, i18n( "Invalid action: %1", action ) );
  }

  if ( action.startsWith( QLatin1String( "counter" ) ) ) {
    emit editorRequested( KCalCore::Incidence::Ptr( incidence->clone() ) );
  }
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
             << "; thatIsMe() = " << d->thatIsMe( incidence->organizer()->email() );
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

    const QStringList myEmails = d->allEmails();
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
  Q_ASSERT( oldEvent );
  Q_ASSERT( newEvent );

  if ( !oldEvent || !newEvent || *oldEvent == *newEvent )
    return InvitationHandler::ResultNoSendingNeeded;

  if ( d->mOutlookCompatCounterProposals ) {
    KCalCore::Incidence::Ptr tmp( oldEvent->clone() );
    tmp->setSummary( i18n( "Counter proposal: %1", newEvent->summary() ) );
    tmp->setDescription( newEvent->description() );
    tmp->addComment( proposalComment( newEvent ) );

    return d->sentInvitation( KMessageBox::Yes, tmp, KCalCore::iTIPReply );
  } else {
    return d->sentInvitation( KMessageBox::Yes, newEvent, KCalCore::iTIPCounter );
  }
}

void InvitationHandler::setOutlookCompatibleCounterProposals( bool enable )
{
  d->mOutlookCompatCounterProposals = enable;
}

void InvitationHandler::setBccMe( bool enable )
{
  d->m_scheduler->setBccMe( enable );
}

void InvitationHandler::onSchedulerFinished( MailScheduler::Result result, const QString &errorMsg )
{
  const bool success = result == MailScheduler::ResultSuccess;

  if ( d->m_status == StatusAcceptingCounterProposal ) {
    if ( success ) {
      d->m_status = StatusNone;
      // send update to all attendees
      Q_ASSERT( d->m_incidence );
      SendResult sendResult = sendIncidenceModifiedMessage( KCalCore::iTIPRequest,
                                                            KCalCore::Incidence::Ptr( d->m_incidence->clone() ),
                                                            false );
      d->m_incidence.clear();
      if ( sendResult == ResultNoSendingNeeded ||
           sendResult == InvitationHandler::ResultCanceled ) {
        emit finished( sendResult, QString() );
      } else {
        // MailScheduler is working hard and this slot will be called again
        return;
      }
    } else {
      //fall through
    }
  } else if ( d->m_status == StatusSendingInvitation ) {
    d->m_status = StatusNone;
    if ( !success ) {
      const QString question( i18n( "Sending group scheduling email failed." ) );
      const int code = d->askUserIfNeeded( question, true, KGuiItem( i18n( "Abort Update" ) ) );
      if ( code == KMessageBox::Yes ) {
        emit finished( ResultFailAbortUpdate, QString() );
      } else {
        emit finished( ResultFailKeepUpdate, QString() );
      }
      return;
    } else {
      //fall through
    }
  }

  emit finished( success ? ResultSuccess : ResultError,
                 success ? QString() : i18n( "Error: %1", errorMsg ) );
}

QStringList InvitationHandler::allEmails() const
{
  return d->allEmails();
}

bool InvitationHandler::thatIsMe( const QString &email ) const
{
  return d->thatIsMe( email );
}

#include "invitationhandler_p.moc"
