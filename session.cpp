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
#include "xdgbasedirs.h"

#include <kdebug.h>

#include <QtCore/QDir>
#include <QtCore/QQueue>
#include <QtCore/QThreadStorage>
#include <QtCore/QTimer>

#ifdef Q_OS_WIN
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#else
#include <klocalsocket.h>
#endif

#define PIPELINE_LENGTH 2

using namespace Akonadi;



void SessionPrivate::startNext()
{
  QTimer::singleShot( 0, mParent, SLOT(doStartNext()) );
}

void SessionPrivate::reconnect()
{
  // should be checking connection method and value validity
#ifdef Q_OS_WIN
  if ( socket->state() != QAbstractSocket::ConnectedState &&
       socket->state() != QAbstractSocket::ConnectingState ) {
    const QString address = mConnectionSettings->value( QLatin1String( "Data/Address" ), QHostAddress(QHostAddress::LocalHost).toString() ).toString();
    const int port = mConnectionSettings->value( QLatin1String( "Data/Port" ), 4444 ).toInt();
    socket->connectToHost( QHostAddress(address), port );
  }
#else
  if ( socket->state() != QAbstractSocket::ConnectedState &&
       socket->state() != QAbstractSocket::ConnectingState ) {
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    const QString path = mConnectionSettings->value( QLatin1String( "Data/UnixPath" ), defaultSocketDir + QLatin1String( "/akonadiserver.socket" ) ).toString();
    socket->connectToPath( path );
  }
#endif
}

void SessionPrivate::socketError()
{
  if ( currentJob )
    currentJob->d->lostConnection();
  connected = false;
  QTimer::singleShot( 1000, mParent, SLOT(reconnect()) );
}

void SessionPrivate::dataReceived()
{
  while ( socket->canReadLine() ) {
    if ( !parser->parseNextLine( socket->readLine() ) )
      continue; // response not yet completed

    // handle login response
    if ( parser->tag() == QByteArray("0") ) {
      if ( parser->data().startsWith( "OK" ) ) {
        connected = true;
        startNext();
      } else {
        kWarning( 5250 ) << "Unable to login to Akonadi server:" << parser->data();
        socket->close();
        QTimer::singleShot( 1000, mParent, SLOT(reconnect()) );
      }
    }

    // send login command
    if ( parser->tag() == "*" && parser->data().startsWith( "OK Akonadi" ) ) {
      mParent->writeData( "0 LOGIN " + sessionId + '\n' );

    // work for the current job
    } else {
      if ( currentJob )
        currentJob->d->handleResponse( parser->tag(), parser->data() );
    }

    // reset parser stuff
    parser->reset();
  }
}

bool SessionPrivate::canPipelineNext()
{
  if ( queue.isEmpty() || pipeline.count() >= PIPELINE_LENGTH )
    return false;
  if ( pipeline.isEmpty() && currentJob )
    return currentJob->d->writeFinished;
  if ( !pipeline.isEmpty() )
    return pipeline.last()->d->writeFinished;
  return false;
}

void SessionPrivate::doStartNext()
{
  if ( !connected || (queue.isEmpty() && pipeline.isEmpty()) )
    return;
  if ( canPipelineNext() ) {
    Akonadi::Job *nextJob = queue.dequeue();
    pipeline.enqueue( nextJob );
    nextJob->d->startQueued();
  }
  if ( jobRunning )
    return;
  jobRunning = true;
  if ( !pipeline.isEmpty() ) {
    currentJob = pipeline.dequeue();
  } else {
    currentJob = queue.dequeue();
    currentJob->d->startQueued();
  }
}

void SessionPrivate::jobDone(KJob * job)
{
  if( job == currentJob ) {
    if ( pipeline.isEmpty() ) {
      jobRunning = false;
      currentJob = 0;
    } else {
      currentJob = pipeline.dequeue();
    }
    startNext();
  }
  // ### better handle the other cases too, user might have canceled jobs
  else {
    kDebug( 5250 ) << job << "Non-current job finished.";
  }
}

void SessionPrivate::jobWriteFinished( Akonadi::Job* job )
{
  Q_ASSERT( (job == currentJob && pipeline.isEmpty()) || (job = pipeline.last()) );
  startNext();
}


Session::Session(const QByteArray & sessionId, QObject * parent) :
    QObject( parent ),
    d( new SessionPrivate( this ) )
{
  if ( !sessionId.isEmpty() )
    d->sessionId = sessionId;
  else
    d->sessionId = QByteArray::number( qrand() );

  d->connected = false;
  d->nextTag = 1;
  d->currentJob = 0;
  d->jobRunning = false;

  const QString connectionConfigFile = XdgBaseDirs::akonadiConnectionConfigFile();

  QFileInfo fileInfo( connectionConfigFile );
  if ( !fileInfo.exists() ) {
    qWarning() << "Akonadi Client Session: connection config file '"
               << "akonadi/akonadiconnectionrc can not be found in '"
               << XdgBaseDirs::homePath( "config" ) << "' nor in any of "
               << XdgBaseDirs::systemPathList( "config" );
  }

  d->mConnectionSettings = new QSettings( connectionConfigFile, QSettings::IniFormat );

  // should check connection method
#ifdef Q_OS_WIN
  d->socket = new QTcpSocket( this );
#else
  d->socket = new KLocalSocket( this );
#endif
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
  connect( job, SIGNAL(result(KJob*)), SLOT(jobDone(KJob*)) );
  connect( job, SIGNAL(writeFinished(Akonadi::Job*)), SLOT(jobWriteFinished(Akonadi::Job*)) );
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
