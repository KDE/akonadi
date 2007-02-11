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

#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include "job.h"
#include "imapparser.h"
#include "session.h"

using namespace Akonadi;

DataReference::DataReference()
  : mPersistanceID( 0 ), mIsNull( true ), d( 0 )
{
}

DataReference::DataReference( uint persistanceID, const QString &externalUrl )
  : mPersistanceID( persistanceID ), mExternalUrl( externalUrl ), mIsNull( false ), d( 0 )
{
}

DataReference::~DataReference()
{
}

uint DataReference::persistanceID() const
{
  return mPersistanceID;
}

QUrl DataReference::externalUrl() const
{
  return mExternalUrl;
}

bool DataReference::isNull() const
{
  return mIsNull;
}

bool DataReference::operator==( const DataReference & other ) const
{
  return mPersistanceID == other.mPersistanceID;
}

bool DataReference::operator !=( const DataReference & other ) const
{
  return mPersistanceID != other.mPersistanceID;
}

bool DataReference::operator<( const DataReference & other ) const
{
  return mPersistanceID < other.mPersistanceID;
}

uint qHash( const DataReference& ref )
{
  return qHash( ref.persistanceID() );
}


class Job::JobPrivate
{
  public:
    Job *parent;
    Job *currentSubJob;
    QByteArray tag;
    Session* session;
};

Job::Job( QObject *parent )
  : KCompositeJob( parent ), d( new JobPrivate )
{
  d->parent = dynamic_cast<Job*>( parent );
  d->currentSubJob = 0;
  d->session = dynamic_cast<Session*>( parent );

  if ( !d->session ) {
    if ( !d->parent )
      d->session = Session::defaultSession();
    else
      d->session = d->parent->d->session;
  }

  if ( !d->parent )
    d->session->addJob( this );
  else
    d->parent->addSubjob( this );
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
  if ( d->parent )
    d->tag = d->parent->newTag();
  else
    d->tag = QByteArray::number( d->session->nextTag() );
  return d->tag;
}

QByteArray Job::tag() const
{
  return d->tag;
}

void Job::writeData( const QByteArray & data )
{
  d->session->writeData( data );
}

void Job::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( d->currentSubJob ) {
    d->currentSubJob->handleResponse( tag, data );
    return;
  }

  if ( tag == d->tag ) {
    if ( data.startsWith( "NO " ) || data.startsWith( "BAD " ) ) {
      QString msg = QString::fromUtf8( data );

      msg.remove( 0, msg.startsWith( QLatin1String( "NO " ) ) ? 3 : 4 );

      if ( msg.endsWith( QLatin1String( "\r\n" ) ) )
        msg.chop( 2 );

      setError( Unknown );
      setErrorText( msg );
      emitResult();
      return;
    } else if ( data.startsWith( "OK" ) ) {
      emitResult();
      return;
    }
  }

  doHandleResponse( tag, data );
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

void Job::slotSubJobAboutToStart( Job * job )
{
  Q_ASSERT( d->currentSubJob == 0 );
  d->currentSubJob = job;
}

void Job::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  qDebug() << "Unhandled response: " << tag << data;
}

QByteArray Job::sessionId() const
{
  if ( d->parent )
    return d->parent->sessionId();
  return d->session->sessionId();
}

void Job::startQueued()
{
  emit aboutToStart( this );
  doStart();
}

void Job::slotResult(KJob * job)
{
  Q_ASSERT( job == d->currentSubJob );
  KCompositeJob::slotResult( job );
  d->currentSubJob = 0;
  if ( !job->error() )
    QTimer::singleShot( 0, this, SLOT(startNext()) );
}

void Job::startNext()
{
  if ( !d->currentSubJob && hasSubjobs() ) {
    Job *job = dynamic_cast<Akonadi::Job*>( subjobs().first() );
    Q_ASSERT( job );
    job->startQueued();
  }
}

void Job::lostConnection()
{
  if ( d->currentSubJob ) {
    d->currentSubJob->lostConnection();
  } else {
    kill( KJob::Quietly );
    setError( ConnectionFailed );
    emitResult();
  }
}

#include "job.moc"
