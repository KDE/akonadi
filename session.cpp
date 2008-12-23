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

#include "imapparser_p.h"
#include "job.h"
#include "job_p.h"
#include "servermanager_p.h"
#include "xdgbasedirs_p.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QDir>
#include <QtCore/QQueue>
#include <QtCore/QThreadStorage>
#include <QtCore/QTimer>

#include <QtNetwork/QLocalSocket>

#define PIPELINE_LENGTH 2

using namespace Akonadi;


//@cond PRIVATE

void SessionPrivate::startNext()
{
  QTimer::singleShot( 0, mParent, SLOT(doStartNext()) );
}

void SessionPrivate::reconnect()
{
  // should be checking connection method and value validity
  if ( socket->state() != QLocalSocket::ConnectedState &&
       socket->state() != QLocalSocket::ConnectingState ) {
#ifdef Q_OS_WIN  //krazy:exclude=cpp
    const QString namedPipe = mConnectionSettings->value( QLatin1String( "Data/NamedPipe" ), QLatin1String( "Akonadi" ) ).toString();
    socket->connectToServer( namedPipe );
#else
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    const QString path = mConnectionSettings->value( QLatin1String( "Data/UnixPath" ), defaultSocketDir + QLatin1String( "/akonadiserver.socket" ) ).toString();
    socket->connectToServer( path );
#endif
  }
}

void SessionPrivate::socketError()
{
  if ( currentJob )
    currentJob->d_ptr->lostConnection();
  connected = false;
  QTimer::singleShot( 1000, mParent, SLOT(reconnect()) );
}

void SessionPrivate::dataReceived()
{
  while ( socket->bytesAvailable() > 0 ) {
    if ( parser->continuationSize() > 1 ) {
      const QByteArray data = socket->read( qMin( socket->bytesAvailable(), parser->continuationSize() - 1 ) );
      parser->parseBlock( data );
    } else if ( socket->canReadLine() ) {
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
        const int pos = parser->data().indexOf( "[PROTOCOL" );
        if ( pos > 0 ) {
          qint64 tmp = 0;
          ImapParser::parseNumber( parser->data(), tmp, 0, pos + 9 );
          protocolVersion = tmp;
          Internal::setServerProtocolVersion( tmp );
        }
        kDebug( 5250 ) << "Server protocol version is:" << protocolVersion;

        writeData( "0 LOGIN " + sessionId + '\n' );

      // work for the current job
      } else {
        if ( currentJob )
          currentJob->d_ptr->handleResponse( parser->tag(), parser->data() );
      }

      // reset parser stuff
      parser->reset();
    } else {
      break; // nothing we can do for now
    }
  }
}

bool SessionPrivate::canPipelineNext()
{
  if ( queue.isEmpty() || pipeline.count() >= PIPELINE_LENGTH )
    return false;
  if ( pipeline.isEmpty() && currentJob )
    return currentJob->d_ptr->mWriteFinished;
  if ( !pipeline.isEmpty() )
    return pipeline.last()->d_ptr->mWriteFinished;
  return false;
}

void SessionPrivate::doStartNext()
{
  if ( !connected || (queue.isEmpty() && pipeline.isEmpty()) )
    return;
  if ( canPipelineNext() ) {
    Akonadi::Job *nextJob = queue.dequeue();
    pipeline.enqueue( nextJob );
    startJob( nextJob );
  }
  if ( jobRunning )
    return;
  jobRunning = true;
  if ( !pipeline.isEmpty() ) {
    currentJob = pipeline.dequeue();
  } else {
    currentJob = queue.dequeue();
    startJob( currentJob );
  }
}

void SessionPrivate::startJob( Job *job )
{
  if ( protocolVersion < minimumProtocolVersion() ) {
    job->setError( Job::ProtocolVersionMismatch );
    job->setErrorText( i18n( "Protocol version %1 found, expected at least %2", protocolVersion, minimumProtocolVersion() ) );
    job->emitResult();
  } else {
    job->d_ptr->startQueued();
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

void SessionPrivate::jobDestroyed(QObject * job)
{
  queue.removeAll( static_cast<Akonadi::Job*>( job ) );
  // ### likely not enough to really cancel already running jobs
  pipeline.removeAll( static_cast<Akonadi::Job*>( job ) );
  if ( currentJob == job )
    currentJob = 0;
}

void SessionPrivate::addJob(Job * job)
{
  queue.append( job );
  QObject::connect( job, SIGNAL(result(KJob*)), mParent, SLOT(jobDone(KJob*)) );
  QObject::connect( job, SIGNAL(writeFinished(Akonadi::Job*)), mParent, SLOT(jobWriteFinished(Akonadi::Job*)) );
  QObject::connect( job, SIGNAL(destroyed(QObject*)), mParent, SLOT(jobDestroyed(QObject*)) );
  startNext();
}

int SessionPrivate::nextTag()
{
  return theNextTag++;
}

void SessionPrivate::writeData(const QByteArray & data)
{
  socket->write( data );
}

//@endcond


Session::Session(const QByteArray & sessionId, QObject * parent) :
    QObject( parent ),
    d( new SessionPrivate( this ) )
{
  if ( !sessionId.isEmpty() )
    d->sessionId = sessionId;
  else
    d->sessionId = QByteArray::number( qrand() );

  d->connected = false;
  d->theNextTag = 1;
  d->currentJob = 0;
  d->jobRunning = false;

  const QString connectionConfigFile = XdgBaseDirs::akonadiConnectionConfigFile();

  QFileInfo fileInfo( connectionConfigFile );
  if ( !fileInfo.exists() ) {
    kWarning( 5250 ) << "Akonadi Client Session: connection config file '"
                     << "akonadi/akonadiconnectionrc can not be found in '"
                     << XdgBaseDirs::homePath( "config" ) << "' nor in any of "
                     << XdgBaseDirs::systemPathList( "config" );
  }

  d->mConnectionSettings = new QSettings( connectionConfigFile, QSettings::IniFormat );

  // should check connection method
  d->socket = new QLocalSocket( this );

  connect( d->socket, SIGNAL(disconnected()), SLOT(socketError()) );
  connect( d->socket, SIGNAL(error(QLocalSocket::LocalSocketError)), SLOT(socketError()) );
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

QThreadStorage<Session*> instances;

void SessionPrivate::createDefaultSession( const QByteArray &sessionId )
{
  Q_ASSERT_X( !sessionId.isEmpty(), "SessionPrivate::createDefaultSession",
              "You tried to create a default session with empty session id!" );
  Q_ASSERT_X( !instances.hasLocalData(), "SessionPrivate::createDefaultSession",
              "You tried to create a default session twice!" );

  instances.setLocalData( new Session( sessionId ) );
}

Session* Session::defaultSession()
{
  if ( !instances.hasLocalData() )
    instances.setLocalData( new Session() );
  return instances.localData();
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
