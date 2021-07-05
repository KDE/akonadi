/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notificationcollector.h"
#include "aggregatedfetchscope.h"
#include "akonadi.h"
#include "cachecleaner.h"
#include "connection.h"
#include "handler/itemfetchhelper.h"
#include "handlerhelper.h"
#include "intervalcheck.h"
#include "notificationmanager.h"
#include "search/searchmanager.h"
#include "selectquerybuilder.h"
#include "shared/akranges.h"
#include "storage/collectionstatistics.h"
#include "storage/datastore.h"
#include "storage/entity.h"

#include "akonadiserver_debug.h"

#include <QScopedValueRollback>

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationCollector::NotificationCollector(AkonadiServer &akonadi, DataStore *db)
    : mDb(db)
    , mAkonadi(akonadi)
{
    QObject::connect(db, &DataStore::transactionCommitted, [this]() {
        if (!mIgnoreTransactions) {
            dispatchNotifications();
        }
    });
    QObject::connect(db, &DataStore::transactionRolledBack, [this]() {
        if (!mIgnoreTransactions) {
            clear();
        }
    });
}

void NotificationCollector::itemAdded(const PimItem &item, bool seen, const Collection &collection, const QByteArray &resource)
{
    mAkonadi.searchManager().scheduleSearchUpdate();
    mAkonadi.collectionStatistics().itemAdded(collection, item.size(), seen);
    itemNotification(Protocol::ItemChangeNotification::Add, item, collection, Collection(), resource);
}

void NotificationCollector::itemChanged(const PimItem &item, const QSet<QByteArray> &changedParts, const Collection &collection, const QByteArray &resource)
{
    mAkonadi.searchManager().scheduleSearchUpdate();
    itemNotification(Protocol::ItemChangeNotification::Modify, item, collection, Collection(), resource, changedParts);
}

void NotificationCollector::itemsFlagsChanged(const PimItem::List &items,
                                              const QSet<QByteArray> &addedFlags,
                                              const QSet<QByteArray> &removedFlags,
                                              const Collection &collection,
                                              const QByteArray &resource)
{
    int seenCount = (addedFlags.contains(AKONADI_FLAG_SEEN) || addedFlags.contains(AKONADI_FLAG_IGNORED) ? items.count() : 0);
    seenCount -= (removedFlags.contains(AKONADI_FLAG_SEEN) || removedFlags.contains(AKONADI_FLAG_IGNORED) ? items.count() : 0);

    mAkonadi.collectionStatistics().itemsSeenChanged(collection, seenCount);
    itemNotification(Protocol::ItemChangeNotification::ModifyFlags, items, collection, Collection(), resource, QSet<QByteArray>(), addedFlags, removedFlags);
}

void NotificationCollector::itemsTagsChanged(const PimItem::List &items,
                                             const QSet<qint64> &addedTags,
                                             const QSet<qint64> &removedTags,
                                             const Collection &collection,
                                             const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyTags,
                     items,
                     collection,
                     Collection(),
                     resource,
                     QSet<QByteArray>(),
                     QSet<QByteArray>(),
                     QSet<QByteArray>(),
                     addedTags,
                     removedTags);
}

void NotificationCollector::itemsRelationsChanged(const PimItem::List &items,
                                                  const Relation::List &addedRelations,
                                                  const Relation::List &removedRelations,
                                                  const Collection &collection,
                                                  const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyRelations,
                     items,
                     collection,
                     Collection(),
                     resource,
                     QSet<QByteArray>(),
                     QSet<QByteArray>(),
                     QSet<QByteArray>(),
                     QSet<qint64>(),
                     QSet<qint64>(),
                     addedRelations,
                     removedRelations);
}

void NotificationCollector::itemsMoved(const PimItem::List &items,
                                       const Collection &collectionSrc,
                                       const Collection &collectionDest,
                                       const QByteArray &sourceResource)
{
    mAkonadi.searchManager().scheduleSearchUpdate();
    itemNotification(Protocol::ItemChangeNotification::Move, items, collectionSrc, collectionDest, sourceResource);
}

