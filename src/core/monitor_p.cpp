/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// @cond PRIVATE

#include "monitor_p.h"

#include "collectionfetchjob.h"
#include "collectionstatistics.h"
#include "itemfetchjob.h"
#include "notificationmanagerinterface.h"
#include "session.h"
#include "changemediator_p.h"
#include "vectorhelper.h"
#include "akonadicore_debug.h"
#include "notificationsubscriber.h"
#include "changenotification.h"
#include "protocolhelper_p.h"

#include <shared/akranges.h>

#include <utility>

using namespace Akonadi;
using namespace AkRanges;

class operation;

static const int PipelineSize = 5;

MonitorPrivate::MonitorPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_, Monitor *parent)
    : q_ptr(parent)
    , dependenciesFactory(dependenciesFactory_ ? dependenciesFactory_ : new ChangeNotificationDependenciesFactory)
    , ntfConnection(nullptr)
    , monitorAll(false)
    , exclusive(false)
    , mFetchChangedOnly(false)
    , session(Session::defaultSession())
    , collectionCache(nullptr)
    , itemCache(nullptr)
    , tagCache(nullptr)
    , mCommandBuffer(parent, "handleCommands")
    , pendingModificationChanges(Protocol::ModifySubscriptionCommand::None)
    , pendingModificationTimer(nullptr)
    , monitorReady(false)
    , fetchCollection(false)
    , fetchCollectionStatistics(false)
    , collectionMoveTranslationEnabled(true)
    , useRefCounting(false)
{
}

MonitorPrivate::~MonitorPrivate()
{
    disconnectFromNotificationManager();
    delete dependenciesFactory;
    delete collectionCache;
    delete itemCache;
    delete tagCache;
}

void MonitorPrivate::init()
{
    // needs to be at least 3x pipeline size for the collection move case
    collectionCache = dependenciesFactory->createCollectionCache(3 * PipelineSize, session);
    // needs to be at least 1x pipeline size
    itemCache = dependenciesFactory->createItemListCache(PipelineSize, session);
    // 20 tags looks like a reasonable amount to keep around
    tagCache = dependenciesFactory->createTagListCache(20, session);

    QObject::connect(collectionCache, &CollectionCache::dataAvailable, q_ptr, [this]() { dataAvailable(); });
    QObject::connect(itemCache, &ItemCache::dataAvailable, q_ptr, [this]() { dataAvailable(); });
    QObject::connect(tagCache, &TagCache::dataAvailable, q_ptr, [this]() { dataAvailable(); });
    QObject::connect(ServerManager::self(), &ServerManager::stateChanged, q_ptr, [this](auto state) { serverStateChanged(state); });

    statisticsCompressionTimer.setSingleShot(true);
    statisticsCompressionTimer.setInterval(500);
    QObject::connect(&statisticsCompressionTimer, &QTimer::timeout, q_ptr, [this]() { slotFlushRecentlyChangedCollections(); });
}

bool MonitorPrivate::connectToNotificationManager()
{
    if (ntfConnection) {
        ntfConnection->deleteLater();
        ntfConnection = nullptr;
    }

    if (!session) {
        return false;
    }

    ntfConnection = dependenciesFactory->createNotificationConnection(session, &mCommandBuffer);
    if (!ntfConnection) {
        return false;
    }

    slotUpdateSubscription();

    ntfConnection->reconnect();

    return true;
}

void MonitorPrivate::disconnectFromNotificationManager()
{
    if (ntfConnection) {
        ntfConnection->disconnect(q_ptr);
        dependenciesFactory->destroyNotificationConnection(session, ntfConnection.data());
    }
}

void MonitorPrivate::serverStateChanged(ServerManager::State state)
{
    if (state == ServerManager::Running) {
        connectToNotificationManager();
    }
}

void MonitorPrivate::invalidateCollectionCache(qint64 id)
{
    collectionCache->update(id, mCollectionFetchScope);
}

void MonitorPrivate::invalidateItemCache(qint64 id)
{
    itemCache->update({ id }, mItemFetchScope);
    // Also invalidate content of all any pending notification for given item
    for (auto it = pendingNotifications.begin(), end = pendingNotifications.end(); it != end; ++it) {
        if ((*it)->type() == Protocol::Command::ItemChangeNotification) {
            auto &ntf = Protocol::cmdCast<Protocol::ItemChangeNotification>(*it);
            const auto items = ntf.items();
            if (std::any_of(items.cbegin(), items.cend(), [id](const Protocol::FetchItemsResponse &r) { return r.id() == id; })) {
                ntf.setMustRetrieve(true);
            }
        }
    }
}

void MonitorPrivate::invalidateTagCache(qint64 id)
{
    tagCache->update({ id }, mTagFetchScope);
}

int MonitorPrivate::pipelineSize() const
{
    return PipelineSize;
}

void MonitorPrivate::scheduleSubscriptionUpdate()
{
    if (pendingModificationTimer || !monitorReady) {
        return;
    }

    pendingModificationTimer = new QTimer(q_ptr);
    pendingModificationTimer->setSingleShot(true);
    pendingModificationTimer->setInterval(0);
    pendingModificationTimer->start();
    q_ptr->connect(pendingModificationTimer, &QTimer::timeout, q_ptr, [this]() { slotUpdateSubscription(); });
}

void MonitorPrivate::slotUpdateSubscription()
{
    if (pendingModificationTimer) {
        pendingModificationTimer->stop();
        std::exchange(pendingModificationTimer, nullptr)->deleteLater();
    }

    if (pendingModificationChanges & Protocol::ModifySubscriptionCommand::ItemFetchScope) {
        pendingModification.setItemFetchScope(ProtocolHelper::itemFetchScopeToProtocol(mItemFetchScope));
    }
    if (pendingModificationChanges & Protocol::ModifySubscriptionCommand::CollectionFetchScope) {
        pendingModification.setCollectionFetchScope(ProtocolHelper::collectionFetchScopeToProtocol(mCollectionFetchScope));
    }
    if (pendingModificationChanges & Protocol::ModifySubscriptionCommand::TagFetchScope) {
        pendingModification.setTagFetchScope(ProtocolHelper::tagFetchScopeToProtocol(mTagFetchScope));
    }
    pendingModificationChanges = Protocol::ModifySubscriptionCommand::None;

    if (ntfConnection) {
        ntfConnection->sendCommand(3, Protocol::ModifySubscriptionCommandPtr::create(pendingModification));
        pendingModification = Protocol::ModifySubscriptionCommand();
    }
}

