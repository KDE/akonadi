/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "notificationcollector.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/collectionstatistics.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "intervalcheck.h"
#include "search/searchmanager.h"
#include "akonadi.h"
#include "handler/search.h"
#include "notificationmanager.h"
#include "aggregatedfetchscope.h"
#include "selectquerybuilder.h"
#include "handler/fetchhelper.h"

#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationCollector::NotificationCollector(QObject *parent)
    : QObject(parent)
    , mDb(nullptr)
{
}

NotificationCollector::NotificationCollector(DataStore *db)
    : QObject(db)
    , mDb(db)
{
    connect(db, &DataStore::transactionCommitted, this, &NotificationCollector::transactionCommitted);
    connect(db, &DataStore::transactionRolledBack, this, &NotificationCollector::transactionRolledBack);
}

NotificationCollector::~NotificationCollector()
{
}

void NotificationCollector::itemAdded(const PimItem &item,
                                      bool seen,
                                      const Collection &collection,
                                      const QByteArray &resource)
{
    SearchManager::instance()->scheduleSearchUpdate();
    CollectionStatistics::self()->itemAdded(collection, item.size(), seen);
    itemNotification(Protocol::ItemChangeNotification::Add, item, collection, Collection(), resource);
}

void NotificationCollector::itemChanged(const PimItem &item,
                                        const QSet<QByteArray> &changedParts,
                                        const Collection &collection,
                                        const QByteArray &resource)
{
    SearchManager::instance()->scheduleSearchUpdate();
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

    CollectionStatistics::self()->itemsSeenChanged(collection, seenCount);
    itemNotification(Protocol::ItemChangeNotification::ModifyFlags, items, collection, Collection(), resource, QSet<QByteArray>(), addedFlags, removedFlags);
}

void NotificationCollector::itemsTagsChanged(const PimItem::List &items,
        const QSet<qint64> &addedTags,
        const QSet<qint64> &removedTags,
        const Collection &collection,
        const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyTags, items, collection, Collection(), resource, QSet<QByteArray>(), QSet<QByteArray>(), QSet<QByteArray>(), addedTags, removedTags);
}

void NotificationCollector::itemsRelationsChanged(const PimItem::List &items,
        const Relation::List &addedRelations,
        const Relation::List &removedRelations,
        const Collection &collection,
        const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyRelations, items, collection, Collection(), resource, QSet<QByteArray>(), QSet<QByteArray>(), QSet<QByteArray>(), QSet<qint64>(), QSet<qint64>(), addedRelations, removedRelations);
}

void NotificationCollector::itemsMoved(const PimItem::List &items,
                                       const Collection &collectionSrc,
                                       const Collection &collectionDest,
                                       const QByteArray &sourceResource)
{
    SearchManager::instance()->scheduleSearchUpdate();
    itemNotification(Protocol::ItemChangeNotification::Move, items, collectionSrc, collectionDest, sourceResource);
}

void NotificationCollector::itemsRemoved(const PimItem::List &items,
        const Collection &collection,
        const QByteArray &resource)
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

void NotificationCollector::collectionAdded(const Collection &collection,
        const QByteArray &resource)
{
    if (auto cleaner = AkonadiServer::instance()->cacheCleaner()) {
        cleaner->collectionAdded(collection.id());
    }
    if (auto checker = AkonadiServer::instance()->intervalChecker()) {
        checker->collectionAdded(collection.id());
    }
    collectionNotification(Protocol::CollectionChangeNotification::Add, collection, collection.parentId(), -1, resource);
}

void NotificationCollector::collectionChanged(const Collection &collection,
        const QList<QByteArray> &changes,
        const QByteArray &resource)
{
    if (auto cleaner = AkonadiServer::instance()->cacheCleaner()) {
        cleaner->collectionChanged(collection.id());
    }
    if (auto checker = AkonadiServer::instance()->intervalChecker()) {
        checker->collectionChanged(collection.id());
    }
    if (changes.contains(AKONADI_PARAM_ENABLED) || changes.contains(AKONADI_PARAM_REFERENCED)) {
        CollectionStatistics::self()->invalidateCollection(collection);
    }
    collectionNotification(Protocol::CollectionChangeNotification::Modify, collection, collection.parentId(), -1, resource, changes.toSet());
}

