/***************************************************************************
 *   Copyright (C) 2006-2009 by Tobias Koenig <tokoe@kde.org>              *
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

#include "fetchhelper.h"

#include "akdebug.h"
#include "akonadi.h"
#include "akonadiconnection.h"
#include "handler.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "libs/imapparser_p.h"
#include "libs/protocol_p.h"
#include "response.h"
#include "storage/selectquerybuilder.h"
#include "resourceinterface.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"
#include "storage/parthelper.h"
#include "storage/transaction.h"

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;

static const int itemQueryRevColumn = 4;
static const int itemQueryRemoteRevisionColumn = 5;
static const int itemQuerySizeColumn = 6;
static const int itemQueryDatetimeColumn = 7;
static const int itemQueryCollectionIdColumn = 8;

static const int partQueryVersionColumn = 4;

FetchHelper::FetchHelper( AkonadiConnection *connection, const Scope &scope )
  : ItemRetriever( connection )
{
  setScope( scope );
  init();
}

void FetchHelper::init()
{
  mAncestorDepth = 0;
  mCacheOnly = false;
  mFullPayload = false;
  mAllAttrs = false;
  mSizeRequested = false;
  mMTimeRequested = false;
  mExternalPayloadSupported = false;
  mRemoteRevisionRequested = false;
}

void FetchHelper::setStreamParser( ImapStreamParser *parser )
{
  mStreamParser = parser;
}

bool FetchHelper::parseStream( const QByteArray &responseIdentifier )
{
  parseCommandStream();
  triggerOnDemandFetch();

  // retrieve missing parts
  QStringList partList, payloadList;
  foreach( const QByteArray &b, mRequestedParts ) {
    // filter out non-part attributes
    if ( b == "REV" || b == "FLAGS" || b == "UID" || b == "REMOTEID" )
      continue;
    if ( b == "SIZE" ) {
      mSizeRequested = true;
      continue;
    }
    if ( b == "DATETIME" ) {
      mMTimeRequested = true;
      continue;
    }
    if ( b == "REMOTEREVISION" ) {
      mRemoteRevisionRequested = true;
      continue;
    }
    partList << QString::fromLatin1( b );
    if ( b.startsWith( "PLD:" ) )
      payloadList << QString::fromLatin1( b );
  }

  if ( !mCacheOnly ) {
    // Prepare for a call to ItemRetriever::exec();
    // From a resource perspective the only parts that can be fetched are payloads.
    setRetrieveParts( payloadList );
    setRetrieveFullPayload( mFullPayload );
    exec(); // There we go, retrieve the missing parts from the resource.
  }

  QueryBuilder itemQuery = buildItemQuery();
  if ( !itemQuery.exec() )
    throw HandlerException("Unable to list items");
  itemQuery.query().next();

  // build part query if needed
  QueryBuilder partQuery;
  if ( !partList.isEmpty() || mFullPayload || mAllAttrs ) {
    partQuery = buildPartQuery( partList, mFullPayload, mAllAttrs );
    if ( !partQuery.exec() ) {
      emit failureResponse( QLatin1String( "Unable to retrieve item parts" ) );
      return false;
    }
    partQuery.query().next();
  }

  // build flag query if needed
  QueryBuilder flagQuery;
  if ( mRequestedParts.contains( "FLAGS" ) ) {
    flagQuery.addTable( PimItem::tableName() );
    flagQuery.addTable( PimItemFlagRelation::tableName() );
    flagQuery.addTable( Flag::tableName() );
    flagQuery.addColumn( PimItem::idFullColumnName() );
    flagQuery.addColumn( Flag::nameFullColumnName() );
    flagQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
    flagQuery.addColumnCondition( Flag::idFullColumnName(), Query::Equals, PimItemFlagRelation::rightFullColumnName() );
    ItemQueryHelper::scopeToQuery( scope(), connection(), flagQuery );
    flagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

    if ( !flagQuery.exec() ) {
      emit failureResponse( QLatin1String( "Unable to retrieve item flags" ) );
      return false;
    }
    flagQuery.query().next();
  }
  const int flagQueryIdColumn = 0;
  const int flagQueryNameColumn = 1;

  // build responses
  Response response;
  response.setUntagged();
  while ( itemQuery.query().isValid() ) {
    const qint64 pimItemId = itemQuery.query().value( sItemQueryPimItemIdColumn ).toLongLong();
    const int pimItemRev = itemQuery.query().value( itemQueryRevColumn ).toInt();

    QList<QByteArray> attributes;
    attributes.append( "UID " + QByteArray::number( pimItemId ) );
    attributes.append( "REV " + QByteArray::number( pimItemRev ) );
    attributes.append( "REMOTEID " + ImapParser::quote( itemQuery.query().value( sItemQueryPimItemRidColumn ).toString().toUtf8() ) );
    attributes.append( "MIMETYPE " + ImapParser::quote( itemQuery.query().value( sItemQueryMimeTypeColumn ).toString().toUtf8() ) );
    Collection::Id parentCollectionId = itemQuery.query().value( itemQueryCollectionIdColumn ).toLongLong();
    attributes.append( "COLLECTIONID " + QByteArray::number( parentCollectionId ) );

    if ( mSizeRequested ) {
      const qint64 pimItemSize = itemQuery.query().value( itemQuerySizeColumn ).toLongLong();
      attributes.append( "SIZE " + QByteArray::number( pimItemSize ) );
    }
    if ( mMTimeRequested ) {
      const QDateTime pimItemDatetime = itemQuery.query().value( itemQueryDatetimeColumn ).toDateTime();
      // Date time is always stored in UTC time zone by the server.
      QString datetime = QLocale::c().toString( pimItemDatetime, QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );
      attributes.append( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) );
    }
    if ( mRemoteRevisionRequested ) {
      attributes.append( "REMOTEREVISION " + ImapParser::quote( itemQuery.query().value( itemQueryRemoteRevisionColumn ).toString().toUtf8() ) );
    }

    if ( mRequestedParts.contains( "FLAGS" ) ) {
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

    if ( mAncestorDepth > 0 )
      attributes.append( HandlerHelper::ancestorsToByteArray( mAncestorDepth, ancestorsForItem( parentCollectionId ) ) );

    while ( partQuery.query().isValid() ) {
      const qint64 id = partQuery.query().value( sPartQueryPimIdColumn ).toLongLong();
      if ( id < pimItemId ) {
        partQuery.query().next();
        continue;
      } else if ( id > pimItemId ) {
        break;
      }
      QByteArray partName = partQuery.query().value( sPartQueryNameColumn ).toString().toUtf8();
      QByteArray part = partQuery.query().value( sPartQueryNameColumn ).toString().toUtf8();
      QByteArray data = partQuery.query().value( sPartQueryDataColumn ).toByteArray();
      bool partIsExternal = partQuery.query().value( sPartQueryExternalColumn ).toBool();
      if ( !mExternalPayloadSupported && partIsExternal ) //external payload not supported by the client, translate the data
        data = PartHelper::translateData( data, partIsExternal );
      int version = partQuery.query().value( partQueryVersionColumn ).toInt();
      if ( version != 0 ) { // '0' is the default, so don't send it
        part += "[" + QByteArray::number( version ) + "]";
      }
      if (  mExternalPayloadSupported && partIsExternal ) { // external data and this is supported by the client
        part += " [FILE] ";
      }
      if ( data.isNull() ) {
        part += " NIL";
      } else if ( data.isEmpty() ) {
        part += " \"\"";
      } else {
        part += " {" + QByteArray::number( data.length() ) + "}\r\n";
        part += data;
      }

      if ( mRequestedParts.contains( partName ) || mFullPayload || mAllAttrs )
        attributes << part;

      partQuery.query().next();
    }

    // IMAP protocol violation: should actually be the sequence number
    QByteArray attr = QByteArray::number( pimItemId ) + ' ' + responseIdentifier + " (";
    attr += ImapParser::join( attributes, " " ) + ')';
    response.setUntagged();
    response.setString( attr );
    emit responseAvailable( response );

    itemQuery.query().next();
  }

  // update atime
  updateItemAccessTime();

  return true;
}

void FetchHelper::updateItemAccessTime()
{
  Transaction transaction( connection()->storageBackend() );
  QueryBuilder qb( QueryBuilder::Update );
  qb.addTable( PimItem::tableName() );
  qb.setColumnValue( PimItem::atimeColumn(), QDateTime::currentDateTime() );
  ItemQueryHelper::scopeToQuery( scope(), connection(), qb );

  if ( !qb.exec() )
    qWarning() << "Unable to update item access time";
  else
    transaction.commit();
}

void FetchHelper::triggerOnDemandFetch()
{
  if ( scope().scope() != Scope::None || connection()->selectedCollectionId() <= 0 || mCacheOnly )
    return;

  Collection collection = connection()->selectedCollection();

  // HACK: don't trigger on-demand syncing if the resource is the one triggering it
  if ( connection()->sessionId() == collection.resource().name().toLatin1() )
    return;

  DataStore *store = connection()->storageBackend();
  store->activeCachePolicy( collection );
  if ( !collection.cachePolicySyncOnDemand() )
    return;

  ItemRetrievalManager::instance()->requestCollectionSync( collection );
}

QueryBuilder FetchHelper::buildPartQuery( const QStringList &partList, bool allPayload, bool allAttrs )
{
  QueryBuilder partQuery;
  if ( !partList.isEmpty() || allPayload || allAttrs ) {
    partQuery = buildGenericPartQuery();
    partQuery.addColumn( Part::versionFullColumnName() );

    Query::Condition cond( Query::Or );
    if ( !partList.isEmpty() )
      cond.addValueCondition( Part::nameFullColumnName(), Query::In, partList );
    if ( allPayload )
      cond.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "PLD:" ) );
    if ( allAttrs )
      cond.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "ATR:" ) );
    partQuery.addCondition( cond );

    ItemQueryHelper::scopeToQuery( scope(), connection(), partQuery );
  }
  return partQuery;
}

QueryBuilder FetchHelper::buildItemQuery()
{
  QueryBuilder itemQuery = buildGenericItemQuery(); // Has 4 colums
  // make sure the columns indexes here and in the constants defined at the top of the file
  itemQuery.addColumn( PimItem::revFullColumnName() );
  itemQuery.addColumn( PimItem::remoteRevisionFullColumnName() );
  itemQuery.addColumn( PimItem::sizeFullColumnName() );
  itemQuery.addColumn( PimItem::datetimeFullColumnName() );
  itemQuery.addColumn( PimItem::collectionIdFullColumnName() );

  if ( !itemQuery.exec() )
    throw HandlerException("Unable to list items");

  itemQuery.query().next();
  return itemQuery;
}

QueryBuilder FetchHelper::buildPartQuery()
{
  return buildPartQuery( retrieveParts() , mFullPayload, false );
}

void FetchHelper::parseCommandStream()
{
  // macro vs. attribute list
  forever {
    if ( mStreamParser->atCommandEnd() )
      break;
    if ( mStreamParser->hasList() ) {
      QList<QByteArray> tmp = mStreamParser->readParenthesizedList();
      mRequestedParts += tmp;
      break;
    } else {
      const QByteArray buffer = mStreamParser->readString();
      if ( buffer == AKONADI_PARAM_CACHEONLY ) {
        mCacheOnly = true;
      } else if ( buffer == AKONADI_PARAM_ALLATTRIBUTES ) {
        mAllAttrs = true;
      } else if ( buffer == AKONADI_PARAM_EXTERNALPAYLOAD ) {
        mExternalPayloadSupported = true;
      } else if ( buffer == AKONADI_PARAM_FULLPAYLOAD ) {
        mRequestedParts << "PLD:RFC822"; // HACK: temporary workaround until we have support for detecting the availability of the full payload in the server
        mFullPayload = true;
      } else if ( buffer == AKONADI_PARAM_ANCESTORS ) {
        mAncestorDepth = HandlerHelper::parseDepth( mStreamParser->readString() );
      } else {
        throw HandlerException( "Invalid command argument" );
      }
    }
  }
}

QStack<Collection> FetchHelper::ancestorsForItem( Collection::Id parentColId )
{
  if ( mAncestorDepth <= 0 )
    return QStack<Collection>();
  if ( mAncestorCache.contains( parentColId ) )
    return mAncestorCache.value( parentColId );

  QStack<Collection> ancestors;
  Collection col = Collection::retrieveById( parentColId );
  for ( int i = 0; i < mAncestorDepth; ++i ) {
    if ( !col.isValid() )
      break;
    ancestors.prepend( col );
    col = col.parent();
  }
  mAncestorCache.insert( parentColId, ancestors );
  return ancestors;
}