bool MonitorPrivate::isLazilyIgnored(const Protocol::ChangeNotificationPtr &msg, bool allowModifyFlagsConversion) const
{
    if (msg->type() == Protocol::Command::CollectionChangeNotification) {
        // Lazy fetching can only affects items.
        return false;
    }

    if (msg->type() == Protocol::Command::TagChangeNotification) {
        const auto op = Protocol::cmdCast<Protocol::TagChangeNotification>(msg).operation();
        return ((op == Protocol::TagChangeNotification::Add && !hasListeners(&Monitor::tagAdded))
                || (op == Protocol::TagChangeNotification::Modify && !hasListeners(&Monitor::tagChanged))
                || (op == Protocol::TagChangeNotification::Remove && !hasListeners(&Monitor::tagRemoved)));
    }

    if (!fetchCollectionStatistics && msg->type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
        const auto op = itemNtf.operation();
        if ((op == Protocol::ItemChangeNotification::Add && !hasListeners(&Monitor::itemAdded))
                || (op == Protocol::ItemChangeNotification::Remove && !hasListeners(&Monitor::itemRemoved) && !hasListeners(&Monitor::itemsRemoved))
                || (op == Protocol::ItemChangeNotification::Modify && !hasListeners(&Monitor::itemChanged))
                || (op == Protocol::ItemChangeNotification::ModifyFlags && !hasListeners(&Monitor::itemsFlagsChanged)
                        // Newly delivered ModifyFlags notifications will be converted to
                        // itemChanged(item, "FLAGS") for legacy clients.
                        && (!allowModifyFlagsConversion || !hasListeners(&Monitor::itemChanged)))
                || (op == Protocol::ItemChangeNotification::ModifyTags && !hasListeners(&Monitor::itemsTagsChanged))
                || (op == Protocol::ItemChangeNotification::Move && !hasListeners(&Monitor::itemMoved) && !hasListeners(&Monitor::itemsMoved))
                || (op == Protocol::ItemChangeNotification::Link && !hasListeners(&Monitor::itemLinked) && !hasListeners(&Monitor::itemsLinked))
                || (op == Protocol::ItemChangeNotification::Unlink && !hasListeners(&Monitor::itemUnlinked) && !hasListeners(&Monitor::itemsUnlinked))) {
            return true;
        }

        if (!useRefCounting) {
            return false;
        }

        const Collection::Id parentCollectionId = itemNtf.parentCollection();

        if ((op == Protocol::ItemChangeNotification::Add)
                || (op == Protocol::ItemChangeNotification::Remove)
                || (op == Protocol::ItemChangeNotification::Modify)
                || (op == Protocol::ItemChangeNotification::ModifyFlags)
                || (op == Protocol::ItemChangeNotification::ModifyTags)
                || (op == Protocol::ItemChangeNotification::Link)
                || (op == Protocol::ItemChangeNotification::Unlink)) {
            if (isMonitored(parentCollectionId)) {
                return false;
            }
        }

        if (op == Protocol::ItemChangeNotification::Move) {
            // We can't ignore the move. It must be transformed later into a removal or insertion.
            return !isMonitored(parentCollectionId) && !isMonitored(itemNtf.parentDestCollection());
        }
        return true;
    }

    return false;
}

void MonitorPrivate::checkBatchSupport(const Protocol::ChangeNotificationPtr &msg, bool &needsSplit, bool &batchSupported) const
{
    if (msg->type() != Protocol::Command::ItemChangeNotification) {
        needsSplit = false;
        batchSupported = false;
        return;
    }

    const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
    const bool isBatch = (itemNtf.items().count() > 1);

    switch (itemNtf.operation()) {
    case Protocol::ItemChangeNotification::Add:
    case Protocol::ItemChangeNotification::Modify:
        needsSplit = isBatch;
        batchSupported = false;
        return;
        needsSplit = isBatch;
        batchSupported = false;
        return;
    case Protocol::ItemChangeNotification::ModifyFlags:
        batchSupported = hasListeners(&Monitor::itemsFlagsChanged);
        needsSplit = isBatch && !batchSupported && hasListeners(&Monitor::itemChanged);
        return;
    case Protocol::ItemChangeNotification::ModifyTags:
    case Protocol::ItemChangeNotification::ModifyRelations:
        // Tags and relations were added after batch notifications, so they are always supported
        batchSupported = true;
        needsSplit = false;
        return;
    case Protocol::ItemChangeNotification::Move:
        needsSplit = isBatch && hasListeners(&Monitor::itemMoved);
        batchSupported = hasListeners(&Monitor::itemsMoved);
        return;
    case Protocol::ItemChangeNotification::Remove:
        needsSplit = isBatch && hasListeners(&Monitor::itemRemoved);
        batchSupported = hasListeners(&Monitor::itemsRemoved);
        return;
    case Protocol::ItemChangeNotification::Link:
        needsSplit = isBatch && hasListeners(&Monitor::itemLinked);
        batchSupported = hasListeners(&Monitor::itemsLinked);
        return;
    case Protocol::ItemChangeNotification::Unlink:
        needsSplit = isBatch && hasListeners(&Monitor::itemUnlinked);
        batchSupported = hasListeners(&Monitor::itemsUnlinked);
        return;
    default:
        needsSplit = isBatch;
        batchSupported = false;
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << itemNtf.operation() << "in item change notification";
        return;
    }
}

Protocol::ChangeNotificationList MonitorPrivate::splitMessage(const Protocol::ItemChangeNotification &msg, bool legacy) const
{
    Protocol::ChangeNotificationList list;

    Protocol::ItemChangeNotification baseMsg;
    baseMsg.setSessionId(msg.sessionId());
    if (legacy && msg.operation() == Protocol::ItemChangeNotification::ModifyFlags) {
        baseMsg.setOperation(Protocol::ItemChangeNotification::Modify);
        baseMsg.setItemParts(QSet<QByteArray>() << "FLAGS");
    } else {
        baseMsg.setOperation(msg.operation());
        baseMsg.setItemParts(msg.itemParts());
    }
    baseMsg.setParentCollection(msg.parentCollection());
    baseMsg.setParentDestCollection(msg.parentDestCollection());
    baseMsg.setResource(msg.resource());
    baseMsg.setDestinationResource(msg.destinationResource());
    baseMsg.setAddedFlags(msg.addedFlags());
    baseMsg.setRemovedFlags(msg.removedFlags());
    baseMsg.setAddedTags(msg.addedTags());
    baseMsg.setRemovedTags(msg.removedTags());

    const auto &items = msg.items();
    list.reserve(items.count());
    for (const auto &item : items) {
        auto copy = Protocol::ItemChangeNotificationPtr::create(baseMsg);
        copy->setItems({Protocol::FetchItemsResponse(item)});
        list.push_back(std::move(copy));
    }

    return list;
}

bool MonitorPrivate::fetchCollections() const
{
    return fetchCollection;
}

bool MonitorPrivate::fetchItems() const
{
    return !mItemFetchScope.isEmpty();
}

