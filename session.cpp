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

#include "imapparser.h"
#include "job.h"

#include <kdebug.h>

#include <QHostAddress>
#include <QQueue>
#include <QTcpSocket>
#include <QThreadStorage>
#include <QTimer>

using namespace Akonadi;

class Akonadi::Session::Private
{
  public:
    QByteArray sessionId;
    QTcpSocket* socket;
    bool connected;
    int nextTag;

    // job management
    QQueue<Job*> queue;
    Job* currentJob;
    bool jobRunning;

    // parser stuff
    QByteArray dataBuffer;
    QByteArray tagBuffer;
    int parenthesesCount;
    int literalSize;

    void clearParserState()
    {
      dataBuffer.clear();
      tagBuffer.clear();
      parenthesesCount = 0;
      literalSize = 0;
    }

    // Returns true iff response is complete, false otherwise.
    bool parseNextLine()
    {
      QByteArray readBuffer = socket->readLine();

      // first line, get the tag
      if ( dataBuffer.isEmpty() ) {
        int startOfData = readBuffer.indexOf( ' ' );
        tagBuffer = readBuffer.left( startOfData );
        dataBuffer = readBuffer.mid( startOfData + 1 );
      } else {
        dataBuffer += readBuffer;
      }

      // literal read in progress
      if ( literalSize > 0 ) {
        literalSize -= readBuffer.size();

        // still not everything read
        if ( literalSize > 0 )
          return false;

        // check the remaining (non-literal) part for parentheses
        if ( literalSize < 0 )
          // the following looks strange but works since literalSize can be negative here
          parenthesesCount = ImapParser::parenthesesBalance( readBuffer, readBuffer.length() + literalSize );

        // literal string finished but still open parentheses
        if ( parenthesesCount > 0 )
            return false;

      } else {

        // open parentheses
        parenthesesCount += ImapParser::parenthesesBalance( readBuffer );

        // start new literal read
        if ( readBuffer.trimmed().endsWith( '}' ) ) {
          int begin = readBuffer.lastIndexOf( '{' );
          int end = readBuffer.lastIndexOf( '}' );
          literalSize = readBuffer.mid( begin + 1, end - begin - 1 ).toInt();
          return false;
        }

        // still open parentheses
        if ( parenthesesCount > 0 )
          return false;

        // just a normal response, fall through
      }

      return true;
    }
};

Session::Session(const QByteArray & sessionId, QObject * parent) :
    QObject( parent ),
    d( new Private )
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
  reconnect();
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
  startNext();
}

void Session::writeData(const QByteArray & data)
{
  d->socket->write( data );
}

void Session::reconnect()
{
  if ( d->socket->state() != QAbstractSocket::ConnectedState &&
       d->socket->state() != QAbstractSocket::ConnectingState )
    d->socket->connectToHost( QHostAddress::LocalHost, 4444 );
}

void Session::socketError()
{
  if ( d->currentJob )
    d->currentJob->lostConnection();
  d->connected = false;
  QTimer::singleShot( 1000, this, SLOT(reconnect()) );
}

void Session::dataReceived()
{
  while ( d->socket->canReadLine() ) {
    if ( !d->parseNextLine() )
      continue; // response not yet completed

    // handle login response
    if ( d->tagBuffer == QByteArray("0") ) {
      if ( d->dataBuffer.startsWith( "OK" ) ) {
        d->connected = true;
        startNext();
      } else {
        kWarning() << k_funcinfo << "Unable to login to Akonadi server: " << d->dataBuffer << endl;
        d->socket->close();
        QTimer::singleShot( 1000, this, SLOT(reconnect()) );
      }
    }

    // send login command
    if ( d->tagBuffer == "*" && d->dataBuffer.startsWith( "OK Akonadi" ) ) {
      writeData( "0 LOGIN " + d->sessionId + "\n" );

    // work for the current job
    } else {
      if ( d->currentJob )
        d->currentJob->handleResponse( d->tagBuffer, d->dataBuffer );
    }

    // reset parser stuff
    d->clearParserState();
  }
}

void Session::startNext()
{
  QTimer::singleShot( 0, this, SLOT(doStartNext()) );
}

void Session::doStartNext()
{
  if ( !d->connected || d->jobRunning || d->queue.isEmpty() )
    return;
  d->jobRunning = true;
  d->currentJob = d->queue.dequeue();
  connect( d->currentJob, SIGNAL(result(KJob*)), SLOT(jobDone(KJob*)) );
  d->currentJob->startQueued();
}

void Session::jobDone(KJob * job)
{
  if( job == d->currentJob ) {
    d->jobRunning = false;
    d->currentJob = 0;
    startNext();
  }
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
