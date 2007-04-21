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
#include <QtCore/QSharedData>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include "job.h"
#include "job_p.h"
#include "imapparser.h"
#include "session.h"

using namespace Akonadi;

class DataReference::Private : public QSharedData
{
  public:
    Private()
      : mId( -1 )
    {
    }

    Private( const Private &other )
      : QSharedData( other )
    {
      mId = other.mId;
      mRemoteId = other.mRemoteId;
    }

    int mId;
    QString mRemoteId;
};

DataReference::DataReference()
  : d( new Private )
{
}

DataReference::DataReference( int id, const QString &remoteId )
  : d( new Private )
{
  d->mId = id;
  d->mRemoteId = remoteId;
}

DataReference::DataReference( const DataReference &other )
  : d( other.d )
{
}

DataReference::~DataReference()
{
}

DataReference& DataReference::operator=( const DataReference &other )
{
  if ( this != &other )
    d = other.d;

  return *this;
}

int DataReference::id() const
{
  return d->mId;
}

QString DataReference::remoteId() const
{
  return d->mRemoteId;
}

bool DataReference::isNull() const
{
  return d->mId < 0;
}

bool DataReference::operator==( const DataReference & other ) const
{
  return d->mId == other.d->mId;
}

bool DataReference::operator !=( const DataReference & other ) const
{
  return !(*this == other);
}

bool DataReference::operator<( const DataReference & other ) const
{
  return d->mId < other.d->mId;
}

uint qHash( const DataReference& reference )
{
  return qHash( reference.id() );
}


void Job::Private::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( currentSubJob ) {
    currentSubJob->d->handleResponse( tag, data );
    return;
  }

  if ( tag == tag ) {
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
  if ( currentSubJob ) {
    currentSubJob->d->lostConnection();
  } else {
    mParent->kill( KJob::Quietly );
    mParent->setError( ConnectionFailed );
    mParent->emitResult();
  }
}

void Job::Private::slotSubJobAboutToStart( Job * job )
{
  Q_ASSERT( currentSubJob == 0 );
  currentSubJob = job;
}

void Job::Private::startNext()
{
  if ( !currentSubJob && mParent->hasSubjobs() ) {
    Job *job = dynamic_cast<Akonadi::Job*>( mParent->subjobs().first() );
    Q_ASSERT( job );
    job->d->startQueued();
  }
}


Job::Job( QObject *parent )
  : KCompositeJob( parent ), d( new Private( this ) )
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
  qDebug() << "Unhandled response: " << tag << data;
}

QByteArray Job::sessionId() const
{
  if ( d->parent )
    return d->parent->sessionId();
  return d->session->sessionId();
}

void Job::slotResult(KJob * job)
{
  Q_ASSERT( job == d->currentSubJob );
  KCompositeJob::slotResult( job );
  d->currentSubJob = 0;
  if ( !job->error() )
    QTimer::singleShot( 0, this, SLOT(startNext()) );
}

#include "job.moc"
