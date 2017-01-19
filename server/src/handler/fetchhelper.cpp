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
#include "akdbus.h"
#include "akonadi.h"
#include "connection.h"
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
#include <storage/parttypehelper.h>
#include "storage/transaction.h"
#include "storage/datastore.h"
#include "utils.h"
#include "intervalcheck.h"
#include "agentmanagerinterface.h"
#include "dbusconnectionpool.h"
#include "tagfetchhelper.h"

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;
using namespace Akonadi::Server;

FetchHelper::FetchHelper( Connection *connection, const Scope &scope, const FetchScope &fetchScope )
  : mStreamParser( 0 )
  , mConnection( connection )
  , mScope( scope )
  , mFetchScope( fetchScope )
{
  std::fill( mItemQueryColumnMap, mItemQueryColumnMap + ItemQueryColumnCount, -1 );
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
      Q_FOREACH ( const QByteArray &b, partList ) {
        if ( b.startsWith( "PLD" ) || b.startsWith( "ATR" ) ) {
          partNameList.push_back( QString::fromLatin1( b ) );
        }
      }
      if ( !partNameList.isEmpty() ) {
        cond.addCondition( PartTypeHelper::conditionFromFqNames( partNameList ) );
      }
    }

    if ( allPayload ) {
      cond.addValueCondition( PartType::nsFullColumnName(), Query::Equals, QLatin1String( "PLD" ) );
    }
    if ( allAttrs ) {
      cond.addValueCondition( PartType::nsFullColumnName(), Query::Equals, QLatin1String( "ATR" ) );
    }

    if ( !cond.isEmpty() ) {
      partQuery.addCondition( cond );
    }

    ItemQueryHelper::scopeToQuery( mScope, mConnection->context(), partQuery );

    if ( !partQuery.exec() ) {
      throw HandlerException( "Unable to list item parts" );
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
  if ( mFetchScope.remoteIdRequested() ) {
    ADD_COLUMN( PimItem::remoteIdFullColumnName(), ItemQueryPimItemRidColumn )
  }
  ADD_COLUMN( MimeType::nameFullColumnName(), ItemQueryMimeTypeColumn )
  ADD_COLUMN( PimItem::revFullColumnName(), ItemQueryRevColumn )
  if ( mFetchScope.remoteRevisionRequested() ) {
    ADD_COLUMN( PimItem::remoteRevisionFullColumnName(), ItemQueryRemoteRevisionColumn )
  }
  if ( mFetchScope.sizeRequested() ) {
    ADD_COLUMN( PimItem::sizeFullColumnName(), ItemQuerySizeColumn )
  }
  if ( mFetchScope.mTimeRequested() ) {
    ADD_COLUMN( PimItem::datetimeFullColumnName(), ItemQueryDatetimeColumn )
  }
  ADD_COLUMN( PimItem::collectionIdFullColumnName(), ItemQueryCollectionIdColumn )
  if ( mFetchScope.gidRequested() ) {
    ADD_COLUMN( PimItem::gidFullColumnName(), ItemQueryPimItemGidColumn )
  }
  #undef ADD_COLUMN

  itemQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

  if ( mScope.scope() != Scope::Invalid ) {
    ItemQueryHelper::scopeToQuery( mScope, mConnection->context(), itemQuery );
  }

  if ( mFetchScope.changedSince().isValid() ) {
    itemQuery.addValueCondition( PimItem::datetimeFullColumnName(), Query::GreaterOrEqual, mFetchScope.changedSince().toUTC() );
  }

  if ( !itemQuery.exec() ) {
    throw HandlerException( "Unable to list items" );
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
  ItemQueryHelper::scopeToQuery( mScope, mConnection->context(), flagQuery );
  flagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

  if ( !flagQuery.exec() ) {
    throw HandlerException( "Unable to retrieve item flags" );
  }

  flagQuery.query().next();

  return flagQuery.query();
}

enum TagQueryColumns {
  TagQueryItemIdColumn,
  TagQueryTagIdColumn,
};

QSqlQuery FetchHelper::buildTagQuery()
{
  QueryBuilder tagQuery( PimItem::tableName() );
  tagQuery.addJoin( QueryBuilder::InnerJoin, PimItemTagRelation::tableName(),
                     PimItem::idFullColumnName(), PimItemTagRelation::leftFullColumnName() );
  tagQuery.addJoin( QueryBuilder::InnerJoin, Tag::tableName(),
                     Tag::idFullColumnName(), PimItemTagRelation::rightFullColumnName() );
  tagQuery.addColumn( PimItem::idFullColumnName() );
  tagQuery.addColumn( Tag::idFullColumnName() );

  ItemQueryHelper::scopeToQuery( mScope, mConnection->context(), tagQuery );
  tagQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

  if ( !tagQuery.exec() ) {
    throw HandlerException( "Unable to retrieve item tags" );
  }

  tagQuery.query().next();

  return tagQuery.query();
}

enum VRefQueryColumns {
  VRefQueryCollectionIdColumn,
  VRefQueryItemIdColumn
};

QSqlQuery FetchHelper::buildVRefQuery()
{
  QueryBuilder vRefQuery( PimItem::tableName() );
  vRefQuery.addJoin( QueryBuilder::LeftJoin, CollectionPimItemRelation::tableName(),
                     CollectionPimItemRelation::rightFullColumnName(),
                     PimItem::idFullColumnName() );
  vRefQuery.addColumn( CollectionPimItemRelation::leftFullColumnName() );
  vRefQuery.addColumn( CollectionPimItemRelation::rightFullColumnName() );
  ItemQueryHelper::scopeToQuery( mScope, mConnection->context(), vRefQuery );
  vRefQuery.addSortColumn( PimItem::idFullColumnName(), Query::Descending );

  if (!vRefQuery.exec() ) {
    throw HandlerException( "Unable to retrieve virtual references" );
  }

  vRefQuery.query().next();

  return vRefQuery.query();
}


bool FetchHelper::isScopeLocal( const Scope &scope )
{
  // The only agent allowed to override local scope is the Baloo Indexer
  if ( !mConnection->sessionId().startsWith( "akonadi_baloo_indexer" ) ) {
    return false;
  }

  // Get list of all resources that own all items in the scope
  QueryBuilder qb( PimItem::tableName(), QueryBuilder::Select );
  qb.setDistinct( true );
  qb.addColumn( Resource::nameFullColumnName() );
  qb.addJoin( QueryBuilder::LeftJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName() );
  qb.addJoin( QueryBuilder::LeftJoin, Resource::tableName(), Collection::resourceIdFullColumnName(), Resource::idFullColumnName() );
  ItemQueryHelper::scopeToQuery( scope, mConnection->context(), qb );
  if ( mConnection->context()->resource().isValid() ) {
    qb.addValueCondition( Resource::nameFullColumnName(), Query::NotEquals, mConnection->context()->resource().name() );
  }

  if ( !qb.exec() ) {
    throw HandlerException( "Failed to query database" );
    return false;
  }

  // If there is more than one resource, i.e. this is a fetch from multiple
  // collections, then don't bother and just return FALSE. This case is aimed
  // specifically on Baloo, which fetches items from each collection independently,
  // so it will pass this check.
  QSqlQuery query = qb.query();
  if ( query.size() != 1) {
    return false;
  }

  query.next();
  const QString resourceName = query.value( 0 ).toString();

  org::freedesktop::Akonadi::AgentManager manager( AkDBus::serviceName( AkDBus::Control ),
                                                   QLatin1String( "/AgentManager" ),
                                                   DBusConnectionPool::threadConnection() );
  const QString typeIdentifier = manager.agentInstanceType( resourceName );
  const QVariantMap properties = manager.agentCustomProperties( typeIdentifier );
  return properties.value( QLatin1String( "HasLocalStorage" ), false ).toBool();
}

QByteArray FetchHelper::tagsToByteArray( const Tag::List &tags )
{
  QByteArray b;
  QList<QByteArray> attributes;
  b += "(";
  Q_FOREACH ( const Tag &tag, tags ) {
    b += "(" + TagFetchHelper::tagToByteArray( tag.id(),
                                               tag.gid().toLatin1(),
                                               tag.parentId(),
                                               tag.tagType().name().toLatin1(),
                                               QByteArray(),
                                               TagFetchHelper::fetchTagAttributes( tag.id() ) ) + ") ";
  }
  b += ")";
  return b;
}

bool FetchHelper::fetchItems( const QByteArray &responseIdentifier )
{
  // retrieve missing parts
  // HACK: isScopeLocal() is a workaround for resources that have cache expiration
  // because when the cache expires, Baloo is not able to content of the items. So
  // we allow fetch of items that belong to local resources (like maildir) to ignore
  // cacheOnly and retrieve missing parts from the resource. However ItemRetriever
  // is painfully slow with many items and is generally designed to fetch a few
  // messages, not all of them. In the long term, we need a better way to do this.
  if ( !mFetchScope.cacheOnly() || isScopeLocal( mScope ) ) {
    // trigger a collection sync if configured to do so
    triggerOnDemandFetch();

    // Prepare for a call to ItemRetriever::exec();
    // From a resource perspective the only parts that can be fetched are payloads.
    ItemRetriever retriever( mConnection );
    retriever.setScope( mScope );
    retriever.setRetrieveParts( mFetchScope.requestedPayloads() );
    retriever.setRetrieveFullPayload( mFetchScope.fullPayload() );
    retriever.setChangedSince( mFetchScope.changedSince() );
    if ( !retriever.exec() && !mFetchScope.ignoreErrors() ) { // There we go, retrieve the missing parts from the resource.
      if ( mConnection->context()->resource().isValid() ) {
        throw HandlerException( QString::fromLatin1( "Unable to fetch item from backend (collection %1, resource %2) : %3" )
                .arg( mConnection->context()->collectionId() )
                .arg( mConnection->context()->resource().id() )
                .arg( QString::fromLatin1( retriever.lastError() ) ) );
      } else {
        throw HandlerException( QString::fromLatin1( "Unable to fetch item from backend (collection %1) : %2" )
                .arg( mConnection->context()->collectionId() )
                .arg( QString::fromLatin1( retriever.lastError() ) ) );
      }
    }
  }

  QSqlQuery itemQuery = buildItemQuery();

  // error if query did not find any item and scope is not listing items but
  // a request for a specific item
  if ( !itemQuery.isValid() ) {
    if ( mFetchScope.ignoreErrors() ) {
      return true;
    }
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
  if ( !mFetchScope.requestedParts().isEmpty() || mFetchScope.fullPayload() || mFetchScope.allAttributes() ) {
    partQuery = buildPartQuery( mFetchScope.requestedParts(), mFetchScope.fullPayload(), mFetchScope.allAttributes() );
  }

  // build flag query if needed
  QSqlQuery flagQuery;
  if ( mFetchScope.flagsRequested() ) {
    flagQuery = buildFlagQuery();
  }

  // build tag query if needed
  QSqlQuery tagQuery;
  if ( mFetchScope.tagsRequested() ) {
    tagQuery = buildTagQuery();
  }

  QSqlQuery vRefQuery;
  if ( mFetchScope.virtualReferencesRequested() ) {
    vRefQuery = buildVRefQuery();
  }

  // build responses
  Response response;
  response.setUntagged();
  while ( itemQuery.isValid() ) {
    const qint64 pimItemId = extractQueryResult( itemQuery, ItemQueryPimItemIdColumn ).toLongLong();
    const int pimItemRev = extractQueryResult( itemQuery, ItemQueryRevColumn ).toInt();

    QList<QByteArray> attributes;
    attributes.append( AKONADI_PARAM_UID " " + QByteArray::number( pimItemId ) );
    attributes.append( AKONADI_PARAM_REVISION " " + QByteArray::number( pimItemRev ) );
    if ( mFetchScope.remoteIdRequested() ) {
      attributes.append( AKONADI_PARAM_REMOTEID " " + ImapParser::quote( Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryPimItemRidColumn ) ) ) );
    }
    attributes.append( AKONADI_PARAM_MIMETYPE " " + ImapParser::quote( Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryMimeTypeColumn ) ) ) );
    Collection::Id parentCollectionId = extractQueryResult( itemQuery, ItemQueryCollectionIdColumn ).toLongLong();
    attributes.append( AKONADI_PARAM_COLLECTIONID " " + QByteArray::number( parentCollectionId ) );

    if ( mFetchScope.sizeRequested() ) {
      const qint64 pimItemSize = extractQueryResult( itemQuery, ItemQuerySizeColumn ).toLongLong();
      attributes.append( AKONADI_PARAM_SIZE " " + QByteArray::number( pimItemSize ) );
    }
    if ( mFetchScope.mTimeRequested() ) {
      const QDateTime pimItemDatetime = extractQueryResult( itemQuery, ItemQueryDatetimeColumn ).toDateTime();
      // Date time is always stored in UTC time zone by the server.
      QString datetime = QLocale::c().toString( pimItemDatetime, QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );
      attributes.append( AKONADI_PARAM_MTIME " " + ImapParser::quote( datetime.toUtf8() ) );
    }
    if ( mFetchScope.remoteRevisionRequested() ) {
      const QByteArray rrev = Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryRemoteRevisionColumn ) );
      if ( !rrev.isEmpty() ) {
        attributes.append( AKONADI_PARAM_REMOTEREVISION " " + ImapParser::quote( rrev ) );
      }
    }
    if ( mFetchScope.gidRequested() ) {
      const QByteArray gid = Utils::variantToByteArray( extractQueryResult( itemQuery, ItemQueryPimItemGidColumn ) );
      if ( !gid.isEmpty() ) {
        attributes.append( AKONADI_PARAM_GID " " + ImapParser::quote( gid ) );
      }
    }

    if ( mFetchScope.flagsRequested() ) {
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
      attributes.append( AKONADI_PARAM_FLAGS " (" + ImapParser::join( flags, " " ) + ')' );
    }

    if ( mFetchScope.tagsRequested() ) {
      ImapSet tags;
      QVector<qint64> tagIds;
      //We don't take the fetch scope into account yet. It's either id only or the full tag.
      const bool fullTagsRequested = !mFetchScope.tagFetchScope().isEmpty();
      while ( tagQuery.isValid() ) {
        const qint64 id = tagQuery.value( TagQueryItemIdColumn ).toLongLong();
        if ( id > pimItemId ) {
          tagQuery.next();
          continue;
        } else if ( id < pimItemId ) {
          break;
        }
        const qint64 tagId = tagQuery.value( TagQueryTagIdColumn ).toLongLong();
        tags.add( QVector<qint64>() << tagId );

        tagIds << tagId;
        tagQuery.next();
      }
      if ( !fullTagsRequested ) {
        if ( !tags.isEmpty() ) {
          attributes.append( AKONADI_PARAM_TAGS " " + tags.toImapSequenceSet() );
        }
      } else {
        Tag::List tagList;
        Q_FOREACH ( qint64 t, tagIds ) {
          tagList << Tag::retrieveById( t );
        }
        attributes.append( AKONADI_PARAM_TAGS " " + tagsToByteArray( tagList ) );
      }
    }

    if ( mFetchScope.virtualReferencesRequested() ) {
      ImapSet cols;
      while ( vRefQuery.isValid() ) {
          const qint64 id = vRefQuery.value( VRefQueryItemIdColumn ).toLongLong();
          if ( id > pimItemId ) {
            vRefQuery.next();
            continue;
          } else if ( id < pimItemId ) {
            break;
          }
          const qint64 collectionId = vRefQuery.value( VRefQueryCollectionIdColumn ).toLongLong();
          cols.add( QVector<qint64>() << collectionId );
          vRefQuery.next();
      }
      if ( !cols.isEmpty() ) {
        attributes.append( AKONADI_PARAM_VIRTREF " " + cols.toImapSequenceSet() );
      }
    }

    if ( mFetchScope.ancestorDepth() > 0 ) {
      attributes.append( HandlerHelper::ancestorsToByteArray( mFetchScope.ancestorDepth(), ancestorsForItem( parentCollectionId ) ) );
    }

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

      if ( mFetchScope.checkCachedPayloadPartsOnly() ) {
        if ( !data.isEmpty() ) {
          cachedParts << part;
        }
        partQuery.next();
     } else {
        if ( mFetchScope.ignoreErrors() && data.isEmpty() ) {
          //We wanted the payload, couldn't get it, and are ignoring errors. Skip the item.
          //This is not an error though, it's fine to have empty payload parts (to denote existing but not cached parts)
          //akDebug() << "item" << id << "has an empty payload part in parttable for part" << partName;
          skipItem = true;
          break;
        }
        const bool partIsExternal = partQuery.value( PartQueryExternalColumn ).toBool();
        if ( !mFetchScope.externalPayloadSupported() && partIsExternal ) { //external payload not supported by the client, translate the data
          data = PartHelper::translateData( data, partIsExternal );
        }
        int version = partQuery.value( PartQueryVersionColumn ).toInt();
        if ( version != 0 ) { // '0' is the default, so don't send it
          part += '[' + QByteArray::number( version ) + ']';
        }
        if (  mFetchScope.externalPayloadSupported() && partIsExternal ) { // external data and this is supported by the client
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

        if ( mFetchScope.requestedParts().contains( partName ) || mFetchScope.fullPayload() || mFetchScope.allAttributes() ) {
          attributes << part;
        }

        partQuery.next();
      }
    }

    if ( skipItem ) {
      itemQuery.next();
      continue;
    }

    if ( mFetchScope.checkCachedPayloadPartsOnly() ) {
      attributes.append( AKONADI_PARAM_CACHEDPARTS " (" + ImapParser::join( cachedParts, " " ) + ')' );
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
  if ( needsAccessTimeUpdate( mFetchScope.requestedParts() ) || mFetchScope.fullPayload() ) {
    updateItemAccessTime();
  }

  return true;
}