bool MonitorPrivate::ensureDataAvailable(const Protocol::ChangeNotificationPtr &msg)
{
    if (msg->type() == Protocol::Command::TagChangeNotification) {
        const auto &tagMsg = Protocol::cmdCast<Protocol::TagChangeNotification>(msg);
        if (tagMsg.metadata().contains("FETCH_TAG")) {
            if (!tagCache->ensureCached({ tagMsg.tag().id() }, mTagFetchScope)) {
                return false;
            }
        }
        return true;
    }

    if (msg->type() == Protocol::Command::RelationChangeNotification) {
        return true;
    }

    if (msg->type() == Protocol::Command::SubscriptionChangeNotification) {
        return true;
    }

    if (msg->type() == Protocol::Command::DebugChangeNotification) {
        return true;
    }

    if (msg->type() == Protocol::Command::CollectionChangeNotification
            && Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg).operation() == Protocol::CollectionChangeNotification::Remove) {
        // For collection removals the collection is gone already, so we can't fetch it,
        // but we have to at least obtain the ancestor chain.
        const qint64 parentCollection = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg).parentCollection();
        return parentCollection <= -1 || collectionCache->ensureCached(parentCollection, mCollectionFetchScope);
    }

    bool allCached = true;
    if (fetchCollections()) {
        const qint64 parentCollection = (msg->type() == Protocol::Command::ItemChangeNotification) ?
                                        Protocol::cmdCast<Protocol::ItemChangeNotification>(msg).parentCollection() :
                                        (msg->type() == Protocol::Command::CollectionChangeNotification) ?
                                        Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg).parentCollection() :
                                        -1;
        if (parentCollection > -1 && !collectionCache->ensureCached(parentCollection, mCollectionFetchScope)) {
            allCached = false;
        }

        qint64 parentDestCollection = -1;

        if ((msg->type() == Protocol::Command::ItemChangeNotification)
                && (Protocol::cmdCast<Protocol::ItemChangeNotification>(msg).operation() == Protocol::ItemChangeNotification::Move)) {
            parentDestCollection = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg).parentDestCollection();
        } else if ((msg->type() == Protocol::Command::CollectionChangeNotification)
                   && (Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg).operation() == Protocol::CollectionChangeNotification::Move)) {
            parentDestCollection = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg).parentDestCollection();
        }
        if (parentDestCollection > -1 && !collectionCache->ensureCached(parentDestCollection, mCollectionFetchScope)) {
            allCached = false;
        }

    }

    if (msg->isRemove()) {
        return allCached;
    }

    if (msg->type() == Protocol::Command::ItemChangeNotification && fetchItems()) {
        const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
        if (mFetchChangedOnly && (itemNtf.operation() == Protocol::ItemChangeNotification::Modify
                || itemNtf.operation() == Protocol::ItemChangeNotification::ModifyFlags)) {

            const auto changedParts = itemNtf.itemParts();
            const auto requestedParts = mItemFetchScope.payloadParts();
            const auto requestedAttrs = mItemFetchScope.attributes();
            QSet<QByteArray> missingParts;
            QSet<QByteArray> missingAttributes;
            for (const QByteArray &part : changedParts)  {
                const auto partName = part.mid(4);
                if (part.startsWith("PLD:") &&    //krazy:exclude=strings since QByteArray
                        (!mItemFetchScope.fullPayload() || !requestedParts.contains(partName))) {
                    missingParts.insert(partName);
                } else if (part.startsWith("ATR:") &&    //krazy:exclude=strings since QByteArray
                        (!mItemFetchScope.allAttributes() || !requestedAttrs.contains(partName))) {
                    missingAttributes.insert(partName);
                }
            }

            if (!missingParts.isEmpty() || !missingAttributes.isEmpty()) {
                ItemFetchScope scope(mItemFetchScope);
                scope.fetchFullPayload(false);
                for (const auto &part : requestedParts) {
                    scope.fetchPayloadPart(part, false);
                }
                for (const auto &attr : requestedAttrs) {
                    scope.fetchAttribute(attr, false);
                }
                for (const auto &part : missingParts) {
                    scope.fetchPayloadPart(part, true);
                }
                for (const auto &attr : missingAttributes) {
                    scope.fetchAttribute(attr, true);
                }

                if (!itemCache->ensureCached(Protocol::ChangeNotification::itemsToUids(itemNtf.items()), scope)) {
                    return false;
                }
            }

            return allCached;
        }

        // Make sure all tags for ModifyTags operation are in cache too
        if (itemNtf.operation() == Protocol::ItemChangeNotification::ModifyTags) {
            if (!tagCache->ensureCached((itemNtf.addedTags() + itemNtf.removedTags()) | Actions::toQList, mTagFetchScope)) {
                return false;
            }
        }

        if (itemNtf.metadata().contains("FETCH_ITEM") || itemNtf.mustRetrieve()) {
            if (!itemCache->ensureCached(Protocol::ChangeNotification::itemsToUids(itemNtf.items()), mItemFetchScope)) {
                return false;
            }
        }

        return allCached;

    } else if (msg->type() == Protocol::Command::CollectionChangeNotification && fetchCollections()) {
        const auto &colMsg = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg);
        if (colMsg.metadata().contains("FETCH_COLLECTION")) {
            if (!collectionCache->ensureCached(colMsg.collection().id(), mCollectionFetchScope)) {
                return false;
            }
        }

        return allCached;
    }

    return allCached;
}

