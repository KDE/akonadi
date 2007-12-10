/*
    This file is part of libakonadi.

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
#include "imapparser.h"
#include "session.h"

#include <kdebug.h>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

using namespace Akonadi;

void Job::Private::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( mCurrentSubJob ) {
    mCurrentSubJob->d->handleResponse( tag, data );
    return;
  }

  if ( tag == mTag ) {
    if ( data.startsWith( "NO " ) || data.startsWith( "BAD " ) ) {
      QString msg = QString::fromUtf8( data );

      msg.remove( 0, msg.startsWith( QLatin1String( "NO " ) ) ? 3 : 4 );

      if ( msg.endsWith( QLatin1String( "\r\n" ) ) )
        msg.chop( 2 );

      mParent->setError( Unknown );
      mParent->setErrorText( msg );
      mParent->emitResult();
      return;
    } else if ( data.startsWith( "OK" ) ) {
      mParent->emitResult();
      return;
    }
  }

  mParent->doHandleResponse( tag, data );
}

void Job::Private::startQueued()
{
  emit mParent->aboutToStart( mParent );
  mParent->doStart();
}

void Job::Private::lostConnection()
{
  if ( mCurrentSubJob ) {
    mCurrentSubJob->d->lostConnection();
  } else {
    mParent->kill( KJob::Quietly );
    mParent->setError( ConnectionFailed );
    mParent->emitResult();
  }
}

void Job::Private::slotSubJobAboutToStart( Job * job )
{
  Q_ASSERT( mCurrentSubJob == 0 );
  mCurrentSubJob = job;
}

void Job::Private::startNext()
{
  if ( !mCurrentSubJob && mParent->hasSubjobs() ) {
    Job *job = dynamic_cast<Akonadi::Job*>( mParent->subjobs().first() );
    Q_ASSERT( job );
    job->d->startQueued();
  }
}


Job::Job( QObject *parent )
  : KCompositeJob( parent ), d( new Private( this ) )
{
  d->mParentJob = dynamic_cast<Job*>( parent );
  d->mCurrentSubJob = 0;
  d->mSession = dynamic_cast<Session*>( parent );

  if ( !d->mSession ) {
    if ( !d->mParentJob )
      d->mSession = Session::defaultSession();
    else
      d->mSession = d->mParentJob->d->mSession;
  }

  if ( !d->mParentJob )
    d->mSession->addJob( this );
  else
    d->mParentJob->addSubjob( this );
}

Job::~Job()
{
  delete d;
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
      str = tr( "Can't connect to pim storage service." );
      break;
    case UserCanceled:
      str = tr( "User canceled transmission." );
      break;
    case Unknown:
    default:
      str = tr( "Unknown Error." );
      break;
  }
  if ( !errorText().isEmpty() ) {
    str += QString::fromLatin1( " (%1)" ).arg( errorText() );
  }
  return str;
}

QByteArray Job::newTag( )
{
  if ( d->mParentJob )
    d->mTag = d->mParentJob->newTag();
  else
    d->mTag = QByteArray::number( d->mSession->nextTag() );
  return d->mTag;
}

QByteArray Job::tag() const
{
  return d->mTag;
}

void Job::writeData( const QByteArray & data )
{
  Q_ASSERT_X( !d->writeFinished, "Job::writeData()", "Calling writeData() after emitting writeFinished()" );
  d->mSession->writeData( data );
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

void Job::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  kDebug( 5250 ) << "Unhandled response: " << tag << data;
}

QByteArray Job::sessionId() const
{
  if ( d->mParentJob )
    return d->mParentJob->sessionId();
  return d->mSession->sessionId();
}

void Job::slotResult(KJob * job)
{
  Q_ASSERT( job == d->mCurrentSubJob );
  KCompositeJob::slotResult( job );
  d->mCurrentSubJob = 0;
  if ( !job->error() )
    QTimer::singleShot( 0, this, SLOT(startNext()) );
}

void Job::emitWriteFinished()
{
  d->writeFinished = true;
  emit writeFinished( this );
}

#include "job.moc"
