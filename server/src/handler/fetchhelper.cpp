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
#include "akonadiconnection.h"
#include "handler.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "libs/imapparser_p.h"
#include "libs/protocol_p.h"
#include "response.h"
#include "storage/selectquerybuilder.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"
#include "storage/parthelper.h"
#include "storage/transaction.h"
#include "utils.h"
#include "intervalcheck.h"

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;

FetchHelper::FetchHelper( AkonadiConnection *connection, const Scope &scope ) :
  mStreamParser( 0 ),
  mConnection( connection ),
  mScope( scope ),
  mAncestorDepth( 0 ),
  mCacheOnly( false ),
  mCheckCachedPayloadPartsOnly( false ),
  mFullPayload( false ),
  mAllAttrs( false ),
  mSizeRequested( false ),
  mMTimeRequested( false ),
  mExternalPayloadSupported( false ),
  mRemoteRevisionRequested( false ),
  mIgnoreErrors( false ),
  mFlagsRequested( false ),
  mRemoteIdRequested( false ),
  mGidRequested( false )
{
  std::fill(mItemQueryColumnMap, mItemQueryColumnMap + ItemQueryColumnCount, -1);
}

void FetchHelper::setStreamParser( ImapStreamParser *parser )
{
  mStreamParser = parser;
}

enum PartQueryColumns {
  PartQueryPimIdColumn,
  PartQueryTypeNamespaceColumn,
  PartQueryTypeNameColumn,
  PartQueryDataColumn,
  PartQueryExternalColumn,
  PartQueryVersionColumn
};

QSqlQuery FetchHelper::buildPartQuery( const QVector<QByteArray> &partList, bool allPayload, bool allAttrs )
{
  ///TODO: merge with ItemQuery
  QueryBuilder partQuery( PimItem::tableName() );
  if ( !partList.isEmpty() || allPayload || allAttrs ) {
    partQuery.addJoin( QueryBuilder::InnerJoin, Part::tableName(), PimItem::idFullColumnName(), Part::pimItemIdFullColumnName() );
    partQuery.addJoin( QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName() );
    partQuery.addColumn( PimItem::idFullColumnName() );
    partQuery.addColumn( PartType::nsFullColumnName() );
    partQuery.addColumn( PartType::nameFullColumnName() );
    partQuery.addColumn( Part::dataFullColumnName() );
    partQuery.addColumn( Part::externalFullColumnName() );

    partQuery.addColumn( Part::versionFullColumnName() );

    partQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

    Query::Condition cond( Query::Or );
    if ( !partList.isEmpty() ) {
      QStringList partNameList;
      partNameList.reserve( partList.size() );
      Q_FOREACH ( const QByteArray &b, partList )
        partNameList.push_back( QString::fromLatin1( b ) );
      cond.addValueCondition( Part::nameFullColumnName(), Query::In, partNameList );
    }
    if ( allPayload )
      cond.addValueCondition( PartType::nsFullColumnName(), Query::Equals, QLatin1String( "PLD:" ) );
    if ( allAttrs )
      cond.addValueCondition( PartType::nsFullColumnName(), Query::Equals, QLatin1String( "ATR:" ) );
    partQuery.addCondition( cond );

    ItemQueryHelper::scopeToQuery( mScope, mConnection, partQuery );

    if ( !partQuery.exec() ) {
      throw HandlerException("Unable to list item parts");
    }

    partQuery.query().next();
  }

  return partQuery.query();
}