bool MonitorPrivate::emitNotification(const Protocol::ChangeNotificationPtr &msg)
{
    bool someoneWasListening = false;
    if (msg->type() == Protocol::Command::TagChangeNotification) {
        const auto &tagNtf = Protocol::cmdCast<Protocol::TagChangeNotification>(msg);
        const bool fetched = tagNtf.metadata().contains("FETCH_TAG");
        Tag tag;
        if (fetched) {
            const auto tags = tagCache->retrieve({ tagNtf.tag().id() });
            tag = tags.isEmpty() ? Tag() : tags.at(0);
        } else {
            tag = ProtocolHelper::parseTag(tagNtf.tag());
        }
        someoneWasListening = emitTagNotification(tagNtf, tag);
    } else if (msg->type() == Protocol::Command::RelationChangeNotification) {
        const auto &relNtf = Protocol::cmdCast<Protocol::RelationChangeNotification>(msg);
        const Relation rel = ProtocolHelper::parseRelationFetchResult(relNtf.relation());
        someoneWasListening = emitRelationNotification(relNtf, rel);
    } else if (msg->type() == Protocol::Command::CollectionChangeNotification) {
        const auto &colNtf = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg);
        const Collection parent = collectionCache->retrieve(colNtf.parentCollection());
        Collection destParent;
        if (colNtf.operation() == Protocol::CollectionChangeNotification::Move) {
            destParent = collectionCache->retrieve(colNtf.parentDestCollection());
        }

        const bool fetched = colNtf.metadata().contains("FETCH_COLLECTION");
        //For removals this will retrieve an invalid collection. We'll deal with that in emitCollectionNotification
        const Collection col = fetched ? collectionCache->retrieve(colNtf.collection().id()) : ProtocolHelper::parseCollection(colNtf.collection(), true);
        //It is possible that the retrieval fails also in the non-removal case (e.g. because the item was meanwhile removed while
        //the changerecorder stored the notification or the notification was in the queue). In order to drop such invalid notifications we have to ignore them.
        if (col.isValid() || colNtf.operation() == Protocol::CollectionChangeNotification::Remove || !fetchCollections()) {
            someoneWasListening = emitCollectionNotification(colNtf, col, parent, destParent);
        }
    } else if (msg->type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
        const Collection parent = collectionCache->retrieve(itemNtf.parentCollection());
        Collection destParent;
        if (itemNtf.operation() == Protocol::ItemChangeNotification::Move) {
            destParent = collectionCache->retrieve(itemNtf.parentDestCollection());
        }
        const bool fetched = itemNtf.metadata().contains("FETCH_ITEM") || itemNtf.mustRetrieve();
        //For removals this will retrieve an empty set. We'll deal with that in emitItemNotification
        Item::List items;
        if (fetched && fetchItems()) {
            items = itemCache->retrieve(Protocol::ChangeNotification::itemsToUids(itemNtf.items()));
        } else {
            const auto &ntfItems = itemNtf.items();
            items.reserve(ntfItems.size());
            for (const auto &ntfItem : ntfItems) {
                items.push_back(ProtocolHelper::parseItemFetchResult(ntfItem, &mItemFetchScope));
            }
        }
        //It is possible that the retrieval fails also in the non-removal case (e.g. because the item was meanwhile removed while
        //the changerecorder stored the notification or the notification was in the queue). In order to drop such invalid notifications we have to ignore them.
        if (!items.isEmpty() || itemNtf.operation() == Protocol::ItemChangeNotification::Remove || !fetchItems()) {
            someoneWasListening = emitItemsNotification(itemNtf, items, parent, destParent);
        }
    } else if (msg->type() == Protocol::Command::SubscriptionChangeNotification) {
        const auto &subNtf = Protocol::cmdCast<Protocol::SubscriptionChangeNotification>(msg);
        NotificationSubscriber subscriber;
        subscriber.setSubscriber(subNtf.subscriber());
        subscriber.setSessionId(subNtf.sessionId());
        subscriber.setMonitoredCollections(subNtf.collections());
        subscriber.setMonitoredItems(subNtf.items());
        subscriber.setMonitoredTags(subNtf.tags());
        QSet<Monitor::Type> monitorTypes;
        Q_FOREACH (auto type, subNtf.types()) {
            if (type == Protocol::ModifySubscriptionCommand::NoType) {
                continue;
            }
            monitorTypes.insert([](Protocol::ModifySubscriptionCommand::ChangeType type) {
                switch (type) {
                case Protocol::ModifySubscriptionCommand::ItemChanges: return Monitor::Items;
                case Protocol::ModifySubscriptionCommand::CollectionChanges: return Monitor::Collections;
                case Protocol::ModifySubscriptionCommand::TagChanges: return Monitor::Tags;
                case Protocol::ModifySubscriptionCommand::RelationChanges: return Monitor::Relations;
                case Protocol::ModifySubscriptionCommand::SubscriptionChanges: return Monitor::Subscribers;
                case Protocol::ModifySubscriptionCommand::ChangeNotifications: return Monitor::Notifications;
                default: Q_ASSERT(false); return Monitor::Items; //unreachable
                }
            }(type));
        }
        subscriber.setMonitoredTypes(monitorTypes);
        subscriber.setMonitoredMimeTypes(subNtf.mimeTypes());
        subscriber.setMonitoredResources(subNtf.resources());
        subscriber.setIgnoredSessions(subNtf.ignoredSessions());
        subscriber.setIsAllMonitored(subNtf.allMonitored());
        subscriber.setIsExclusive(subNtf.exclusive());
        subscriber.setItemFetchScope(ProtocolHelper::parseItemFetchScope(subNtf.itemFetchScope()));
        subscriber.setCollectionFetchScope(ProtocolHelper::parseCollectionFetchScope(subNtf.collectionFetchScope()));
        someoneWasListening = emitSubscriptionChangeNotification(subNtf, subscriber);
    } else if (msg->type() == Protocol::Command::DebugChangeNotification) {
        const auto &changeNtf = Protocol::cmdCast<Protocol::DebugChangeNotification>(msg);
        ChangeNotification notification;
        notification.setListeners(changeNtf.listeners());
        notification.setTimestamp(QDateTime::fromMSecsSinceEpoch(changeNtf.timestamp()));
        notification.setNotification(changeNtf.notification());
        switch (changeNtf.notification()->type()) {
        case Protocol::Command::ItemChangeNotification:
            notification.setType(ChangeNotification::Items);
            break;
        case Protocol::Command::CollectionChangeNotification:
            notification.setType(ChangeNotification::Collection);
            break;
        case Protocol::Command::TagChangeNotification:
            notification.setType(ChangeNotification::Tag);
            break;
        case Protocol::Command::RelationChangeNotification:
            notification.setType(ChangeNotification::Relation);
            break;
        case  Protocol::Command::SubscriptionChangeNotification:
            notification.setType(ChangeNotification::Subscription);
            break;
        default:
            Q_ASSERT(false); // huh?
            return false;
        }

        someoneWasListening = emitDebugChangeNotification(changeNtf, notification);
    }

    return someoneWasListening;
}

void MonitorPrivate::updatePendingStatistics(const Protocol::ChangeNotificationPtr &msg)
{
    if (msg->type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
        notifyCollectionStatisticsWatchers(itemNtf.parentCollection(), itemNtf.resource());
        // FIXME use the proper resource of the target collection, for cross resource moves
        notifyCollectionStatisticsWatchers(itemNtf.parentDestCollection(), itemNtf.destinationResource());
    } else if (msg->type() == Protocol::Command::CollectionChangeNotification) {
        const auto &colNtf = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg);
        if (colNtf.operation() == Protocol::CollectionChangeNotification::Remove) {
            // no need for statistics updates anymore
            recentlyChangedCollections.remove(colNtf.collection().id());
        }
    }
}

void MonitorPrivate::slotSessionDestroyed(QObject *object)
{
    auto *objectSession = qobject_cast<Session *>(object);
    if (objectSession) {
        sessions.removeAll(objectSession->sessionId());
        pendingModification.stopIgnoringSession(objectSession->sessionId());
        scheduleSubscriptionUpdate();
    }
}

void MonitorPrivate::slotFlushRecentlyChangedCollections()
{
    for (Collection::Id collection : qAsConst(recentlyChangedCollections)) {
        Q_ASSERT(collection >= 0);
        if (fetchCollectionStatistics) {
            fetchStatistics(collection);
        } else {
            static const CollectionStatistics dummyStatistics;
            Q_EMIT q_ptr->collectionStatisticsChanged(collection, dummyStatistics);
        }
    }
    recentlyChangedCollections.clear();
}

