/***************************************************************************
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
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

#include "akappend.h"

#include "fetchhelper.h"
#include "connection.h"
#include "preprocessormanager.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/parttypehelper.h"
#include "storage/dbconfig.h"
#include "storage/partstreamer.h"
#include "storage/parthelper.h"
#include "storage/selectquerybuilder.h"
#include <private/externalpartstorage_p.h>

#include <numeric> //std::accumulate

using namespace Akonadi;
using namespace Akonadi::Server;

static QVector<QByteArray> localFlagsToPreserve = QVector<QByteArray>() << "$ATTACHMENT"
                                                                        << "$INVITATION"
                                                                        << "$ENCRYPTED"
                                                                        << "$SIGNED"
                                                                        << "$WATCHED";

bool AkAppend::buildPimItem(const Protocol::CreateItemCommand &cmd, PimItem &item,
                            Collection &parentCol)
{
    parentCol = HandlerHelper::collectionFromScope(cmd.collection(), connection());
    if (!parentCol.isValid()) {
        return failureResponse("Invalid parent collection");
    }
    if (parentCol.isVirtual()) {
        return failureResponse("Cannot append item into virtual collection");
    }

    MimeType mimeType = MimeType::retrieveByNameOrCreate(cmd.mimeType());
    if (!mimeType.isValid()) {
        return failureResponse(QStringLiteral("Unable to create mimetype '") % cmd.mimeType() % QStringLiteral("'."));
    }

    item.setRev(0);
    item.setSize(cmd.itemSize());
    item.setMimeTypeId(mimeType.id());
    item.setCollectionId(parentCol.id());
    item.setDatetime(cmd.dateTime());
    if (cmd.remoteId().isEmpty()) {
        // from application
        item.setDirty(true);
    } else {
        // from resource
        item.setRemoteId(cmd.remoteId());
        item.setDirty(false);
    }
    item.setRemoteRevision(cmd.remoteRevision());
    item.setGid(cmd.gid());
    item.setAtime(QDateTime::currentDateTime());

    return true;
}

bool AkAppend::insertItem(const Protocol::CreateItemCommand &cmd, PimItem &item,
                          const Collection &parentCol)
{
    if (!item.datetime().isValid()) {
        item.setDatetime(QDateTime::currentDateTimeUtc());
    }

    if (!item.insert()) {
        return failureResponse("Failed to append item");
    }

    // set message flags
    const QSet<QByteArray> flags = cmd.mergeModes() == Protocol::CreateItemCommand::None ? cmd.flags() : cmd.addedFlags();
    if (!flags.isEmpty()) {
        // This will hit an entry in cache inserted there in buildPimItem()
        const Flag::List flagList = HandlerHelper::resolveFlags(flags);
        bool flagsChanged = false;
        if (!DataStore::self()->appendItemsFlags(PimItem::List() << item, flagList, &flagsChanged, false, parentCol, true)) {
            return failureResponse("Unable to append item flags.");
        }
    }

    const Scope tags = cmd.mergeModes() == Protocol::CreateItemCommand::None ? cmd.tags() : cmd.addedTags();
    if (!tags.isEmpty()) {
        const Tag::List tagList = HandlerHelper::tagsFromScope(tags, connection());
        bool tagsChanged = false;
        if (!DataStore::self()->appendItemsTags(PimItem::List() << item, tagList, &tagsChanged, false, parentCol, true)) {
            return failureResponse("Unable to append item tags.");
        }
    }

    // Handle individual parts
    qint64 partSizes = 0;
    PartStreamer streamer(connection(), item, this);
    connect(&streamer, &PartStreamer::responseAvailable,
            this, static_cast<void(Handler::*)(const Protocol::Command &)>(&Handler::sendResponse));
    Q_FOREACH (const QByteArray &partName, cmd.parts()) {
        qint64 partSize = 0;
        if (!streamer.stream(true, partName, partSize)) {
            return failureResponse(streamer.error());
        }
        partSizes += partSize;
    }
    const Protocol::Attributes attrs = cmd.attributes();
    for (auto iter = attrs.cbegin(), end = attrs.cend(); iter != end; ++iter) {
       if (!streamer.streamAttribute(true, iter.key(), iter.value())) {
           return failureResponse(streamer.error());
       }
    }

    // TODO: Try to avoid this addition query
    if (partSizes > item.size()) {
        item.setSize(partSizes);
        item.update();
    }

    // Preprocessing
    if (PreprocessorManager::instance()->isActive()) {
        Part hiddenAttribute;
        hiddenAttribute.setPimItemId(item.id());
        hiddenAttribute.setPartType(PartTypeHelper::fromFqName(QStringLiteral(AKONADI_ATTRIBUTE_HIDDEN)));
        hiddenAttribute.setData(QByteArray());
        hiddenAttribute.setDatasize(0);
        // TODO: Handle errors? Technically, this is not a critical issue as no data are lost
        PartHelper::insert(&hiddenAttribute);
    }

    const bool seen = flags.contains(AKONADI_FLAG_SEEN) || flags.contains(AKONADI_FLAG_IGNORED);
    notify(item, seen, item.collection());
    sendResponse(item, Protocol::CreateItemCommand::None);

    return true;
}

bool AkAppend::mergeItem(const Protocol::CreateItemCommand &cmd,
                         PimItem &newItem, PimItem &currentItem,
                         const Collection &parentCol)
{
    bool needsUpdate = false;
    QSet<QByteArray> changedParts;

    if (!newItem.remoteId().isEmpty() && currentItem.remoteId() != newItem.remoteId()) {
        currentItem.setRemoteId(newItem.remoteId());
        changedParts.insert(AKONADI_PARAM_REMOTEID);
        needsUpdate = true;
    }
    if (!newItem.remoteRevision().isEmpty() && currentItem.remoteRevision() != newItem.remoteRevision()) {
        currentItem.setRemoteRevision(newItem.remoteRevision());
        changedParts.insert(AKONADI_PARAM_REMOTEREVISION);
        needsUpdate = true;
    }
    if (!newItem.gid().isEmpty() && currentItem.gid() != newItem.gid()) {
        currentItem.setGid(newItem.gid());
        changedParts.insert(AKONADI_PARAM_GID);
        needsUpdate = true;
    }
    if (newItem.datetime().isValid() && newItem.datetime() != currentItem.datetime()) {
        currentItem.setDatetime(newItem.datetime());
        needsUpdate = true;
    }

    if (newItem.size() > 0 && newItem.size() != currentItem.size()) {
        currentItem.setSize(newItem.size());
        needsUpdate = true;
    }

    const Collection col = Collection::retrieveById(parentCol.id());
    if (cmd.flags().isEmpty()) {
        bool flagsAdded = false, flagsRemoved = false;
        if (!cmd.addedFlags().isEmpty()) {
            const Flag::List addedFlags = HandlerHelper::resolveFlags(cmd.addedFlags());
            DataStore::self()->appendItemsFlags(PimItem::List() << currentItem, addedFlags,
                                                &flagsAdded, true, col, true);
        }
        if (!cmd.removedFlags().isEmpty()) {
            const Flag::List removedFlags = HandlerHelper::resolveFlags(cmd.removedFlags());
            DataStore::self()->removeItemsFlags(PimItem::List() << currentItem, removedFlags,
                                                &flagsRemoved, col, true);
        }
        if (flagsAdded || flagsRemoved) {
            changedParts.insert(AKONADI_PARAM_FLAGS);
            needsUpdate = true;
        }
    } else {
        bool flagsChanged = false;
        QSet<QByteArray> flagNames = cmd.flags();

        // Make sure we don't overwrite some local-only flags that can't come
        // through from Resource during ItemSync, like $ATTACHMENT, because the
        // resource is not aware of them (they are usually assigned by client
        // upon inspecting the payload)
        Q_FOREACH (const Flag &currentFlag, currentItem.flags()) {
            const QByteArray currentFlagName = currentFlag.name().toLatin1();
            if (localFlagsToPreserve.contains(currentFlagName)) {
                flagNames.insert(currentFlagName);
            }
        }
        const Flag::List flags = HandlerHelper::resolveFlags(flagNames);
        DataStore::self()->setItemsFlags(PimItem::List() << currentItem, flags,
                                         &flagsChanged, col, true);
        if (flagsChanged) {
            changedParts.insert(AKONADI_PARAM_FLAGS);
            needsUpdate = true;
        }
    }

    if (cmd.tags().isEmpty()) {
        bool tagsAdded = false, tagsRemoved = false;
        if (!cmd.addedTags().isEmpty()) {
            const Tag::List addedTags = HandlerHelper::tagsFromScope(cmd.addedTags(), connection());
            DataStore::self()->appendItemsTags(PimItem::List() << currentItem, addedTags,
                                               &tagsAdded, true, col, true);
        }
        if (!cmd.removedTags().isEmpty()) {
            const Tag::List removedTags = HandlerHelper::tagsFromScope(cmd.removedTags(), connection());
            DataStore::self()->removeItemsTags(PimItem::List() << currentItem, removedTags,
                                               &tagsRemoved, true);
        }

        if (tagsAdded || tagsRemoved) {
            changedParts.insert(AKONADI_PARAM_TAGS);
            needsUpdate = true;
        }
    } else {
        bool tagsChanged = false;
        const Tag::List tags = HandlerHelper::tagsFromScope(cmd.tags(), connection());
        DataStore::self()->setItemsTags(PimItem::List() << currentItem, tags,
                                        &tagsChanged, true);
        if (tagsChanged) {
            changedParts.insert(AKONADI_PARAM_TAGS);
            needsUpdate = true;
        }
    }

    const Part::List existingParts = Part::retrieveFiltered(Part::pimItemIdColumn(), currentItem.id());
    QMap<QByteArray, qint64> partsSizes;
    for (const Part &part : existingParts ) {
        partsSizes.insert(PartTypeHelper::fullName(part.partType()).toLatin1(), part.datasize());
    }

    PartStreamer streamer(connection(), currentItem);
    connect(&streamer, &PartStreamer::responseAvailable,
            this, static_cast<void(Handler::*)(const Protocol::Command &)>(&Handler::sendResponse));
    Q_FOREACH (const QByteArray &partName, cmd.parts()) {
        bool changed = false;
        qint64 partSize = 0;
        if (!streamer.stream(true, partName, partSize, &changed)) {
            return failureResponse(streamer.error());
        }

        if (changed) {
            changedParts.insert(partName);
            partsSizes.insert(partName, partSize);
            needsUpdate = true;
        }
    }

    const qint64 size = std::accumulate(partsSizes.begin(), partsSizes.end(), 0);
    if (size > currentItem.size()) {
        currentItem.setSize(size);
        needsUpdate = true;
    }

    if (needsUpdate) {
        currentItem.setRev(qMax(newItem.rev(), currentItem.rev()) + 1);
        currentItem.setAtime(QDateTime::currentDateTime());
        // Only mark dirty when merged from application
        currentItem.setDirty(!connection()->context()->resource().isValid());

        // Store all changes
        if (!currentItem.update()) {
            return failureResponse("Failed to store merged item");
        }

        notify(currentItem, currentItem.collection(), changedParts);
    }

    sendResponse(currentItem, cmd.mergeModes());

    return true;
}

bool AkAppend::sendResponse(const PimItem& item, Protocol::CreateItemCommand::MergeModes mergeModes)
{
    if (mergeModes & Protocol::CreateItemCommand::Silent || mergeModes & Protocol::CreateItemCommand::None) {
        Protocol::FetchItemsResponse resp(item.id());
        resp.setMTime(item.datetime());
        Handler::sendResponse(resp);
        return true;
    }

    Protocol::FetchScope fetchScope;
    fetchScope.setAncestorDepth(Protocol::Ancestor::ParentAncestor);
    fetchScope.setFetch(Protocol::FetchScope::AllAttributes |
                        Protocol::FetchScope::FullPayload |
                        Protocol::FetchScope::CacheOnly |
                        Protocol::FetchScope::Flags |
                        Protocol::FetchScope::GID |
                        Protocol::FetchScope::MTime |
                        Protocol::FetchScope::RemoteID |
                        Protocol::FetchScope::RemoteRevision |
                        Protocol::FetchScope::Size |
                        Protocol::FetchScope::Tags);
    fetchScope.setTagFetchScope({ "GID" });

    ImapSet set;
    set.add(QVector<qint64>() << item.id());
    Scope scope;
    scope.setUidSet(set);

    FetchHelper fetchHelper(connection(), scope, fetchScope);
    if (!fetchHelper.fetchItems()) {
        return failureResponse("Failed to retrieve item");
    }

    return true;
}


bool AkAppend::notify(const PimItem &item, bool seen, const Collection &collection)
{
    DataStore::self()->notificationCollector()->itemAdded(item, seen, collection);

    if (PreprocessorManager::instance()->isActive()) {
        // enqueue the item for preprocessing
        PreprocessorManager::instance()->beginHandleItem(item, DataStore::self());
    }
    return true;
}

bool AkAppend::notify(const PimItem &item, const Collection &collection,
                      const QSet<QByteArray> &changedParts)
{
    if (!changedParts.isEmpty()) {
        DataStore::self()->notificationCollector()->itemChanged(item, changedParts, collection);
    }
    return true;
}


bool AkAppend::parseStream()
{
    Protocol::CreateItemCommand cmd(m_command);

    // FIXME: The streaming/reading of all item parts can hold the transaction for
    // unnecessary long time -> should we wrap the PimItem into one transaction
    // and try to insert Parts independently? In case we fail to insert a part,
    // it's not a problem as it can be re-fetched at any time, except for attributes.
    DataStore *db = DataStore::self();
    Transaction transaction(db);
    ExternalPartStorageTransaction storageTrx;

    PimItem item;
    Collection parentCol;
    if (!buildPimItem(cmd, item, parentCol)) {
        return false;
    }

    if (cmd.mergeModes() == Protocol::CreateItemCommand::None) {
        if (!insertItem(cmd, item, parentCol)) {
            return false;
        }
        if (!transaction.commit()) {
            return failureResponse("Failed to commit transaction");
        }
        storageTrx.commit();
    } else {
        // Merging is always restricted to the same collection
        SelectQueryBuilder<PimItem> qb;
        qb.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, parentCol.id());
        if (cmd.mergeModes() & Protocol::CreateItemCommand::GID) {
            qb.addValueCondition(PimItem::gidColumn(), Query::Equals, item.gid());
        }
        if (cmd.mergeModes() & Protocol::CreateItemCommand::RemoteID) {
            qb.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, item.remoteId());
        }

        if (!qb.exec()) {
            return failureResponse("Failed to query database for item");
        }

        const QVector<PimItem> result = qb.result();
        if (result.count() == 0) {
            // No item with such GID/RID exists, so call AkAppend::insert() and behave
            // like if this was a new item
            if (!insertItem(cmd, item, parentCol)) {
                return false;
            }
            if (!transaction.commit()) {
                return failureResponse("Failed to commit transaction");
            }
            storageTrx.commit();

        } else if (result.count() == 1) {
            // Item with matching GID/RID combination exists, so merge this item into it
            // and send itemChanged()
            PimItem existingItem = result.at(0);

            if (!mergeItem(cmd, item, existingItem, parentCol)) {
                return false;
            }
            if (!transaction.commit()) {
                return failureResponse("Failed to commit transaction");
            }
            storageTrx.commit();
        } else {
            qCDebug(AKONADISERVER_LOG) << "Multiple merge candidates:";
            Q_FOREACH (const PimItem &item, result) {
                qCDebug(AKONADISERVER_LOG) << "\tID:" << item.id() << ", RID:" << item.remoteId()
                                           << ", GID:" << item.gid()
                                           << ", Collection:" << item.collection().name() << "(" << item.collectionId() << ")"
                                           << ", Resource:" << item.collection().resource().name() << "(" << item.collection().resourceId() << ")";
            }
            // Nor GID or RID are guaranteed to be unique, so make sure we don't merge
            // something we don't want
            return failureResponse("Multiple merge candidates, aborting");
        }
    }

    return successResponse<Protocol::CreateItemResponse>();
}
