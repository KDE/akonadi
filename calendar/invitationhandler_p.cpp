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

using namespace Akonadi;

InvitationHandler::Private::Private( InvitationHandler *qq )
                                     : m_handleInvitationCalled( false )
                                     , m_calendarLoadError( false )
                                     , m_calendar( FetchJobCalendar::Ptr( new FetchJobCalendar() ) )
                                     , m_scheduler( new MailScheduler( q ) )
                                     , m_method( KCalCore::iTIPNoMethod )
                                     , m_helper( new InvitationHandlerHelper() ) //TODO parent
                                     , q( qq )
{
  connect( m_scheduler, SIGNAL(transactionFinished(Akonadi::MailScheduler::Result,QString)),
           SLOT(onSchedulerFinished(Akonadi::MailScheduler::Result,QString)) );

  connect( m_helper, SIGNAL(finished(Akonadi::InvitationHandlerHelper::SendResult,QString)),
           SLOT(onHelperFinished(Akonadi::InvitationHandlerHelper::SendResult,QString)) );

  connect( m_calendar.data(), SIGNAL(loadFinished(bool,QString)), SLOT(onLoadFinished(bool,QString)) );
}

void InvitationHandler::Private::onSchedulerFinished( Akonadi::MailScheduler::Result result,
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
        emit q->finished( ResultSuccess, QString() );
      } else {
        // InvitationHandlerHelper is working hard and slot onHelperFinished will be called soon
        return;
      }
    } else {
      //fall through
    }
  }

  emit q->finished( success ? ResultSuccess : ResultError,
                    success ? QString() : i18n( "Error: %1", errorMessage ) );
  
}

void InvitationHandler::Private::onHelperFinished( Akonadi::InvitationHandlerHelper::SendResult result,
                                                   const QString &errorMessage )
{
  const bool success = result == InvitationHandlerHelper::ResultSuccess;
  emit q->finished( success ? ResultSuccess : ResultError,
                    success ? QString() : i18n( "Error: %1", errorMessage ) );
}

void InvitationHandler::Private::onLoadFinished( bool success, const QString &errorMessage )
{
  if ( m_handleInvitationCalled ) {
    if ( success ) {
      q->handleInvitation( m_queuedInvitation.receiver,
                           m_queuedInvitation.iCal,
                           m_queuedInvitation.action );
    } else {
      emit q->finished( ResultError, i18n( "Error loading calendar: %1", errorMessage ) );
    }
  } else if ( !success ) {
    m_calendarLoadError = true;
  }
}


#include "invitationhandler_p.moc"