QSqlQuery FetchHelper::buildItemQuery()
{
  QueryBuilder itemQuery( PimItem::tableName() );

  itemQuery.addJoin( QueryBuilder::InnerJoin, MimeType::tableName(),
                     PimItem::mimeTypeIdFullColumnName(), MimeType::idFullColumnName() );

  int column = 0;
  #define ADD_COLUMN(colName, colId) { itemQuery.addColumn( colName ); mItemQueryColumnMap[colId] = column++; }
  ADD_COLUMN( PimItem::idFullColumnName(), ItemQueryPimItemIdColumn );
  if (mRemoteIdRequested)
    ADD_COLUMN( PimItem::remoteIdFullColumnName(), ItemQueryPimItemRidColumn )
  ADD_COLUMN( MimeType::nameFullColumnName(), ItemQueryMimeTypeColumn )
  ADD_COLUMN( PimItem::revFullColumnName(), ItemQueryRevColumn )
  if (mRemoteRevisionRequested)
    ADD_COLUMN( PimItem::remoteRevisionFullColumnName(), ItemQueryRemoteRevisionColumn )
  if (mSizeRequested)
    ADD_COLUMN( PimItem::sizeFullColumnName(), ItemQuerySizeColumn )
  if (mMTimeRequested)
    ADD_COLUMN( PimItem::datetimeFullColumnName(), ItemQueryDatetimeColumn )
  ADD_COLUMN( PimItem::collectionIdFullColumnName(), ItemQueryCollectionIdColumn )
  if (mGidRequested)
    ADD_COLUMN( PimItem::gidFullColumnName(), ItemQueryPimItemGidColumn )
  #undef ADD_COLUMN

  itemQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

  if ( mScope.scope() != Scope::Invalid )
    ItemQueryHelper::scopeToQuery( mScope, mConnection, itemQuery );

  if ( mChangedSince.isValid() ) {
    itemQuery.addValueCondition( PimItem::datetimeFullColumnName(), Query::GreaterOrEqual, mChangedSince.toUTC() );
  }

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
  flagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

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
  if ( !mCacheOnly ) {
    // Prepare for a call to ItemRetriever::exec();
    // From a resource perspective the only parts that can be fetched are payloads.
    ItemRetriever retriever( mConnection );
    retriever.setScope( mScope );
    retriever.setRetrieveParts( mRequestedPayloads );
    retriever.setRetrieveFullPayload( mFullPayload );
    if ( !retriever.exec() && !mIgnoreErrors ) { // There we go, retrieve the missing parts from the resource.
      if (mConnection->resourceContext().isValid())
        throw HandlerException( QString::fromLatin1("Unable to fetch item from backend (collection %1, resource %2) : %3")
                .arg(mConnection->selectedCollectionId())
                .arg(mConnection->resourceContext().id())
                .arg(QString::fromLatin1(retriever.lastError())));
      else
        throw HandlerException( QString::fromLatin1("Unable to fetch item from backend (collection %1) : %2")
                .arg(mConnection->selectedCollectionId())
                .arg(QString::fromLatin1(retriever.lastError())));
    }
  }

  QSqlQuery itemQuery = buildItemQuery();

  // error if query did not find any item and scope is not listing items but
  // a request for a specific item
  if ( !itemQuery.isValid() ) {
    switch ( mScope.scope() ) {
      case Scope::Uid: // fall through
      case Scope::Rid: // fall through
      case Scope::HierarchicalRid: // fall through
      case Scope::Gid:
        throw HandlerException( "Item query returned empty result set" );
      break;
      default:
        break;
    }
  }
  // build part query if needed
  QSqlQuery partQuery;
  if ( !mRequestedParts.isEmpty() || mFullPayload || mAllAttrs ) {
    partQuery = buildPartQuery( mRequestedParts, mFullPayload, mAllAttrs );
  }

  // build flag query if needed
  QSqlQuery flagQuery;
  if ( mFlagsRequested ) {
    flagQuery = buildFlagQuery();
  }

  // build responses
  Response response;
  response.setUntagged();
  while ( itemQuery.isValid() ) {
    const qint64 pimItemId = extractQueryResult( itemQuery, ItemQueryPimItemIdColumn ).toLongLong();
    const int pimItemRev = extractQueryResult( itemQuery, ItemQueryRevColumn ).toInt();

    QList<QByteArray> attributes;
    attributes.append( "UID " + QByteArray::number( pimItemId ) );
    attributes.append( "REV " + QByteArray::number( pimItemRev ) );
    if ( mRemoteIdRequested )
      attributes.append( "REMOTEID " + ImapParser::quote( Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryPimItemRidColumn ) ) ) );
    attributes.append( "MIMETYPE " + ImapParser::quote( Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryMimeTypeColumn ) ) ) );
    Collection::Id parentCollectionId = extractQueryResult( itemQuery, ItemQueryCollectionIdColumn ).toLongLong();
    attributes.append( "COLLECTIONID " + QByteArray::number( parentCollectionId ) );

    if ( mSizeRequested ) {
      const qint64 pimItemSize = extractQueryResult( itemQuery, ItemQuerySizeColumn ).toLongLong();
      attributes.append( "SIZE " + QByteArray::number( pimItemSize ) );
    }
    if ( mMTimeRequested ) {
      const QDateTime pimItemDatetime = extractQueryResult( itemQuery, ItemQueryDatetimeColumn ).toDateTime();
      // Date time is always stored in UTC time zone by the server.
      QString datetime = QLocale::c().toString( pimItemDatetime, QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );
      attributes.append( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) );
    }
    if ( mRemoteRevisionRequested ) {
      const QByteArray rrev = Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryRemoteRevisionColumn ) );
      if ( !rrev.isEmpty() )
        attributes.append( AKONADI_PARAM_REMOTEREVISION " " + ImapParser::quote( rrev ) );
    }
    if ( mGidRequested ) {
      const QByteArray gid = Utils::variantToByteArray( itemQuery.value( ItemQueryPimItemGidColumn ) );
      if ( !gid.isEmpty() )
        attributes.append( AKONADI_PARAM_GID " " + ImapParser::quote( gid ) );
    }

    if ( mFlagsRequested ) {
      QList<QByteArray> flags;
      while ( flagQuery.isValid() ) {
        const qint64 id = flagQuery.value( FlagQueryIdColumn ).toLongLong();
        if ( id > pimItemId ) {
          flagQuery.next();
          continue;
        } else if ( id < pimItemId ) {
          break;
        }
        flags << Utils::variantToByteArray( flagQuery.value( FlagQueryNameColumn ) );
        flagQuery.next();
      }
      attributes.append( "FLAGS (" + ImapParser::join( flags, " " ) + ')' );
    }

    if ( mAncestorDepth > 0 )
      attributes.append( HandlerHelper::ancestorsToByteArray( mAncestorDepth, ancestorsForItem( parentCollectionId ) ) );


    bool skipItem = false;

    QList<QByteArray> cachedParts;

    while ( partQuery.isValid() ) {
      const qint64 id = partQuery.value( PartQueryPimIdColumn ).toLongLong();
      if ( id > pimItemId ) {
        partQuery.next();
        continue;
      } else if ( id < pimItemId ) {
        break;
      }
      const QByteArray partName = Utils::variantToByteArray( partQuery.value( PartQueryTypeNamespaceColumn ) ) + ':' +
          Utils::variantToByteArray( partQuery.value( PartQueryTypeNameColumn ) );
      QByteArray part = partName;
      QByteArray data = Utils::variantToByteArray( partQuery.value( PartQueryDataColumn ) );

      if ( mCheckCachedPayloadPartsOnly ) {
        if ( !data.isEmpty() ) {
           cachedParts << part;
        }
         partQuery.next();
     } else {
        if ( mIgnoreErrors && data.isEmpty() ) {
          //We wanted the payload, couldn't get it, and are ignoring errors. Skip the item.
          akDebug() << id << " no payload";
          skipItem = true;
          break;
        }
        const bool partIsExternal = partQuery.value( PartQueryExternalColumn ).toBool();
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
          if ( partIsExternal ) {
            if ( !mConnection->capabilities().noPayloadPath() ) {
              data = PartHelper::resolveAbsolutePath( data ).toLocal8Bit();
            }
          }

          part += " {" + QByteArray::number( data.length() ) + "}\r\n";
          part += data;
        }

        if ( mRequestedParts.contains( partName ) || mFullPayload || mAllAttrs )
          attributes << part;

        partQuery.next();
      }
    }

    if (skipItem) {
      itemQuery.next();
      continue;
    }

    if ( mCheckCachedPayloadPartsOnly ) {
      attributes.append( "CACHEDPARTS (" + ImapParser::join( cachedParts, " " ) + ')');
    }


    // IMAP protocol violation: should actually be the sequence number
    QByteArray attr = QByteArray::number( pimItemId ) + ' ' + responseIdentifier + " (";
    attr += ImapParser::join( attributes, " " ) + ')';
    response.setUntagged();
    response.setString( attr );
    Q_EMIT responseAvailable( response );

    itemQuery.next();
  }

  // update atime (only if the payload was actually requested, otherwise a simple resource sync prevents cache clearing)
  if ( needsAccessTimeUpdate(mRequestedParts) || mFullPayload )
    updateItemAccessTime();

  return true;
}