void NotificationCollector::itemsRemoved(const PimItem::List &items, const Collection &collection, const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::Remove, items, collection, Collection(), resource);
}

void NotificationCollector::itemsLinked(const PimItem::List &items, const Collection &collection)
{
    itemNotification(Protocol::ItemChangeNotification::Link, items, collection, Collection(), QByteArray());
}

void NotificationCollector::itemsUnlinked(const PimItem::List &items, const Collection &collection)
{
    itemNotification(Protocol::ItemChangeNotification::Unlink, items, collection, Collection(), QByteArray());
}

void NotificationCollector::collectionAdded(const Collection &collection, const QByteArray &resource)
{
    if (auto cleaner = mAkonadi.cacheCleaner()) {
        cleaner->collectionAdded(collection.id());
    }
    mAkonadi.intervalChecker().collectionAdded(collection.id());
    collectionNotification(Protocol::CollectionChangeNotification::Add, collection, collection.parentId(), -1, resource);
}

void NotificationCollector::collectionChanged(const Collection &collection, const QList<QByteArray> &changes, const QByteArray &resource)
{
    if (auto cleaner = mAkonadi.cacheCleaner()) {
        cleaner->collectionChanged(collection.id());
    }
    mAkonadi.intervalChecker().collectionChanged(collection.id());
    if (changes.contains(AKONADI_PARAM_ENABLED)) {
        mAkonadi.collectionStatistics().invalidateCollection(collection);
    }
    collectionNotification(Protocol::CollectionChangeNotification::Modify,
                           collection,
                           collection.parentId(),
                           -1,
                           resource,
                           changes | AkRanges::Actions::toQSet);
}

void NotificationCollector::collectionMoved(const Collection &collection, const Collection &source, const QByteArray &resource, const QByteArray &destResource)
{
    if (auto cleaner = mAkonadi.cacheCleaner()) {
        cleaner->collectionChanged(collection.id());
    }
    mAkonadi.intervalChecker().collectionChanged(collection.id());
    collectionNotification(Protocol::CollectionChangeNotification::Move,
                           collection,
                           source.id(),
                           collection.parentId(),
                           resource,
                           QSet<QByteArray>(),
                           destResource);
}

void NotificationCollector::collectionRemoved(const Collection &collection, const QByteArray &resource)
{
    if (auto cleaner = mAkonadi.cacheCleaner()) {
        cleaner->collectionRemoved(collection.id());
    }
    mAkonadi.intervalChecker().collectionRemoved(collection.id());
    mAkonadi.collectionStatistics().invalidateCollection(collection);
    collectionNotification(Protocol::CollectionChangeNotification::Remove, collection, collection.parentId(), -1, resource);
}

void NotificationCollector::collectionSubscribed(const Collection &collection, const QByteArray &resource)
{
    if (auto cleaner = mAkonadi.cacheCleaner()) {
        cleaner->collectionAdded(collection.id());
    }
    mAkonadi.intervalChecker().collectionAdded(collection.id());
    collectionNotification(Protocol::CollectionChangeNotification::Subscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>());
}

void NotificationCollector::collectionUnsubscribed(const Collection &collection, const QByteArray &resource)
{
    if (auto cleaner = mAkonadi.cacheCleaner()) {
        cleaner->collectionRemoved(collection.id());
    }
    mAkonadi.intervalChecker().collectionRemoved(collection.id());
    mAkonadi.collectionStatistics().invalidateCollection(collection);
    collectionNotification(Protocol::CollectionChangeNotification::Unsubscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>());
}

void NotificationCollector::tagAdded(const Tag &tag)
{
    tagNotification(Protocol::TagChangeNotification::Add, tag);
}

void NotificationCollector::tagChanged(const Tag &tag)
{
    tagNotification(Protocol::TagChangeNotification::Modify, tag);
}

void NotificationCollector::tagRemoved(const Tag &tag, const QByteArray &resource, const QString &remoteId)
{
    tagNotification(Protocol::TagChangeNotification::Remove, tag, resource, remoteId);
}

