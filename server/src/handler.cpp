
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

#include "akonadiconnection.h"
#include "response.h"
#include "handler/akappend.h"
#include "handler/aklist.h"
#include "handler/append.h"
#include "handler/capability.h"
#include "handler/create.h"
#include "handler/delete.h"
#include "handler/expunge.h"
#include "handler/fetch.h"
#include "handler/list.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/modify.h"
#include "handler/noop.h"
#include "handler/rename.h"
#include "handler/searchpersistent.h"
#include "handler/select.h"
#include "handler/subscribe.h"
#include "handler/status.h"
#include "handler/store.h"
#include "handler/transaction.h"
#include "uid.h"

#include "storage/querybuilder.h"
#include "imapset.h"

using namespace Akonadi;

Handler::Handler()
    :QObject()
    , m_connection( 0 )
{
}


Handler::~Handler()
{
}

bool Handler::handleLine( const QByteArray & command )
{
    Response response;
    response.setError();
    response.setTag( m_tag );
    response.setString( "Unrecognized command: " + command );

    emit responseAvailable( response );
    deleteLater();
    return true;
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

Handler * Handler::findHandlerForCommandAuthenticated( const QByteArray & command )
{
    // allowed commands are listed below ;-).
    if ( command == "APPEND" )
        return new Append();
    if ( command == "CREATE" )
        return new Create();
    if ( command == "LIST" )
        return new List();
    if ( command == "SELECT" )
        return new Select();
    if ( command == "SEARCH_STORE" )
        return new SearchPersistent();
    if ( command == "NOOP" )
        return new Noop();
    if ( command == "FETCH" )
        return new Fetch();
    if ( command == "EXPUNGE" )
        return new Expunge();
    if ( command == "UID" )
        return new Uid();
    if ( command == "STORE" )
        return new Store();
    if ( command == "STATUS" )
        return new Status();
    if ( command == "DELETE" )
      return new Delete();
    if ( command == "MODIFY" )
      return new Modify();
    if ( command == "RENAME" )
      return new Rename();
    if ( command == "BEGIN" || command == "ROLLBACK" || command == "COMMIT" )
      return new TransactionHandler();
    if ( command == "X-AKAPPEND" )
      return new AkAppend();
    if ( command == "X-AKLIST" )
      return new AkList();
    if ( command == "SUBSCRIBE" || command == "UNSUBSCRIBE" )
      return new Subscribe();

    return 0;
}


void Akonadi::Handler::setConnection( AkonadiConnection* connection )
{
    m_connection = connection;
}


AkonadiConnection* Akonadi::Handler::connection()
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
    deleteLater();
    return true;
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
  deleteLater();
  return true;
}

void Handler::imapSetToQuery(const ImapSet & set, bool isUid, QueryBuilder & qb)
{
  Query::Condition cond( Query::Or );
  foreach ( const ImapInterval i, set.intervals() ) {
    if ( i.hasDefinedBegin() && i.hasDefinedEnd() ) {
      if ( i.size() == 1 ) {
        cond.addValueCondition( PimItem::idFullColumnName(), Query::Equals, i.begin() );
      } else {
        Query::Condition subCond( Query::And );
        subCond.addValueCondition( PimItem::idFullColumnName(), Query::GreaterOrEqual, i.begin() );
        subCond.addValueCondition( PimItem::idFullColumnName(), Query::LessOrEqual, i.end() );
        cond.addCondition( subCond );
      }
    } else if ( i.hasDefinedBegin() ) {
      cond.addValueCondition( PimItem::idFullColumnName(), Query::GreaterOrEqual, i.begin() );
    } else if ( i.hasDefinedEnd() ) {
      cond.addValueCondition( PimItem::idFullColumnName(), Query::LessOrEqual, i.end() );
    }
  }
  if ( !cond.isEmpty() )
    qb.addCondition( cond );

  if ( !isUid && connection()->selectedCollection() >= 0 )
    qb.addValueCondition( PimItem::locationIdColumn(), Query::Equals, connection()->selectedCollection() );
}

#include "handler.moc"