bool FetchHelper::needsAccessTimeUpdate( const QVector<QByteArray> &parts ) const
{
  // TODO technically we should compare the part list with the cache policy of
  // the parent collection of the retrieved items, but that's kinda expensive
  // Only updating the atime if the full payload was requested is a good
  // approximation though.

  // see ItemQueryHelper::scopeToQuery - if mScope is none and we have a collection
  // in context, then we are doing a full collection retrieval. In that case we
  // can check if it makes sense to update Item's atime, because it is only
  // relevant if we want to expire it in the future.
  // This check is not performed for per-item retrievals, because that would be
  // too expensive, as stated in the TODO above.
  if ( mScope.scope() == Scope::None && mConnection->context()->collectionId() > 0 ) {
    // Optimize even further: if only non-expiratory parts were requested, then
    // we don't need to update atime at all.
    if ( !parts.contains (AKONADI_PARAM_PLD_RFC822 ) ) {
      return false;
    }

    Collection col = mConnection->context()->collection();
    DataStore::self()->activeCachePolicy(col);
    // Based on CacheCleaner::shouldScheduleCollection()
    return !col.cachePolicyLocalParts().contains( QLatin1String("ALL") )
             && col.cachePolicyCacheTimeout() > -1;
  }

  return parts.contains( AKONADI_PARAM_PLD_RFC822 );
}