void NotificationCollector::relationAdded(const Relation &relation)
{
    relationNotification(Protocol::RelationChangeNotification::Add, relation);
}

void NotificationCollector::relationRemoved(const Relation &relation)
{
    relationNotification(Protocol::RelationChangeNotification::Remove, relation);
}

void NotificationCollector::clear()
{
    mNotifications.clear();
}

void NotificationCollector::setConnection(Connection *connection)
{
    mConnection = connection;
}

void NotificationCollector::itemNotification(Protocol::ItemChangeNotification::Operation op,
                                             const PimItem &item,
                                             const Collection &collection,
                                             const Collection &collectionDest,
                                             const QByteArray &resource,
                                             const QSet<QByteArray> &parts)
{
    PimItem::List items;
    items << item;
    itemNotification(op, items, collection, collectionDest, resource, parts);
}

void NotificationCollector::itemNotification(Protocol::ItemChangeNotification::Operation op,
                                             const PimItem::List &items,
                                             const Collection &collection,
                                             const Collection &collectionDest,
                                             const QByteArray &resource,
                                             const QSet<QByteArray> &parts,
                                             const QSet<QByteArray> &addedFlags,
                                             const QSet<QByteArray> &removedFlags,
                                             const QSet<qint64> &addedTags,
                                             const QSet<qint64> &removedTags,
                                             const Relation::List &addedRelations,
                                             const Relation::List &removedRelations)
{
    QMap<Entity::Id, QList<PimItem>> vCollections;

    if ((op == Protocol::ItemChangeNotification::Modify) || (op == Protocol::ItemChangeNotification::ModifyFlags)
        || (op == Protocol::ItemChangeNotification::ModifyTags) || (op == Protocol::ItemChangeNotification::ModifyRelations)) {
        vCollections = DataStore::self()->virtualCollections(items);
    }

    auto msg = Protocol::ItemChangeNotificationPtr::create();
    if (mConnection) {
        msg->setSessionId(mConnection->sessionId());
    }
    msg->setOperation(op);

    msg->setItemParts(parts);
    msg->setAddedFlags(addedFlags);
    msg->setRemovedFlags(removedFlags);
    msg->setAddedTags(addedTags);
    msg->setRemovedTags(removedTags);
    if (!addedRelations.isEmpty()) {
        QSet<Protocol::ItemChangeNotification::Relation> rels;
        Q_FOREACH (const Relation &rel, addedRelations) {
            rels.insert(Protocol::ItemChangeNotification::Relation(rel.leftId(), rel.rightId(), rel.relationType().name()));
        }
        msg->setAddedRelations(rels);
    }
    if (!removedRelations.isEmpty()) {
        QSet<Protocol::ItemChangeNotification::Relation> rels;
        Q_FOREACH (const Relation &rel, removedRelations) {
            rels.insert(Protocol::ItemChangeNotification::Relation(rel.leftId(), rel.rightId(), rel.relationType().name()));
        }
        msg->setRemovedRelations(rels);
    }

    if (collectionDest.isValid()) {
        QByteArray destResourceName;
        destResourceName = collectionDest.resource().name().toLatin1();
        msg->setDestinationResource(destResourceName);
    }

    msg->setParentDestCollection(collectionDest.id());

    QVector<Protocol::FetchItemsResponse> ntfItems;
    Q_FOREACH (const PimItem &item, items) {
        Protocol::FetchItemsResponse i;
        i.setId(item.id());
        i.setRemoteId(item.remoteId());
        i.setRemoteRevision(item.remoteRevision());
        i.setMimeType(item.mimeType().name());
        ntfItems.push_back(std::move(i));
    }

    /* Notify all virtual collections the items are linked to. */
    QHash<qint64, Protocol::FetchItemsResponse> virtItems;
    for (const auto &ntfItem : ntfItems) {
        virtItems.insert(ntfItem.id(), ntfItem);
    }
    for (auto iter = vCollections.cbegin(), end = vCollections.constEnd(); iter != end; ++iter) {
        auto copy = Protocol::ItemChangeNotificationPtr::create(*msg);
        QVector<Protocol::FetchItemsResponse> items;
        items.reserve(iter->size());
        for (const auto &item : std::as_const(*iter)) {
            items.append(virtItems.value(item.id()));
        }
        copy->setItems(items);
        copy->setParentCollection(iter.key());
        copy->setResource(resource);

        mAkonadi.collectionStatistics().invalidateCollection(Collection::retrieveById(iter.key()));
        dispatchNotification(copy);
    }

    msg->setItems(ntfItems);

    Collection col;
    if (!collection.isValid()) {
        msg->setParentCollection(items.first().collection().id());
        col = items.first().collection();
    } else {
        msg->setParentCollection(collection.id());
        col = collection;
    }

    QByteArray res = resource;
    if (res.isEmpty()) {
        if (col.resourceId() <= 0) {
            col = Collection::retrieveById(col.id());
        }
        res = col.resource().name().toLatin1();
    }
    msg->setResource(res);

    // Add and ModifyFlags are handled incrementally
    // (see itemAdded() and itemsFlagsChanged())
    if (msg->operation() != Protocol::ItemChangeNotification::Add && msg->operation() != Protocol::ItemChangeNotification::ModifyFlags) {
        mAkonadi.collectionStatistics().invalidateCollection(col);
    }
    dispatchNotification(msg);
}