void NotificationCollector::collectionMoved(const Collection &collection,
        const Collection &source,
        const QByteArray &resource,
        const QByteArray &destResource)
{
    if (auto cleaner = AkonadiServer::instance()->cacheCleaner()) {
        cleaner->collectionChanged(collection.id());
    }
    if (auto checker = AkonadiServer::instance()->intervalChecker()) {
        checker->collectionChanged(collection.id());
    }
    collectionNotification(Protocol::CollectionChangeNotification::Move, collection, source.id(), collection.parentId(), resource, QSet<QByteArray>(), destResource);
}

void NotificationCollector::collectionRemoved(const Collection &collection,
        const QByteArray &resource)
{
    if (auto cleaner = AkonadiServer::instance()->cacheCleaner()) {
        cleaner->collectionRemoved(collection.id());
    }
    if (auto checker = AkonadiServer::instance()->intervalChecker()) {
        checker->collectionRemoved(collection.id());
    }
    CollectionStatistics::self()->invalidateCollection(collection);
    collectionNotification(Protocol::CollectionChangeNotification::Remove, collection, collection.parentId(), -1, resource);
}

void NotificationCollector::collectionSubscribed(const Collection &collection,
        const QByteArray &resource)
{
    if (auto cleaner = AkonadiServer::instance()->cacheCleaner()) {
        cleaner->collectionAdded(collection.id());
    }
    if (auto checker = AkonadiServer::instance()->intervalChecker()) {
        checker->collectionAdded(collection.id());
    }
    collectionNotification(Protocol::CollectionChangeNotification::Subscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>());
}