bool FetchHelper::needsAccessTimeUpdate(const QVector<QByteArray>& parts)
{
  // TODO technically we should compare the part list with the cache policy of
  // the parent collection of the retrieved items, but that's kinda expensive
  // Only updating the atime if the full payload was requested is a good
  // approximation though.
  return parts.contains( "PLD:RFC822" );
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

  IntervalCheck::instance()->requestCollectionSync( collection );
}

void FetchHelper::parseCommandStream()
{
  Q_ASSERT( mStreamParser );

  // macro vs. attribute list
  Q_FOREVER {
    if ( mStreamParser->atCommandEnd() )
      break;
    if ( mStreamParser->hasList() ) {
      parsePartList();
      break;
    } else {
      const QByteArray buffer = mStreamParser->readString();
      if ( buffer == AKONADI_PARAM_CACHEONLY ) {
        mCacheOnly = true;
      } else if ( buffer == AKONADI_PARAM_CHECKCACHEDPARTSONLY ) {
        mCheckCachedPayloadPartsOnly = true;
      } else if ( buffer == AKONADI_PARAM_ALLATTRIBUTES ) {
        mAllAttrs = true;
      } else if ( buffer == AKONADI_PARAM_EXTERNALPAYLOAD ) {
        mExternalPayloadSupported = true;
      } else if ( buffer == AKONADI_PARAM_FULLPAYLOAD ) {
        mRequestedParts << "PLD:RFC822"; // HACK: temporary workaround until we have support for detecting the availability of the full payload in the server
        mFullPayload = true;
      } else if ( buffer == AKONADI_PARAM_ANCESTORS ) {
        mAncestorDepth = HandlerHelper::parseDepth( mStreamParser->readString() );
      } else if ( buffer == AKONADI_PARAM_IGNOREERRORS ) {
        mIgnoreErrors = true;
      } else if ( buffer == AKONADI_PARAM_CHANGEDSINCE ) {
        bool ok = false;
        mChangedSince = QDateTime::fromTime_t( mStreamParser->readNumber( &ok ) );
        if ( !ok ) {
          throw HandlerException( "Invalid CHANGEDSINCE timestamp" );
        }
      } else {
        throw HandlerException( "Invalid command argument" );
      }
    }
  }
}