int MonitorPrivate::translateAndCompress(QQueue<Protocol::ChangeNotificationPtr> &notificationQueue, const Protocol::ChangeNotificationPtr &msg)
{
    // Always handle tags and relations
    if (msg->type() == Protocol::Command::TagChangeNotification
            || msg->type() == Protocol::Command::RelationChangeNotification) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    // We have to split moves into insert or remove if the source or destination
    // is not monitored.
    if (!msg->isMove()) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    bool sourceWatched = false;
    bool destWatched = false;

    if (msg->type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
        if (useRefCounting) {
            sourceWatched = isMonitored(itemNtf.parentCollection());
            destWatched = isMonitored(itemNtf.parentDestCollection());
        } else {
            if (!resources.isEmpty()) {
                sourceWatched = resources.contains(itemNtf.resource());
                destWatched = isMoveDestinationResourceMonitored(itemNtf);
            }
            if (!sourceWatched) {
                sourceWatched = isCollectionMonitored(itemNtf.parentCollection());
            }
            if (!destWatched) {
                destWatched = isCollectionMonitored(itemNtf.parentDestCollection());
            }
        }
    } else if (msg->type() == Protocol::Command::CollectionChangeNotification) {
        const auto &colNtf = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg);
        if (!resources.isEmpty()) {
            sourceWatched = resources.contains(colNtf.resource());
            destWatched = isMoveDestinationResourceMonitored(colNtf);
        }
        if (!sourceWatched) {
            sourceWatched = isCollectionMonitored(colNtf.parentCollection());
        }
        if (!destWatched) {
            destWatched = isCollectionMonitored(colNtf.parentDestCollection());
        }
    } else {
        Q_ASSERT(false);
        return 0;
    }

    if (!sourceWatched && !destWatched) {
        return 0;
    }

    if ((sourceWatched && destWatched) || (!collectionMoveTranslationEnabled && msg->type() == Protocol::Command::CollectionChangeNotification)) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    if (sourceWatched) {
        if (msg->type() == Protocol::Command::ItemChangeNotification) {
            auto removalMessage = Protocol::ItemChangeNotificationPtr::create(
                    Protocol::cmdCast<Protocol::ItemChangeNotification>(msg));
            removalMessage->setOperation(Protocol::ItemChangeNotification::Remove);
            removalMessage->setParentDestCollection(-1);
            notificationQueue.enqueue(removalMessage);
            return 1;
        } else {
            auto removalMessage = Protocol::CollectionChangeNotificationPtr::create(
                    Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg));
            removalMessage->setOperation(Protocol::CollectionChangeNotification::Remove);
            removalMessage->setParentDestCollection(-1);
            notificationQueue.enqueue(removalMessage);
            return 1;
        }
    }

    // Transform into an insertion
    if (msg->type() == Protocol::Command::ItemChangeNotification) {
        auto insertionMessage = Protocol::ItemChangeNotificationPtr::create(
                Protocol::cmdCast<Protocol::ItemChangeNotification>(msg));
        insertionMessage->setOperation(Protocol::ItemChangeNotification::Add);
        insertionMessage->setParentCollection(insertionMessage->parentDestCollection());
        insertionMessage->setParentDestCollection(-1);
        // We don't support batch insertion, so we have to do it one by one
        const auto split = splitMessage(*insertionMessage, false);
        for (const Protocol::ChangeNotificationPtr &insertion : split) {
            notificationQueue.enqueue(insertion);
        }
        return split.count();
    } else if (msg->type() == Protocol::Command::CollectionChangeNotification) {
        auto insertionMessage = Protocol::CollectionChangeNotificationPtr::create(
                Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg));
        insertionMessage->setOperation(Protocol::CollectionChangeNotification::Add);
        insertionMessage->setParentCollection(insertionMessage->parentDestCollection());
        insertionMessage->setParentDestCollection(-1);
        notificationQueue.enqueue(insertionMessage);
        return 1;
    }

    Q_ASSERT(false);
    return 0;
}

void MonitorPrivate::handleCommands()
{
    Q_Q(Monitor);

    CommandBufferLocker lock(&mCommandBuffer);
    CommandBufferNotifyBlocker notify(&mCommandBuffer);
    while (!mCommandBuffer.isEmpty()) {
        const auto cmd = mCommandBuffer.dequeue();
        lock.unlock();
        const auto command = cmd.command;

        if (command->isResponse()) {
            switch (command->type()) {
            case Protocol::Command::Hello: {
                qCDebug(AKONADICORE_LOG) << q_ptr << "Connected to notification bus";
                QByteArray subname;
                if (!q->objectName().isEmpty()) {
                    subname = q->objectName().toLatin1();
                } else {
                    subname = session->sessionId();
                }
                subname += " - " + QByteArray::number(quintptr(q));
                qCDebug(AKONADICORE_LOG) << q_ptr << "Subscribing as \"" << subname << "\"";
                auto subCmd = Protocol::CreateSubscriptionCommandPtr::create(subname, session->sessionId());
                ntfConnection->sendCommand(2, subCmd);
                break;
            }

            case Protocol::Command::CreateSubscription: {
                auto msubCmd = Protocol::ModifySubscriptionCommandPtr::create();
                for (const auto &col : qAsConst(collections)) {
                    msubCmd->startMonitoringCollection(col.id());
                }
                for (const auto &res : qAsConst(resources)) {
                    msubCmd->startMonitoringResource(res);
                }
                for (auto itemId : qAsConst(items)) {
                    msubCmd->startMonitoringItem(itemId);
                }
                for (auto tagId : qAsConst(tags)) {
                    msubCmd->startMonitoringTag(tagId);
                }
                for (auto type : qAsConst(types)) {
                    msubCmd->startMonitoringType(monitorTypeToProtocol(type));
                }
                for (const auto &mimetype : qAsConst(mimetypes)) {
                    msubCmd->startMonitoringMimeType(mimetype);
                }
                for (const auto &session : qAsConst(sessions)) {
                    msubCmd->startIgnoringSession(session);
                }
                msubCmd->setAllMonitored(monitorAll);
                msubCmd->setIsExclusive(exclusive);
                msubCmd->setItemFetchScope(ProtocolHelper::itemFetchScopeToProtocol(mItemFetchScope));
                msubCmd->setCollectionFetchScope(ProtocolHelper::collectionFetchScopeToProtocol(mCollectionFetchScope));
                msubCmd->setTagFetchScope(ProtocolHelper::tagFetchScopeToProtocol(mTagFetchScope));
                pendingModification = Protocol::ModifySubscriptionCommand();
                ntfConnection->sendCommand(3, msubCmd);
                break;
            }

            case Protocol::Command::ModifySubscription:
                // TODO: Handle errors
                if (!monitorReady) {
                    monitorReady = true;
                    Q_EMIT q_ptr->monitorReady();
                }
                break;

            default:
                qCWarning(AKONADICORE_LOG) << "Received an unexpected response on Notification stream: " << Protocol::debugString(command);
                break;
            }
        } else {
            switch (command->type()) {
            case Protocol::Command::ItemChangeNotification:
            case Protocol::Command::CollectionChangeNotification:
            case Protocol::Command::TagChangeNotification:
            case Protocol::Command::RelationChangeNotification:
            case Protocol::Command::SubscriptionChangeNotification:
            case Protocol::Command::DebugChangeNotification:
                slotNotify(command.staticCast<Protocol::ChangeNotification>());
                break;
            default:
                qCWarning(AKONADICORE_LOG) << "Received an unexpected message on Notification stream:" << Protocol::debugString(command);
                break;
            }
        }

        lock.relock();
    }
    notify.unblock();
    lock.unlock();
}

