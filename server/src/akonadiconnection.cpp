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
#include "imapparser.h"

#include <assert.h>

using namespace Akonadi;

AkonadiConnection::AkonadiConnection( int socketDescriptor, QObject *parent )
    : QThread(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_tcpSocket( 0 )
    , m_currentHandler( 0 )
    , m_connectionState( NonAuthenticated )
    , m_backend( 0 )
    , m_selectedConnection( 0 )
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
    m_tcpSocket = new QTcpSocket();

    if ( !m_tcpSocket->setSocketDescriptor( m_socketDescriptor ) ) {
        emit error(m_tcpSocket->error());
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

    connect( m_tcpSocket, SIGNAL( readyRead() ),
             this, SLOT( slotNewData() ), Qt::DirectConnection );
    connect( m_tcpSocket, SIGNAL( disconnected() ),
             this, SLOT( slotDisconnected() ), Qt::DirectConnection );

    writeOut( "* OK Akonadi Almost IMAP Server");

    exec();
    delete m_tcpSocket;
    m_tcpSocket = 0;
}

void AkonadiConnection::slotDisconnected()
{
    quit();
}

void AkonadiConnection::slotNewData()
{
  while ( m_tcpSocket->bytesAvailable() > 0 ) {
    if ( m_parser->continuationSize() > 1 ) {
      const QByteArray data = m_tcpSocket->read( qMin( m_tcpSocket->bytesAvailable(), m_parser->continuationSize() - 1 ) );
      Tracer::self()->connectionInput( m_identifier, QLatin1String("[binary data]") );
      m_parser->parseBlock( data );
    } else if ( m_tcpSocket->canReadLine() ) {
      const QByteArray line = m_tcpSocket->readLine();
      Tracer::self()->connectionInput( m_identifier, QString::fromUtf8( line ) );

      if ( m_parser->parseNextLine( line ) ) {
        // parse the command
        QByteArray command;
        ImapParser::parseString( m_parser->data(), command );

        m_currentHandler = findHandlerForCommand( command );
        m_currentHandler->setTag( m_parser->tag() );
        assert( m_currentHandler );
        connect( m_currentHandler, SIGNAL( responseAvailable( const Response & ) ),
                this, SLOT( slotResponseAvailable( const Response & ) ), Qt::DirectConnection );
        connect( m_currentHandler, SIGNAL( connectionStateChange( ConnectionState ) ),
                this, SLOT( slotConnectionStateChange( ConnectionState ) ),
                Qt::DirectConnection );
        try {
            // FIXME: remove the tag, it's only there for backward compatibility with the handlers!
            if ( m_currentHandler->handleLine( m_parser->tag() + ' ' + m_parser->data() ) )
                m_currentHandler = 0;
        } catch ( ... ) {
            delete m_currentHandler;
            m_currentHandler = 0;
        }
        m_parser->reset();
      } else {
        if ( m_parser->continuationStarted() ) {
          Response response;
          response.setContinuation();
          response.setString( "Ready for literal data (expecting " +
              QByteArray::number( m_parser->continuationSize() ) + " bytes)" );
          slotResponseAvailable( response );
        }
      }
    } else {
      break; // nothing we can do for now with the available data
    }
  }
}

void AkonadiConnection::writeOut( const QByteArray &data )
{
    QByteArray block = data + "\r\n";
    m_tcpSocket->write(block);
    m_tcpSocket->waitForBytesWritten();

    Tracer::self()->connectionOutput( m_identifier, QString::fromUtf8( block ) );
}


Handler * AkonadiConnection::findHandlerForCommand( const QByteArray & command )
{
    Handler * handler = Handler::findHandlerForCommandAlwaysAllowed( command );
    if ( handler ) return handler;

    switch ( m_connectionState ) {
        case NonAuthenticated:
            handler =  Handler::findHandlerForCommandNonAuthenticated( command );            break;
        case Authenticated:
            handler =  Handler::findHandlerForCommandAuthenticated( command );
            break;
        case Selected:
            break;
        case LoggingOut:
            break;
    }
    // we didn't have a handler for this, let the default one do its thing
    if ( !handler ) handler = new Handler();
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
            m_tcpSocket->disconnectFromHost();
            break;
    }
}

int Akonadi::AkonadiConnection::selectedCollection( ) const
{
    return m_selectedConnection;
}

void Akonadi::AkonadiConnection::setSelectedCollection( int collection )
{
    m_selectedConnection = collection;
}

const Location Akonadi::AkonadiConnection::selectedLocation()
{
  return Location::retrieveById( selectedCollection() );
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

#include "akonadiconnection.moc"
