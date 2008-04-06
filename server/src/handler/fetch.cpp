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
#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/protocol_p.h>
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
  : Handler(),
    mIsUidFetch( false ),
    mAllParts( false ),
    mCacheOnly( false )
{
}

Fetch::~Fetch()
{
}

bool Fetch::parseCommand( const QByteArray &line )
{
  // parse the command
  QByteArray buffer;
  int pos = ImapParser::parseString( line, buffer );

  // command
  pos = ImapParser::parseString( line, buffer, pos );
  if ( buffer == AKONADI_CMD_UID ) {
    mIsUidFetch = true;
    pos = ImapParser::parseString( line, buffer, pos );
  }

  // sequence set
  pos = ImapParser::parseSequenceSet( line, mSet, pos );
  if ( mSet.isEmpty() )
    return false;

  // macro vs. attribute list
  forever {
    pos = ImapParser::stripLeadingSpaces( line, pos );
    if ( pos >= line.count() )
      break;
    if ( line[pos] == '(' ) {
      pos = ImapParser::parseParenthesizedList( line, mAttrList, pos );
      break;
    } else {
      pos = ImapParser::parseString( line, buffer, pos );
      if ( buffer == AKONADI_PARAM_CACHEONLY ) {
        mCacheOnly = true;
      } else if ( buffer == AKONADI_PARAM_ALLATTRIBUTES ) {
        mAllParts = true; // ### temporary
      } else if ( buffer == AKONADI_PARAM_FULLPAYLOAD ) {
        mAllParts = true; // ### temporary;
        mAttrList << "RFC822"; // HACK: temporary workaround until we have support for detecting the availability of the full payload in the server
      }
      // IMAP compat stuff
      else if ( buffer == "ALL" ) {
        mAttrList << "FLAGS" << "INTERNALDATE" << "RFC822.SIZE" << "ENVELOPE";
      } else if ( buffer == "FULL" ) {
        mAttrList << "FLAGS" << "INTERNALDATE" << "RFC822.SIZE";
      } else if ( buffer == "FAST" ) {
        mAttrList << "FLAGS" << "INTERNALDATE" << "RFC822.SIZE" << "ENVELOPE" << "BODY";
      } else {
        return false;
      }
    }
  }

  return true;
}

