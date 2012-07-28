/*
  Copyright (c) 2001,2004 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2010,2012 Sérgio Martins <iamsergio@gmail.com>

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

#include "mailscheduler_p.h"
#include "fetchjobcalendar.h"

#include <Akonadi/Item>

#include <KCalCore/ICalFormat>
#include <KCalCore/Incidence>
#include <KCalCore/ScheduleMessage>
#include <KCalUtils/Scheduler>
#include <KPIMIdentities/IdentityManager>

#include <KStandardDirs>
#include <KSystemTimeZone>
#include <KLocale>
#include <KEMailSettings>

#include <QDir>

using namespace Akonadi;
using namespace KPIMIdentities;

class MailScheduler::Private {
public:
  bool m_bccMe;
  QString m_transport;
  KPIMIdentities::IdentityManager *m_identityManager;
  MailClient *m_mailer;
};

static QString email()
{
  KEMailSettings emailSettings;
  return emailSettings.getSetting( KEMailSettings::EmailAddress );
}

MailScheduler::MailScheduler( const Akonadi::FetchJobCalendar::Ptr &calendar,
                              bool bccMe,
                              const QString &mailTransport,
                              QObject *parent ) : Scheduler( calendar, parent )
                                                , d( new Private() )

{
  d->m_bccMe = bccMe;
  d->m_transport = mailTransport;
  d->m_identityManager = new IdentityManager( /*ro=*/true, this );
  d->m_mailer = new MailClient();
  connect( d->m_mailer, SIGNAL(finished(Akonadi::MailClient::Result,QString)),
           SLOT(onMailerFinished(Akonadi::MailClient::Result,QString)) );
}

MailScheduler::~MailScheduler()
{
  delete d->m_mailer;
  delete d;
}

void MailScheduler::publish( const KCalCore::IncidenceBase::Ptr &incidence,
                             const QString &recipients )
{
  Q_ASSERT( incidence );
  if ( !incidence )
    return;

  const QString messageText = mFormat->createScheduleMessage( incidence, KCalCore::iTIPPublish );
  d->m_mailer->mailTo( incidence,
                       d->m_identityManager->identityForAddress( email() ),
                       email(), d->m_bccMe, recipients, messageText,
                       d->m_transport );
}

void MailScheduler::performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                                        KCalCore::iTIPMethod method,
                                        const QString &recipients )
{
  Q_ASSERT( incidence );
  if ( !incidence )
    return;
  const QString messageText = mFormat->createScheduleMessage( incidence, method );

  // TODO: Catch signal
  d->m_mailer->mailTo( incidence,
                       d->m_identityManager->identityForAddress( email() ),
                       email(), d->m_bccMe, recipients, messageText,
                       d->m_transport );
}

void MailScheduler::performTransaction( const KCalCore::IncidenceBase::Ptr &incidence,
                                        KCalCore::iTIPMethod method )
{
  Q_ASSERT( incidence );
  if ( !incidence )
    return;

  const QString messageText = mFormat->createScheduleMessage( incidence, method );

  if ( method == KCalCore::iTIPRequest ||
       method == KCalCore::iTIPCancel ||
       method == KCalCore::iTIPAdd ||
       method == KCalCore::iTIPDeclineCounter ) {
       // TODO handle error
    d->m_mailer->mailAttendees( incidence,
                                d->m_identityManager->identityForAddress( email() ),
                                d->m_bccMe, messageText, d->m_transport );
  } else {
    QString subject;
    KCalCore::Incidence::Ptr inc = incidence.dynamicCast<KCalCore::Incidence>() ;
    if ( inc && method == KCalCore::iTIPCounter ) {
      subject = i18n( "Counter proposal: %1", inc->summary() );
    }
    // TODO: handle error
    d->m_mailer->mailOrganizer( incidence,
                                d->m_identityManager->identityForAddress( email() ),
                                email(), d->m_bccMe, messageText, subject, d->m_transport );
  }
}

QString MailScheduler::freeBusyDir() const
{
  return KStandardDirs::locateLocal( "data", QLatin1String( "korganizer/freebusy" ) );
}

//TODO: AKONADI_PORT review following code
void MailScheduler::acceptCounterProposal( const KCalCore::Incidence::Ptr &incidence )
{
  Q_ASSERT( incidence );
  if ( !incidence )
    return;

  Akonadi::Item exInc = mCalendar->item( incidence->uid() );
  if ( !exInc.isValid() ) {
    KCalCore::Incidence::Ptr exIncidence = mCalendar->incidenceFromSchedulingID( incidence->uid() );
    if ( exIncidence ) {
      exInc = mCalendar->item( exIncidence->uid() );
    }
    //exInc = exIncItem.isValid() && exIncItem.hasPayload<KCalCore::Incidence::Ptr>() ?
    //        exIncItem.payload<KCalCore::Incidence::Ptr>() : KCalCore::Incidence::Ptr();
  }

  incidence->setRevision( incidence->revision() + 1 );
  int changeId;
  Result result = ResultSuccess;

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

    changeId = mCalendar->modifyIncidence( exIncPtr );
    result = ResultModifyingError;
  } else {
    changeId = mCalendar->addIncidence( KCalCore::Incidence::Ptr( incidence->clone() ) );
    result = ResultCreatingError;
  }

  if ( changeId > 0 ) {
  } else {
      emit transactionFinished( result, QLatin1String( "Error creating job" ) );
  }
}

void MailScheduler::onMailerFinished( Akonadi::MailClient::Result result,
                                      const QString &errorMsg )
{
  if ( result == MailClient::ResultSuccess ) {
      emit transactionFinished( ResultSuccess, QString() );
  } else {
      const QString message = i18n( "Error sending e-mail: ") + errorMsg;
      emit transactionFinished( ResultGenericError, message );
  }
}
