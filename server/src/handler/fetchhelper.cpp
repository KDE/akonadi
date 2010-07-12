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

FetchHelper::FetchHelper( AkonadiConnection *connection, const Scope &scope )
  : mConnection( connection ), mScope( scope )
{
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

enum PartQueryColumns {
  PartQueryPimIdColumn,
  PartQueryNameColumn,
  PartQueryDataColumn,
  PartQueryExternalColumn,
  PartQueryVersionColumn
};

QSqlQuery FetchHelper::buildPartQuery( const QStringList &partList, bool allPayload, bool allAttrs )
{
  ///TODO: merge with ItemQuery
  QueryBuilder partQuery( PimItem::tableName() );
  if ( !partList.isEmpty() || allPayload || allAttrs ) {
    partQuery.addJoin( QueryBuilder::InnerJoin, Part::tableName(), PimItem::idFullColumnName(), Part::pimItemIdFullColumnName() );
    partQuery.addColumn( PimItem::idFullColumnName() );
    partQuery.addColumn( Part::nameFullColumnName() );
    partQuery.addColumn( Part::dataFullColumnName() );
    partQuery.addColumn( Part::externalFullColumnName() );

    partQuery.addColumn( Part::versionFullColumnName() );

    partQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

    Query::Condition cond( Query::Or );
    if ( !partList.isEmpty() )
      cond.addValueCondition( Part::nameFullColumnName(), Query::In, partList );
    if ( allPayload )
      cond.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "PLD:" ) );
    if ( allAttrs )
      cond.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "ATR:" ) );
    partQuery.addCondition( cond );

    ItemQueryHelper::scopeToQuery( mScope, mConnection, partQuery );

    if ( !partQuery.exec() ) {
      throw HandlerException("Unable to list item parts");
    }

    partQuery.query().next();
  }

  return partQuery.query();
}

enum ItemQueryColumns {
  ItemQueryPimItemIdColumn,
  ItemQueryPimItemRidColumn,
  ItemQueryMimeTypeColumn,
  ItemQueryResourceColumn,
  ItemQueryRevColumn,
  ItemQueryRemoteRevisionColumn,
  ItemQuerySizeColumn,
  ItemQueryDatetimeColumn,
  ItemQueryCollectionIdColumn
};

