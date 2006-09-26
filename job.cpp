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
#include <QTextStream>

#include "job.h"
#include "jobqueue.h"
#include "imapparser.h"

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
    int mError;
    QString mErrorMessage;
    QTcpSocket *socket;
    int lastTag;
    QByteArray loginTag;
    Job *parent;
    QByteArray tag;
};

Job::Job( QObject *parent )
  : QObject( parent ), d( new JobPrivate )
{
  d->mError = None;
  d->parent = dynamic_cast<Job*>( parent );
  if ( !d->parent ) {
    d->socket = new QTcpSocket( this );
    connect( d->socket, SIGNAL(disconnected()), SLOT(slotDisconnected()) );
  } else {
    qDebug() << "Sharing socket with parent job.";
    d->socket = d->parent->d->socket;
    d->parent->addSubJob( this );
  }
  connect( d->socket, SIGNAL(readyRead()), SLOT(slotDataReceived()) );
  connect( d->socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(slotSocketError()) );
  d->lastTag = 0;
}

Job::~Job()
{
  if ( !d->parent )
    d->socket->close();
  delete d;
  d = 0;
}

bool Job::exec()
{
  QEventLoop loop( this );
  connect( this, SIGNAL( done( PIM::Job* ) ), &loop, SLOT( quit() ) );
  // if the parent is a JobQueue we don't need to start the job
  if ( dynamic_cast<JobQueue*>( d->parent ) == 0 )
    start();
  loop.exec();

  return ( d->mError == 0 );
}

void Job::start()
{
  emit aboutToStart( this );
  if ( !d->parent )
    d->socket->connectToHost( QHostAddress::LocalHost, 4444 );
  else
    doStart(); // can we get the exec() dead-lock here again?
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

QString Job::errorMessage() const
{
  return d->mErrorMessage;
}

QString Job::errorText() const
{
  QString str;
  switch ( d->mError ) {
    case None:
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
  if ( !d->mErrorMessage.isEmpty() ) {
    str += " (" + d->mErrorMessage + ')';
  }
  return str;
}

void Job::setError( int error, const QString &msg )
{
  d->mError = error;
  d->mErrorMessage = msg;
}

void PIM::Job::slotDisconnected( )
{
  setError( ConnectionFailed );
  // FIXME: we must make sure to emit done() only once!
//   emit done( this );
}

void PIM::Job::slotDataReceived( )
{
  static int literalSize = 0;
  static QByteArray dataBuffer;
  static int parenthesesCount = 0;

  while ( d->socket->canReadLine() ) {
    QByteArray readBuffer = d->socket->readLine();
    dataBuffer += readBuffer;

    // literal read in progress
    if ( literalSize > 0 ) {
      literalSize -= readBuffer.size();

      // still not everything read
      if ( literalSize > 0 )
        continue;

      // check the remaining (non-literal) part for parentheses
      if ( literalSize < 0 )
        parenthesesCount = ImapParser::parenthesesBalance( readBuffer, readBuffer.length() + literalSize );

      // literal string finished but still open parentheses
      if ( parenthesesCount > 0 )
        continue;

    } else {

      // open parentheses
      parenthesesCount += ImapParser::parenthesesBalance( readBuffer );

      // start new literal read
      if ( readBuffer.trimmed().endsWith( '}' ) ) {
        int begin = readBuffer.lastIndexOf( '{' );
        int end = readBuffer.lastIndexOf( '}' );
        literalSize = readBuffer.mid( begin + 1, end - begin - 1 ).toInt();
        continue;
      }

      // still open parentheses
      if ( parenthesesCount > 0 )
        continue;

      // just a normal response, fall through
    }

    QTextStream dbg( stderr );
    dbg << "data received: " << dataBuffer;

    int startOfData = dataBuffer.indexOf( ' ' );
    QByteArray tag = dataBuffer.left( startOfData );
    QByteArray data = dataBuffer.mid( startOfData + 1 );

    // reset parser stuff
    dataBuffer.clear();
    literalSize = 0;
    parenthesesCount = 0;

    // handle login stuff (if we are the parent job)
    if ( tag == d->loginTag && !d->parent ) {
      if ( data.startsWith( "OK" ) )
        doStart();
      else {
        setError( ConnectionFailed );
        emit done( this );
      }
      return;
    }
    if ( tag == "*" && data.startsWith( "OK Akonadi" ) && !d->parent ) {
      d->loginTag = newTag();
      writeData( d->loginTag + " LOGIN\n" );
    }
    // work for our subclasses
    else
      handleResponse( tag, data );
  }
}

void PIM::Job::slotSocketError()
{
  qWarning() << "Socket error: " << d->socket->errorString();
  setError( ConnectionFailed );
  emit done( this );
}

QByteArray PIM::Job::newTag( )
{
  if ( d->parent )
    d->tag = d->parent->newTag();
  else {
    d->lastTag++;
    d->tag = QByteArray::number( d->lastTag );
  }
  return d->tag;
}

QByteArray PIM::Job::tag() const
{
  return d->tag;
}

void PIM::Job::writeData( const QByteArray & data )
{
  d->socket->write( data );
}

void PIM::Job::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) ) {
      QString msg = data;
      if ( msg.startsWith( "NO " ) ) msg.remove( 0, 3 );
      if ( msg.endsWith( "\r\n" ) ) msg.chop( 2 );
      setError( Unknown, msg );
    }
    emit done( this );
    return;
  }
  doHandleResponse( tag, data );
}

void PIM::Job::addSubJob( Job * job )
{
  connect( job, SIGNAL(aboutToStart(PIM::Job*)), SLOT(slotSubJobAboutToStart(PIM::Job*)) );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(slotSubJobDone(PIM::Job*)) );
}

void PIM::Job::slotSubJobAboutToStart( PIM::Job * job )
{
  // reconnect socket signals to the subjob
  qDebug() << "re-routing socket signals to sub-job";
  disconnect( d->socket, SIGNAL(readyRead()), this, SLOT(slotDataReceived()) );
  connect( d->socket, SIGNAL(readyRead()), job, SLOT(slotDataReceived()) );
}

void PIM::Job::slotSubJobDone( PIM::Job * job )
{
  // get our signals back from the sub-job
  qDebug() << "re-routing socket signals back to parent job";
  disconnect( d->socket, SIGNAL(readyRead()), job, SLOT(slotDataReceived()) );
  connect( d->socket, SIGNAL(readyRead()), this, SLOT(slotDataReceived()) );
}

void PIM::Job::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  qDebug() << "Unhandled response: " << tag << data;
}

#include "job.moc"
