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
#include "response.h"
#include "storage/selectquerybuilder.h"
#include "resourceinterface.h"

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
  // parse the command
  QByteArray buffer;
  int pos = ImapParser::parseString( line, buffer );

  // command
  bool isUidFetch = false;
  pos = ImapParser::parseString( line, buffer, pos );
  if ( buffer == "UID" ) {
    isUidFetch = true;
    pos = ImapParser::parseString( line, buffer, pos );
  }

  // sequence set
  ImapSet set;
  pos = ImapParser::parseSequenceSet( line, set, pos );
  if ( set.isEmpty() )
    return failureResponse( "Invalid sequence set" );

  // macro vs. attribute list
  pos = ImapParser::stripLeadingSpaces( line, pos );
  if ( pos >= line.count() )
    return failureResponse( "Premature end of command" );
  QList<QByteArray> attrList;
  bool allParts = false;
  if ( line[pos] == '(' ) {
    pos = ImapParser::parseParenthesizedList( line, attrList, pos );
  } else {
    pos = ImapParser::parseString( line, buffer, pos );
    if ( buffer == "AKALL" ) {
      allParts = true;
    } else if ( buffer == "ALL" ) {
      attrList << "FLAGS" << "INTERNALDATE" << "RFC822.SIZE" << "ENVELOPE";
    } else if ( buffer == "FULL" ) {
      attrList << "FLAGS" << "INTERNALDATE" << "RFC822.SIZE";
    } else if ( buffer == "FAST" ) {
      attrList << "FLAGS" << "INTERNALDATE" << "RFC822.SIZE" << "ENVELOPE" << "BODY";
    } else {
      return failureResponse( "Unsupported macro" );
    }
  }

  triggerOnDemandFetch( isUidFetch );

  // build item query
  QueryBuilder itemQuery;
  itemQuery.addTable( PimItem::tableName() );
  itemQuery.addTable( MimeType::tableName() );
  itemQuery.addTable( Location::tableName() );
  itemQuery.addTable( Resource::tableName() );
  itemQuery.addColumn( PimItem::idFullColumnName() );
  itemQuery.addColumn( PimItem::revFullColumnName() );
  itemQuery.addColumn( PimItem::remoteIdFullColumnName() );
  itemQuery.addColumn( MimeType::nameFullColumnName() );
  itemQuery.addColumn( Resource::nameFullColumnName() );
  itemQuery.addColumnCondition( PimItem::mimeTypeIdFullColumnName(), Query::Equals, MimeType::idFullColumnName() );
  itemQuery.addColumnCondition( PimItem::locationIdFullColumnName(), Query::Equals, Location::idFullColumnName() );
  itemQuery.addColumnCondition( Location::resourceIdFullColumnName(), Query::Equals, Resource::idFullColumnName() );
  imapSetToQuery( set, isUidFetch, itemQuery );
  itemQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );
  const int itemQueryIdColumn = 0;
  const int itemQueryRevColumn = 1;
  const int itemQueryRidColumn = 2;
  const int itemQueryMimeTypeColumn = 3;
  const int itemQueryResouceColumn = 4;

  if ( !itemQuery.exec() )
    return failureResponse( "Unable to list items" );
  itemQuery.query().next();

  // build flag query if needed
  QueryBuilder flagQuery;
  if ( attrList.contains( "FLAGS" ) || allParts ) {
    flagQuery.addTable( PimItem::tableName() );
    flagQuery.addTable( PimItemFlagRelation::tableName() );
    flagQuery.addTable( Flag::tableName() );
    flagQuery.addColumn( PimItem::idFullColumnName() );
    flagQuery.addColumn( Flag::nameFullColumnName() );
    flagQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
    flagQuery.addColumnCondition( Flag::idFullColumnName(), Query::Equals, PimItemFlagRelation::rightFullColumnName() );
    imapSetToQuery( set, isUidFetch, flagQuery );
    flagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

    if ( !flagQuery.exec() )
      return failureResponse( "Unable to retrieve item flags" );
    flagQuery.query().next();
  }
  const int flagQueryIdColumn = 0;
  const int flagQueryNameColumn = 1;

  // build part query if needed
  QueryBuilder partQuery;
  QStringList partList;
  foreach( const QByteArray b, attrList ) {
    // filter out non-part attributes
    if ( b == "REV" || b == "FLAGS" || b == "UID" || b == "REMOTEID" )
      continue;
    if ( b == "RFC822.SIZE" )
      partList << QString::fromLatin1( "RFC822" );
    else
      partList << QString::fromLatin1( b );
  }
  if ( !partList.isEmpty() || allParts || attrList.contains( "RFC822.SIZE" ) ) {
    partQuery.addTable( PimItem::tableName() );
    partQuery.addTable( Part::tableName() );
    partQuery.addColumn( PimItem::idFullColumnName() );
    partQuery.addColumn( Part::nameFullColumnName() );
    partQuery.addColumn( Part::dataFullColumnName() );
    partQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName() );
    if ( !allParts )
      partQuery.addValueCondition( Part::nameFullColumnName(), Query::In, partList );
    imapSetToQuery( set, isUidFetch, partQuery );
    partQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

    if ( !partQuery.exec() )
      return failureResponse( "Unable to retrieve item parts" );
    partQuery.query().next();
  }
  const int partQueryIdColumn = 0;
  const int partQueryNameColumn = 1;
  const int partQueryDataColumn = 2;

  // fetch not yet cached item parts
  // TODO: I'm sure this can be done with a single query instead of manually
  DataStore *store = connection()->storageBackend();
  if ( !partList.isEmpty() || allParts ) {
    while ( itemQuery.query().isValid() ) {
      const int pimItemId = itemQuery.query().value( itemQueryIdColumn ).toInt();
      QStringList missingParts = partList;
      while ( partQuery.query().isValid() ) {
        const int id = partQuery.query().value( partQueryIdColumn ).toInt();
        if ( id < pimItemId ) {
          partQuery.query().next();
          continue;
        } else if ( id > pimItemId ) {
          break;
        }
        const QString partName = partQuery.query().value( partQueryNameColumn ).toString();
        if ( partQuery.query().value( partQueryDataColumn ).toByteArray().isNull() ) {
          if ( allParts )
            missingParts << partName;
        } else {
          missingParts.removeAll( partName );
        }
        partQuery.query().next();
      }
      if ( !missingParts.isEmpty() ) {
        store->retrieveDataFromResource( pimItemId, itemQuery.query().value( itemQueryRidColumn ).toString().toUtf8(),
                                         itemQuery.query().value( itemQueryResouceColumn ).toString(), missingParts );
      }
      itemQuery.query().next();
    }

    // re-exec part query, rewind item query
    itemQuery.query().first();
    if ( !partQuery.query().exec() )
      return failureResponse( "Unable to retrieve item parts" );
    partQuery.query().next();
  }

  // build responses
  Response response;
  response.setUntagged();
  while ( itemQuery.query().isValid() ) {
    const int pimItemId = itemQuery.query().value( itemQueryIdColumn ).toInt();
    const int pimItemRev = itemQuery.query().value( itemQueryRevColumn ).toInt();
    QList<QByteArray> attributes;
    attributes.append( "UID " + QByteArray::number( pimItemId ) );
    attributes.append( "REV " + QByteArray::number( pimItemRev ) );
    attributes.append( "REMOTEID " + ImapParser::quote( itemQuery.query().value( itemQueryRidColumn ).toString().toUtf8() ) );
    attributes.append( "MIMETYPE " + ImapParser::quote( itemQuery.query().value( itemQueryMimeTypeColumn ).toString().toUtf8() ) );

    if ( attrList.contains( "FLAGS" ) ) {
      QList<QByteArray> flags;
      while ( flagQuery.query().isValid() ) {
        const int id = flagQuery.query().value( flagQueryIdColumn ).toInt();
        if ( id < pimItemId ) {
          flagQuery.query().next();
          continue;
        } else if ( id > pimItemId ) {
          break;
        }
        flags << flagQuery.query().value( flagQueryNameColumn ).toString().toUtf8();
        flagQuery.query().next();
      }
      attributes.append( "FLAGS (" + ImapParser::join( flags, " " ) + ')' );
    }

    while ( partQuery.query().isValid() ) {
      const int id = partQuery.query().value( partQueryIdColumn ).toInt();
      if ( id < pimItemId ) {
        partQuery.query().next();
        continue;
      } else if ( id > pimItemId ) {
        break;
      }
      QByteArray partName = partQuery.query().value( partQueryNameColumn ).toString().toUtf8();
      QByteArray part = partQuery.query().value( partQueryNameColumn ).toString().toUtf8();
      QByteArray data = partQuery.query().value( partQueryDataColumn ).toByteArray();
      if ( data.isNull() ) {
        part += " NIL";
      } else if ( data.isEmpty() ) {
        part += " \"\"";
      } else {
        part += " {" + QByteArray::number( data.length() ) + "}\n";
        part += data;
      }
      // return only requested parts (when RFC822.SIZE is requested, RFC822 is retrieved, but should not be returned)
      if ( attrList.contains( partName ) || allParts )
        attributes << part;

      if ( partName == "RFC822" && attrList.contains( "RFC822.SIZE" ) )
        attributes.append( "RFC822.SIZE " + QByteArray::number( data.length() ) );

      partQuery.query().next();
    }

    // IMAP protocol violation: should actually be the sequence number
    QByteArray attr = QByteArray::number( pimItemId ) + " FETCH (";
    attr += ImapParser::join( attributes, " " ) + ')';
    response.setUntagged();
    response.setString( attr );
    emit responseAvailable( response );

    itemQuery.query().next();
  }

  // update atime
  updateItemAccessTime( set, isUidFetch );

  response.setTag( tag() );
  response.setSuccess();
  if ( isUidFetch )
    response.setString( "UID FETCH completed" );
  else
    response.setString( "FETCH completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

void Fetch::updateItemAccessTime(const ImapSet & set, bool isUidFetch)
{
  QueryBuilder qb( QueryBuilder::Update );
  qb.addTable( PimItem::tableName() );
  qb.updateColumnValue( PimItem::atimeColumn(), QDateTime::currentDateTime() );
  imapSetToQuery( set, isUidFetch, qb );
  if ( !qb.exec() )
    qWarning() << "Unable to update item access time";
}

void Fetch::triggerOnDemandFetch(bool isUidFetch)
{
  if ( isUidFetch || connection()->selectedCollection() <= 0 )
    return;
  Location loc = connection()->selectedLocation();
  DataStore *store = connection()->storageBackend();
  store->activeCachePolicy( loc );
  if ( !loc.cachePolicySyncOnDemand() )
    return;
  org::kde::Akonadi::Resource *interface = store->resourceInterface( loc.resource().name() );
  if ( interface )
    interface->synchronizeCollection( loc.id() );
}