void NotificationCollector::collectionUnsubscribed(const Collection &collection,
        const QByteArray &resource)
{
    if (auto cleaner = AkonadiServer::instance()->cacheCleaner()) {
        cleaner->collectionRemoved(collection.id());
    }
    if (auto checker = AkonadiServer::instance()->intervalChecker()) {
        checker->collectionRemoved(collection.id());
    }
    CollectionStatistics::self()->invalidateCollection(collection);
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

void NotificationCollector::transactionCommitted()
{
    dispatchNotifications();
}

void NotificationCollector::transactionRolledBack()
{
    clear();
}

void NotificationCollector::clear()
{
    mNotifications.clear();
}

void NotificationCollector::setSessionId(const QByteArray &sessionId)
{
    mSessionId = sessionId;
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
    QMap<Entity::Id, QList<PimItem> > vCollections;

    if ((op == Protocol::ItemChangeNotification::Modify) ||
            (op == Protocol::ItemChangeNotification::ModifyFlags) ||
            (op == Protocol::ItemChangeNotification::ModifyTags) ||
            (op == Protocol::ItemChangeNotification::ModifyRelations)) {
        vCollections = DataStore::self()->virtualCollections(items);
    }

    auto msg = Protocol::ItemChangeNotificationPtr::create();
    msg->setSessionId(mSessionId);
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

    QVector<Protocol::FetchItemsResponsePtr> ntfItems;
    Q_FOREACH (const PimItem &item, items) {
        auto i = Protocol::FetchItemsResponsePtr::create();
        i->setId(item.id());
        i->setRemoteId(item.remoteId());
        i->setRemoteRevision(item.remoteRevision());
        i->setMimeType(item.mimeType().name());
        ntfItems.push_back(i);
    }

    /* Notify all virtual collections the items are linked to. */
    QHash<qint64, Protocol::FetchItemsResponsePtr> virtItems;
    for (const auto &ntfItem : ntfItems) {
        virtItems.insert(ntfItem->id(), ntfItem);
    }
    auto iter = vCollections.constBegin(), endIter = vCollections.constEnd();
    for (; iter != endIter; ++iter) {
        auto copy = Protocol::ItemChangeNotificationPtr::create(*msg);
        QVector<Protocol::FetchItemsResponsePtr> items;
        items.reserve(iter->size());
        for (const auto &item : qAsConst(*iter)) {
            items.append(virtItems.value(item.id()));
        }
        copy->setItems(items);
        copy->setParentCollection(iter.key());
        copy->setResource(resource);

        CollectionStatistics::self()->invalidateCollection(Collection::retrieveById(iter.key()));
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
    if (msg->operation() != Protocol::ItemChangeNotification::Add
            && msg->operation() != Protocol::ItemChangeNotification::ModifyFlags) {
        CollectionStatistics::self()->invalidateCollection(col);
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
    msg->setSessionId(mSessionId);
    msg->setParentCollection(source);
    msg->setParentDestCollection(destination);
    msg->setDestinationResource(destResource);
    msg->setChangedParts(changes);

    auto msgCollection = HandlerHelper::fetchCollectionsResponse(collection);
    if (auto mgr = AkonadiServer::instance()->notificationManager()) {
        auto fetchScope = mgr->collectionFetchScope();
        // Make sure we have all the data
        if (!fetchScope->fetchIdOnly() && msgCollection->name().isEmpty()) {
            const auto col = Collection::retrieveById(msgCollection->id());
            const auto mts = col.mimeTypes();
            QStringList mimeTypes;
            mimeTypes.reserve(mts.size());
            for (const auto &mt : mts) {
                mimeTypes.push_back(mt.name());
            }
            msgCollection = HandlerHelper::fetchCollectionsResponse(col, {}, false, 0, {}, {}, false, mimeTypes);
        }
        // Get up-to-date statistics
        if (fetchScope->fetchStatistics()) {
            Collection col;
            col.setId(msgCollection->id());
            const auto stats = CollectionStatistics::self()->statistics(col);
            msgCollection->setStatistics(Protocol::FetchCollectionStatsResponse(stats.count, stats.count - stats.read, stats.size));
        }
        // Get attributes
        const auto requestedAttrs = fetchScope->attributes();
        auto msgColAttrs = msgCollection->attributes();
        // TODO: This assumes that we have either none or all attributes in msgCollection
        if (msgColAttrs.isEmpty() && !requestedAttrs.isEmpty()) {
            SelectQueryBuilder<CollectionAttribute> qb;
            qb.addColumn(CollectionAttribute::typeFullColumnName());
            qb.addColumn(CollectionAttribute::valueFullColumnName());
            qb.addValueCondition(CollectionAttribute::collectionIdFullColumnName(),
                                    Query::Equals, msgCollection->id());
            Query::Condition cond(Query::Or);
            for (const auto &attr : requestedAttrs) {
                cond.addValueCondition(CollectionAttribute::typeFullColumnName(), Query::Equals, attr);
            }
            qb.addCondition(cond);
            if (!qb.exec()) {
                qCWarning(AKONADISERVER_LOG) << "Failed to obtain collection attributes!";
            }
            const auto attrs = qb.result();
            for (const auto &attr : attrs)  {
                msgColAttrs.insert(attr.type(), attr.value());
            }
            msgCollection->setAttributes(msgColAttrs);
        }
    }
    msg->setCollection(msgCollection);

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

void NotificationCollector::tagNotification(Protocol::TagChangeNotification::Operation op,
        const Tag &tag,
        const QByteArray &resource,
        const QString &remoteId)
{
    auto msg = Protocol::TagChangeNotificationPtr::create();
    msg->setOperation(op);
    msg->setSessionId(mSessionId);
    msg->setResource(resource);
    auto msgTag = HandlerHelper::fetchTagsResponse(tag, false);
    msgTag->setRemoteId(remoteId.toUtf8());
    if (auto mgr = AkonadiServer::instance()->notificationManager()) {
        auto fetchScope = mgr->tagFetchScope();
        if (!fetchScope->fetchIdOnly() && msgTag->gid().isEmpty()) {
            msgTag = HandlerHelper::fetchTagsResponse(Tag::retrieveById(msgTag->id()), false);
        }

        const auto requestedAttrs = fetchScope->attributes();
        auto msgTagAttrs = msgTag->attributes();
        if (msgTagAttrs.isEmpty() && !requestedAttrs.isEmpty()) {
            SelectQueryBuilder<TagAttribute> qb;
            qb.addColumn(TagAttribute::typeFullColumnName());
            qb.addColumn(TagAttribute::valueFullColumnName());
            qb.addValueCondition(TagAttribute::tagIdFullColumnName(), Query::Equals, msgTag->id());
            Query::Condition cond(Query::Or);
            for (const auto &attr : requestedAttrs) {
                cond.addValueCondition(TagAttribute::typeFullColumnName(), Query::Equals, attr);
            }
            qb.addCondition(cond);
            if (!qb.exec()) {
                qCWarning(AKONADISERVER_LOG) << "Failed to obtain tag attributes!";
            }
            const auto attrs = qb.result();
            for (const auto &attr : attrs) {
                msgTagAttrs.insert(attr.type(), attr.value());
            }
            msgTag->setAttributes(msgTagAttrs);
        }
    }
    msg->setTag(msgTag);

    dispatchNotification(msg);
}

void NotificationCollector::relationNotification(Protocol::RelationChangeNotification::Operation op,
        const Relation &relation)
{
    auto msg = Protocol::RelationChangeNotificationPtr::create();
    msg->setOperation(op);
    msg->setSessionId(mSessionId);
    msg->setRelation(HandlerHelper::fetchRelationsResponse(relation));

    dispatchNotification(msg);
}

void NotificationCollector::completeNotification(const Protocol::ChangeNotificationPtr &changeMsg)
{
    if (changeMsg->type() == Protocol::Command::ItemChangeNotification) {
        const auto msg = changeMsg.staticCast<Protocol::ItemChangeNotification>();
        const auto mgr = AkonadiServer::instance()->notificationManager();
        if (mgr && msg->operation() != Protocol::ItemChangeNotification::Remove) {
            if (mDb->inTransaction()) {
                qCWarning(AKONADISERVER_LOG) << "FetchHelper requested from within a transaction, aborting, since this would deadlock!";
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
            for (const auto &item : items) {
                ids.push_back(item->id());
            }
            // Prevent transactions inside FetchHelper to recursively call our slot
            disconnect(mDb, &DataStore::transactionCommitted, this, &NotificationCollector::transactionCommitted);
            disconnect(mDb, &DataStore::transactionRolledBack, this, &NotificationCollector::transactionRolledBack);
            CommandContext context;
            auto scope = fetchScope->toFetchScope();
            FetchHelper helper(Connection::self(), &context, Scope(ids), scope);
            QVector<Protocol::FetchItemsResponsePtr> fetchedItems;
            std::function<void(Protocol::CommandPtr)> callback = [&fetchedItems](Protocol::CommandPtr cmd) {
                fetchedItems.push_back(cmd.staticCast<Protocol::FetchItemsResponse>());
            };
            if (helper.fetchItems(std::move(callback))) {
                msg->setItems(fetchedItems);
            } else {
                qCWarning(AKONADISERVER_LOG) << "Failed to retrieve Items for notification!";
            }
            connect(mDb, &DataStore::transactionCommitted, this, &NotificationCollector::transactionCommitted);
            connect(mDb, &DataStore::transactionRolledBack, this, &NotificationCollector::transactionRolledBack);
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
        Q_EMIT notify({ msg });
    }
}

void NotificationCollector::dispatchNotifications()
{
    if (!mNotifications.isEmpty()) {
        for (auto &ntf : mNotifications) {
            completeNotification(ntf);
        }
        Q_EMIT notify(mNotifications);
        clear();
    }
}
