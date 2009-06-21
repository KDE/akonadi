/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "akonadiconnection.h"

#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtCore/QLatin1String>

#include "storage/datastore.h"
#include "handler.h"
#include "response.h"
#include "tracer.h"

#include "libs/imapparser_p.h"
#include "imapstreamparser.h"
#include "shared/akdebug.h"
#include "shared/akcrash.h"

#include <assert.h>

using namespace Akonadi;

AkonadiConnection::AkonadiConnection( quintptr socketDescriptor, QObject *parent )
    : QThread(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_socket( 0 )
    , m_currentHandler( 0 )
    , m_connectionState( NonAuthenticated )
    , m_backend( 0 )
    , m_selectedConnection( 0 )
    , m_parser( 0 )
    , m_streamParser( 0 )
{
    m_identifier.sprintf( "%p", static_cast<void*>( this ) );
    Tracer::self()->beginConnection( m_identifier, QString() );
    m_parser = new ImapParser;
}

DataStore * Akonadi::AkonadiConnection::storageBackend()
{
    if ( !m_backend )
      m_backend = DataStore::self();
    return m_backend;
}


AkonadiConnection::~AkonadiConnection()
{
    Tracer::self()->endConnection( m_identifier, QString() );

    delete m_parser;
}

void AkonadiConnection::run()
{
    m_socket = new QLocalSocket();

    if ( !m_socket->setSocketDescriptor( m_socketDescriptor ) ) {
        qWarning() << "AkonadiConnection(" << m_identifier
                 << ")::run: failed to set socket descriptor: "
                 << m_socket->error() << "(" << m_socket->errorString() << ")";
        delete m_socket;
        m_socket = 0;
        return;
    }

    /* Start a local event loop and start processing incoming data. Whenever
     * a full command has been read, it is delegated to the responsible
     * handler and processed by that. If that command needs to do something
     * asynchronous such as ask the db for data, it returns and the input
     * queue can continue to be processed. Whenever there is something to
     * be sent back to the user it is queued in the form of a Response object.
     * All this is meant to make it possible to process large incoming or
     * outgoing data transfers in a streaming manner, without having to
     * hold them in memory 'en gros'. */

    connect( m_socket, SIGNAL( readyRead() ),
             this, SLOT( slotNewData() ), Qt::DirectConnection );
    connect( m_socket, SIGNAL( disconnected() ),
             this, SLOT( slotDisconnected() ), Qt::DirectConnection );

    writeOut( "* OK Akonadi Almost IMAP Server [PROTOCOL 15]");

    m_streamParser = new ImapStreamParser( m_socket );
    exec();
    delete m_socket;
    m_socket = 0;
    delete m_streamParser;
    m_streamParser = 0;
}

void AkonadiConnection::slotDisconnected()
{
    quit();
}

void AkonadiConnection::slotNewData()
{
  while ( m_socket->bytesAvailable() > 0 || !m_streamParser->readRemainingData().isEmpty()) {
    try {
      const QByteArray tag = m_streamParser->readString();
      // deal with stray newlines
      if ( tag.isEmpty() && m_streamParser->atCommandEnd() )
        continue;
      const QByteArray command = m_streamParser->readString();
      Tracer::self()->connectionInput( m_identifier, QString::fromUtf8( tag + " " + command + " " + m_streamParser->readRemainingData() ) );
      m_currentHandler = findHandlerForCommand( command );
      assert( m_currentHandler );
      connect( m_currentHandler, SIGNAL( responseAvailable( const Response & ) ),
              this, SLOT( slotResponseAvailable( const Response & ) ), Qt::DirectConnection );
      connect( m_currentHandler, SIGNAL( connectionStateChange( ConnectionState ) ),
              this, SLOT( slotConnectionStateChange( ConnectionState ) ),
               Qt::DirectConnection );
      m_currentHandler->setTag( tag );
      m_currentHandler->setStreamParser( m_streamParser );
      if ( !m_currentHandler->parseStream() ) {
        m_streamParser->readUntilCommandEnd(); //just eat the ending newline
      }
    } catch ( const Akonadi::HandlerException &e ) {
      m_currentHandler->failureResponse( e.what() );
      m_streamParser->readUntilCommandEnd(); //just eat the ending newline
    } catch ( const Akonadi::Exception &e ) {
      m_currentHandler->failureResponse( QString::fromLatin1( e.type() )
          + QLatin1String( ": " ) + QString::fromLatin1( e.what()  ) );
      m_streamParser->readUntilCommandEnd(); //just eat the ending newline
    } catch ( ... ) {
      akError() << "Unknown exception caught: " << akBacktrace();
      m_currentHandler->failureResponse( "Unknown exception caught" );
      m_streamParser->readUntilCommandEnd(); //just eat the ending newline
    }
    delete m_currentHandler;
    m_currentHandler = 0;

    if (m_streamParser->readRemainingData().startsWith('\n') || m_streamParser->readRemainingData().startsWith("\r\n"))
      m_streamParser->readUntilCommandEnd(); //just eat the ending newline
  }
}


void AkonadiConnection::writeOut( const QByteArray &data )
{
    QByteArray block = data + "\r\n";
    m_socket->write(block);
    m_socket->waitForBytesWritten();

    Tracer::self()->connectionOutput( m_identifier, QString::fromUtf8( block ) );
}


Handler * AkonadiConnection::findHandlerForCommand( const QByteArray & command )
{
    Handler * handler = Handler::findHandlerForCommandAlwaysAllowed( command );
    if ( handler ) return handler;

    switch ( m_connectionState ) {
        case NonAuthenticated:
            handler =  Handler::findHandlerForCommandNonAuthenticated( command );
            break;
        case Authenticated:
            handler =  Handler::findHandlerForCommandAuthenticated( command, m_streamParser );
            break;
        case Selected:
            break;
        case LoggingOut:
            break;
    }
    // we didn't have a handler for this, let the default one do its thing
    if ( !handler ) handler = new UnknownCommandHandler( command );
    handler->setConnection( this );
    return handler;
}

void AkonadiConnection::slotResponseAvailable( const Response& response )
{
    // FIXME handle reentrancy in the presence of continuation. Something like:
    // "if continuation pending, queue responses, once continuation is done, replay them"
    writeOut( response.asString() );
}

void AkonadiConnection::slotConnectionStateChange( ConnectionState state )
{
    if ( state == m_connectionState ) return;
    m_connectionState = state;
    switch ( m_connectionState ) {
        case NonAuthenticated:
            assert( 0 ); // can't happen, it's only the initial state, we can't go back to it
            break;
        case Authenticated:
            break;
        case Selected:
            break;
        case LoggingOut:
            m_socket->disconnectFromServer();
            break;
    }
}

qint64 Akonadi::AkonadiConnection::selectedCollectionId( ) const
{
    return m_selectedConnection;
}

void Akonadi::AkonadiConnection::setSelectedCollection( qint64 collection )
{
    m_selectedConnection = collection;
}

const Collection Akonadi::AkonadiConnection::selectedCollection()
{
  return Collection::retrieveById( selectedCollectionId() );
}

void Akonadi::AkonadiConnection::addStatusMessage( const QByteArray& msg )
{
    m_statusMessageQueue.append( msg );
}

void Akonadi::AkonadiConnection::flushStatusMessageQueue()
{
    for ( int i = 0; i < m_statusMessageQueue.count(); ++i ) {
      Response response;
      response.setUntagged();
      response.setString( m_statusMessageQueue[ i ] );

      slotResponseAvailable( response );
    }

    m_statusMessageQueue.clear();
}

void AkonadiConnection::setSessionId(const QByteArray &id)
{
  m_sessionId = id;
  storageBackend()->setSessionId( id );
  storageBackend()->notificationCollector()->setSessionId( id );
}

QByteArray AkonadiConnection::sessionId() const
{
  return m_sessionId;
}

Resource Akonadi::AkonadiConnection::resourceContext() const
{
  return m_resourceContext;
}

void AkonadiConnection::setResourceContext(const Resource& res)
{
  m_resourceContext = res;
}

bool AkonadiConnection::isOwnerResource(const PimItem& item) const
{
  if ( resourceContext().isValid() )
    return true;
  // fallback for older resources
  if ( sessionId() == item.collection().resource().name().toUtf8() )
    return true;
  return false;
}

#include "akonadiconnection.moc"
