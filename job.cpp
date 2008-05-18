/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>
                  2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "job.h"
#include "job_p.h"
#include "imapparser_p.h"
#include "session.h"
#include "session_p.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

using namespace Akonadi;

//@cond PRIVATE
void JobPrivate::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_Q( Job );

  if ( mCurrentSubJob ) {
    mCurrentSubJob->d_ptr->handleResponse( tag, data );
    return;
  }

  if ( tag == mTag ) {
    if ( data.startsWith( "NO " ) || data.startsWith( "BAD " ) ) {
      QString msg = QString::fromUtf8( data );

      msg.remove( 0, msg.startsWith( QLatin1String( "NO " ) ) ? 3 : 4 );

      if ( msg.endsWith( QLatin1String( "\r\n" ) ) )
        msg.chop( 2 );

      q->setError( Job::Unknown );
      q->setErrorText( msg );
      q->emitResult();
      return;
    } else if ( data.startsWith( "OK" ) ) {
      q->emitResult();
      return;
    }
  }

  q->doHandleResponse( tag, data );
}

void JobPrivate::init( QObject *parent )
{
  Q_Q( Job );

  mParentJob = dynamic_cast<Job*>( parent );
  mSession = dynamic_cast<Session*>( parent );

  if ( !mSession ) {
    if ( !mParentJob )
      mSession = Session::defaultSession();
    else
      mSession = mParentJob->d_ptr->mSession;
  }

  if ( !mParentJob )
    mSession->d->addJob( q );
  else
    mParentJob->addSubjob( q );
}

void JobPrivate::startQueued()
{
  Q_Q( Job );

  emit q->aboutToStart( q );
  q->doStart();
}

void JobPrivate::lostConnection()
{
  Q_Q( Job );

  if ( mCurrentSubJob ) {
    mCurrentSubJob->d_ptr->lostConnection();
  } else {
    q->setError( Job::ConnectionFailed );
    q->kill( KJob::EmitResult );
  }
}

void JobPrivate::slotSubJobAboutToStart( Job * job )
{
  Q_ASSERT( mCurrentSubJob == 0 );
  mCurrentSubJob = job;
}

void JobPrivate::startNext()
{
  Q_Q( Job );

  if ( !mCurrentSubJob && q->hasSubjobs() ) {
    Job *job = dynamic_cast<Akonadi::Job*>( q->subjobs().first() );
    Q_ASSERT( job );
    job->d_ptr->startQueued();
  }
}

QByteArray JobPrivate::newTag( )
{
  if ( mParentJob )
    mTag = mParentJob->d_ptr->newTag();
  else
    mTag = QByteArray::number( mSession->d->nextTag() );
  return mTag;
}

QByteArray JobPrivate::tag() const
{
  return mTag;
}

void JobPrivate::writeData( const QByteArray & data )
{
  Q_ASSERT_X( !mWriteFinished, "Job::writeData()", "Calling writeData() after emitting writeFinished()" );
  mSession->d->writeData( data );
}
//@endcond


Job::Job( QObject *parent )
  : KCompositeJob( parent ),
    d_ptr( new JobPrivate( this ) )
{
  d_ptr->init( parent );
}

Job::Job( JobPrivate *dd, QObject *parent )
  : KCompositeJob( parent ),
    d_ptr( dd )
{
  d_ptr->init( parent );
}

Job::~Job()
{
  delete d_ptr;
}

void Job::start()
{
}

bool Job::doKill()
{
  return true;
}

QString Job::errorString() const
{
  QString str;
  switch ( error() ) {
    case NoError:
      break;
    case ConnectionFailed:
      str = i18n( "Cannot connect to the Akonadi service." );
      break;
    case ProtocolVersionMismatch:
      str = i18n( "The protocol version of the Akonadi server is incompatible. Make sure you have a compatible version installed." );
      break;
    case UserCanceled:
      str = i18n( "User canceled operation." );
      break;
    case Unknown:
    default:
      str = i18n( "Unknown error." );
      break;
  }
  if ( !errorText().isEmpty() ) {
    str += QString::fromLatin1( " (%1)" ).arg( errorText() );
  }
  return str;
}

bool Job::addSubjob( KJob * job )
{
  bool rv = KCompositeJob::addSubjob( job );
  if ( rv ) {
    connect( job, SIGNAL(aboutToStart(Akonadi::Job*)), SLOT(slotSubJobAboutToStart(Akonadi::Job*)) );
    QTimer::singleShot( 0, this, SLOT(startNext()) );
  }
  return rv;
}

bool Job::removeSubjob(KJob * job)
{
  bool rv = KCompositeJob::removeSubjob( job );
  if ( job == d_ptr->mCurrentSubJob ) {
    d_ptr->mCurrentSubJob = 0;
    QTimer::singleShot( 0, this, SLOT(startNext()) );
  }
  return rv;
}

void Job::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  kDebug( 5250 ) << "Unhandled response: " << tag << data;
}

void Job::slotResult(KJob * job)
{
  Q_ASSERT( job == d_ptr->mCurrentSubJob );
  d_ptr->mCurrentSubJob = 0;
  KCompositeJob::slotResult( job );
  if ( !job->error() )
    QTimer::singleShot( 0, this, SLOT(startNext()) );
}

void Job::emitWriteFinished()
{
  d_ptr->mWriteFinished = true;
  emit writeFinished( this );
}

#include "job.moc"
