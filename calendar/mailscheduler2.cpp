/*
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2010 Sérgio Martins <iamsergio@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "mailscheduler2.h"
#include "kcalprefs.h"
#include "identitymanager.h"
#include "mailclient.h"
#include "utils.h"
#include "incidencechanger.h"
#include "fetchjobcalendar.h"

#include <Akonadi/Item>

#include <KCalCore/ICalFormat>
#include <KCalCore/Incidence>
#include <KCalCore/ScheduleMessage>

#include <KCalUtils/Scheduler>

#include <KStandardDirs>
#include <KSystemTimeZone>

#include <QDir>

using namespace CalendarSupport;

class MailScheduler2::Private {
  public:
    Private()
    {
      mFormat = new KCalCore::ICalFormat();
    }

   ~Private()
    {
      delete mFormat;
    }

    QHash<int, CallId> mCallIdByChangeId;
    KCalCore::ICalFormat *mFormat;
};


MailScheduler2::MailScheduler2( const CalendarSupport::FetchJobCalendar::Ptr &cal )
  : Scheduler( cal, new IncidenceChanger() ), d( new Private )
{
  if ( calendar() ) {
    d->mFormat->setTimeSpec( cal->timeSpec() );
  } else {
    d->mFormat->setTimeSpec( KSystemTimeZones::local() );
  }

  connect( changer(), SIGNAL(createFinished(int,Akonadi::Item,CalendarSupport::IncidenceChanger::ResultCode,QString)),
          SLOT(createFinished(int,Akonadi::Item,CalendarSupport::IncidenceChanger::ResultCode,QString)) );

  connect( changer(),SIGNAL(modifyFinished(int,Akonadi::Item,CalendarSupport::IncidenceChanger::ResultCode,QString)),
          SLOT(modifyFinished(int,Akonadi::Item,CalendarSupport::IncidenceChanger::ResultCode,QString)) );
}

MailScheduler2::~MailScheduler2()
{
  delete d;
}

CallId MailScheduler2::publish( const KCalCore::IncidenceBase::Ptr &incidence,
                                const QString &recipients )
{
  if ( !incidence ) {
    return -1;
  }

  const QString from = KCalPrefs::instance()->email();
  const bool bccMe = KCalPrefs::instance()->mBcc;
  const QString messageText = d->mFormat->createScheduleMessage( incidence, KCalCore::iTIPPublish );

  MailClient mailer;
  // TODO: Why doesn't MailClient return an error message too?, a bool is not enough.
  // TODO: Refactor MailClient, currently it execs()
  const bool result = mailer.mailTo( incidence, CalendarSupport::identityManager()->identityForAddress( from ),
                                     from, bccMe, recipients, messageText,
                                     KCalPrefs::instance()->mailTransport() );

  ResultCode resultCode = ResultCodeSuccess;
  QString errorMessage;
  if ( !result ) {
    errorMessage = QLatin1String( "Error sending e-mail" );
    resultCode = ResultCodeErrorSendingEmail;
  }
  const CallId callId = nextCallId();
  emitOperationFinished( callId, resultCode, errorMessage );
  return callId;
}

CallId MailScheduler2::performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                                           KCalCore::iTIPMethod method,
                                           const QString &recipients )
{
  if ( !incidence ) {
    return -1;
  }

  const QString from = KCalPrefs::instance()->email();
  const bool bccMe = KCalPrefs::instance()->mBcc;
  const QString messageText = d->mFormat->createScheduleMessage( incidence, method );

  MailClient mailer;
  // TODO: Why doesn't MailClient return an error message too?, a bool is not enough.
  // TODO: Refactor MailClient, currently it execs()
  const bool result = mailer.mailTo( incidence, CalendarSupport::identityManager()->identityForAddress( from ),
                                     from, bccMe, recipients, messageText,
                                     KCalPrefs::instance()->mailTransport() );

  ResultCode resultCode = ResultCodeSuccess;
  QString errorMessage;
  if ( !result ) {
    errorMessage = QLatin1String( "Error sending e-mail" );
    resultCode = ResultCodeErrorSendingEmail;
  }
  const CallId callId = nextCallId();
  emitOperationFinished( callId, resultCode, errorMessage );
  return callId;
}

CallId MailScheduler2::performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                                           KCalCore::iTIPMethod method )
{
  if ( !incidence ) {
    return -1;
  }

  const QString from = KCalPrefs::instance()->email();
  const bool bccMe = KCalPrefs::instance()->mBcc;
  const QString messageText = d->mFormat->createScheduleMessage( incidence, method );

  MailClient mailer;
  bool status;
  if ( method == KCalCore::iTIPRequest ||
       method == KCalCore::iTIPCancel ||
       method == KCalCore::iTIPAdd ||
       method == KCalCore::iTIPDeclineCounter ) {
    status = mailer.mailAttendees(
      incidence,
      CalendarSupport::identityManager()->identityForAddress( from ),
      bccMe, messageText, KCalPrefs::instance()->mailTransport() );
  } else {
    QString subject;
    KCalCore::Incidence::Ptr inc = incidence.dynamicCast<KCalCore::Incidence>() ;
    if ( inc && method == KCalCore::iTIPCounter ) {
      subject = i18n( "Counter proposal: %1", inc->summary() );
    }
    status = mailer.mailOrganizer(
      incidence,
      CalendarSupport::identityManager()->identityForAddress( from ),
      from, bccMe, messageText, subject, KCalPrefs::instance()->mailTransport() );
  }

  ResultCode resultCode = ResultCodeSuccess;
  QString errorMessage;
  if ( !status ) {
    errorMessage = QLatin1String( "Error sending e-mail" );
    resultCode = ResultCodeErrorSendingEmail;
  }
  const CallId callId = nextCallId();
  emitOperationFinished( callId, resultCode, errorMessage );
  return callId;
}

QString MailScheduler2::freeBusyDir() const
{
  return KStandardDirs::locateLocal( "data", QLatin1String( "korganizer/freebusy" ) );
}

//AKONADI_PORT review following code
CallId MailScheduler2::acceptCounterProposal( const KCalCore::Incidence::Ptr &incidence )
{
  if ( !incidence ) {
    return -1;
  }

  Akonadi::Item exInc = calendar()->item( incidence->uid() );
  if ( !exInc.isValid() ) {
    KCalCore::Incidence::Ptr exIncidence = calendar()->incidenceFromSchedulingID( incidence->uid() );
    if ( exIncidence ) {
      exInc = calendar()->item( exIncidence->uid() );
    }
    //exInc = exIncItem.isValid() && exIncItem.hasPayload<KCalCore::Incidence::Ptr>() ?
    //        exIncItem.payload<KCalCore::Incidence::Ptr>() : KCalCore::Incidence::Ptr();
  }

  incidence->setRevision( incidence->revision() + 1 );
  int changeId;
  ResultCode inCaseOfError;
  const CallId callId = nextCallId();
  if ( exInc.isValid() && exInc.hasPayload<KCalCore::Incidence::Ptr>() ) {
    KCalCore::Incidence::Ptr exIncPtr = exInc.payload<KCalCore::Incidence::Ptr>();
    incidence->setRevision( qMax( incidence->revision(), exIncPtr->revision() + 1 ) );
    // some stuff we don't want to change, just to be safe
    incidence->setSchedulingID( exIncPtr->schedulingID() );
    incidence->setUid( exIncPtr->uid() );

    Q_ASSERT( exIncPtr && incidence );

    KCalCore::IncidenceBase::Ptr i1 = exIncPtr;
    KCalCore::IncidenceBase::Ptr i2 = incidence;

    if ( i1->type() == i2->type() ) {
      *i1 = *i2;
    }

    exIncPtr->updated();

    changeId = changer()->modifyIncidence( exInc );
    d->mCallIdByChangeId.insert( changeId, callId );
    inCaseOfError = ResultCodeErrorUpdatingIncidence;
  } else {
    changeId = changer()->createIncidence( KCalCore::Incidence::Ptr( incidence->clone() ) );
    inCaseOfError = ResultCodeErrorCreatingIncidence;
  }

  if ( changeId > 0 ) {
    d->mCallIdByChangeId.insert( changeId, callId );
  } else {
    emitOperationFinished( callId, inCaseOfError, QLatin1String( "Error creating job" ) );
  }

  return callId;
}

void MailScheduler2::modifyFinished( int changeId,
                                    const Akonadi::Item &item,
                                    IncidenceChanger::ResultCode changerResultCode,
                                    const QString &errorMessage )
{
  if ( d->mCallIdByChangeId.contains( changeId ) ) {
    const ResultCode resultCode = ( changerResultCode == IncidenceChanger::ResultCodeSuccess ) ? ResultCodeSuccess :
                                                                                                  ResultCodeErrorUpdatingIncidence;
    emitOperationFinished( changeId, resultCode, errorMessage );
    d->mCallIdByChangeId.remove( changeId );
  }

  Q_UNUSED( item );
}

void MailScheduler2::createFinished( int changeId,
                                     const Akonadi::Item &item,
                                     IncidenceChanger::ResultCode changerResultCode,
                                     const QString &errorMessage )
{
  if ( d->mCallIdByChangeId.contains( changeId ) ) {
    const ResultCode resultCode = ( changerResultCode == IncidenceChanger::ResultCodeSuccess ) ? ResultCodeSuccess :
                                                                                                  ResultCodeErrorCreatingIncidence;
    emitOperationFinished( changeId, resultCode, errorMessage );
    d->mCallIdByChangeId.remove( changeId );
  }

  Q_UNUSED( item );
}