/*

  server notification --> ?accepted --> pendingNotifications --> ?dataAvailable --> emit
                                  |                                           |
                                  x --> discard                               x --> pipeline

  fetchJobDone --> pipeline ?dataAvailable --> emit
 */

void MonitorPrivate::slotNotify(const Protocol::ChangeNotificationPtr &msg)
{
    int appendedMessages = 0;
    int modifiedMessages = 0;
    int erasedMessages = 0;

    invalidateCaches(msg);
    updatePendingStatistics(msg);
    bool needsSplit = true;
    bool supportsBatch = false;

    if (isLazilyIgnored(msg, true)) {
        return;
    }

    checkBatchSupport(msg, needsSplit, supportsBatch);

    const bool isModifyFlags = (msg->type() == Protocol::Command::ItemChangeNotification
                                && Protocol::cmdCast<Protocol::ItemChangeNotification>(msg).operation() == Protocol::ItemChangeNotification::ModifyFlags);
    if (supportsBatch
            || (!needsSplit && !supportsBatch && !isModifyFlags)
            || msg->type() == Protocol::Command::CollectionChangeNotification) {
        // Make sure the batch msg is always queued before the split notifications
        const int oldSize = pendingNotifications.size();
        const int appended = translateAndCompress(pendingNotifications, msg);
        if (appended > 0) {
            appendedMessages += appended;
        } else {
            ++modifiedMessages;
        }
        // translateAndCompress can remove an existing "modify" when msg is a "delete".
        // Or it can merge two ModifyFlags and return false.
        // We need to detect such removals, for ChangeRecorder.
        if (pendingNotifications.count() != oldSize + appended) {
            ++erasedMessages; // this count isn't exact, but it doesn't matter
        }
    } else if (needsSplit) {
        // If it's not queued at least make sure we fetch all the items from split
        // notifications in one go.
        if (msg->type() == Protocol::Command::ItemChangeNotification) {
            const auto items = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg).items();
            itemCache->ensureCached(Protocol::ChangeNotification::itemsToUids(items), mItemFetchScope);
        }
    }

    // if the message contains more items, but we need to emit single-item notification,
    // split the message into one message per item and queue them
    // if the message contains only one item, but batches are not supported
    // (and thus neither is flagsModified), splitMessage() will convert the
    // notification to regular Modify with "FLAGS" part changed
    if (needsSplit || (!needsSplit && !supportsBatch && isModifyFlags)) {
        // Make sure inter-resource move notifications are translated into
        // Add/Remove notifications
        if (msg->type() == Protocol::Command::ItemChangeNotification) {
            const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
            if (itemNtf.operation() == Protocol::ItemChangeNotification::Move
                && itemNtf.resource() != itemNtf.destinationResource()) {
                if (needsSplit) {
                    const Protocol::ChangeNotificationList split = splitMessage(itemNtf, !supportsBatch);
                    for (const auto &splitMsg : split) {
                        appendedMessages += translateAndCompress(pendingNotifications, splitMsg);
                    }
                } else {
                    appendedMessages += translateAndCompress(pendingNotifications, msg);
                }
            } else {
                const Protocol::ChangeNotificationList split = splitMessage(itemNtf, !supportsBatch);
                pendingNotifications << (split | Actions::toQList);
                appendedMessages += split.count();
            }
        }
    }

    // tell ChangeRecorder (even if 0 appended, the compression could have made changes to existing messages)
    if (appendedMessages > 0 || modifiedMessages > 0 || erasedMessages > 0) {
        if (erasedMessages > 0) {
            notificationsErased();
        } else {
            notificationsEnqueued(appendedMessages);
        }
    }

    dispatchNotifications();
}

void MonitorPrivate::flushPipeline()
{
    while (!pipeline.isEmpty()) {
        const auto msg = pipeline.head();
        if (ensureDataAvailable(msg)) {
            // dequeue should be before emit, otherwise stuff might happen (like dataAvailable
            // being called again) and we end up dequeuing an empty pipeline
            pipeline.dequeue();
            emitNotification(msg);
        } else {
            break;
        }
    }
}

void MonitorPrivate::dataAvailable()
{
    flushPipeline();
    dispatchNotifications();
}

void MonitorPrivate::dispatchNotifications()
{
    // Note that this code is not used in a ChangeRecorder (pipelineSize==0)
    while (pipeline.size() < pipelineSize() && !pendingNotifications.isEmpty()) {
        const auto msg = pendingNotifications.dequeue();
        const bool avail = ensureDataAvailable(msg);
        if (avail && pipeline.isEmpty()) {
            emitNotification(msg);
        } else {
            pipeline.enqueue(msg);
        }
    }
}

static Relation::List extractRelations(const QSet<Protocol::ItemChangeNotification::Relation> &rels)
{
    Relation::List relations;
    if (rels.isEmpty()) {
        return relations;
    }

    relations.reserve(rels.size());
    for (const auto &rel : rels) {
        relations.push_back(Relation(rel.type.toLatin1(), Akonadi::Item(rel.leftId), Akonadi::Item(rel.rightId)));
    }
    return relations;
}