bool Fetch::handleLine( const QByteArray& line )
{
  if ( !parseCommand( line ) )
    return failureResponse( "Unable to parse command" );

  triggerOnDemandFetch();

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
  imapSetToQuery( mSet, mIsUidFetch, itemQuery );
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
  if ( mAttrList.contains( "FLAGS" ) || mAllParts ) {
    flagQuery.addTable( PimItem::tableName() );
    flagQuery.addTable( PimItemFlagRelation::tableName() );
    flagQuery.addTable( Flag::tableName() );
    flagQuery.addColumn( PimItem::idFullColumnName() );
    flagQuery.addColumn( Flag::nameFullColumnName() );
    flagQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
    flagQuery.addColumnCondition( Flag::idFullColumnName(), Query::Equals, PimItemFlagRelation::rightFullColumnName() );
    imapSetToQuery( mSet, mIsUidFetch, flagQuery );
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
  foreach( const QByteArray b, mAttrList ) {
    // filter out non-part attributes
    if ( b == "REV" || b == "FLAGS" || b == "UID" || b == "REMOTEID" || b.startsWith( "akonadi-" ) )
      continue;
    if ( b == "RFC822.SIZE" )
      partList << QString::fromLatin1( "RFC822" );
    else
      partList << QString::fromLatin1( b );
  }
  if ( !partList.isEmpty() || mAllParts || mAttrList.contains( "RFC822.SIZE" ) ) {
    partQuery = buildPartQuery( partList );
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
  if ( !mCacheOnly && (!partList.isEmpty() || mAllParts) ) {
    while ( itemQuery.query().isValid() ) {
      const qint64 pimItemId = itemQuery.query().value( itemQueryIdColumn ).toLongLong();
      QStringList missingParts = partList;
      while ( partQuery.query().isValid() ) {
        const qint64 id = partQuery.query().value( partQueryIdColumn ).toLongLong();
        if ( id < pimItemId ) {
          partQuery.query().next();
          continue;
        } else if ( id > pimItemId ) {
          break;
        }
        const QString partName = partQuery.query().value( partQueryNameColumn ).toString();
        if ( partQuery.query().value( partQueryDataColumn ).toByteArray().isNull() ) {
          if ( mAllParts )
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
    partQuery = buildPartQuery( partList );
    if ( !partQuery.exec() )
      return failureResponse( "Unable to retrieve item parts" );
    partQuery.query().next();
  }

  // build responses
  Response response;
  response.setUntagged();
  while ( itemQuery.query().isValid() ) {
    const qint64 pimItemId = itemQuery.query().value( itemQueryIdColumn ).toLongLong();
    const int pimItemRev = itemQuery.query().value( itemQueryRevColumn ).toInt();
    QList<QByteArray> attributes;
    attributes.append( "UID " + QByteArray::number( pimItemId ) );
    attributes.append( "REV " + QByteArray::number( pimItemRev ) );
    attributes.append( "REMOTEID " + ImapParser::quote( itemQuery.query().value( itemQueryRidColumn ).toString().toUtf8() ) );
    attributes.append( "MIMETYPE " + ImapParser::quote( itemQuery.query().value( itemQueryMimeTypeColumn ).toString().toUtf8() ) );

    if ( mAttrList.contains( "FLAGS" ) ) {
      QList<QByteArray> flags;
      while ( flagQuery.query().isValid() ) {
        const qint64 id = flagQuery.query().value( flagQueryIdColumn ).toLongLong();
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
      const qint64 id = partQuery.query().value( partQueryIdColumn ).toLongLong();
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
      if ( mAttrList.contains( partName ) || mAllParts )
        attributes << part;

      if ( partName == "RFC822" && mAttrList.contains( "RFC822.SIZE" ) )
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
  updateItemAccessTime();

  response.setTag( tag() );
  response.setSuccess();
  if ( mIsUidFetch )
    response.setString( "UID FETCH completed" );
  else
    response.setString( "FETCH completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

void Fetch::updateItemAccessTime()
{
  QueryBuilder qb( QueryBuilder::Update );
  qb.addTable( PimItem::tableName() );
  qb.updateColumnValue( PimItem::atimeColumn(), QDateTime::currentDateTime() );
  imapSetToQuery( mSet, mIsUidFetch, qb );
  if ( !qb.exec() )
    qWarning() << "Unable to update item access time";
}

void Fetch::triggerOnDemandFetch()
{
  if ( mIsUidFetch || connection()->selectedCollection() <= 0 )
    return;
  Location loc = connection()->selectedLocation();
  // HACK: don't trigger on-demand syncing if the resource is the one triggering it
  if ( connection()->sessionId() == loc.resource().name().toLatin1() )
    return;
  DataStore *store = connection()->storageBackend();
  store->activeCachePolicy( loc );
  if ( !loc.cachePolicySyncOnDemand() )
    return;
  store->triggerCollectionSync( loc );
}

QueryBuilder Fetch::buildPartQuery( const QStringList &partList )
{
  QueryBuilder partQuery;
  if ( !partList.isEmpty() || mAllParts || mAttrList.contains( "RFC822.SIZE" ) ) {
    partQuery.addTable( PimItem::tableName() );
    partQuery.addTable( Part::tableName() );
    partQuery.addColumn( PimItem::idFullColumnName() );
    partQuery.addColumn( Part::nameFullColumnName() );
    partQuery.addColumn( Part::dataFullColumnName() );
    partQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName() );
    if ( !mAllParts )
      partQuery.addValueCondition( Part::nameFullColumnName(), Query::In, partList );
    imapSetToQuery( mSet, mIsUidFetch, partQuery );
    partQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );
  }
  return partQuery;
}
