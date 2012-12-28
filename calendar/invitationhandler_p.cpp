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

#include "invitationhandler_p.h"
#include <kcalcore/incidence.h>
#include <KMessageBox>
#include <KLocale>

using namespace Akonadi;

InvitationHandler::Private::Private( InvitationHandler *qq )
                                     : m_calendarLoadError( false )
                                     , m_calendar( FetchJobCalendar::Ptr( new FetchJobCalendar() ) )
                                     , m_scheduler( new MailScheduler( qq ) )
                                     , m_method( KCalCore::iTIPNoMethod )
                                     , m_helper( new InvitationHandlerHelper() ) //TODO parent
                                     , m_currentOperation( OperationNone )
                                     , q( qq )
{
  connect( m_scheduler, SIGNAL(transactionFinished(Akonadi::Scheduler::Result,QString)),
           SLOT(onSchedulerFinished(Akonadi::Scheduler::Result,QString)) );

  connect( m_helper, SIGNAL(finished(Akonadi::InvitationHandlerHelper::SendResult,QString)),
           SLOT(onHelperFinished(Akonadi::InvitationHandlerHelper::SendResult,QString)) );

  connect( m_calendar.data(), SIGNAL(loadFinished(bool,QString)), SLOT(onLoadFinished(bool,QString)) );
}

void InvitationHandler::Private::onSchedulerFinished( Akonadi::Scheduler::Result result,
                                                      const QString &errorMessage )
{
  Q_ASSERT( m_currentOperation != OperationProcessiTIPMessage );
  if ( m_currentOperation == OperationNone ) {
    kError() << "Operation can't be none!" << result << errorMessage;
    return;
  }

  if ( m_currentOperation == OperationProcessiTIPMessage ) {
    m_currentOperation = OperationNone;
    finishProcessiTIPMessage( result, errorMessage );
  } else if ( m_currentOperation == OperationSendiTIPMessage ) {
    m_currentOperation = OperationNone;
    finishSendiTIPMessage( result, errorMessage );
  } else if ( m_currentOperation == OperationPublishInformation ) {
    m_currentOperation = OperationNone;
    finishPublishInformation( result, errorMessage );
  } else {
    Q_ASSERT( false );
    kError() << "Unknown operation" << m_currentOperation;
  }
}

void InvitationHandler::Private::onHelperFinished( Akonadi::InvitationHandlerHelper::SendResult result,
                                                   const QString &errorMessage )
{
  const bool success = result == InvitationHandlerHelper::ResultSuccess;
  emit q->iTipMessageSent( success ? ResultSuccess : ResultError,
                           success ? QString() : i18n( "Error: %1", errorMessage ) );
}

void InvitationHandler::Private::onLoadFinished( bool success, const QString &errorMessage )
{
  if ( m_currentOperation == OperationProcessiTIPMessage ) {
    if ( success ) {
      q->processiTIPMessage( m_queuedInvitation.receiver,
                             m_queuedInvitation.iCal,
                             m_queuedInvitation.action );
    } else {
      emit q->iTipMessageSent( ResultError, i18n( "Error loading calendar: %1", errorMessage ) );
    }
  } else if ( m_currentOperation ==  OperationSendiTIPMessage ) {
    q->sendiTIPMessage( m_queuedInvitation.method,
                        m_queuedInvitation.incidence,
                        m_parentWidget );
  } else if ( !success ) { //TODO
    m_calendarLoadError = true;
  }
}

