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

#include "fetch.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "imapparser.h"
#include "fetchquery.h"
#include "response.h"
#include "storage/selectquerybuilder.h"

#include <QtCore/QStringList>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

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
    pimItems = store->fetchMatchingPimItemsByUID( fetchQuery );
  } else {
    if ( fetchQuery.isUidFetch() ) {
      pimItems = store->fetchMatchingPimItemsByUID( fetchQuery /*, connection()->selectedLocation()*/ ) ;
    } else {
      // HACK to make fetching content of the comple collection work
      if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822 ) )
        pimItems = store->matchingPimItemsBySequenceNumbers( fetchQuery.sequences(), connection()->selectedLocation(), fetchQuery.type() );
      else
        pimItems = store->fetchMatchingPimItemsBySequenceNumbers( fetchQuery, connection()->selectedLocation() );
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
  attributes.append( "UID " + QByteArray::number( item.id() ) );
  attributes.append( "REMOTEID \"" + item.remoteId() + '"' );
  MimeType mimeType = item.mimeType();
  attributes.append( "MIMETYPE \"" + mimeType.name().toUtf8() + "\"" );

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Flags ) ) {
    QList<Flag> flagList = item.flags();

    QList<QByteArray> flags;
    for ( int i = 0; i < flagList.count(); ++i )
      flags.append( flagList[ i ].name().toUtf8() );


    attributes.append( "FLAGS (" + ImapParser::join( flags, " " ) + ')' );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::InternalDate ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822 ) ||
       fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Text ) ||
       fetchQuery.hasAttributeType( FetchQuery::Attribute::Body ) ) {
    attributes.append( "RFC822 {" + QByteArray::number( item.data().length() ) +
                       "}\r\n" + item.data() );
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Header ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::RFC822_Size ) ) {
    attributes.append( "RFC822.SIZE " + QByteArray::number( item.data().length() ) );
  }


  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Body_Structure ) ) {
  }

  if ( fetchQuery.hasAttributeType( FetchQuery::Attribute::Custom ) ) {
    foreach ( const FetchQuery::Attribute attr, fetchQuery.attributes() ) {
      if ( attr.type() != FetchQuery::Attribute::Custom )
        continue;
      SelectQueryBuilder<Part> qb;
      qb.addValueCondition( Part::pimItemIdColumn(), "=", item.id() );
      qb.addValueCondition( Part::nameColumn(), "=",attr.name() );
      if ( !qb.exec() ) {
        failureResponse( "Error retrieving item part" );
        // ### Fix error handling
        return QByteArray();
      }
      Part::List list  = qb.result();
      if ( list.isEmpty() )
        attributes.append( attr.name().toUtf8() + " NIL" );
      else
        attributes.append( attr.name().toUtf8() + " {" + QByteArray::number( list.first().data().size() ) + "}\n"
            + list.first().data() );
    }
  }

  QByteArray attributesString;
  for ( int i = 0; i < attributes.count(); ++i ) {
    if ( i != 0 )
      attributesString += ' ' + attributes[ i ];
    else
      attributesString += attributes[ i ];
  }

  int itemPosition = connection()->storageBackend()->pimItemPosition( item );

  if ( attributes.isEmpty() )
    return QByteArray::number( itemPosition ) + " FETCH";
  else
    return QByteArray::number( itemPosition ) + " FETCH (" + attributesString + ')';
}