QSqlQuery FetchHelper::buildItemQuery()
{
  QueryBuilder itemQuery( PimItem::tableName() );

  itemQuery.addJoin( QueryBuilder::InnerJoin, MimeType::tableName(),
                     PimItem::mimeTypeIdFullColumnName(), MimeType::idFullColumnName() );
  itemQuery.addJoin( QueryBuilder::InnerJoin, Collection::tableName(),
                     PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
  itemQuery.addJoin( QueryBuilder::InnerJoin, Resource::tableName(),
                     Collection::resourceIdFullColumnName(), Resource::idFullColumnName() );

  // make sure the columns indexes here and in the constants defined at the top of the file
  itemQuery.addColumn( PimItem::idFullColumnName() );
  itemQuery.addColumn( PimItem::remoteIdFullColumnName() );
  itemQuery.addColumn( MimeType::nameFullColumnName() );
  itemQuery.addColumn( Resource::nameFullColumnName() );
  itemQuery.addColumn( PimItem::revFullColumnName() );
  itemQuery.addColumn( PimItem::remoteRevisionFullColumnName() );
  itemQuery.addColumn( PimItem::sizeFullColumnName() );
  itemQuery.addColumn( PimItem::datetimeFullColumnName() );
  itemQuery.addColumn( PimItem::collectionIdFullColumnName() );

  itemQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

  if ( mScope.scope() != Scope::Invalid )
    ItemQueryHelper::scopeToQuery( mScope, mConnection, itemQuery );

  if ( !itemQuery.exec() ) {
    throw HandlerException("Unable to list items");
  }

  itemQuery.query().next();

  return itemQuery.query();
}

enum FlagQueryColumns {
  FlagQueryIdColumn,
  FlagQueryNameColumn
};

QSqlQuery FetchHelper::buildFlagQuery()
{
  QueryBuilder flagQuery( PimItem::tableName() );
  flagQuery.addJoin( QueryBuilder::InnerJoin, PimItemFlagRelation::tableName(),
                     PimItem::idFullColumnName(), PimItemFlagRelation::leftFullColumnName() );
  flagQuery.addJoin( QueryBuilder::InnerJoin, Flag::tableName(),
                     Flag::idFullColumnName(), PimItemFlagRelation::rightFullColumnName() );
  flagQuery.addColumn( PimItem::idFullColumnName() );
  flagQuery.addColumn( Flag::nameFullColumnName() );
  ItemQueryHelper::scopeToQuery( mScope, mConnection, flagQuery );
  flagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

  if ( !flagQuery.exec() ) {
    throw HandlerException("Unable to retrieve item flags");
  }

  flagQuery.query().next();

  return flagQuery.query();
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
    ItemRetriever retriever( mConnection );
    retriever.setScope( mScope );
    retriever.setRetrieveParts( payloadList );
    retriever.setRetrieveFullPayload( mFullPayload );
    retriever.exec(); // There we go, retrieve the missing parts from the resource.
  }

  QSqlQuery itemQuery = buildItemQuery();

  // build part query if needed
  QSqlQuery partQuery;
  if ( !partList.isEmpty() || mFullPayload || mAllAttrs ) {
    partQuery = buildPartQuery( partList, mFullPayload, mAllAttrs );
  }

  // build flag query if needed
  QSqlQuery flagQuery;
  if ( mRequestedParts.contains( "FLAGS" ) ) {
    flagQuery = buildFlagQuery();
  }

  // build responses
  Response response;
  response.setUntagged();
  while ( itemQuery.isValid() ) {
    const qint64 pimItemId = itemQuery.value( ItemQueryPimItemIdColumn ).toLongLong();
    const int pimItemRev = itemQuery.value( ItemQueryRevColumn ).toInt();

    QList<QByteArray> attributes;
    attributes.append( "UID " + QByteArray::number( pimItemId ) );
    attributes.append( "REV " + QByteArray::number( pimItemRev ) );
    attributes.append( "REMOTEID " + ImapParser::quote( itemQuery.value( ItemQueryPimItemRidColumn ).toByteArray() ) );
    attributes.append( "MIMETYPE " + ImapParser::quote( itemQuery.value( ItemQueryMimeTypeColumn ).toByteArray() ) );
    Collection::Id parentCollectionId = itemQuery.value( ItemQueryCollectionIdColumn ).toLongLong();
    attributes.append( "COLLECTIONID " + QByteArray::number( parentCollectionId ) );

    if ( mSizeRequested ) {
      const qint64 pimItemSize = itemQuery.value( ItemQuerySizeColumn ).toLongLong();
      attributes.append( "SIZE " + QByteArray::number( pimItemSize ) );
    }
    if ( mMTimeRequested ) {
      const QDateTime pimItemDatetime = itemQuery.value( ItemQueryDatetimeColumn ).toDateTime();
      // Date time is always stored in UTC time zone by the server.
      QString datetime = QLocale::c().toString( pimItemDatetime, QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );
      attributes.append( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) );
    }
    if ( mRemoteRevisionRequested ) {
      attributes.append( "REMOTEREVISION " + ImapParser::quote( itemQuery.value( ItemQueryRemoteRevisionColumn ).toByteArray() ) );
    }

    if ( mRequestedParts.contains( "FLAGS" ) ) {
      QList<QByteArray> flags;
      while ( flagQuery.isValid() ) {
        const qint64 id = flagQuery.value( FlagQueryIdColumn ).toLongLong();
        if ( id < pimItemId ) {
          flagQuery.next();
          continue;
        } else if ( id > pimItemId ) {
          break;
        }
        flags << flagQuery.value( FlagQueryNameColumn ).toByteArray();
        flagQuery.next();
      }
      attributes.append( "FLAGS (" + ImapParser::join( flags, " " ) + ')' );
    }

    if ( mAncestorDepth > 0 )
      attributes.append( HandlerHelper::ancestorsToByteArray( mAncestorDepth, ancestorsForItem( parentCollectionId ) ) );

    while ( partQuery.isValid() ) {
      const qint64 id = partQuery.value( PartQueryPimIdColumn ).toLongLong();
      if ( id < pimItemId ) {
        partQuery.next();
        continue;
      } else if ( id > pimItemId ) {
        break;
      }
      QByteArray partName = partQuery.value( PartQueryNameColumn ).toByteArray();
      QByteArray part = partQuery.value( PartQueryNameColumn ).toByteArray();
      QByteArray data = partQuery.value( PartQueryDataColumn ).toByteArray();
      bool partIsExternal = partQuery.value( PartQueryExternalColumn ).toBool();
      if ( !mExternalPayloadSupported && partIsExternal ) //external payload not supported by the client, translate the data
        data = PartHelper::translateData( data, partIsExternal );
      int version = partQuery.value( PartQueryVersionColumn ).toInt();
      if ( version != 0 ) { // '0' is the default, so don't send it
        part += '[' + QByteArray::number( version ) + ']';
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

      partQuery.next();
    }

    // IMAP protocol violation: should actually be the sequence number
    QByteArray attr = QByteArray::number( pimItemId ) + ' ' + responseIdentifier + " (";
    attr += ImapParser::join( attributes, " " ) + ')';
    response.setUntagged();
    response.setString( attr );
    emit responseAvailable( response );

    itemQuery.next();
  }

  // update atime
  updateItemAccessTime();

  return true;
}

void FetchHelper::updateItemAccessTime()
{
  Transaction transaction( mConnection->storageBackend() );
  QueryBuilder qb( PimItem::tableName(), QueryBuilder::Update );
  qb.setColumnValue( PimItem::atimeColumn(), QDateTime::currentDateTime() );
  ItemQueryHelper::scopeToQuery( mScope, mConnection, qb );

  if ( !qb.exec() )
    qWarning() << "Unable to update item access time";
  else
    transaction.commit();
}

void FetchHelper::triggerOnDemandFetch()
{
  if ( mScope.scope() != Scope::None || mConnection->selectedCollectionId() <= 0 || mCacheOnly )
    return;

  Collection collection = mConnection->selectedCollection();

  // HACK: don't trigger on-demand syncing if the resource is the one triggering it
  if ( mConnection->sessionId() == collection.resource().name().toLatin1() )
    return;

  DataStore *store = mConnection->storageBackend();
  store->activeCachePolicy( collection );
  if ( !collection.cachePolicySyncOnDemand() )
    return;

  ItemRetrievalManager::instance()->requestCollectionSync( collection );
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
