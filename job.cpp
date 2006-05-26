/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>

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

#include <QDebug>
#include <QEventLoop>
#include <QHash>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#include "job.h"

using namespace PIM;

DataReference::DataReference()
{
}

DataReference::DataReference( const QString &persistanceID, const QString &externalUrl )
  : mPersistanceID( persistanceID ), mExternalUrl( externalUrl )
{
}

DataReference::~DataReference()
{
}

QString DataReference::persistanceID() const
{
  return mPersistanceID;
}

QUrl DataReference::externalUrl() const
{
  return mExternalUrl;
}

bool PIM::DataReference::isNull() const
{
  return mPersistanceID.isEmpty();
}

bool DataReference::operator==( const DataReference & other ) const
{
  return mPersistanceID == other.mPersistanceID;
}

bool PIM::DataReference::operator !=( const DataReference & other ) const
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
    enum State { Idle, LoggedIn, StartRequested, Started };
    int mError;
    QTcpSocket *socket;
    int lastTag;
    QByteArray loginTag;
    State state;
};

Job::Job( QObject *parent )
  : QObject( parent ), d( new JobPrivate )
{
  qDebug() << "new job created";
  d->mError = None;
  d->socket = new QTcpSocket( this );
  connect( d->socket, SIGNAL(connected()), SLOT(slotConnected()) );
  connect( d->socket, SIGNAL(readyRead()), SLOT(slotDataReceived()) );
  connect( d->socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(slotSocketError()) );
  d->socket->connectToHost( QHostAddress::LocalHost, 4444 );
  d->lastTag = 0;
  d->state = JobPrivate::Idle;
}

Job::~Job()
{
  d->socket->close();
  delete d;
  d = 0;
}

bool Job::exec()
{
  qDebug() << "exec()";
  QEventLoop loop( this );
  connect( this, SIGNAL( done( PIM::Job* ) ), &loop, SLOT( quit() ) );
  start();
  loop.exec();

  return ( d->mError == 0 );
}

void Job::start()
{
  qDebug() << "start()";
  if ( d->state == JobPrivate::LoggedIn )
    startInternal();
  else
    d->state = JobPrivate::StartRequested;
}

void Job::kill()
{
  d->mError = UserCanceled;
  emit done( this );
}

int Job::error() const
{
  return d->mError;
}

QString Job::errorText() const
{
  switch ( d->mError ) {
    case None:
      return QString();
      break;
    case ConnectionFailed:
      return tr( "Can't connect to pim storage service." );
      break;
    case UserCanceled:
      return tr( "User canceled transmission." );
      break;
    case Unknown:
    default:
      return tr( "Unknown Error." );
      break;
  }
}

void Job::setError( int error )
{
  d->mError = error;
}

void PIM::Job::slotConnected( )
{
  d->loginTag = newTag();
  writeData( d->loginTag + " LOGIN\n" );
}

void PIM::Job::slotDataReceived( )
{
  while ( d->socket->canReadLine() ) {
    // TODO: implement reading/parsing of raw data blocks ({size} suffix)
    QByteArray buffer = d->socket->readLine();
    qDebug() << "data received " << buffer;
    int startOfData = buffer.indexOf( ' ' );
    QByteArray tag = buffer.left( startOfData );
    QByteArray data = buffer.mid( startOfData + 1 );
    // handle our stuff
    if ( tag == d->loginTag ) {
      if ( d->state == JobPrivate::StartRequested )
        startInternal();
      else
        d->state = JobPrivate::LoggedIn;
    }
    // work for our subclasses
    else
      handleResponse( tag, data );
  }
}
void PIM::Job::slotSocketError( )
{
  qWarning() << "Socket error: " << d->socket->errorString();
  setError( ConnectionFailed );
}

QByteArray PIM::Job::newTag( )
{
  d->lastTag++;
  return QByteArray::number( d->lastTag );
}

void PIM::Job::writeData( const QByteArray & data )
{
  d->socket->write( data );
}

void PIM::Job::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  qWarning() << "Unhandled backend response: " << tag << " " << data;
}

void PIM::Job::startInternal( )
{
  qDebug() << "start internal";
  d->state = JobPrivate::Started;
  if ( error() != None ) {
    // we weren't able to connect to the backend
    QTimer::singleShot( 0, this, SLOT(emitDone()) );
    return;
  }
  doStart();
}

void PIM::Job::emitDone( )
{
  emit done( this );
}

#include "job.moc"
