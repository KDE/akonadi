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

  DataStore *store = connection()->storageBackend();

  QList<PimItem> pimItems;
  if ( connection()->selectedLocation().id() == -1 ) {
    pimItems = store->matchingPimItems( fetchQuery.sequences() );
  } else {
    if ( fetchQuery.isUidFetch() ) {
      pimItems = store->matchingPimItems( fetchQuery.sequences() );
    } else {
      pimItems = store->matchingPimItemsByLocation( fetchQuery.sequences(), connection()->selectedLocation() );
    }
  }

  for ( int i = 0; i < pimItems.count(); ++i ) {
    response.setUntagged();
    response.setString( buildResponse( pimItems[ i ], fetchQuery ) );
    emit responseAvailable( response );
  }

  response.setTag( tag() );
  response.setSuccess();

  if ( fetchQuery.isUidFetch() )
    response.setString( "UID FETCH completed" );
  else
    response.setString( "FETCH completed" );

  emit responseAvailable( response );

  deleteLater();

  return true;
}

QByteArray Fetch::buildResponse( const PimItem &item, const FetchQuery &fetchQuery  )
{
  QList<QByteArray> attributes;
  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Envelope ) ) {
    attributes.append( "ENVELOPE " + buildEnvelope( item, fetchQuery ) );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Flags ) ) {
    QList<Flag> flagList = connection()->storageBackend()->itemFlags( item );

    QStringList flags;
    for ( int i = 0; i < flagList.count(); ++i )
      flags.append( flagList[ i ].name() );

    attributes.append( "FLAGS (" + flags.join( " " ).toLatin1() + ")" );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::InternalDate ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822 ) ) {
    attributes.append( "RFC822 {" + QByteArray::number( item.data().length() ) +
                       "}\r\n" + item.data() );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Header ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Size ) ) {
    attributes.append( "RFC822.SIZE " + QByteArray::number( item.data().length() ) );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Text ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Body ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Body_Structure ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Uid ) || fetchQuery.isUidFetch() ) {
    attributes.append( "UID " + QByteArray::number( item.id() ) );
  }

  QByteArray attributesString;
  for ( int i = 0; i < attributes.count(); ++i ) {
    if ( i != 0 )
      attributesString += " " + attributes[ i ];
    else
      attributesString += attributes[ i ];
  }

  int itemPosition = connection()->storageBackend()->pimItemPosition( item );

  if ( attributes.isEmpty() )
    return QByteArray::number( itemPosition ) + " FETCH";
  else
    return QByteArray::number( itemPosition ) + " FETCH (" + attributesString + ")";
}

// FIXME build from database
QByteArray Fetch::buildEnvelope( const PimItem&, const FetchQuery& )
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