void NotificationCollector::collectionNotification(Protocol::CollectionChangeNotification::Operation op,
                                                   const Collection &collection,
                                                   Collection::Id source,
                                                   Collection::Id destination,
                                                   const QByteArray &resource,
                                                   const QSet<QByteArray> &changes,
                                                   const QByteArray &destResource)
{
    auto msg = Protocol::CollectionChangeNotificationPtr::create();
    msg->setOperation(op);
    if (mConnection) {
        msg->setSessionId(mConnection->sessionId());
    }
    msg->setParentCollection(source);
    msg->setParentDestCollection(destination);
    msg->setDestinationResource(destResource);
    msg->setChangedParts(changes);

    auto msgCollection = HandlerHelper::fetchCollectionsResponse(mAkonadi, collection);
    if (auto mgr = mAkonadi.notificationManager()) {
        auto fetchScope = mgr->collectionFetchScope();
        // Make sure we have all the data
        if (!fetchScope->fetchIdOnly() && msgCollection.name().isEmpty()) {
            const auto col = Collection::retrieveById(msgCollection.id());
            const auto mts = col.mimeTypes();
            QStringList mimeTypes;
            mimeTypes.reserve(mts.size());
            for (const auto &mt : mts) {
                mimeTypes.push_back(mt.name());
            }
            msgCollection = HandlerHelper::fetchCollectionsResponse(mAkonadi, col, {}, false, 0, {}, {}, mimeTypes);
        }
        // Get up-to-date statistics
        if (fetchScope->fetchStatistics()) {
            Collection col;
            col.setId(msgCollection.id());
            const auto stats = mAkonadi.collectionStatistics().statistics(col);
            msgCollection.setStatistics(Protocol::FetchCollectionStatsResponse(stats.count, stats.count - stats.read, stats.size));
        }
        // Get attributes
        const auto requestedAttrs = fetchScope->attributes();
        auto msgColAttrs = msgCollection.attributes();
        // TODO: This assumes that we have either none or all attributes in msgCollection
        if (msgColAttrs.isEmpty() && !requestedAttrs.isEmpty()) {
            SelectQueryBuilder<CollectionAttribute> qb;
            qb.addColumn(CollectionAttribute::typeFullColumnName());
            qb.addColumn(CollectionAttribute::valueFullColumnName());
            qb.addValueCondition(CollectionAttribute::collectionIdFullColumnName(), Query::Equals, msgCollection.id());
            Query::Condition cond(Query::Or);
            for (const auto &attr : requestedAttrs) {
                cond.addValueCondition(CollectionAttribute::typeFullColumnName(), Query::Equals, attr);
            }
            qb.addCondition(cond);
            if (!qb.exec()) {
                qCWarning(AKONADISERVER_LOG) << "NotificationCollector failed to query attributes for Collection" << collection.name() << "(ID"
                                             << collection.id() << ")";
            }
            const auto attrs = qb.result();
            for (const auto &attr : attrs) {
                msgColAttrs.insert(attr.type(), attr.value());
            }
            msgCollection.setAttributes(msgColAttrs);
        }
    }
    msg->setCollection(std::move(msgCollection));

    if (!collection.enabled()) {
        msg->addMetadata("DISABLED");
    }

    QByteArray res = resource;
    if (res.isEmpty()) {
        res = collection.resource().name().toLatin1();
    }
    msg->setResource(res);

    dispatchNotification(msg);
}

