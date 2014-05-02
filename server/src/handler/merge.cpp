/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "merge.h"
#include "fetchhelper.h"
#include "imapstreamparser.h"
#include "storage/selectquerybuilder.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/parthelper.h"
#include "storage/parttypehelper.h"
#include <storage/itemretriever.h>
#include "connection.h"
#include "handlerhelper.h"
#include <response.h>

#include <libs/imapparser_p.h>
#include <libs/protocol_p.h>

#include <numeric>

using namespace Akonadi;
using namespace Akonadi::Server;

Merge::Merge()
  : AkAppend()
{
}

Merge::~Merge()
{
}

bool Merge::mergeItem( PimItem &newItem, PimItem &currentItem,
                       const QList<QByteArray> &newFlags )
{
    currentItem.setRev( newItem.rev() );
    if ( currentItem.remoteId() != newItem.remoteId() ) {
      currentItem.setRemoteId( newItem.remoteId() );
      mChangedParts << AKONADI_PARAM_REMOTEID;
    }
    if ( currentItem.remoteRevision() != newItem.remoteRevision() ) {
      currentItem.setRemoteRevision( newItem.remoteRevision() );
      mChangedParts << AKONADI_PARAM_REMOTEREVISION;
    }
    if ( currentItem.gid() != newItem.gid() ) {
      currentItem.setGid( newItem.gid() );
      mChangedParts << AKONADI_PARAM_GID;
    }
    currentItem.setDatetime( newItem.datetime() );
    currentItem.setAtime( QDateTime::currentDateTime() );
    // Only mark dirty when merged from application
    currentItem.setDirty( !connection()->context()->resource().isValid() );

    const Flag::List flags = HandlerHelper::resolveFlags( newFlags );
    const Collection col = Collection::retrieveById( newItem.collectionId() );
    bool flagsChanged = false;
    DataStore::self()->appendItemsFlags( PimItem::List() << currentItem, flags,
                                         flagsChanged, true, col, true );
    if ( flagsChanged ) {
      mChangedParts << AKONADI_PARAM_FLAGS;
    }

    Part::List existingParts = Part::retrieveFiltered( Part::pimItemIdColumn(), currentItem.id() );
    QMap<QByteArray, qint64> partsSizes;
    Q_FOREACH ( const Part &part, existingParts ) {
      partsSizes.insert( PartTypeHelper::fullName( part.partType() ).toLatin1(), part.datasize() );
    }

    m_streamParser->beginList();
    while ( !m_streamParser->atListEnd() ) {
      QByteArray error, partName;
      qint64 partSize;
      const QByteArray command = m_streamParser->readString();
      bool changed = false;
      if ( command.isEmpty() ) {
        throw HandlerException( "Syntax error" );
      }
      if ( !PartHelper::storeStreamedParts( command, m_streamParser, currentItem, true, partName, partSize, error, &changed ) ) {
        return failureResponse( error );
      }
      if ( changed ) {
        mChangedParts << partName;
        partsSizes.insert( partName, partSize );
      }
    }

    qint64 size = std::accumulate( partsSizes.begin(), partsSizes.end(), 0);
    currentItem.setSize( size );

    // Store all changes
    if ( !currentItem.update() ) {
      return failureResponse( "Failed to store merged item" );
    }

    return true;
}

bool Merge::notify( const PimItem &item, const Collection &collection )
{
    if ( !mChangedParts.isEmpty() ) {
      DataStore::self()->notificationCollector()->itemChanged( item, mChangedParts, collection );
    }

    ImapSet set;
    set.add( QVector<qint64>() << item.id() );
    Scope scope( Scope::Uid );
    scope.setUidSet( set );

    FetchScope fetchScope;
    // FetchHelper requires collection context
    fetchScope.setAllAttributes( true );
    fetchScope.setFullPayload( true );
    fetchScope.setAncestorDepth( 1 );
    fetchScope.setCacheOnly( true );
    fetchScope.setExternalPayloadSupported( true );
    fetchScope.setFlagsRequested( true );
    fetchScope.setGidRequested( true );
    fetchScope.setMTimeRequested( true );
    fetchScope.setRemoteIdRequested( true );
    fetchScope.setRemoteRevisionRequested( true );
    fetchScope.setSizeRequested( true );
    fetchScope.setTagsRequested( true );

    FetchHelper fetch( connection(), scope, fetchScope );
    connect( &fetch, SIGNAL(responseAvailable(Akonadi::Server::Response)),
             this, SIGNAL(responseAvailable(Akonadi::Server::Response)) );
    if ( !fetch.fetchItems( AKONADI_CMD_ITEMFETCH ) ) {
      return failureResponse( "Failed to retrieve merged item" );
    }

    Response response;
    response.setTag( tag() );
    response.setSuccess();
    response.setString( "Merge completed" );
    Q_EMIT responseAvailable( response );
    return true;
}

bool Merge::parseStream()
{
    const QList<QByteArray> mergeParts = m_streamParser->readParenthesizedList();

    DataStore *db = DataStore::self();
    Transaction transaction( db );

    Collection parentCol;
    QList<QByteArray> itemFlags;
    PimItem item;
    // Parse the rest of the command, assuming X-AKAPPEND syntax
    if ( !buildPimItem( item, parentCol, itemFlags ) ) {
      return false;
    }

    // Merging is always restricted to the same collection and mimetype
    SelectQueryBuilder<PimItem> qb;
    qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, parentCol.id() );
    qb.addValueCondition( PimItem::mimeTypeIdColumn(), Query::Equals, item.mimeTypeId() );
    Q_FOREACH ( const QByteArray &part, mergeParts ) {
      if ( part == AKONADI_PARAM_GID ) {
        qb.addValueCondition( PimItem::gidColumn(), Query::Equals, item.gid() );
      } else if ( part == AKONADI_PARAM_REMOTEID ) {
        qb.addValueCondition( PimItem::remoteIdColumn(), Query::Equals, item.remoteId() );
      } else {
        throw HandlerException( "Only merging by RID or GID is allowed" );
      }
    }

    if ( !qb.exec() ) {
      return failureResponse( "Failed to query database for item" );
    }

    QVector<PimItem> result = qb.result();
    if ( result.count() == 0 ) {
      // No item with such GID/RID exists, so call AkAppend::insert() and behave
      // like if this was a new item
      if ( !insertItem( item, parentCol, itemFlags ) ) {
        return false;
      }
      if ( !transaction.commit() ) {
        return failureResponse( "Failed to commit transaction" );
      }
      return AkAppend::notify( item, parentCol );

    } else if ( result.count() == 1 ) {
      // Item with matching GID/RID combination exists, so merge this item into it
      // and send itemChanged()
      PimItem existingItem = result.first();
      if ( !mergeItem( item, existingItem, itemFlags ) ) {
        return false;
      }
      if ( !transaction.commit() ) {
        return failureResponse( "Failed to commit transaction" );
      }
      return notify( existingItem, parentCol );

    } else {
      // Nor GID or RID are guaranteed to be unique, so make sure we don't merge
      // something we don't want
      return failureResponse( "Multiple merge candidates, aborting" );
    }
}

