
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
#include "handler.h"

#include <QtCore/QDebug>
#include <QtCore/QLatin1String>

#include "libs/imapset_p.h"
#include "libs/protocol_p.h"

#include "akonadiconnection.h"
#include "response.h"
#include "scope.h"
#include "handler/akappend.h"
#include "handler/aklist.h"
#include "handler/append.h"
#include "handler/capability.h"
#include "handler/copy.h"
#include "handler/colcopy.h"
#include "handler/colmove.h"
#include "handler/create.h"
#include "handler/delete.h"
#include "handler/expunge.h"
#include "handler/fetch.h"
#include "handler/link.h"
#include "handler/list.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/modify.h"
#include "handler/move.h"
#include "handler/noop.h"
#include "handler/remove.h"
#include "handler/rename.h"
#include "handler/resourceselect.h"
#include "handler/searchpersistent.h"
#include "handler/select.h"
#include "handler/subscribe.h"
#include "handler/status.h"
#include "handler/store.h"
#include "handler/transaction.h"

#include "storage/querybuilder.h"
#include "imapstreamparser.h"

using namespace Akonadi;

Handler::Handler()
    :QObject()
    , m_connection( 0 )
    , m_streamParser( 0 )
{
}


Handler::~Handler()
{
}

Handler * Handler::findHandlerForCommandNonAuthenticated( const QByteArray & command )
{
    // allowed are LOGIN and AUTHENTICATE
    if ( command == "LOGIN" )
        return new Login();

    return 0;
}

Handler * Handler::findHandlerForCommandAlwaysAllowed( const QByteArray & command )
{
    // allowed commands CAPABILITY, NOOP, and LOGOUT
    if ( command == "LOGOUT" )
        return new Logout();
    if ( command == "CAPABILITY" )
        return new Capability();
    return 0;
}

void Handler::setTag( const QByteArray & tag )
{
    m_tag = tag;
}

QByteArray Handler::tag( ) const
{
    return m_tag;
}

Handler * Handler::findHandlerForCommandAuthenticated( const QByteArray &_command, ImapStreamParser *streamParser )
{
  QByteArray command( _command );
  Scope::SelectionScope scope = Scope::None;

  // deal with command prefixes
  if ( command == AKONADI_CMD_UID ) {
    scope = Scope::Uid;
    command = streamParser->readString();
  } else if ( command == AKONADI_CMD_RID ) {
    scope = Scope::Rid;
    command = streamParser->readString();
  }

    // allowed commands are listed below ;-).
    if ( command == "APPEND" )
        return new Append();
    if ( command == AKONADI_CMD_COLLECTIONCREATE )
        return new Create( scope );
    if ( command == "LIST" )
        return new List();
    if ( command == "SELECT" )
        return new Select( scope );
    if ( command == "SEARCH_STORE" )
        return new SearchPersistent();
    if ( command == "NOOP" )
        return new Noop();
    if ( command == AKONADI_CMD_ITEMFETCH )
        return new Fetch( scope );
    if ( command == "EXPUNGE" )
        return new Expunge();
    if ( command == AKONADI_CMD_ITEMMODIFY )
        return new Store( scope );
    if ( command == "STATUS" )
        return new Status();
    if ( command == AKONADI_CMD_COLLECTIONDELETE )
      return new Delete( scope );
    if ( command == AKONADI_CMD_COLLECTIONMODIFY )
      return new Modify( scope );
    if ( command == "RENAME" )
      return new Rename();
    if ( command == "BEGIN" )
      return new TransactionHandler( TransactionHandler::Begin );
    if ( command == "ROLLBACK" )
      return new TransactionHandler( TransactionHandler::Rollback );
    if ( command == "COMMIT" )
      return new TransactionHandler( TransactionHandler::Commit );
    if ( command == AKONADI_CMD_ITEMCREATE )
      return new AkAppend();
    if ( command == "X-AKLIST" )
      return new AkList( scope, false );
    if ( command == "X-AKLSUB" )
      return new AkList( scope, true );
    if ( command == "SUBSCRIBE" )
      return new Subscribe( true );
    if ( command == "UNSUBSCRIBE" )
      return new Subscribe( false );
    if ( command == AKONADI_CMD_ITEMCOPY )
      return new Copy();
    if ( command == AKONADI_CMD_COLLECTIONCOPY )
      return new ColCopy();
    if ( command == "LINK" )
      return new Link( true );
    if ( command == "UNLINK" )
      return new Link( false );
    if ( command == AKONADI_CMD_RESOURCESELECT )
      return new ResourceSelect();
    if ( command == AKONADI_CMD_ITEMDELETE )
      return new Remove( scope );
    if ( command == AKONADI_CMD_ITEMMOVE )
      return new Move( scope );
    if ( command == AKONADI_CMD_COLLECTIONMOVE )
      return new ColMove( scope );

    return 0;
}


void Akonadi::Handler::setConnection( AkonadiConnection* connection )
{
    m_connection = connection;
}


AkonadiConnection* Akonadi::Handler::connection() const
{
    return m_connection;
}



bool Akonadi::Handler::failureResponse( const QString& failureMessage )
{
    Response response;
    response.setTag( tag() );
    response.setFailure();
    response.setString( failureMessage );
    emit responseAvailable( response );
    return false;
}

bool Akonadi::Handler::failureResponse( const QByteArray &failureMessage )
{
  return failureResponse( QString::fromLatin1( failureMessage ) );
}

bool Akonadi::Handler::failureResponse(const char * failureMessage)
{
  return failureResponse( QLatin1String( failureMessage ) );
}

bool Handler::successResponse(const char * successMessage)
{
  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( QString::fromLatin1(successMessage) );
  emit responseAvailable( response );
  return true;
}

void Handler::setStreamParser( ImapStreamParser *parser )
{
  m_streamParser = parser;
}


UnknownCommandHandler::UnknownCommandHandler(const QByteArray command) :
  mCommand( command )
{
}

bool UnknownCommandHandler::parseStream()
{
  Response response;
  response.setError();
  response.setTag( tag() );
  if ( mCommand.isEmpty() )
    response.setString( "No command specified" );
  else
    response.setString( "Unrecognized command: "  + mCommand );
  m_streamParser->readUntilCommandEnd();

  emit responseAvailable( response );
  return true;
}

#include "handler.moc"