void NotificationCollector::tagNotification(Protocol::TagChangeNotification::Operation op, const Tag &tag, const QByteArray &resource, const QString &remoteId)
{
    auto msg = Protocol::TagChangeNotificationPtr::create();
    msg->setOperation(op);
    if (mConnection) {
        msg->setSessionId(mConnection->sessionId());
    }
    msg->setResource(resource);
    Protocol::FetchTagsResponse msgTag;
    msgTag.setId(tag.id());
    msgTag.setRemoteId(remoteId.toUtf8());
    msgTag.setParentId(tag.parentId());
    if (auto mgr = mAkonadi.notificationManager()) {
        auto fetchScope = mgr->tagFetchScope();
        if (!fetchScope->fetchIdOnly() && msgTag.gid().isEmpty()) {
            msgTag = HandlerHelper::fetchTagsResponse(Tag::retrieveById(msgTag.id()), fetchScope->toFetchScope(), mConnection);
        }

        const auto requestedAttrs = fetchScope->attributes();
        auto msgTagAttrs = msgTag.attributes();
        if (msgTagAttrs.isEmpty() && !requestedAttrs.isEmpty()) {
            SelectQueryBuilder<TagAttribute> qb;
            qb.addColumn(TagAttribute::typeFullColumnName());
            qb.addColumn(TagAttribute::valueFullColumnName());
            qb.addValueCondition(TagAttribute::tagIdFullColumnName(), Query::Equals, msgTag.id());
            Query::Condition cond(Query::Or);
            for (const auto &attr : requestedAttrs) {
                cond.addValueCondition(TagAttribute::typeFullColumnName(), Query::Equals, attr);
            }
            qb.addCondition(cond);
            if (!qb.exec()) {
                qCWarning(AKONADISERVER_LOG) << "NotificationCollection failed to query attributes for Tag" << tag.id();
            }
            const auto attrs = qb.result();
            for (const auto &attr : attrs) {
                msgTagAttrs.insert(attr.type(), attr.value());
            }
            msgTag.setAttributes(msgTagAttrs);
        }
    }
    msg->setTag(std::move(msgTag));

    dispatchNotification(msg);
}

void NotificationCollector::relationNotification(Protocol::RelationChangeNotification::Operation op, const Relation &relation)
{
    auto msg = Protocol::RelationChangeNotificationPtr::create();
    msg->setOperation(op);
    if (mConnection) {
        msg->setSessionId(mConnection->sessionId());
    }
    msg->setRelation(HandlerHelper::fetchRelationsResponse(relation));

    dispatchNotification(msg);
}

