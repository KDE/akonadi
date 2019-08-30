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
#include "storage/itemretrievalmanager.h"
#include <private/externalpartstorage_p.h>

#include "shared/akranges.h"
#include "shared/akscopeguard.h"

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
        return failureResponse(QStringLiteral("Invalid parent collection"));
    }
    if (parentCol.isVirtual()) {
        return failureResponse(QStringLiteral("Cannot append item into virtual collection"));
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
    item.setAtime(QDateTime::currentDateTimeUtc());

    return true;
}

bool AkAppend::insertItem(const Protocol::CreateItemCommand &cmd, PimItem &item,
                          const Collection &parentCol)
{
    if (!item.datetime().isValid()) {
        item.setDatetime(QDateTime::currentDateTimeUtc());
    }

    if (!item.insert()) {
        return failureResponse(QStringLiteral("Failed to append item"));
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
            return failureResponse(QStringLiteral("Unable to append item tags."));
        }
    }

    // Handle individual parts
    qint64 partSizes = 0;
    PartStreamer streamer(connection(), item);
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
    if (cmd.flags().isEmpty() && !cmd.flagsOverwritten()) {
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
    for (const Part &part : existingParts) {
        partsSizes.insert(PartTypeHelper::fullName(part.partType()).toLatin1(), part.datasize());
    }

    PartStreamer streamer(connection(), currentItem);
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
        currentItem.setAtime(QDateTime::currentDateTimeUtc());
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

bool AkAppend::sendResponse(const PimItem &item, Protocol::CreateItemCommand::MergeModes mergeModes)
{
    if (mergeModes & Protocol::CreateItemCommand::Silent || mergeModes & Protocol::CreateItemCommand::None) {
        Protocol::FetchItemsResponse resp;
        resp.setId(item.id());
        resp.setMTime(item.datetime());
        Handler::sendResponse(std::move(resp));
        return true;
    }

    Protocol::ItemFetchScope fetchScope;
    fetchScope.setAncestorDepth(Protocol::ItemFetchScope::ParentAncestor);
    fetchScope.setFetch(Protocol::ItemFetchScope::AllAttributes |
                        Protocol::ItemFetchScope::FullPayload |
                        Protocol::ItemFetchScope::CacheOnly |
                        Protocol::ItemFetchScope::Flags |
                        Protocol::ItemFetchScope::GID |
                        Protocol::ItemFetchScope::MTime |
                        Protocol::ItemFetchScope::RemoteID |
                        Protocol::ItemFetchScope::RemoteRevision |
                        Protocol::ItemFetchScope::Size |
                        Protocol::ItemFetchScope::Tags);
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

void AkAppend::recoverFromMultipleMergeCandidates(const PimItem::List &items, const Collection &collection)
{
    // HACK HACK HACK: When this happens within ItemSync, we are running inside a client-side
    // transaction, so just calling commit here won't have any effect, since this handler will
    // ultimately fail and the client will rollback the transaction. To circumvent this, we
    // will forcibly commit the transaction, do our changes here within a new transaction and
    // then we open a new transaction so that the client won't notice.

    DataStore *db = DataStore::self();
    int transactionDepth = 0;
    while (db->inTransaction()) {
        ++transactionDepth;
        db->commitTransaction();
    }
    const AkScopeGuard restoreTransaction([&]() {
        for (int i = 0; i < transactionDepth; ++i) {
            db->beginTransaction(QStringLiteral("RestoredTransactionAfterMMCRecovery"));
        }
    });

    Transaction transaction(db, QStringLiteral("MMC Recovery Transaction"));

    // If any of the conflicting items is dirty or does not have a remote ID, we don't want to remove
    // them as it would cause data loss. There's a chance next changeReplay will fix this, so
    // next time the ItemSync hits this multiple merge candidates, all changes will be committed
    // and this check will succeed
    if (items | any([](const auto &item) { return item.dirty() || item.remoteId().isEmpty(); })) {
        qCWarning(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery failed: at least one of the candidates has uncommitted changes!";
        return;
    }

    // This cannot happen with ItemSync, but in theory could happen during individual GID merge.
    if (items | any([collection](const auto &item) { return item.collectionId() != collection.id(); })) {
        qCWarning(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery failed: all candidates do not belong to the same collection.";
        return;
    }

    db->cleanupPimItems(items, DataStore::Silent);
    if (!transaction.commit()) {
        qCWarning(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery failed: failed to commit database transaction.";
        return;
    }


    // Schedule a new sync of the collection, one that will succeed
    const auto resource = collection.resource().name();
    QMetaObject::invokeMethod(ItemRetrievalManager::instance(), "triggerCollectionSync",
                              Qt::QueuedConnection,
                              Q_ARG(QString, resource), Q_ARG(qint64, collection.id()));

    qCInfo(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery successful: conflicting items" << (items | transform([](const auto &i) { return i.id(); }) | toQVector)
                              << "in collection" << collection.name() << "(ID:" << collection.id() << ") were removed and a new sync was scheduled in the resource"
                              << resource;
}

bool AkAppend::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::CreateItemCommand>(m_command);

    // FIXME: The streaming/reading of all item parts can hold the transaction for
    // unnecessary long time -> should we wrap the PimItem into one transaction
    // and try to insert Parts independently? In case we fail to insert a part,
    // it's not a problem as it can be re-fetched at any time, except for attributes.
    DataStore *db = DataStore::self();
    Transaction transaction(db, QStringLiteral("AKAPPEND"));
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
            return failureResponse(QStringLiteral("Failed to commit transaction"));
        }
        storageTrx.commit();
    } else {
        // Merging is always restricted to the same collection
        SelectQueryBuilder<PimItem> qb;
        qb.setForUpdate();
        qb.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, parentCol.id());
        Query::Condition rootCondition(Query::Or);

        Query::Condition mergeCondition(Query::And);
        if (cmd.mergeModes() & Protocol::CreateItemCommand::GID) {
            mergeCondition.addValueCondition(PimItem::gidColumn(), Query::Equals, item.gid());
        }
        if (cmd.mergeModes() & Protocol::CreateItemCommand::RemoteID) {
            mergeCondition.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, item.remoteId());
        }
        rootCondition.addCondition(mergeCondition);

        // If an Item with matching RID but empty GID exists during GID merge,
        // merge into this item instead of creating a new one
        if (cmd.mergeModes() & Protocol::CreateItemCommand::GID && !item.remoteId().isEmpty()) {
            mergeCondition = Query::Condition(Query::And);
            mergeCondition.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, item.remoteId());
            mergeCondition.addValueCondition(PimItem::gidColumn(), Query::Equals, QStringLiteral(""));
            rootCondition.addCondition(mergeCondition);
        }
        qb.addCondition(rootCondition);

        if (!qb.exec()) {
            return failureResponse("Failed to query database for item");
        }

        const QVector<PimItem> result = qb.result();
        if (result.isEmpty()) {
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
            qCWarning(AKONADISERVER_LOG) << "Multiple merge candidates, will attempt to recover:";
            for (const PimItem &item : result) {
                qCDebug(AKONADISERVER_LOG) << "\tID:" << item.id() << ", RID:" << item.remoteId()
                                           << ", GID:" << item.gid()
                                           << ", Collection:" << item.collection().name() << "(" << item.collectionId() << ")"
                                           << ", Resource:" << item.collection().resource().name() << "(" << item.collection().resourceId() << ")";
            }

            transaction.commit(); // commit the current transaction, before we attempt MMC recovery
            recoverFromMultipleMergeCandidates(result, parentCol);

            // Even if the recovery was successful, indicate error to force the client to abort the
            // sync, since we've interfered with the overall state.
            return failureResponse(QStringLiteral("Multiple merge candidates in collection '%1', aborting").arg(item.collection().name()));
        }
    }

    return successResponse<Protocol::CreateItemResponse>();
}