void InvitationHandler::Private::finishProcessiTIPMessage( Akonadi::MailScheduler::Result result,
                                                           const QString &errorMessage )
{
  const bool success = result == MailScheduler::ResultSuccess;

  if ( m_method != KCalCore::iTIPCounter) {
    if ( success ) {
      // send update to all attendees
      Q_ASSERT( m_incidence );
      InvitationHandlerHelper::SendResult sendResult
         = m_helper->sendIncidenceModifiedMessage( KCalCore::iTIPRequest,
                                                   KCalCore::Incidence::Ptr( m_incidence->clone() ),
                                                   false );
      m_incidence.clear();
      if ( sendResult == InvitationHandlerHelper::ResultNoSendingNeeded ||
           sendResult == InvitationHandlerHelper::ResultCanceled ) {
        emit q->iTipMessageSent( ResultSuccess, QString() );
      } else {
        // InvitationHandlerHelper is working hard and slot onHelperFinished will be called soon
        return;
      }
    } else {
      //fall through
    }
  }

  emit q->iTipMessageSent( success ? ResultSuccess : ResultError,
                           success ? QString() : i18n( "Error: %1", errorMessage ) );
}

void InvitationHandler::Private::finishSendiTIPMessage( Akonadi::MailScheduler::Result result,
                                                        const QString &errorMessage )
{
  if ( result == Scheduler::ResultSuccess ) {
    if ( m_parentWidget ) {
      KMessageBox::information( m_parentWidget,
                          i18n( "The groupware message for item '%1' "
                                "was successfully sent.\nMethod: %2",
                                m_queuedInvitation.incidence->summary(),
                                KCalCore::ScheduleMessage::methodName( m_queuedInvitation.method ) ),
                          i18n( "Sending Free/Busy" ),
                          "FreeBusyPublishSuccess" );
    }
    emit q->iTipMessageSent( InvitationHandler::ResultSuccess, QString() );
  } else {
     const QString error = i18nc( "Groupware message sending failed. "
                                  "%2 is request/reply/add/cancel/counter/etc.",
                                  "Unable to send the item '%1'.\nMethod: %2",
                                  m_queuedInvitation.incidence->summary(),
                                  KCalCore::ScheduleMessage::methodName( m_queuedInvitation.method ) );
    if ( m_parentWidget ) {
      KMessageBox::error( m_parentWidget, error );
    }
    kError() << "Groupware message sending failed." << error << errorMessage;
    emit q->iTipMessageSent( InvitationHandler::ResultError, error + errorMessage );
  }
}

void InvitationHandler::Private::finishPublishInformation( Akonadi::MailScheduler::Result result,
                                                           const QString &errorMessage )
{
  if ( result == Scheduler::ResultSuccess ) {
    if ( m_parentWidget ) {
      KMessageBox::information( m_parentWidget,
                                i18n( "The item information was successfully sent." ),
                                i18n( "Publishing" ),
                                "IncidencePublishSuccess" );
    }
    emit q->informationPublished( InvitationHandler::ResultSuccess, QString() );
  } else {
    const QString error = i18n( "Unable to publish the item '%1'",
                                m_queuedInvitation.incidence->summary() );
    if ( m_parentWidget ) {
      KMessageBox::error( m_parentWidget, error );
    }
    kError() << "Publish failed." << error << errorMessage;
    emit q->informationPublished( InvitationHandler::ResultError, error + errorMessage );
  }
}

void InvitationHandler::Private::finishSendAsICalendar( Akonadi::MailScheduler::Result result,
                                                        const QString &errorMessage )
{
  if ( result == Scheduler::ResultSuccess ) {
    if ( m_parentWidget ) {
      KMessageBox::information( m_parentWidget,
                                i18n( "The item information was successfully sent." ),
                                i18n( "Forwarding" ),
                                "IncidenceForwardSuccess" );
    }
    emit q->sentAsICalendar( InvitationHandler::ResultSuccess, QString() );
  } else {
    if ( m_parentWidget ) {
      KMessageBox::error( m_parentWidget,
                          i18n( "Unable to forward the item '%1'",
                                m_queuedInvitation.incidence->summary() ),
                          i18n( "Forwarding Error" ) );
    }
    kError() << "Sent as iCalendar failed." << errorMessage;
    emit q->sentAsICalendar( InvitationHandler::ResultError, errorMessage );
  }

  sender()->deleteLater(); // Delete the mailer
}

#include "invitationhandler_p.moc"
