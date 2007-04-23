/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "session.h"
#include "session_p.h"

#include "imapparser.h"
#include "job.h"
#include "job_p.h"

#include <kdebug.h>

#include <QtCore/QQueue>
#include <QtCore/QThreadStorage>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

using namespace Akonadi;



void Session::Private::startNext()
{
  QTimer::singleShot( 0, mParent, SLOT(doStartNext()) );
}

void Session::Private::reconnect()
{
  if ( socket->state() != QAbstractSocket::ConnectedState &&
       socket->state() != QAbstractSocket::ConnectingState )
    socket->connectToHost( QHostAddress::LocalHost, 4444 );
}

void Session::Private::socketError()
{
  if ( currentJob )
    currentJob->d->lostConnection();
  connected = false;
  QTimer::singleShot( 1000, mParent, SLOT(reconnect()) );
}

void Session::Private::dataReceived()
{
  while ( socket->canReadLine() ) {
    if ( !parseNextLine() )
      continue; // response not yet completed

    // handle login response
    if ( tagBuffer == QByteArray("0") ) {
      if ( dataBuffer.startsWith( "OK" ) ) {
        connected = true;
        startNext();
      } else {
        kWarning() << k_funcinfo << "Unable to login to Akonadi server: " << dataBuffer << endl;
        socket->close();
        QTimer::singleShot( 1000, mParent, SLOT(reconnect()) );
      }
    }

    // send login command
    if ( tagBuffer == "*" && dataBuffer.startsWith( "OK Akonadi" ) ) {
      mParent->writeData( "0 LOGIN " + sessionId + "\n" );

    // work for the current job
    } else {
      if ( currentJob )
        currentJob->d->handleResponse( tagBuffer, dataBuffer );
    }

    // reset parser stuff
    clearParserState();
  }
}

void Session::Private::doStartNext()
{
  if ( !connected || jobRunning || queue.isEmpty() )
    return;
  jobRunning = true;
  currentJob = queue.dequeue();
  mParent->connect( currentJob, SIGNAL(result(KJob*)), mParent, SLOT(jobDone(KJob*)) );
  currentJob->d->startQueued();
}

void Session::Private::jobDone(KJob * job)
{
  if( job == currentJob ) {
    jobRunning = false;
    currentJob = 0;
    startNext();
  }
}


Session::Session(const QByteArray & sessionId, QObject * parent) :
    QObject( parent ),
    d( new Private( this ) )
{
  if ( !sessionId.isEmpty() )
    d->sessionId = sessionId;
  else
    d->sessionId = QByteArray::number( qrand() );

  d->connected = false;
  d->nextTag = 1;
  d->currentJob = 0;
  d->jobRunning = false;
  d->clearParserState();

  d->socket = new QTcpSocket( this );
  connect( d->socket, SIGNAL(disconnected()), SLOT(socketError()) );
  connect( d->socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketError()) );
  connect( d->socket, SIGNAL(readyRead()), SLOT(dataReceived()) );
  d->reconnect();
}

Session::~Session()
{
  clear();
  delete d;
}

QByteArray Session::sessionId() const
{
  return d->sessionId;
}

void Session::addJob(Job * job)
{
  d->queue.append( job );
  d->startNext();
}

void Session::writeData(const QByteArray & data)
{
  d->socket->write( data );
}

QThreadStorage<Session*> instances;

Session* Session::defaultSession()
{
  if ( !instances.hasLocalData() )
    instances.setLocalData( new Session() );
  return instances.localData();
}

int Session::nextTag()
{
  return d->nextTag++;
}

void Session::clear()
{
  foreach ( Job* job, d->queue )
    job->kill( KJob::EmitResult );
  d->queue.clear();
  if ( d->currentJob )
    d->currentJob->kill( KJob::EmitResult );
}

#include "session.moc"
