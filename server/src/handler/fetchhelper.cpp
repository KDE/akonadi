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
#include "storage/parthelper.h"

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;

static const int itemQueryIdColumn = 0;
static const int itemQueryRevColumn = 1;
static const int itemQueryRidColumn = 2;
static const int itemQueryMimeTypeColumn = 3;
static const int itemQueryResouceColumn = 4;
static const int itemQuerySizeColumn = 5;
static const int itemQueryDatetimeColumn = 6;
static const int itemQueryCollectionIdColumn = 7;

static const int partQueryIdColumn = 0;
static const int partQueryNameColumn = 1;
static const int partQueryDataColumn = 2;
static const int partQueryVersionColumn = 3;
static const int partQueryExternalColumn = 4;

FetchHelper::FetchHelper( const Scope &scope )
  : mScope( scope ), mUseScope( true )
{
  init();
}

FetchHelper::FetchHelper( const ImapSet &set )
  : mScope( Scope::None ), mSet( set ), mUseScope( false )
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
  mConnection = 0;
}

void FetchHelper::setConnection( AkonadiConnection *connection )
{
  mConnection = connection;
}

void FetchHelper::setStreamParser( ImapStreamParser *parser )
{
  mStreamParser = parser;
}

bool FetchHelper::parseStream( const QByteArray &responseIdentifier )
{
  parseCommandStream();

  if ( mUseScope )
    triggerOnDemandFetch();

  buildItemQuery();

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
    if ( mUseScope )
      ItemQueryHelper::scopeToQuery( mScope, mConnection, flagQuery );
    else
      ItemQueryHelper::itemSetToQuery( mSet, true, mConnection, flagQuery );
    flagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

    if ( !flagQuery.exec() ) {
      emit failureResponse( QLatin1String( "Unable to retrieve item flags" ) );
      return false;
    }
    flagQuery.query().next();
  }
  const int flagQueryIdColumn = 0;
  const int flagQueryNameColumn = 1;

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
    partList << QString::fromLatin1( b );
    if ( b.startsWith( "PLD:" ) )
      payloadList << QString::fromLatin1( b );
  }
  retrieveMissingPayloads( payloadList );

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

  // build responses
  Response response;
  response.setUntagged();
  while ( mItemQuery.query().isValid() ) {
    const qint64 pimItemId = mItemQuery.query().value( itemQueryIdColumn ).toLongLong();
    const int pimItemRev = mItemQuery.query().value( itemQueryRevColumn ).toInt();

    QList<QByteArray> attributes;
    attributes.append( "UID " + QByteArray::number( pimItemId ) );
    attributes.append( "REV " + QByteArray::number( pimItemRev ) );
    attributes.append( "REMOTEID " + ImapParser::quote( mItemQuery.query().value( itemQueryRidColumn ).toString().toUtf8() ) );
    attributes.append( "MIMETYPE " + ImapParser::quote( mItemQuery.query().value( itemQueryMimeTypeColumn ).toString().toUtf8() ) );
    Collection::Id parentCollectionId = mItemQuery.query().value( itemQueryCollectionIdColumn ).toLongLong();
    attributes.append( "COLLECTIONID " + QByteArray::number( parentCollectionId ) );

    if ( mSizeRequested ) {
      const qint64 pimItemSize = mItemQuery.query().value( itemQuerySizeColumn ).toLongLong();
      attributes.append( "SIZE " + QByteArray::number( pimItemSize ) );
    }
    if ( mMTimeRequested ) {
      const QDateTime pimItemDatetime = mItemQuery.query().value( itemQueryDatetimeColumn ).toDateTime();
      // Date time is always stored in UTC time zone by the server.
      QString datetime = QLocale::c().toString( pimItemDatetime, QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );
      attributes.append( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) );
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
      bool partIsExternal = partQuery.query().value( partQueryExternalColumn ).toBool();
      if ( !mExternalPayloadSupported && partIsExternal ) //external payload not supported by the client, translate the data
        data = PartHelper::translateData(id, data, partIsExternal );
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

    mItemQuery.query().next();
  }

  // update atime
  updateItemAccessTime();

  return true;
}

void FetchHelper::updateItemAccessTime()
{
  QueryBuilder qb( QueryBuilder::Update );
  qb.addTable( PimItem::tableName() );
  qb.setColumnValue( PimItem::atimeColumn(), QDateTime::currentDateTime() );
  if ( mUseScope )
    ItemQueryHelper::scopeToQuery( mScope, mConnection, qb );
  else
    ItemQueryHelper::itemSetToQuery( mSet, true, mConnection, qb );

  if ( !qb.exec() )
    qWarning() << "Unable to update item access time";
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

QueryBuilder FetchHelper::buildPartQuery( const QStringList &partList, bool allPayload, bool allAttrs )
{
  QueryBuilder partQuery;
  if ( !partList.isEmpty() || allPayload || allAttrs ) {
    partQuery.addTable( PimItem::tableName() );
    partQuery.addTable( Part::tableName() );
    partQuery.addColumn( PimItem::idFullColumnName() );
    partQuery.addColumn( Part::nameFullColumnName() );
    partQuery.addColumn( Part::dataFullColumnName() );
    partQuery.addColumn( Part::versionFullColumnName() );
    partQuery.addColumn( Part::externalFullColumnName() );
    partQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName() );
    Query::Condition cond( Query::Or );
    if ( !partList.isEmpty() )
      cond.addValueCondition( Part::nameFullColumnName(), Query::In, partList );
    if ( allPayload )
      cond.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "PLD:" ) );
    if ( allAttrs )
      cond.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "ATR:" ) );
    partQuery.addCondition( cond );
    if ( mUseScope )
      ItemQueryHelper::scopeToQuery( mScope, mConnection, partQuery );
    else
      ItemQueryHelper::itemSetToQuery( mSet, true, mConnection, partQuery );
    partQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );
  }
  return partQuery;
}