bool MonitorPrivate::emitItemsNotification(const Protocol::ItemChangeNotification &msg, const Item::List &items, const Collection &collection, const Collection &collectionDest)
{
    Collection col = collection;
    Collection colDest = collectionDest;
    if (!col.isValid()) {
        col = Collection(msg.parentCollection());
        col.setResource(QString::fromUtf8(msg.resource()));
    }
    if (!colDest.isValid()) {
        colDest = Collection(msg.parentDestCollection());
        // HACK: destination resource is delivered in the parts field...
        if (!msg.itemParts().isEmpty()) {
            colDest.setResource(QString::fromLatin1(*(msg.itemParts().cbegin())));
        }
    }

    Relation::List addedRelations;
    Relation::List removedRelations;
    if (msg.operation() == Protocol::ItemChangeNotification::ModifyRelations) {
        addedRelations = extractRelations(msg.addedRelations());
        removedRelations = extractRelations(msg.removedRelations());
    }

    Tag::List addedTags;
    Tag::List removedTags;
    if (msg.operation() == Protocol::ItemChangeNotification::ModifyTags) {
        addedTags = tagCache->retrieve(msg.addedTags() | Actions::toQList);
        removedTags = tagCache->retrieve(msg.removedTags() | Actions::toQList);
    }

    Item::List its = items;
    for (auto it = its.begin(), end = its.end(); it != end; ++it) {
        if (msg.operation() == Protocol::ItemChangeNotification::Move) {
            it->setParentCollection(colDest);
        } else {
            it->setParentCollection(col);
        }
    }
    bool handled = false;
    switch (msg.operation()) {
    case Protocol::ItemChangeNotification::Add:
        return emitToListeners(&Monitor::itemAdded, its.first(), col);
    case Protocol::ItemChangeNotification::Modify:
        return emitToListeners(&Monitor::itemChanged, its.first(), msg.itemParts());
    case Protocol::ItemChangeNotification::ModifyFlags:
        return emitToListeners(&Monitor::itemsFlagsChanged, its, msg.addedFlags(), msg.removedFlags());
    case Protocol::ItemChangeNotification::Move:
        handled |= emitToListeners(&Monitor::itemMoved, its.first(), col, colDest);
        handled |= emitToListeners(&Monitor::itemsMoved, its, col, colDest);
        return handled;
    case Protocol::ItemChangeNotification::Remove:
        handled |= emitToListeners(&Monitor::itemRemoved, its.first());
        handled |= emitToListeners(&Monitor::itemsRemoved, its);
        return handled;
    case Protocol::ItemChangeNotification::Link:
        handled |= emitToListeners(&Monitor::itemLinked, its.first(), col);
        handled |= emitToListeners(&Monitor::itemsLinked, its, col);
        return handled;
    case Protocol::ItemChangeNotification::Unlink:
        handled |= emitToListeners(&Monitor::itemUnlinked, its.first(), col);
        handled |= emitToListeners(&Monitor::itemsUnlinked, its, col);
        return handled;
    case Protocol::ItemChangeNotification::ModifyTags:
        return emitToListeners(&Monitor::itemsTagsChanged, its, addedTags | Actions::toQSet, removedTags | Actions::toQSet);
    case Protocol::ItemChangeNotification::ModifyRelations:
        return emitToListeners(&Monitor::itemsRelationsChanged, its, addedRelations, removedRelations);
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in item change notification";
        return false;
    }
}

bool MonitorPrivate::emitCollectionNotification(const Protocol::CollectionChangeNotification &msg, const Collection &col, const Collection &par, const Collection &dest)
{
    Collection parent = par;
    if (!parent.isValid()) {
        parent = Collection(msg.parentCollection());
    }
    Collection destination = dest;
    if (!destination.isValid()) {
        destination = Collection(msg.parentDestCollection());
    }

    Collection collection = col;
    Q_ASSERT(collection.isValid());
    if (!collection.isValid()) {
        qCWarning(AKONADICORE_LOG) << "Failed to get valid Collection for a Collection change!";
        return true; // prevent Monitor disconnecting from a signal
    }

    if (msg.operation() == Protocol::CollectionChangeNotification::Move) {
        collection.setParentCollection(destination);
    } else {
        collection.setParentCollection(parent);
    }

    bool handled = false;
    switch (msg.operation()) {
    case Protocol::CollectionChangeNotification::Add:
        return emitToListeners(&Monitor::collectionAdded, collection, parent);
    case Protocol::CollectionChangeNotification::Modify:
        handled |= emitToListeners(QOverload<const Akonadi::Collection &>::of(&Monitor::collectionChanged), collection);
        handled |= emitToListeners(QOverload<const Akonadi::Collection &, const QSet<QByteArray> &>::of(&Monitor::collectionChanged), collection, msg.changedParts());
        return handled;
    case Protocol::CollectionChangeNotification::Move:
        return emitToListeners(&Monitor::collectionMoved, collection, parent, destination);
    case Protocol::CollectionChangeNotification::Remove:
        return emitToListeners(&Monitor::collectionRemoved, collection);
    case Protocol::CollectionChangeNotification::Subscribe:
        return emitToListeners(&Monitor::collectionSubscribed, collection, parent);
    case Protocol::CollectionChangeNotification::Unsubscribe:
        return emitToListeners(&Monitor::collectionUnsubscribed, collection);
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in collection change notification";
        return false;
    }
}

bool MonitorPrivate::emitTagNotification(const Protocol::TagChangeNotification &msg, const Tag &tag)
{
    Q_UNUSED(msg)
    switch (msg.operation()) {
    case Protocol::TagChangeNotification::Add:
        return emitToListeners(&Monitor::tagAdded, tag);
    case Protocol::TagChangeNotification::Modify:
        return emitToListeners(&Monitor::tagChanged, tag);
    case Protocol::TagChangeNotification::Remove:
        return emitToListeners(&Monitor::tagRemoved, tag);
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in tag change notification";
        return false;
    }
}

bool MonitorPrivate::emitRelationNotification(const Protocol::RelationChangeNotification &msg, const Relation &relation)
{
    if (!relation.isValid()) {
        return false;
    }

    switch (msg.operation()) {
    case Protocol::RelationChangeNotification::Add:
        return emitToListeners(&Monitor::relationAdded, relation);
    case Protocol::RelationChangeNotification::Remove:
        return emitToListeners(&Monitor::relationRemoved, relation);
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in tag change notification";
        return false;
    }
}

bool MonitorPrivate::emitSubscriptionChangeNotification(const Protocol::SubscriptionChangeNotification &msg,
        const Akonadi::NotificationSubscriber &subscriber)
{
    if (!subscriber.isValid()) {
        return false;
    }

    switch (msg.operation()) {
    case Protocol::SubscriptionChangeNotification::Add:
        return emitToListeners(&Monitor::notificationSubscriberAdded, subscriber);
    case Protocol::SubscriptionChangeNotification::Modify:
        return emitToListeners(&Monitor::notificationSubscriberChanged, subscriber);
    case Protocol::SubscriptionChangeNotification::Remove:
        return emitToListeners(&Monitor::notificationSubscriberRemoved, subscriber);
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in subscription change notification";
        return false;
    }
}

bool MonitorPrivate::emitDebugChangeNotification(const Protocol::DebugChangeNotification &msg,
        const ChangeNotification &ntf)
{
    Q_UNUSED(msg)

    if (!ntf.isValid()) {
        return false;
    }

    return emitToListeners(&Monitor::debugNotification, ntf);
}