void NotificationCollector::completeNotification(const Protocol::ChangeNotificationPtr &changeMsg)
{
    if (changeMsg->type() == Protocol::Command::ItemChangeNotification) {
        const auto msg = changeMsg.staticCast<Protocol::ItemChangeNotification>();
        auto const mgr = mAkonadi.notificationManager();
        if (mgr && msg->operation() != Protocol::ItemChangeNotification::Remove) {
            if (mDb->inTransaction()) {
                qCWarning(AKONADISERVER_LOG) << "NotificationCollector requested FetchHelper from within a transaction."
                                             << "Aborting since this would deadlock!";
                return;
            }
            auto fetchScope = mgr->itemFetchScope();
            // NOTE: Checking and retrieving missing elements for each Item manually
            // here would require a complex code (and I'm too lazy), so instead we simply
            // feed the Items to FetchHelper and retrieve them all with the setup from
            // the aggregated fetch scope. The worst case is that we re-fetch everything
            // we already have, but that's stil better than the pre-ntf-payload situation
            QVector<qint64> ids;
            const auto items = msg->items();
            ids.reserve(items.size());
            bool allHaveRID = true;
            for (const auto &item : items) {
                ids.push_back(item.id());
                allHaveRID &= !item.remoteId().isEmpty();
            }

            // FetchHelper may trigger ItemRetriever, which needs RemoteID. If we
            // don't have one (maybe because the Resource has not stored it yet,
            // we emit a notification without it and leave it up to the Monitor
            // to retrieve the Item on demand - we should have a RID stored in
            // Akonadi by then.
            if (mConnection && (allHaveRID || msg->operation() != Protocol::ItemChangeNotification::Add)) {
                // Prevent transactions inside FetchHelper to recursively call our slot
                QScopedValueRollback<bool> ignoreTransactions(mIgnoreTransactions);
                mIgnoreTransactions = true;
                CommandContext context;
                auto itemFetchScope = fetchScope->toFetchScope();
                auto tagFetchScope = mgr->tagFetchScope()->toFetchScope();
                itemFetchScope.setFetch(Protocol::ItemFetchScope::CacheOnly);
                ItemFetchHelper helper(mConnection, context, Scope(ids), itemFetchScope, tagFetchScope, mAkonadi);
                // The Item was just changed, which means the atime was
                // updated, no need to do it again a couple milliseconds later.
                helper.disableATimeUpdates();
                QVector<Protocol::FetchItemsResponse> fetchedItems;
                auto callback = [&fetchedItems](Protocol::FetchItemsResponse &&cmd) {
                    fetchedItems.push_back(std::move(cmd));
                };
                if (helper.fetchItems(std::move(callback))) {
                    msg->setItems(fetchedItems);
                } else {
                    qCWarning(AKONADISERVER_LOG) << "NotificationCollector railed to retrieve Items for notification!";
                }
            } else {
                QVector<Protocol::FetchItemsResponse> fetchedItems;
                for (const auto &item : items) {
                    Protocol::FetchItemsResponse resp;
                    resp.setId(item.id());
                    resp.setRevision(item.revision());
                    resp.setMimeType(item.mimeType());
                    resp.setParentId(item.parentId());
                    resp.setGid(item.gid());
                    resp.setSize(item.size());
                    resp.setMTime(item.mTime());
                    resp.setFlags(item.flags());
                    fetchedItems.push_back(std::move(resp));
                }
                msg->setItems(fetchedItems);
                msg->setMustRetrieve(true);
            }
        }
    }
}

void NotificationCollector::dispatchNotification(const Protocol::ChangeNotificationPtr &msg)
{
    if (!mDb || mDb->inTransaction()) {
        if (msg->type() == Protocol::Command::CollectionChangeNotification) {
            Protocol::CollectionChangeNotification::appendAndCompress(mNotifications, msg);
        } else {
            mNotifications.append(msg);
        }
    } else {
        completeNotification(msg);
        notify({msg});
    }
}

bool NotificationCollector::dispatchNotifications()
{
    if (!mNotifications.isEmpty()) {
        for (auto &ntf : mNotifications) {
            completeNotification(ntf);
        }
        notify(std::move(mNotifications));
        clear();
        return true;
    }

    return false;
}

void NotificationCollector::notify(Protocol::ChangeNotificationList &&msgs)
{
    if (auto mgr = mAkonadi.notificationManager()) {
        QMetaObject::invokeMethod(mgr, "slotNotify", Qt::QueuedConnection, Q_ARG(Akonadi::Protocol::ChangeNotificationList, msgs));
    }
}
