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
#include "storage/itemretriever.h"
#include "storage/partstreamer.h"
#include "connection.h"
#include "handlerhelper.h"
#include <response.h>

#include <libs/imapparser_p.h>
#include <libs/protocol_p.h>

#include <numeric>

using namespace Akonadi;
using namespace Akonadi::Server;

static QVector<QByteArray> localFlagsToPreserve = QVector<QByteArray>() << "$ATTACHMENT"
                                                                        << "$INVITATION"
                                                                        << "$ENCRYPTED"
                                                                        << "$SIGNED"
                                                                        << "$WATCHED";

Merge::Merge()
  : AkAppend()
{
}

Merge::~Merge()
{
}

bool Merge::mergeItem( PimItem &newItem, PimItem &currentItem,
                       const ChangedAttributes &itemFlags,
                       const ChangedAttributes &itemTagsRID,
                       const ChangedAttributes &itemTagsGID )
{
    if ( newItem.rev() > 0 ) {
        currentItem.setRev( newItem.rev() );
    }
    if ( !newItem.remoteId().isEmpty() && currentItem.remoteId() != newItem.remoteId() ) {
      currentItem.setRemoteId( newItem.remoteId() );
      mChangedParts << AKONADI_PARAM_REMOTEID;
    }
    if ( !newItem.remoteRevision().isEmpty() && currentItem.remoteRevision() != newItem.remoteRevision() ) {
      currentItem.setRemoteRevision( newItem.remoteRevision() );
      mChangedParts << AKONADI_PARAM_REMOTEREVISION;
    }
    if ( !newItem.gid().isEmpty() && currentItem.gid() != newItem.gid() ) {
      currentItem.setGid( newItem.gid() );
      mChangedParts << AKONADI_PARAM_GID;
    }
    if ( newItem.datetime().isValid() ) {
        currentItem.setDatetime( newItem.datetime() );
    }
    currentItem.setAtime( QDateTime::currentDateTime() );

    // Only mark dirty when merged from application
    currentItem.setDirty( !connection()->context()->resource().isValid() );

    const Collection col = Collection::retrieveById( newItem.collectionId() );
    if ( itemFlags.incremental ) {
      bool flagsAdded = false, flagsRemoved = false;
      if ( !itemFlags.added.isEmpty() ) {
        const Flag::List addedFlags = HandlerHelper::resolveFlags( itemFlags.added );
        DataStore::self()->appendItemsFlags( PimItem::List() << currentItem, addedFlags,
                                             &flagsAdded, true, col, true );
      }

      if ( !itemFlags.removed.isEmpty() ) {
        const Flag::List removedFlags = HandlerHelper::resolveFlags( itemFlags.removed );
        DataStore::self()->removeItemsFlags( PimItem::List() << currentItem, removedFlags,
                                             &flagsRemoved, col, true );
      }

      if ( flagsAdded || flagsRemoved ) {
        mChangedParts << AKONADI_PARAM_FLAGS;
      }
    } else if ( !itemFlags.added.isEmpty() ) {
      bool flagsChanged = false;

      QVector<QByteArray> flagNames = itemFlags.added;
      // Make sure we don't overwrite some local-only flags that can't come
      // through from Resource during ItemSync, like $ATTACHMENT, because the
      // resource is not aware of them (they are usually assigned by client
      // upon inspecting the payload)
      Q_FOREACH (const Flag &currentFlag, currentItem.flags()) {
          const QByteArray flagName = currentFlag.name().toLatin1();
          if (localFlagsToPreserve.contains(flagName) && !flagNames.contains(flagName)) {
              flagNames.push_back(flagName);
          }
      }
      const Flag::List flags = HandlerHelper::resolveFlags( flagNames );

      DataStore::self()->setItemsFlags( PimItem::List() << currentItem, flags,
                                        &flagsChanged, col, true );
      if ( flagsChanged ) {
        mChangedParts << AKONADI_PARAM_FLAGS;
      }
    }

    if ( itemTagsRID.incremental ) {
      bool tagsAdded = false, tagsRemoved = false;
      if ( !itemTagsRID.added.isEmpty() ) {
        const Tag::List addedTags = HandlerHelper::resolveTagsByRID( itemTagsRID.added, connection()->context() );
        DataStore::self()->appendItemsTags( PimItem::List() << currentItem, addedTags,
                                            &tagsAdded, true, col, true );
      }

      if ( !itemTagsRID.removed.isEmpty() ) {
        const Tag::List removedTags = HandlerHelper::resolveTagsByRID( itemTagsRID.removed, connection()->context() );
        DataStore::self()->removeItemsTags( PimItem::List() << currentItem, removedTags,
                                            &tagsRemoved, true );
      }

      if ( tagsAdded || tagsRemoved ) {
        mChangedParts << AKONADI_PARAM_TAGS;
      }
    } else if ( !itemTagsRID.added.isEmpty() ) {
      bool tagsChanged = false;
      const Tag::List tags = HandlerHelper::resolveTagsByRID( itemTagsRID.added, connection()->context() );
      DataStore::self()->setItemsTags( PimItem::List() << currentItem, tags,
                                       &tagsChanged, true );
      if ( tagsChanged ) {
        mChangedParts << AKONADI_PARAM_TAGS;
      }
    }

    if ( itemTagsGID.incremental ) {
      bool tagsAdded = false, tagsRemoved = false;
      if ( !itemTagsGID.added.isEmpty() ) {
        const Tag::List addedTags = HandlerHelper::resolveTagsByGID( itemTagsGID.added );
        DataStore::self()->appendItemsTags( PimItem::List() << currentItem, addedTags,
                                            &tagsAdded, true, col, true );
      }

      if ( !itemTagsGID.removed.isEmpty() ) {
        const Tag::List removedTags = HandlerHelper::resolveTagsByGID( itemTagsGID.removed );
        DataStore::self()->removeItemsTags( PimItem::List() << currentItem, removedTags,
                                            &tagsRemoved, true );
      }

      if ( tagsAdded || tagsRemoved ) {
        mChangedParts << AKONADI_PARAM_TAGS;
      }
    } else if ( !itemTagsGID.added.isEmpty() ) {
      bool tagsChanged = false;
      const Tag::List tags = HandlerHelper::resolveTagsByGID( itemTagsGID.added );
      DataStore::self()->setItemsTags( PimItem::List() << currentItem, tags,
                                       &tagsChanged, true );
      if ( tagsChanged ) {
        mChangedParts << AKONADI_PARAM_TAGS;
      }
    }


    Part::List existingParts = Part::retrieveFiltered( Part::pimItemIdColumn(), currentItem.id() );
    QMap<QByteArray, qint64> partsSizes;
    Q_FOREACH ( const Part &part, existingParts ) {
      partsSizes.insert( PartTypeHelper::fullName( part.partType() ).toLatin1(), part.datasize() );
    }

    PartStreamer streamer(connection(), m_streamParser, currentItem);
    connect( &streamer, SIGNAL(responseAvailable(Akonadi::Server::Response)),
             this, SIGNAL(responseAvailable(Akonadi::Server::Response)) );
    m_streamParser->beginList();
    while ( !m_streamParser->atListEnd() ) {
      QByteArray partName;
      qint64 partSize;
      QByteArray command = m_streamParser->readString();
      bool changed = false;
      if ( command.isEmpty() ) {
        throw HandlerException( "Syntax error" );
      }

      if ( command.startsWith( '-' ) ) {
        command = command.mid( 1 );
        Q_FOREACH ( const Part &part, existingParts ) {
          if ( part.partType().name() == QString::fromUtf8( command ) ) {
            DataStore::self()->removeItemParts( currentItem, QList<QByteArray>() << command );
            mChangedParts << command;
            partsSizes.remove( command );
            break;
          }
        }
        break;
      } else if ( command.startsWith( '+' ) ) {
        command = command.mid( 1 );
      }

      if ( !streamer.stream( command, true, partName, partSize, &changed ) ) {
        return failureResponse( streamer.error() );
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

    return true;
}

bool Merge::sendResponse( const QByteArray &responseStr, const PimItem &item )
{
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
    response.setString( responseStr );
    Q_EMIT responseAvailable( response );
    return true;
}

bool Merge::parseStream()
{
    const QList<QByteArray> mergeParts = m_streamParser->readParenthesizedList();

    DataStore *db = DataStore::self();
    Transaction transaction( db );

    Collection parentCol;
    ChangedAttributes itemFlags, itemTagsRID, itemTagsGID;
    PimItem item;
    // Parse the rest of the command, assuming X-AKAPPEND syntax
    if ( !buildPimItem( item, parentCol, itemFlags, itemTagsRID, itemTagsGID ) ) {
      return false;
    }

    bool silent = false;

    // Merging is always restricted to the same collection and mimetype
    SelectQueryBuilder<PimItem> qb;
    qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, parentCol.id() );
    qb.addValueCondition( PimItem::mimeTypeIdColumn(), Query::Equals, item.mimeTypeId() );
    Q_FOREACH ( const QByteArray &part, mergeParts ) {
      if ( part == AKONADI_PARAM_GID ) {
        qb.addValueCondition( PimItem::gidColumn(), Query::Equals, item.gid() );
      } else if ( part == AKONADI_PARAM_REMOTEID ) {
        qb.addValueCondition( PimItem::remoteIdColumn(), Query::Equals, item.remoteId() );
      } else if ( part == AKONADI_PARAM_SILENT ) {
        silent = true;
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
      if ( !insertItem( item, parentCol, itemFlags.added, itemTagsRID.added, itemTagsGID.added ) ) {
        return false;
      }
      if ( !transaction.commit() ) {
        return failureResponse( "Failed to commit transaction" );
      }
      AkAppend::notify( item, parentCol );
      return AkAppend::sendResponse( "Append completed", item );

    } else if ( result.count() == 1 ) {
      // Item with matching GID/RID combination exists, so merge this item into it
      // and send itemChanged()
      PimItem existingItem = result.first();
      if ( !mergeItem( item, existingItem, itemFlags, itemTagsRID, itemTagsGID ) ) {
        return false;
      }
      if ( !transaction.commit() ) {
        return failureResponse( "Failed to commit transaction" );
      }

      notify( existingItem, parentCol );
      if ( silent ) {
          return AkAppend::sendResponse( "Merge completed", existingItem );
      } else {
          return sendResponse( "Merge completed", existingItem );
      }

    } else {
      akDebug() << "Multiple merge candidates:";
      Q_FOREACH (const PimItem &item, result) {
          akDebug() << "\t" << item.id() << item.remoteId() << item.gid();
      }
      // Nor GID or RID are guaranteed to be unique, so make sure we don't merge
      // something we don't want
      return failureResponse( "Multiple merge candidates, aborting" );
    }
}

