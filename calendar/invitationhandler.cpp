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

#include "invitationhandler.h"
#include "invitationhandler_p.h"
#include "invitationhandlerhelper_p.h"
#include "calendarsettings.h"

#include <kcalcore/icalformat.h>
#include <kcalcore/incidence.h>
#include <kcalcore/schedulemessage.h>
#include <kcalcore/attendee.h>
#include <kcalutils/stringify.h>

#include <KMessageBox>
#include <KLocale>

using namespace Akonadi;

GroupwareUiDelegate::~GroupwareUiDelegate()
{
}


InvitationHandler::InvitationHandler( QObject *parent )
                  : QObject( parent )
                  , d( new Private( this ) )
{
}

InvitationHandler::~InvitationHandler()
{
  delete d;
}

void InvitationHandler::handleInvitation( const QString &receiver,
                                          const QString &iCal,
                                          const QString &action )
{
  if ( !d->m_calendar->isLoaded() ) {
    d->m_queuedInvitation.receiver = receiver;
    d->m_queuedInvitation.iCal     = iCal;
    d->m_queuedInvitation.action   = action;
    d->m_handleInvitationCalled = true;
    return;
  }

  if ( d->m_calendarLoadError ) {
    emit finished( ResultError, i18n( "Error loading calendar." ) );
    return;
  }

  KCalCore::ICalFormat format;
  KCalCore::ScheduleMessage::Ptr message = format.parseScheduleMessage( d->m_calendar, iCal );

  if ( !message ) {
    const QString errorMessage = format.exception() ? i18n( "Error message: %1", KCalUtils::Stringify::errorMessage( *format.exception() ) )
                                                    : i18n( "Unknown error while parsing iCal invitation" );

    kError() << "Error parsing" << errorMessage;
    KMessageBox::detailedError( 0,// mParent, TODO
                                i18n( "Error while processing an invitation or update." ),
                                errorMessage );
    emit finished( ResultError, errorMessage );
    return;
  }

  d->m_method = static_cast<KCalCore::iTIPMethod>( message->method() );
  d->m_incidence.clear();
  
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
        } else if ( CalendarSettings::self()->outlookCompatCounterProposals() &&
                    action.startsWith( QLatin1String( "counter" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Tentative );
        } else if ( action.startsWith( QLatin1String( "delegated" ) ) ) {
          attendee->setStatus( KCalCore::Attendee::Delegated );
        }
        break;
      }
    }
    if ( CalendarSettings::self()->outlookCompatCounterProposals() ||
         !action.startsWith( QLatin1String( "counter" ) ) ) {
      d->m_scheduler->acceptTransaction( incidence, d->m_calendar, d->m_method, status, receiver );
      return; // signal emitted in onSchedulerFinished().
    }
    //TODO: what happens here? we must emit a signal
  } else if ( action.startsWith( QLatin1String( "cancel" ) ) ) {
    // Delete the old incidence, if one is present
    d->m_scheduler->acceptTransaction( incidence, d->m_calendar, KCalCore::iTIPCancel, status, receiver );
    return; // signal emitted in onSchedulerFinished().
  } else if ( action.startsWith( QLatin1String( "reply" ) ) ) {
    if ( d->m_method != KCalCore::iTIPCounter ) {
      d->m_scheduler->acceptTransaction( incidence, d->m_calendar, d->m_method, status, QString() );
    } else {
      d->m_incidence = incidence; // so we can access it in the slot
      d->m_scheduler->acceptCounterProposal( incidence, d->m_calendar );
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

#include "invitationhandler.moc"