void FetchHelper::buildItemQuery()
{
  mItemQuery.addTable( PimItem::tableName() );
  mItemQuery.addTable( MimeType::tableName() );
  mItemQuery.addTable( Collection::tableName() );
  mItemQuery.addTable( Resource::tableName() );
  // make sure the columns indexes here and in the constants defined above match
  mItemQuery.addColumn( PimItem::idFullColumnName() );
  mItemQuery.addColumn( PimItem::revFullColumnName() );
  mItemQuery.addColumn( PimItem::remoteIdFullColumnName() );
  mItemQuery.addColumn( MimeType::nameFullColumnName() );
  mItemQuery.addColumn( Resource::nameFullColumnName() );
  mItemQuery.addColumn( PimItem::sizeFullColumnName() );
  mItemQuery.addColumn( PimItem::datetimeFullColumnName() );
  mItemQuery.addColumn( PimItem::collectionIdFullColumnName() );
  mItemQuery.addColumnCondition( PimItem::mimeTypeIdFullColumnName(), Query::Equals, MimeType::idFullColumnName() );
  mItemQuery.addColumnCondition( PimItem::collectionIdFullColumnName(), Query::Equals, Collection::idFullColumnName() );
  mItemQuery.addColumnCondition( Collection::resourceIdFullColumnName(), Query::Equals, Resource::idFullColumnName() );
  if ( mUseScope )
    ItemQueryHelper::scopeToQuery( mScope, mConnection, mItemQuery );
  else
    ItemQueryHelper::itemSetToQuery( mSet, true, mConnection, mItemQuery );
  mItemQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

  if ( !mItemQuery.exec() )
    throw HandlerException("Unable to list items");
  mItemQuery.query().next();
}

void FetchHelper::retrieveMissingPayloads(const QStringList & payloadList)
{
  if ( mCacheOnly || (payloadList.isEmpty() && !mFullPayload) )
    return;

  // TODO: I'm sure this can be done with a single query instead of manually
  QueryBuilder partQuery = buildPartQuery( payloadList, mFullPayload, false );
  if ( !partQuery.exec() )
    throw HandlerException( "Unable to retrieve item parts" );
  partQuery.query().next();

  while ( mItemQuery.query().isValid() ) {
    const qint64 pimItemId = mItemQuery.query().value( itemQueryIdColumn ).toLongLong();
    QStringList missingParts = payloadList;
    while ( partQuery.query().isValid() ) {
      const qint64 id = partQuery.query().value( partQueryIdColumn ).toLongLong();
      if ( id < pimItemId ) {
        partQuery.query().next();
        continue;
      } else if ( id > pimItemId ) {
        break;
      }
      QString partName = partQuery.query().value( partQueryNameColumn ).toString();
      if ( partName.startsWith( QLatin1String( "PLD:" ) ) ) {
        qint64 partId = partQuery.query().value( partQueryIdColumn ).toLongLong();
        QByteArray data = partQuery.query().value( partQueryDataColumn ).toByteArray();
        data = PartHelper::translateData(partId, data, partQuery.query().value( partQueryExternalColumn ).toBool());
        if ( data.isNull() ) {
          if ( mFullPayload && !missingParts.contains( partName ) )
            missingParts << partName;
        } else {
          missingParts.removeAll( partName );
        }
      }
      partQuery.query().next();
    }
    if ( !missingParts.isEmpty() ) {
      QStringList missingPayloadIds;
      foreach ( const QString &s, missingParts )
        missingPayloadIds << s.mid( 4 );
      // TODO: how should we handle retrieval errors here? so far they have been ignored,
      // which makes sense in some cases, do we need a command parameter for this?
      try {
        ItemRetrievalManager::instance()->requestItemDelivery( pimItemId,
          mItemQuery.query().value( itemQueryRidColumn ).toString().toUtf8(),
          mItemQuery.query().value( itemQueryMimeTypeColumn ).toString().toUtf8(),
          mItemQuery.query().value( itemQueryResouceColumn ).toString(), missingPayloadIds );
      } catch ( const ItemRetrieverException &e ) {
        akError() << e.type() << ": " << e.what();
      }
    }
    mItemQuery.query().next();
  }

  // rewind item query
  mItemQuery.query().first();
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
