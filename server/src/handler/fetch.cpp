/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStringList>
#include <QUuid>
#include <QVariant> 

#include "akonadi.h"
#include "akonadiconnection.h"
#include "fetchquery.h"
#include "response.h"

#include "fetch.h"

using namespace Akonadi;

static QByteArray buildResponse( QSqlQuery &query, const FetchQuery &fetchQuery, int pos );
static QByteArray buildEnvelope( QSqlQuery &query, const FetchQuery &fetchQuery, int pos );

Fetch::Fetch()
  : Handler()
{
}

Fetch::~Fetch()
{
}

bool Fetch::handleLine( const QByteArray& line )
{
  Response response;
  response.setUntagged();

  int start = line.indexOf( ' ' ); // skip tag

  FetchQuery fetchQuery;
  if ( !fetchQuery.parse( line.mid( start + 1 ) ) ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Syntax error" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  if ( connection()->selectedLocation().getId() == -1 ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Select a mailbox first" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  QString locationId = QString::number( connection()->selectedLocation().getId() );

  QSqlDatabase database = QSqlDatabase::addDatabase( "QSQLITE", QUuid::createUuid().toString() );
  database.setDatabaseName( "akonadi.db" );
  database.open();
  if ( !database.isOpen() ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Unable to open backend storage" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  QSqlQuery query( database );

  QString statement = QString( "SELECT COUNT(*) AS count FROM PimItems WHERE location_id = %1" ).arg( locationId );
  if ( !query.exec( statement ) ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Unable to access backend storage: " + query.lastError().text().toLatin1() );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }
  query.next();
  const int maxNumberOfEntry = query.value( 0 ).toInt() + 1;

  statement = QString( "SELECT id, data FROM PimItems WHERE location_id = %1" ).arg( locationId );
  if ( !query.exec( statement ) ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Unable to access backend storage: " + query.lastError().text().toLatin1() );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  for ( int i = 0; i < fetchQuery.mSequences.count(); ++i ) {
    if ( fetchQuery.mSequences[ i ].contains( ':' ) ) {
      int min = 0;
      int max = 0;

      QList<QByteArray> pair = fetchQuery.mSequences[ i ].split( ':' );
      if ( pair[ 0 ] == "*" && pair[ 1 ] == "*" ) {
        min = max = maxNumberOfEntry;
      } else if ( pair[ 0 ] == "*" ) {
        min = 0;
        max = pair[ 1 ].toInt();
      } else if ( pair[ 1 ] == "*" ) {
        min = pair[ 0 ].toInt();
        max = maxNumberOfEntry;
      } else {
        min = pair[ 0 ].toInt();
        max = pair[ 1 ].toInt();
      }

      if ( max < min )
        qSwap( max, min );

      if ( min < 1 )
        min = 1;

      // transform from imap index to query index
      min--; max--;

      for ( int i = min; i < max; ++i ) {
        response.setUntagged();
        response.setString( buildResponse( query, fetchQuery, i ) );
        emit responseAvailable( response );
      }
    } else {
      int pos;
      if ( fetchQuery.mSequences[ i ] == "*" )
        pos = query.size() - 1;
      else
        pos = fetchQuery.mSequences[ i ].toInt();

      if ( pos < 1 )
        pos = 1;

      // transform from imap index to query index
      pos--;

      response.setUntagged();
      response.setString( buildResponse( query, fetchQuery, pos ) );
      emit responseAvailable( response );
    }
  }

  response.setTag( tag() );
  response.setSuccess();
  response.setString( "FETCH completed" );

  emit responseAvailable( response );

  deleteLater();

  return true;
}

static QByteArray buildResponse( QSqlQuery &query, const FetchQuery &fetchQuery, int pos )
{
  /**
   * Synthesize the response.
   */
  query.seek( pos );

  QList<QByteArray> attributes;
  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Envelope ) ) {
    attributes.append( "ENVELOPE " + buildEnvelope( query, fetchQuery, pos ) );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Flags ) ) {
    attributes.append( "FLAGS (\\Seen)" );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::InternalDate ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822 ) ) {
    const QByteArray data = query.value( 1 ).toByteArray();
    attributes.append( "RFC822 {" + QByteArray::number( data.length() ) +
                       "}\r\n" + data );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Header ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Size ) ) {
    const QByteArray data = query.value( 1 ).toByteArray();
    attributes.append( "RFC822.SIZE " + QByteArray::number( data.length() ) );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Text ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Body ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Body_Structure ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Uid ) ) {
    attributes.append( "UID " + query.value( 0 ).toByteArray() );
  }

  QByteArray attributesString;
  for ( int i = 0; i < attributes.count(); ++i ) {
    if ( i != 0 )
      attributesString += " " + attributes[ i ];
    else
      attributesString += attributes[ i ];
  }

  if ( attributes.isEmpty() )
    return QByteArray::number( pos + 1 ) + " FETCH";
  else
    return QByteArray::number( pos + 1 ) + " FETCH (" + attributesString + ")";
}

// FIXME build from database
static QByteArray buildEnvelope( QSqlQuery&, const FetchQuery&, int )
{
  const QByteArray date( "\"Wed, 1 Feb 2006 13:37:19 UT\"" );
  const QByteArray subject( "\"IMPORTANT: Akonadi Test\"" );
  const QByteArray from( "\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\"" );
  const QByteArray sender = from;
  const QByteArray replyTo( "NIL" );
  const QByteArray to( "\"Ingo Kloecker\" NIL \"kloecker\" \"kde.org\"" );
  const QByteArray cc( "NIL" );
  const QByteArray bcc( "NIL" );
  const QByteArray inReplyTo( "NIL" );
  const QByteArray messageId( "<" + QUuid::createUuid().toString().toLatin1() + "@server.kde.org>" );

  return QByteArray( "("+date+" "+subject+" (("+from+")) (("+sender+")) "+replyTo+" (("+to+")) "+cc+" "+bcc+" "+inReplyTo+" "+messageId+")" );
}