void MonitorPrivate::invalidateCaches(const Protocol::ChangeNotificationPtr &msg)
{
    // remove invalidates
    // modify removes the cache entry, as we need to re-fetch
    // And subscription modify the visibility of the collection by the collectionFetchScope.
    switch (msg->type()) {
    case Protocol::Command::CollectionChangeNotification: {
        const auto &colNtf = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg);
        switch (colNtf.operation()) {
        case Protocol::CollectionChangeNotification::Modify:
        case Protocol::CollectionChangeNotification::Move:
        case Protocol::CollectionChangeNotification::Subscribe:
            collectionCache->update(colNtf.collection().id(), mCollectionFetchScope);
            break;
        case Protocol::CollectionChangeNotification::Remove:
            collectionCache->invalidate(colNtf.collection().id());
            break;
        default:
            break;
        }
    } break;
    case Protocol::Command::ItemChangeNotification: {
        const auto &itemNtf = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg);
        switch (itemNtf.operation()) {
        case Protocol::ItemChangeNotification::Modify:
        case Protocol::ItemChangeNotification::ModifyFlags:
        case Protocol::ItemChangeNotification::ModifyTags:
        case Protocol::ItemChangeNotification::ModifyRelations:
        case Protocol::ItemChangeNotification::Move:
            itemCache->update(Protocol::ChangeNotification::itemsToUids(itemNtf.items()), mItemFetchScope);
            break;
        case Protocol::ItemChangeNotification::Remove:
            itemCache->invalidate(Protocol::ChangeNotification::itemsToUids(itemNtf.items()));
            break;
        default:
            break;
        }
    } break;
    case Protocol::Command::TagChangeNotification: {
        const auto &tagNtf = Protocol::cmdCast<Protocol::TagChangeNotification>(msg);
        switch (tagNtf.operation()) {
        case Protocol::TagChangeNotification::Modify:
            tagCache->update({ tagNtf.tag().id() }, mTagFetchScope);
            break;
        case Protocol::TagChangeNotification::Remove:
            tagCache->invalidate({ tagNtf.tag().id() });
            break;
        default:
            break;
        }
    } break;
    default:
        break;
    }
}

void MonitorPrivate::invalidateCache(const Collection &col)
{
    collectionCache->update(col.id(), mCollectionFetchScope);
}

void MonitorPrivate::ref(Collection::Id id)
{
    if (!refCountMap.contains(id)) {
        refCountMap.insert(id, 0);
    }
    ++refCountMap[id];

    if (m_buffer.isBuffered(id)) {
        m_buffer.purge(id);
    }
}

Akonadi::Collection::Id MonitorPrivate::deref(Collection::Id id)
{
    Q_ASSERT(refCountMap.contains(id));
    if (--refCountMap[id] == 0) {
        refCountMap.remove(id);
        return m_buffer.buffer(id);
    }
    return -1;
}

void MonitorPrivate::PurgeBuffer::purge(Collection::Id id)
{
    m_buffer.removeOne(id);
}

Akonadi::Collection::Id MonitorPrivate::PurgeBuffer::buffer(Collection::Id id)
{
    // Ensure that we don't put a duplicate @p id into the buffer.
    purge(id);

    Collection::Id bumpedId = -1;
    if (m_buffer.size() == MAXBUFFERSIZE) {
        bumpedId = m_buffer.dequeue();
        purge(bumpedId);
    }

    m_buffer.enqueue(id);

    return bumpedId;
}

int MonitorPrivate::PurgeBuffer::buffersize()
{
    return MAXBUFFERSIZE;
}

bool MonitorPrivate::isMonitored(Collection::Id colId) const
{
    if (!useRefCounting) {
        return true;
    }
    return refCountMap.contains(colId) || m_buffer.isBuffered(colId);
}

void MonitorPrivate::notifyCollectionStatisticsWatchers(Collection::Id collection, const QByteArray &resource)
{
    if (collection > 0 && (monitorAll || isCollectionMonitored(collection) || resources.contains(resource))) {
        recentlyChangedCollections.insert(collection);
        if (!statisticsCompressionTimer.isActive()) {
            statisticsCompressionTimer.start();
        }
    }
}

Protocol::ModifySubscriptionCommand::ChangeType MonitorPrivate::monitorTypeToProtocol(Monitor::Type type)
{
    switch (type) {
    case Monitor::Collections:
        return Protocol::ModifySubscriptionCommand::CollectionChanges;
    case Monitor::Items:
        return Protocol::ModifySubscriptionCommand::ItemChanges;
    case Monitor::Tags:
        return Protocol::ModifySubscriptionCommand::TagChanges;
    case Monitor::Relations:
        return Protocol::ModifySubscriptionCommand::RelationChanges;
    case Monitor::Subscribers:
        return Protocol::ModifySubscriptionCommand::SubscriptionChanges;
    case Monitor::Notifications:
        return Protocol::ModifySubscriptionCommand::ChangeNotifications;
    default:
        Q_ASSERT(false);
        return Protocol::ModifySubscriptionCommand::NoType;
    }
}

void MonitorPrivate::updateListeners(QMetaMethod signal, ListenerAction action)
{
    #define UPDATE_LISTENERS(sig) \
    if (signal == QMetaMethod::fromSignal(sig)) { \
        updateListener(sig, action); \
        return; \
    }

    UPDATE_LISTENERS(&Monitor::itemChanged)
    UPDATE_LISTENERS(&Monitor::itemChanged)
    UPDATE_LISTENERS(&Monitor::itemsFlagsChanged)
    UPDATE_LISTENERS(&Monitor::itemsTagsChanged)
    UPDATE_LISTENERS(&Monitor::itemsRelationsChanged)
    UPDATE_LISTENERS(&Monitor::itemMoved)
    UPDATE_LISTENERS(&Monitor::itemsMoved)
    UPDATE_LISTENERS(&Monitor::itemAdded)
    UPDATE_LISTENERS(&Monitor::itemRemoved)
    UPDATE_LISTENERS(&Monitor::itemsRemoved)
    UPDATE_LISTENERS(&Monitor::itemLinked)
    UPDATE_LISTENERS(&Monitor::itemsLinked)
    UPDATE_LISTENERS(&Monitor::itemUnlinked)
    UPDATE_LISTENERS(&Monitor::itemsUnlinked)
    UPDATE_LISTENERS(&Monitor::collectionAdded)

    UPDATE_LISTENERS(QOverload<const Akonadi::Collection &>::of(&Monitor::collectionChanged))
    UPDATE_LISTENERS((QOverload<const Akonadi::Collection &, const QSet<QByteArray> &>::of(&Monitor::collectionChanged)))
    UPDATE_LISTENERS(&Monitor::collectionMoved)
    UPDATE_LISTENERS(&Monitor::collectionRemoved)
    UPDATE_LISTENERS(&Monitor::collectionSubscribed)
    UPDATE_LISTENERS(&Monitor::collectionUnsubscribed)
    UPDATE_LISTENERS(&Monitor::collectionStatisticsChanged)

    UPDATE_LISTENERS(&Monitor::tagAdded)
    UPDATE_LISTENERS(&Monitor::tagChanged)
    UPDATE_LISTENERS(&Monitor::tagRemoved)

    UPDATE_LISTENERS(&Monitor::relationAdded)
    UPDATE_LISTENERS(&Monitor::relationRemoved)

    UPDATE_LISTENERS(&Monitor::notificationSubscriberAdded)
    UPDATE_LISTENERS(&Monitor::notificationSubscriberChanged)
    UPDATE_LISTENERS(&Monitor::notificationSubscriberRemoved)
    UPDATE_LISTENERS(&Monitor::debugNotification)

#undef UPDATE_LISTENERS
}

// @endcond