void FetchHelper::parsePartList()
{
  mStreamParser->beginList();
  while ( !mStreamParser->atListEnd() ) {
    const QByteArray b = mStreamParser->readString();
    // filter out non-part attributes we send all the time
    if ( b == AKONADI_PARAM_REVISION || b == AKONADI_PARAM_UID ) {
      continue;
    } else if ( b == AKONADI_PARAM_REMOTEID ) {
      mRemoteIdRequested = true;
    } else if ( b == AKONADI_PARAM_FLAGS ) {
      mFlagsRequested = true;
    } else if ( b == AKONADI_PARAM_SIZE ) {
      mSizeRequested = true;
    } else if ( b == AKONADI_PARAM_MTIME ) {
      mMTimeRequested = true;
    } else if ( b == AKONADI_PARAM_REMOTEREVISION ) {
      mRemoteRevisionRequested = true;
    } else if ( b == AKONADI_PARAM_GID ) {
      mGidRequested = true;
    } else {
      mRequestedParts.push_back( b );
      if ( b.startsWith( "PLD:" ) )
        mRequestedPayloads.push_back( QString::fromLatin1( b ) );
    }
  }
}

QStack<Collection> FetchHelper::ancestorsForItem( Collection::Id parentColId )
{
  if ( mAncestorDepth <= 0 || parentColId == 0 )
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

QVariant FetchHelper::extractQueryResult(const QSqlQuery& query, FetchHelper::ItemQueryColumns column) const
{
  Q_ASSERT(mItemQueryColumnMap[column] >= 0);
  return query.value(mItemQueryColumnMap[column]);
}