void FetchHelper::updateItemAccessTime()
{
  Transaction transaction( mConnection->storageBackend() );
  QueryBuilder qb( PimItem::tableName(), QueryBuilder::Update );
  qb.setColumnValue( PimItem::atimeColumn(), QDateTime::currentDateTime() );
  ItemQueryHelper::scopeToQuery( mScope, mConnection->context(), qb );

  if ( !qb.exec() ) {
    qWarning() << "Unable to update item access time";
  } else {
    transaction.commit();
  }
}

void FetchHelper::triggerOnDemandFetch()
{
  if ( mScope.scope() != Scope::None || mConnection->context()->collectionId() <= 0 || mFetchScope.cacheOnly() ) {
    return;
  }

  Collection collection = mConnection->context()->collection();

  // HACK: don't trigger on-demand syncing if the resource is the one triggering it
  if ( mConnection->sessionId() == collection.resource().name().toLatin1() ) {
    return;
  }

  DataStore *store = mConnection->storageBackend();
  store->activeCachePolicy( collection );
  if ( !collection.cachePolicySyncOnDemand() ) {
    return;
  }

  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->requestCollectionSync( collection );
  }
}

QStack<Collection> FetchHelper::ancestorsForItem( Collection::Id parentColId )
{
  if ( mFetchScope.ancestorDepth() <= 0 || parentColId == 0 ) {
    return QStack<Collection>();
  }
  if ( mAncestorCache.contains( parentColId ) ) {
    return mAncestorCache.value( parentColId );
  }

  QStack<Collection> ancestors;
  Collection col = Collection::retrieveById( parentColId );
  for ( int i = 0; i < mFetchScope.ancestorDepth(); ++i ) {
    if ( !col.isValid() ) {
      break;
    }
    ancestors.prepend( col );
    col = col.parent();
  }
  mAncestorCache.insert( parentColId, ancestors );
  return ancestors;
}

QVariant FetchHelper::extractQueryResult( const QSqlQuery &query, FetchHelper::ItemQueryColumns column ) const
{
  Q_ASSERT( mItemQueryColumnMap[column] >= 0 );
  return query.value( mItemQueryColumnMap[column] );
}
