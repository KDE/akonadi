/***************************************************************************
 *   SPDX-FileCopyrightText: 2007 Robert Zwerus <arzie@dds.nl>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "itemcreatehandler.h"

#include "itemfetchhelper.h"
#include "akonadi.h"
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
using namespace AkRanges;

ItemCreateHandler::ItemCreateHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool ItemCreateHandler::buildPimItem(const Protocol::CreateItemCommand &cmd, PimItem &item,
                                     Collection &parentCol)
{
    parentCol = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
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

bool ItemCreateHandler::insertItem(const Protocol::CreateItemCommand &cmd, PimItem &item,
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
        if (!storageBackend()->appendItemsFlags({item}, flagList, &flagsChanged, false, parentCol, true)) {
            return failureResponse("Unable to append item flags.");
        }
    }

    const Scope tags = cmd.mergeModes() == Protocol::CreateItemCommand::None ? cmd.tags() : cmd.addedTags();
    if (!tags.isEmpty()) {
        const Tag::List tagList = HandlerHelper::tagsFromScope(tags, connection()->context());
        bool tagsChanged = false;
        if (!storageBackend()->appendItemsTags({item}, tagList, &tagsChanged, false, parentCol, true)) {
            return failureResponse(QStringLiteral("Unable to append item tags."));
        }
    }

    // Handle individual parts
    qint64 partSizes = 0;
    PartStreamer streamer(connection(), item);
    Q_FOREACH (const QByteArray &partName, cmd.parts()) {
        qint64 partSize = 0;
        try {
            streamer.stream(true, partName, partSize);
        } catch (const PartStreamerException &e) {
            return failureResponse(e.what());
        }
        partSizes += partSize;
    }
    const Protocol::Attributes attrs = cmd.attributes();
    for (auto iter = attrs.cbegin(), end = attrs.cend(); iter != end; ++iter) {
        try {
            streamer.streamAttribute(true, iter.key(), iter.value());
        } catch (const PartStreamerException &e) {
            return failureResponse(e.what());
        }
    }

    // TODO: Try to avoid this addition query
    if (partSizes > item.size()) {
        item.setSize(partSizes);
        item.update();
    }

    // Preprocessing
    if (akonadi().preprocessorManager().isActive()) {
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

bool ItemCreateHandler::mergeItem(const Protocol::CreateItemCommand &cmd,
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
        bool flagsAdded = false;
        bool flagsRemoved = false;
        if (!cmd.addedFlags().isEmpty()) {
            const auto addedFlags = HandlerHelper::resolveFlags(cmd.addedFlags());
            storageBackend()->appendItemsFlags({currentItem}, addedFlags, &flagsAdded, true, col, true);
        }
        if (!cmd.removedFlags().isEmpty()) {
            const auto removedFlags = HandlerHelper::resolveFlags(cmd.removedFlags());
            storageBackend()->removeItemsFlags({currentItem}, removedFlags, &flagsRemoved, col, true);
        }
        if (flagsAdded || flagsRemoved) {
            changedParts.insert(AKONADI_PARAM_FLAGS);
            needsUpdate = true;
        }
    } else {
        bool flagsChanged = false;
        QSet<QByteArray> flagNames = cmd.flags();

        static QVector<QByteArray> localFlagsToPreserve = {
            "$ATTACHMENT",
            "$INVITATION",
            "$ENCRYPTED",
            "$SIGNED",
            "$WATCHED" };

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
        const auto flags = HandlerHelper::resolveFlags(flagNames);
        storageBackend()->setItemsFlags({currentItem}, flags, &flagsChanged, col, true);
        if (flagsChanged) {
            changedParts.insert(AKONADI_PARAM_FLAGS);
            needsUpdate = true;
        }
    }

    if (cmd.tags().isEmpty()) {
        bool tagsAdded = false;
        bool tagsRemoved = false;
        if (!cmd.addedTags().isEmpty()) {
            const auto addedTags = HandlerHelper::tagsFromScope(cmd.addedTags(), connection()->context());
            storageBackend()->appendItemsTags({currentItem}, addedTags, &tagsAdded, true, col, true);
        }
        if (!cmd.removedTags().isEmpty()) {
            const Tag::List removedTags = HandlerHelper::tagsFromScope(cmd.removedTags(), connection()->context());
            storageBackend()->removeItemsTags({currentItem}, removedTags, &tagsRemoved, true);
        }

        if (tagsAdded || tagsRemoved) {
            changedParts.insert(AKONADI_PARAM_TAGS);
            needsUpdate = true;
        }
    } else {
        bool tagsChanged = false;
        const auto tags = HandlerHelper::tagsFromScope(cmd.tags(), connection()->context());
        storageBackend()->setItemsTags({currentItem}, tags, &tagsChanged, true);
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
        try {
            streamer.stream(true, partName, partSize, &changed);
        } catch (const PartStreamerException &e) {
            return failureResponse(e.what());
        }

        if (changed) {
            changedParts.insert(partName);
            partsSizes.insert(partName, partSize);
            needsUpdate = true;
        }
    }

    const qint64 size = std::accumulate(partsSizes.begin(), partsSizes.end(), 0LL);
    if (size > currentItem.size()) {
        currentItem.setSize(size);
        needsUpdate = true;
    }

    if (needsUpdate) {
        currentItem.setRev(qMax(newItem.rev(), currentItem.rev()) + 1);
        currentItem.setAtime(QDateTime::currentDateTimeUtc());
        // Only mark dirty when merged from application
        currentItem.setDirty(!connection()->context().resource().isValid());

        // Store all changes
        if (!currentItem.update()) {
            return failureResponse("Failed to store merged item");
        }

        notify(currentItem, currentItem.collection(), changedParts);
    }

    sendResponse(currentItem, cmd.mergeModes());

    return true;
}

bool ItemCreateHandler::sendResponse(const PimItem &item, Protocol::CreateItemCommand::MergeModes mergeModes)
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
    ImapSet set;
    set.add(QVector<qint64>() << item.id());
    Scope scope;
    scope.setUidSet(set);

    ItemFetchHelper fetchHelper(connection(), scope, fetchScope, Protocol::TagFetchScope{}, akonadi());
    if (!fetchHelper.fetchItems()) {
        return failureResponse("Failed to retrieve item");
    }

    return true;
}

bool ItemCreateHandler::notify(const PimItem &item, bool seen, const Collection &collection)
{
    storageBackend()->notificationCollector()->itemAdded(item, seen, collection);

    if (akonadi().preprocessorManager().isActive()) {
        // enqueue the item for preprocessing
        akonadi().preprocessorManager().beginHandleItem(item, storageBackend());
    }
    return true;
}

bool ItemCreateHandler::notify(const PimItem &item, const Collection &collection,
                      const QSet<QByteArray> &changedParts)
{
    if (!changedParts.isEmpty()) {
        storageBackend()->notificationCollector()->itemChanged(item, changedParts, collection);
    }
    return true;
}

void ItemCreateHandler::recoverFromMultipleMergeCandidates(const PimItem::List &items, const Collection &collection)
{
    // HACK HACK HACK: When this happens within ItemSync, we are running inside a client-side
    // transaction, so just calling commit here won't have any effect, since this handler will
    // ultimately fail and the client will rollback the transaction. To circumvent this, we
    // will forcibly commit the transaction, do our changes here within a new transaction and
    // then we open a new transaction so that the client won't notice.

    int transactionDepth = 0;
    while (storageBackend()->inTransaction()) {
        ++transactionDepth;
        storageBackend()->commitTransaction();
    }
    const AkScopeGuard restoreTransaction([&]() {
        for (int i = 0; i < transactionDepth; ++i) {
            storageBackend()->beginTransaction(QStringLiteral("RestoredTransactionAfterMMCRecovery"));
        }
    });

    Transaction transaction(storageBackend(), QStringLiteral("MMC Recovery Transaction"));

    // If any of the conflicting items is dirty or does not have a remote ID, we don't want to remove
    // them as it would cause data loss. There's a chance next changeReplay will fix this, so
    // next time the ItemSync hits this multiple merge candidates, all changes will be committed
    // and this check will succeed
    if (items | Actions::any([](const auto &item) { return item.dirty() || item.remoteId().isEmpty(); })) {
        qCWarning(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery failed: at least one of the candidates has uncommitted changes!";
        return;
    }

    // This cannot happen with ItemSync, but in theory could happen during individual GID merge.
    if (items | Actions::any([collection](const auto &item) { return item.collectionId() != collection.id(); })) {
        qCWarning(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery failed: all candidates do not belong to the same collection.";
        return;
    }

    storageBackend()->cleanupPimItems(items, DataStore::Silent);
    if (!transaction.commit()) {
        qCWarning(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery failed: failed to commit database transaction.";
        return;
    }

    // Schedule a new sync of the collection, one that will succeed
    akonadi().itemRetrievalManager().triggerCollectionSync(collection.resource().name(), collection.id());

    qCInfo(AKONADISERVER_LOG) << "Automatic multiple merge candidates recovery successful: conflicting items" << (items | Views::transform([](const auto &i) { return i.id(); }) | Actions::toQVector)
                              << "in collection" << collection.name() << "(ID:" << collection.id() << ") were removed and a new sync was scheduled in the resource"
                              << collection.resource().name();
}

bool ItemCreateHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::CreateItemCommand>(m_command);

    // FIXME: The streaming/reading of all item parts can hold the transaction for
    // unnecessary long time -> should we wrap the PimItem into one transaction
    // and try to insert Parts independently? In case we fail to insert a part,
    // it's not a problem as it can be re-fetched at any time, except for attributes.
    Transaction transaction(storageBackend(), QStringLiteral("ItemCreateHandler"));
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
            mergeCondition.addValueCondition(PimItem::gidColumn(), Query::Equals, QLatin1String(""));
            rootCondition.addCondition(mergeCondition);
        }
        qb.addCondition(rootCondition);

        if (!qb.exec()) {
            return failureResponse("Failed to query database for item");
        }

        const QVector<PimItem> result = qb.result();
        if (result.isEmpty()) {
            // No item with such GID/RID exists, so call ItemCreateHandler::insert() and behave
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
                qCWarning(AKONADISERVER_LOG) << "\tID:" << item.id() << ", RID:" << item.remoteId()
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
