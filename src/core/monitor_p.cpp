/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

// @cond PRIVATE

#include "monitor_p.h"

#include "collectionfetchjob.h"
#include "collectionstatistics.h"
#include "KDBusConnectionPool"
#include "itemfetchjob.h"
#include "notificationmanagerinterface.h"
#include "session.h"
#include "changemediator_p.h"
#include "vectorhelper.h"
#include "akonadicore_debug.h"
#include "notificationsubscriber.h"
#include "changenotification.h"

using namespace Akonadi;
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
    delete dependenciesFactory;
    delete collectionCache;
    delete itemCache;
    delete tagCache;
    ntfConnection->disconnect(q_ptr);
    ntfConnection->deleteLater();
}

void MonitorPrivate::init()
{
    // needs to be at least 3x pipeline size for the collection move case
    collectionCache = dependenciesFactory->createCollectionCache(3 * PipelineSize, session);
    // needs to be at least 1x pipeline size
    itemCache = dependenciesFactory->createItemListCache(PipelineSize, session);
    // 20 tags looks like a reasonable amount to keep around
    tagCache = dependenciesFactory->createTagListCache(20, session);

    QObject::connect(collectionCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()));
    QObject::connect(itemCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()));
    QObject::connect(tagCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()));
    QObject::connect(ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
                     q_ptr, SLOT(serverStateChanged(Akonadi::ServerManager::State)));

    statisticsCompressionTimer.setSingleShot(true);
    statisticsCompressionTimer.setInterval(500);
    QObject::connect(&statisticsCompressionTimer, SIGNAL(timeout()), q_ptr, SLOT(slotFlushRecentlyChangedCollections()));
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

    ntfConnection = dependenciesFactory->createNotificationConnection(session);
    if (!ntfConnection) {
        return false;
    }
    q_ptr->connect(ntfConnection, SIGNAL(commandReceived(qint64,Akonadi::Protocol::Command)),
                   q_ptr, SLOT(commandReceived(qint64,Akonadi::Protocol::Command)));

    pendingModification = Protocol::ModifySubscriptionCommand();
    for (const auto &col : collections) {
        pendingModification.startMonitoringCollection(col.id());
    }
    for (const auto &res : resources) {
        pendingModification.startMonitoringResource(res);
    }
    for (auto itemId : items) {
        pendingModification.startMonitoringItem(itemId);
    }
    for (auto tagId : tags) {
        pendingModification.startMonitoringTag(tagId);
    }
    for (auto type : types) {
        pendingModification.startMonitoringType(static_cast<Protocol::ModifySubscriptionCommand::ChangeType>(type));
    }
    for (const auto &mimetype : mimetypes) {
        pendingModification.startMonitoringMimeType(mimetype);
    }
    for (const auto &session : sessions) {
        pendingModification.startIgnoringSession(session);
    }
    pendingModification.setAllMonitored(monitorAll);
    pendingModification.setExclusive(exclusive);

    ntfConnection->reconnect();

    return true;
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
    itemCache->update(QList<Item::Id>() << id, mItemFetchScope);
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

    pendingModificationTimer = new QTimer();
    pendingModificationTimer->setSingleShot(true);
    pendingModificationTimer->setInterval(0);
    pendingModificationTimer->start();
    q_ptr->connect(pendingModificationTimer, SIGNAL(timeout()),
                   q_ptr, SLOT(slotUpdateSubscription()));
}

void MonitorPrivate::slotUpdateSubscription()
{
    delete pendingModificationTimer;
    pendingModificationTimer = nullptr;

    if (ntfConnection) {
        ntfConnection->sendCommand(3, pendingModification);
        pendingModification = Protocol::ModifySubscriptionCommand();
    }
}

bool MonitorPrivate::isLazilyIgnored(const Protocol::ChangeNotification &msg, bool allowModifyFlagsConversion) const
{
    if (msg.type() == Protocol::Command::CollectionChangeNotification) {
        // Lazy fetching can only affects items.
        return false;
    }

    if (msg.type() == Protocol::Command::TagChangeNotification) {
        const auto op = static_cast<const Protocol::TagChangeNotification &>(msg).operation();
        return ((op == Protocol::TagChangeNotification::Add && q_ptr->receivers(SIGNAL(tagAdded(Akonadi::Tag))) == 0)
                || (op == Protocol::TagChangeNotification::Modify && q_ptr->receivers(SIGNAL(tagChanged(Akonadi::Tag))) == 0)
                || (op == Protocol::TagChangeNotification::Remove && q_ptr->receivers(SIGNAL(tagRemoved(Akonadi::Tag))) == 0));
    }

    if (!fetchCollectionStatistics && msg.type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = static_cast<const Protocol::ItemChangeNotification &>(msg);
        const auto op = itemNtf.operation();
        if ((op == Protocol::ItemChangeNotification::Add && q_ptr->receivers(SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection))) == 0)
                || (op == Protocol::ItemChangeNotification::Remove && q_ptr->receivers(SIGNAL(itemRemoved(Akonadi::Item))) == 0
                    && q_ptr->receivers(SIGNAL(itemsRemoved(Akonadi::Item::List))) == 0)
                || (op == Protocol::ItemChangeNotification::Modify && q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) == 0)
                || (op == Protocol::ItemChangeNotification::ModifyFlags
                    && (q_ptr->receivers(SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>))) == 0
                        // Newly delivered ModifyFlags notifications will be converted to
                        // itemChanged(item, "FLAGS") for legacy clients.
                        && (!allowModifyFlagsConversion || q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) == 0)))
                || (op == Protocol::ItemChangeNotification::ModifyTags && q_ptr->receivers(SIGNAL(itemsTagsChanged(Akonadi::Item::List,QSet<Akonadi::Tag>,QSet<Akonadi::Tag>))) == 0)
                || (op == Protocol::ItemChangeNotification::Move && q_ptr->receivers(SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection))) == 0
                    && q_ptr->receivers(SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection))) == 0)
                || (op == Protocol::ItemChangeNotification::Link && q_ptr->receivers(SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection))) == 0
                    && q_ptr->receivers(SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection))) == 0)
                || (op == Protocol::ItemChangeNotification::Unlink && q_ptr->receivers(SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection))) == 0
                    && q_ptr->receivers(SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection))) == 0)) {
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
            if (!isMonitored(parentCollectionId) && !isMonitored(itemNtf.parentDestCollection())) {
                return true;
            }
            // We can't ignore the move. It must be transformed later into a removal or insertion.
            return false;
        }
        return true;
    }

    return false;
}

void MonitorPrivate::checkBatchSupport(const Protocol::ChangeNotification &msg, bool &needsSplit, bool &batchSupported) const
{
    if (msg.type() != Protocol::Command::ItemChangeNotification) {
        needsSplit = false;
        batchSupported = false;
        return;
    }

    const auto itemNtf = static_cast<const Protocol::ItemChangeNotification *>(&msg);
    const bool isBatch = (itemNtf->items().count() > 1);

    switch (itemNtf->operation()) {
    case Protocol::ItemChangeNotification::Add:
        needsSplit = isBatch;
        batchSupported = false;
        return;
    case Protocol::ItemChangeNotification::Modify:
        needsSplit = isBatch;
        batchSupported = false;
        return;
    case Protocol::ItemChangeNotification::ModifyFlags:
        batchSupported = q_ptr->receivers(SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>))) > 0;
        needsSplit = isBatch && !batchSupported && q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) > 0;
        return;
    case Protocol::ItemChangeNotification::ModifyTags:
        // Tags were added after batch notifications, so they are always supported
        batchSupported = true;
        needsSplit = false;
        return;
    case Protocol::ItemChangeNotification::ModifyRelations:
        // Relations were added after batch notifications, so they are always supported
        batchSupported = true;
        needsSplit = false;
        return;
    case Protocol::ItemChangeNotification::Move:
        needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection))) > 0;
        batchSupported = q_ptr->receivers(SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection))) > 0;
        return;
    case Protocol::ItemChangeNotification::Remove:
        needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemRemoved(Akonadi::Item))) > 0;
        batchSupported = q_ptr->receivers(SIGNAL(itemsRemoved(Akonadi::Item::List))) > 0;
        return;
    case Protocol::ItemChangeNotification::Link:
        needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection))) > 0;
        batchSupported = q_ptr->receivers(SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection))) > 0;
        return;
    case Protocol::ItemChangeNotification::Unlink:
        needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection))) > 0;
        batchSupported = q_ptr->receivers(SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection))) > 0;
        return;
    default:
        needsSplit = isBatch;
        batchSupported = false;
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << itemNtf->operation() << "in item change notification";
        return;
    }
}

Protocol::ChangeNotification::List MonitorPrivate::splitMessage(const Protocol::ItemChangeNotification &msg, bool legacy) const
{
    Protocol::ChangeNotification::List list;

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

    const auto items = msg.items();
    list.reserve(items.count());
    for (const Protocol::ItemChangeNotification::Item &item : items) {
        Protocol::ItemChangeNotification copy = baseMsg;
        copy.addItem(item.id, item.remoteId, item.remoteRevision, item.mimeType);
        list << copy;
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

bool MonitorPrivate::ensureDataAvailable(const Protocol::ChangeNotification &msg)
{
    bool allCached = true;

    if (msg.type() == Protocol::Command::TagChangeNotification) {
        return tagCache->ensureCached({ static_cast<const Protocol::TagChangeNotification &>(msg).id() }, mTagFetchScope);
    }
    if (msg.type() == Protocol::Command::RelationChangeNotification) {
        return true;
    }

    if (msg.type() == Protocol::Command::SubscriptionChangeNotification) {
        return true;
    }

    if (msg.type() == Protocol::Command::DebugChangeNotification) {
        return true;
    }

    if (msg.type() == Protocol::Command::CollectionChangeNotification
            && static_cast<const Protocol::CollectionChangeNotification &>(msg).operation() == Protocol::CollectionChangeNotification::Remove) {
        //For collection removals the collection is gone anyways, so we can't fetch it. Rid will be set later on instead.
        return true;
    }

    if (fetchCollections()) {
        const qint64 parentCollection = (msg.type() == Protocol::Command::ItemChangeNotification) ?
                                        static_cast<const Protocol::ItemChangeNotification &>(msg).parentCollection() :
                                        (msg.type() == Protocol::Command::CollectionChangeNotification) ?
                                        static_cast<const Protocol::CollectionChangeNotification &>(msg).parentCollection() :
                                        -1;
        if (parentCollection > -1 && !collectionCache->ensureCached(parentCollection, mCollectionFetchScope)) {
            allCached = false;
        }

        qint64 parentDestCollection = -1;

        if ((msg.type() == Protocol::Command::ItemChangeNotification)
                && (static_cast<const Protocol::ItemChangeNotification &>(msg).operation() == Protocol::ItemChangeNotification::Move)) {
            parentDestCollection = static_cast<const Protocol::ItemChangeNotification &>(msg).parentDestCollection();
        } else if ((msg.type() == Protocol::Command::CollectionChangeNotification)
                   && (static_cast<const Protocol::CollectionChangeNotification &>(msg).operation() == Protocol::CollectionChangeNotification::Move)) {
            parentDestCollection = static_cast<const Protocol::CollectionChangeNotification &>(msg).parentDestCollection();
        }
        if (parentDestCollection > -1 && !collectionCache->ensureCached(parentDestCollection, mCollectionFetchScope)) {
            allCached = false;
        }

    }

    if (msg.isRemove()) {
        return allCached;
    }

    if (msg.type() == Protocol::Command::ItemChangeNotification && fetchItems()) {
        ItemFetchScope scope(mItemFetchScope);
        const auto &itemNtf = static_cast<const Protocol::ItemChangeNotification &>(msg);
        if (mFetchChangedOnly && (itemNtf.operation() == Protocol::ItemChangeNotification::Modify || itemNtf.operation() == Protocol::ItemChangeNotification::ModifyFlags)) {
            bool fullPayloadWasRequested = scope.fullPayload();
            scope.fetchFullPayload(false);
            const QSet<QByteArray> requestedPayloadParts = scope.payloadParts();
            for (const QByteArray &part : requestedPayloadParts) {
                scope.fetchPayloadPart(part, false);
            }

            bool allAttributesWereRequested = scope.allAttributes();
            const QSet<QByteArray> requestedAttrParts = scope.attributes();
            for (const QByteArray &part : requestedAttrParts) {
                scope.fetchAttribute(part, false);
            }

            const QSet<QByteArray> changedParts = itemNtf.itemParts();
            for (const QByteArray &part : changedParts)  {
                if (part.startsWith("PLD:") &&    //krazy:exclude=strings since QByteArray
                        (fullPayloadWasRequested || requestedPayloadParts.contains(part))) {
                    scope.fetchPayloadPart(part.mid(4), true);;
                }
                if (part.startsWith("ATR:") &&    //krazy:exclude=strings since QByteArray
                        (allAttributesWereRequested || requestedAttrParts.contains(part))) {
                    scope.fetchAttribute(part.mid(4), true);
                }
            }
        }
        if (!itemCache->ensureCached(itemNtf.uids(), scope)) {
            allCached = false;

        }

        // Make sure all tags for ModifyTags operation are in cache too
        if (itemNtf.operation() == Protocol::ItemChangeNotification::ModifyTags) {
            if (!tagCache->ensureCached((itemNtf.addedTags() + itemNtf.removedTags()).toList(), mTagFetchScope)) {
                allCached = false;
            }
        }

    } else if (msg.type() == Protocol::Command::CollectionChangeNotification && fetchCollections()) {
        const qint64 colId = static_cast<const Protocol::CollectionChangeNotification &>(msg).id();
        if (!collectionCache->ensureCached(colId, mCollectionFetchScope)) {
            allCached = false;
        }
    }

    return allCached;
}

bool MonitorPrivate::emitNotification(const Protocol::ChangeNotification &msg)
{
    bool someoneWasListening = false;
    if (msg.type() == Protocol::Command::TagChangeNotification) {
        const auto &tagNtf = static_cast<const Protocol::TagChangeNotification &>(msg);
        //In case of a Remove notification this will return a list of invalid entities (we'll deal later with them)
        const Tag::List tags = tagCache->retrieve({ tagNtf.id() });
        someoneWasListening = emitTagNotification(tagNtf, tags.isEmpty() ? Tag() : tags[0]);
    } else if (msg.type() == Protocol::Command::RelationChangeNotification) {
        const auto &relNtf = static_cast<const Protocol::RelationChangeNotification &>(msg);
        Relation rel;
        rel.setLeft(Akonadi::Item(relNtf.leftItem()));
        rel.setRight(Akonadi::Item(relNtf.rightItem()));
        rel.setType(relNtf.type().toLatin1());
        rel.setRemoteId(relNtf.remoteId().toLatin1());
        someoneWasListening = emitRelationNotification(relNtf, rel);
    } else if (msg.type() == Protocol::Command::CollectionChangeNotification) {
        const auto &colNtf = static_cast<const Protocol::CollectionChangeNotification &>(msg);
        const Collection parent = collectionCache->retrieve(colNtf.parentCollection());
        Collection destParent;
        if (colNtf.operation() == Protocol::CollectionChangeNotification::Move) {
            destParent = collectionCache->retrieve(colNtf.parentDestCollection());
        }

        //For removals this will retrieve an invalid collection. We'll deal with that in emitCollectionNotification
        const Collection col = collectionCache->retrieve(colNtf.id());
        //It is possible that the retrieval fails also in the non-removal case (e.g. because the item was meanwhile removed while
        //the changerecorder stored the notification or the notification was in the queue). In order to drop such invalid notifications we have to ignore them.
        if (col.isValid() || colNtf.operation() == Protocol::CollectionChangeNotification::Remove || !fetchCollections()) {
            someoneWasListening = emitCollectionNotification(msg, col, parent, destParent);
        }
    } else if (msg.type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = static_cast<const Protocol::ItemChangeNotification &>(msg);
        const Collection parent = collectionCache->retrieve(itemNtf.parentCollection());
        Collection destParent;
        if (itemNtf.operation() == Protocol::ItemChangeNotification::Move) {
            destParent = collectionCache->retrieve(itemNtf.parentDestCollection());
        }
        //For removals this will retrieve an empty set. We'll deal with that in emitItemNotification
        const Item::List items = itemCache->retrieve(itemNtf.uids());
        //It is possible that the retrieval fails also in the non-removal case (e.g. because the item was meanwhile removed while
        //the changerecorder stored the notification or the notification was in the queue). In order to drop such invalid notifications we have to ignore them.
        if (!items.isEmpty() || itemNtf.operation() == Protocol::ItemChangeNotification::Remove || !fetchItems()) {
            someoneWasListening = emitItemsNotification(msg, items, parent, destParent);
        }
    } else if (msg.type() == Protocol::Command::SubscriptionChangeNotification) {
        const auto &subNtf = static_cast<const Protocol::SubscriptionChangeNotification &>(msg);
        NotificationSubscriber subscriber;
        subscriber.setSubscriber(subNtf.subscriber());
        subscriber.setSessionId(subNtf.sessionId());
        subscriber.setMonitoredCollections(subNtf.collections());
        subscriber.setMonitoredItems(subNtf.items());
        subscriber.setMonitoredTags(subNtf.tags());
        QSet<Monitor::Type> monitorTypes;
        Q_FOREACH (auto type, subNtf.types()) {
            monitorTypes.insert(static_cast<Monitor::Type>(type));
        }
        subscriber.setMonitoredTypes(monitorTypes);
        subscriber.setMonitoredMimeTypes(subNtf.mimeTypes());
        subscriber.setMonitoredResources(subNtf.resources());
        subscriber.setIgnoredSessions(subNtf.ignoredSessions());
        subscriber.setIsAllMonitored(subNtf.isAllMonitored());
        subscriber.setIsExclusive(subNtf.isExclusive());
        someoneWasListening = emitSubscriptionChangeNotification(subNtf, subscriber);
    } else if (msg.type() == Protocol::Command::DebugChangeNotification) {
        const auto &changeNtf = static_cast<const Protocol::DebugChangeNotification &>(msg);
        ChangeNotification notification;
        notification.setListeners(changeNtf.listeners());
        notification.setTimestamp(QDateTime::fromMSecsSinceEpoch(changeNtf.timestamp()));
        notification.setNotification(changeNtf.notification());
        switch (changeNtf.notification().type()) {
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

void MonitorPrivate::updatePendingStatistics(const Protocol::ChangeNotification &msg)
{
    if (msg.type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = static_cast<const Protocol::ItemChangeNotification &>(msg);
        notifyCollectionStatisticsWatchers(itemNtf.parentCollection(), itemNtf.resource());
        // FIXME use the proper resource of the target collection, for cross resource moves
        notifyCollectionStatisticsWatchers(itemNtf.parentDestCollection(), itemNtf.destinationResource());
    } else if (msg.type() == Protocol::Command::CollectionChangeNotification) {
        const auto &colNtf = static_cast<const Protocol::CollectionChangeNotification &>(msg);
        if (colNtf.operation() == Protocol::CollectionChangeNotification::Remove) {
            // no need for statistics updates anymore
            recentlyChangedCollections.remove(colNtf.id());
        }
    }
}

void MonitorPrivate::slotSessionDestroyed(QObject *object)
{
    Session *objectSession = qobject_cast<Session *>(object);
    if (objectSession) {
        sessions.removeAll(objectSession->sessionId());
        pendingModification.stopIgnoringSession(objectSession->sessionId());
        scheduleSubscriptionUpdate();
    }
}

void MonitorPrivate::slotStatisticsChangedFinished(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Error on fetching collection statistics: " << job->errorText();
    } else {
        CollectionStatisticsJob *statisticsJob = static_cast<CollectionStatisticsJob *>(job);
        Q_ASSERT(statisticsJob->collection().isValid());
        emit q_ptr->collectionStatisticsChanged(statisticsJob->collection().id(),
                                                statisticsJob->statistics());
    }
}

void MonitorPrivate::slotFlushRecentlyChangedCollections()
{
    foreach (Collection::Id collection, recentlyChangedCollections) {
        Q_ASSERT(collection >= 0);
        if (fetchCollectionStatistics) {
            fetchStatistics(collection);
        } else {
            static const CollectionStatistics dummyStatistics;
            emit q_ptr->collectionStatisticsChanged(collection, dummyStatistics);
        }
    }
    recentlyChangedCollections.clear();
}

int MonitorPrivate::translateAndCompress(QQueue<Protocol::ChangeNotification> &notificationQueue, const Protocol::ChangeNotification &msg)
{
    // Always handle tags and relations
    if (msg.type() == Protocol::Command::TagChangeNotification
            || msg.type() == Protocol::Command::RelationChangeNotification) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    // We have to split moves into insert or remove if the source or destination
    // is not monitored.
    if (!msg.isMove()) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    bool sourceWatched = false;
    bool destWatched = false;

    if (msg.type() == Protocol::Command::ItemChangeNotification) {
        const auto &itemNtf = static_cast<const Protocol::ItemChangeNotification &>(msg);
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
    } else if (msg.type() == Protocol::Command::CollectionChangeNotification) {
        const auto &colNtf = static_cast<const Protocol::CollectionChangeNotification &>(msg);
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

    if ((sourceWatched && destWatched) || (!collectionMoveTranslationEnabled && msg.type() == Protocol::Command::CollectionChangeNotification)) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    if (sourceWatched) {
        if (msg.type() == Protocol::Command::ItemChangeNotification) {
            Protocol::ItemChangeNotification removalMessage = msg;
            removalMessage.setOperation(Protocol::ItemChangeNotification::Remove);
            removalMessage.setParentDestCollection(-1);
            notificationQueue.enqueue(removalMessage);
            return 1;
        } else {
            Protocol::CollectionChangeNotification removalMessage = msg;
            removalMessage.setOperation(Protocol::CollectionChangeNotification::Remove);
            removalMessage.setParentDestCollection(-1);
            notificationQueue.enqueue(removalMessage);
            return 1;
        }
    }

    // Transform into an insertion
    if (msg.type() == Protocol::Command::ItemChangeNotification) {
        Protocol::ItemChangeNotification insertionMessage = msg;
        insertionMessage.setOperation(Protocol::ItemChangeNotification::Add);
        insertionMessage.setParentCollection(insertionMessage.parentDestCollection());
        insertionMessage.setParentDestCollection(-1);
        // We don't support batch insertion, so we have to do it one by one
        const auto split = splitMessage(insertionMessage, false);
        for (const Protocol::ChangeNotification &insertion : split) {
            notificationQueue.enqueue(insertion);
        }
        return split.count();
    } else if (msg.type() == Protocol::Command::CollectionChangeNotification) {
        Protocol::CollectionChangeNotification insertionMessage = msg;
        insertionMessage.setOperation(Protocol::CollectionChangeNotification::Add);
        insertionMessage.setParentCollection(insertionMessage.parentDestCollection());
        insertionMessage.setParentDestCollection(-1);
        notificationQueue.enqueue(insertionMessage);
        return 1;
    }

    Q_ASSERT(false);
    return 0;
}

void MonitorPrivate::commandReceived(qint64 tag, const Protocol::Command &command)
{
    Q_Q(Monitor);
    Q_UNUSED(tag);
    if (command.isResponse()) {
        switch (command.type()) {
        case Protocol::Command::Hello: {
            Protocol::HelloResponse hello(command);
            qCDebug(AKONADICORE_LOG) << q_ptr << "Connected to notification bus";
            QByteArray subname = session->sessionId() + " - ";
            if (!q->objectName().isEmpty()) {
                subname += q->objectName().toLatin1();
            } else {
                subname += QByteArray::number(quintptr(q));
            }
            qCDebug(AKONADICORE_LOG) << q_ptr << "Subscribing as \"" << subname << "\"";
            Protocol::CreateSubscriptionCommand subCmd(subname, session->sessionId());
            ntfConnection->sendCommand(2, subCmd);
            break;
        }

        case Protocol::Command::CreateSubscription: {
            Protocol::ModifySubscriptionCommand msubCmd = pendingModification;
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
            qCWarning(AKONADICORE_LOG) << "Received an unexpected response on Notification stream: " << command.debugString();
            break;
        }
    } else {
        switch (command.type()) {
        case Protocol::Command::ItemChangeNotification:
        case Protocol::Command::CollectionChangeNotification:
        case Protocol::Command::TagChangeNotification:
        case Protocol::Command::RelationChangeNotification:
        case Protocol::Command::SubscriptionChangeNotification:
        case Protocol::Command::DebugChangeNotification:
            slotNotify(command);
            break;
        default:
            qCWarning(AKONADICORE_LOG) << "Received an unexpected message on Notification stream:" << command.debugString();
            break;
        }
    }
}

/*

  server notification --> ?accepted --> pendingNotifications --> ?dataAvailable --> emit
                                  |                                           |
                                  x --> discard                               x --> pipeline

  fetchJobDone --> pipeline ?dataAvailable --> emit
 */

void MonitorPrivate::slotNotify(const Protocol::ChangeNotification &msg)
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

    const bool isModifyFlags = (msg.type() == Protocol::Command::ItemChangeNotification
                                && static_cast<const Protocol::ItemChangeNotification &>(msg).operation() == Protocol::ItemChangeNotification::ModifyFlags);
    if (supportsBatch
            || (!needsSplit && !supportsBatch && !isModifyFlags)
            || msg.type() == Protocol::Command::CollectionChangeNotification) {
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
        if (msg.type() == Protocol::Command::ItemChangeNotification) {
            itemCache->ensureCached(static_cast<const Protocol::ItemChangeNotification &>(msg).uids(), mItemFetchScope);
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
        if (msg.type() == Protocol::Command::ItemChangeNotification
                && static_cast<const Protocol::ItemChangeNotification &>(msg).type() == Protocol::Command::MoveItems
                && static_cast<const Protocol::ItemChangeNotification &>(msg).resource() != static_cast<const Protocol::ItemChangeNotification &>(msg).destinationResource()) {
            if (needsSplit) {
                const Protocol::ChangeNotification::List split = splitMessage(msg, !supportsBatch);
                for (const auto &splitMsg : split) {
                    appendedMessages += translateAndCompress(pendingNotifications, splitMsg);
                }
            } else {
                appendedMessages += translateAndCompress(pendingNotifications, msg);
            }
        } else {
            const Protocol::ChangeNotification::List split = splitMessage(msg, !supportsBatch);
            pendingNotifications << split.toList();
            appendedMessages += split.count();
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
        const Protocol::ChangeNotification msg = pipeline.head();
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
        const Protocol::ChangeNotification msg = pendingNotifications.dequeue();
        if (ensureDataAvailable(msg) && pipeline.isEmpty()) {
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

bool MonitorPrivate::emitItemsNotification(const Protocol::ItemChangeNotification &msg_, const Item::List &items, const Collection &collection, const Collection &collectionDest)
{
    Protocol::ItemChangeNotification msg = msg_;
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

    const QSet<QByteArray> addedFlags = msg.addedFlags();
    const QSet<QByteArray> removedFlags = msg.removedFlags();

    Relation::List addedRelations, removedRelations;
    if (msg.operation() == Protocol::ItemChangeNotification::ModifyRelations) {
        addedRelations = extractRelations(msg.addedRelations());
        removedRelations = extractRelations(msg.removedRelations());
    }

    Tag::List addedTags, removedTags;
    if (msg.operation() == Protocol::ItemChangeNotification::ModifyTags) {
        addedTags = tagCache->retrieve(msg.addedTags().toList());
        removedTags = tagCache->retrieve(msg.removedTags().toList());
    }

    QMap<Protocol::ChangeNotification::Id, Protocol::ItemChangeNotification::Item> msgItems = msg.items();
    Item::List its = items;
    QMutableVectorIterator<Item> iter(its);
    while (iter.hasNext()) {
        Item it = iter.next();
        if (it.isValid()) {
            const Protocol::ItemChangeNotification::Item msgItem = msgItems[it.id()];
            if (msg.operation() == Protocol::ItemChangeNotification::Remove) {
                it.setRemoteId(msgItem.remoteId);
                it.setRemoteRevision(msgItem.remoteRevision);
                it.setMimeType(msgItem.mimeType);
            }

            if (!it.parentCollection().isValid()) {
                if (msg.operation() == Protocol::ItemChangeNotification::Move) {
                    it.setParentCollection(colDest);
                } else {
                    it.setParentCollection(col);
                }
            } else {
                // item has a valid parent collection, most likely due to retrieved ancestors
                // still, collection might contain extra info, so inject that
                if (it.parentCollection() == col) {
                    const Collection oldParent = it.parentCollection();
                    if (oldParent.parentCollection().isValid() && !col.parentCollection().isValid()) {
                        col.setParentCollection(oldParent.parentCollection());   // preserve ancestor chain
                    }
                    it.setParentCollection(col);
                } else {
                    // If one client does a modify followed by a move we have to make sure that the
                    // AgentBase::itemChanged() in another client always sees the parent collection
                    // of the item before it has been moved.
                    if (msg.operation() != Protocol::ItemChangeNotification::Move) {
                        it.setParentCollection(col);
                    } else {
                        it.setParentCollection(colDest);
                    }
                }
            }
            iter.setValue(it);
            msgItems.remove(it.id());
        } else {
            // remove the invalid item
            iter.remove();
        }
    }

    its.reserve(its.size() + msgItems.size());
    // Now reconstruct any items there were left in msgItems
    Q_FOREACH (const Protocol::ItemChangeNotification::Item &msgItem, msgItems) {
        Item it(msgItem.id);
        it.setRemoteId(msgItem.remoteId);
        it.setRemoteRevision(msgItem.remoteRevision);
        it.setMimeType(msgItem.mimeType);
        if (msg.operation() == Protocol::ItemChangeNotification::Move) {
            it.setParentCollection(colDest);
        } else {
            it.setParentCollection(col);
        }
        its << it;
    }

    bool handled = false;
    switch (msg.operation()) {
    case Protocol::ItemChangeNotification::Add:
        if (q_ptr->receivers(SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemAdded(its.first(), col);
            return true;
        }
        return false;
    case Protocol::ItemChangeNotification::Modify:
        if (q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemChanged(its.first(), msg.itemParts());
            return true;
        }
        return false;
    case Protocol::ItemChangeNotification::ModifyFlags:
        if (q_ptr->receivers(SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>))) > 0) {
            emit q_ptr->itemsFlagsChanged(its, msg.addedFlags(), msg.removedFlags());
            handled = true;
        }
        return handled;
    case Protocol::ItemChangeNotification::Move:
        if (q_ptr->receivers(SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemMoved(its.first(), col, colDest);
            handled = true;
        }
        if (q_ptr->receivers(SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection))) > 0) {
            emit q_ptr->itemsMoved(its, col, colDest);
            handled = true;
        }
        return handled;
    case Protocol::ItemChangeNotification::Remove:
        if (q_ptr->receivers(SIGNAL(itemRemoved(Akonadi::Item))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemRemoved(its.first());
            handled = true;
        }
        if (q_ptr->receivers(SIGNAL(itemsRemoved(Akonadi::Item::List))) > 0) {
            emit q_ptr->itemsRemoved(its);
            handled = true;
        }
        return handled;
    case Protocol::ItemChangeNotification::Link:
        if (q_ptr->receivers(SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemLinked(its.first(), col);
            handled = true;
        }
        if (q_ptr->receivers(SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection))) > 0) {
            emit q_ptr->itemsLinked(its, col);
            handled = true;
        }
        return handled;
    case Protocol::ItemChangeNotification::Unlink:
        if (q_ptr->receivers(SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemUnlinked(its.first(), col);
            handled = true;
        }
        if (q_ptr->receivers(SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection))) > 0) {
            emit q_ptr->itemsUnlinked(its, col);
            handled = true;
        }
        return handled;
    case Protocol::ItemChangeNotification::ModifyTags:
        if (q_ptr->receivers(SIGNAL(itemsTagsChanged(Akonadi::Item::List,QSet<Akonadi::Tag>,QSet<Akonadi::Tag>))) > 0) {
            emit q_ptr->itemsTagsChanged(its, Akonadi::vectorToSet(addedTags), Akonadi::vectorToSet(removedTags));
            return true;
        }
        return false;
    case Protocol::ItemChangeNotification::ModifyRelations:
        if (q_ptr->receivers(SIGNAL(itemsRelationsChanged(Akonadi::Item::List,Akonadi::Relation::List,Akonadi::Relation::List))) > 0) {
            emit q_ptr->itemsRelationsChanged(its, addedRelations, removedRelations);
            return true;
        }
        return false;
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in item change notification";
    }

    return false;
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
    if (!collection.isValid() || msg.operation() == Protocol::CollectionChangeNotification::Remove) {
        collection = Collection(msg.id());
        collection.setResource(QString::fromUtf8(msg.resource()));
        collection.setRemoteId(msg.remoteId());
    }

    if (!collection.parentCollection().isValid()) {
        if (msg.operation() == Protocol::CollectionChangeNotification::Move) {
            collection.setParentCollection(destination);
        } else {
            collection.setParentCollection(parent);
        }
    }

    switch (msg.operation()) {
    case Protocol::CollectionChangeNotification::Add:
        if (q_ptr->receivers(SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection))) == 0) {
            return false;
        }
        emit q_ptr->collectionAdded(collection, parent);
        return true;
    case Protocol::CollectionChangeNotification::Modify:
        if (q_ptr->receivers(SIGNAL(collectionChanged(Akonadi::Collection))) == 0
                && q_ptr->receivers(SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>))) == 0) {
            return false;
        }
        emit q_ptr->collectionChanged(collection);
        emit q_ptr->collectionChanged(collection, msg.changedParts());
        return true;
    case Protocol::CollectionChangeNotification::Move:
        if (q_ptr->receivers(SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection))) == 0) {
            return false;
        }
        emit q_ptr->collectionMoved(collection, parent, destination);
        return true;
    case Protocol::CollectionChangeNotification::Remove:
        if (q_ptr->receivers(SIGNAL(collectionRemoved(Akonadi::Collection))) == 0) {
            return false;
        }
        emit q_ptr->collectionRemoved(collection);
        return true;
    case Protocol::CollectionChangeNotification::Subscribe:
        if (q_ptr->receivers(SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection))) == 0) {
            return false;
        }
        if (!monitorAll) {  // ### why??
            emit q_ptr->collectionSubscribed(collection, parent);
        }
        return true;
    case Protocol::CollectionChangeNotification::Unsubscribe:
        if (q_ptr->receivers(SIGNAL(collectionUnsubscribed(Akonadi::Collection))) == 0) {
            return false;
        }
        if (!monitorAll) {  // ### why??
            emit q_ptr->collectionUnsubscribed(collection);
        }
        return true;
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in collection change notification";
    }

    return false;
}

bool MonitorPrivate::emitTagNotification(const Protocol::TagChangeNotification &msg, const Tag &tag)
{
    Tag validTag;
    if (msg.operation() == Protocol::TagChangeNotification::Remove) {
        //In case of a removed signal the cache entry was already invalidated, and we therefore received an empty list of tags
        validTag = Tag(msg.id());
        validTag.setRemoteId(msg.remoteId().toLatin1());
    } else {
        validTag = tag;
    }

    if (!validTag.isValid()) {
        return false;
    }

    switch (msg.operation()) {
    case Protocol::TagChangeNotification::Add:
        if (q_ptr->receivers(SIGNAL(tagAdded(Akonadi::Tag))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->tagAdded(validTag);
        return true;
    case Protocol::TagChangeNotification::Modify:
        if (q_ptr->receivers(SIGNAL(tagChanged(Akonadi::Tag))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->tagChanged(validTag);
        return true;
    case Protocol::TagChangeNotification::Remove:
        if (q_ptr->receivers(SIGNAL(tagRemoved(Akonadi::Tag))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->tagRemoved(validTag);
        return true;
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in tag change notification";
    }

    return false;
}

bool MonitorPrivate::emitRelationNotification(const Protocol::RelationChangeNotification &msg, const Relation &relation)
{
    if (!relation.isValid()) {
        return false;
    }

    switch (msg.operation()) {
    case Protocol::RelationChangeNotification::Add:
        if (q_ptr->receivers(SIGNAL(relationAdded(Akonadi::Relation))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->relationAdded(relation);
        return true;
    case Protocol::RelationChangeNotification::Remove:
        if (q_ptr->receivers(SIGNAL(relationRemoved(Akonadi::Relation))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->relationRemoved(relation);
        return true;
    default:
        qCDebug(AKONADICORE_LOG) << "Unknown operation type" << msg.operation() << "in tag change notification";
    }

    return false;
}

bool MonitorPrivate::emitSubscriptionChangeNotification(const Protocol::SubscriptionChangeNotification &msg,
        const Akonadi::NotificationSubscriber &subscriber)
{
    if (!subscriber.isValid()) {
        return false;
    }

    switch (msg.operation()) {
    case Protocol::SubscriptionChangeNotification::Add:
        if (q_ptr->receivers(SIGNAL(notificationSubscriberAdded(Akonadi::NotificationSubscriber))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->notificationSubscriberAdded(subscriber);
        return true;
    case Protocol::SubscriptionChangeNotification::Modify:
        if (q_ptr->receivers(SIGNAL(notificationSubscriberChanged(Akonadi::NotificationSubscriber))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->notificationSubscriberChanged(subscriber);
        return true;
    case Protocol::SubscriptionChangeNotification::Remove:
        if (q_ptr->receivers(SIGNAL(notificationSubscriberRemoved(Akonadi::NotificationSubscriber))) == 0) {
            return false;
        }
        Q_EMIT q_ptr->notificationSubscriberRemoved(subscriber);
        return true;
    default:
        break;
    }

    return false;
}

bool MonitorPrivate::emitDebugChangeNotification(const Protocol::DebugChangeNotification &msg,
        const ChangeNotification &ntf)
{
    Q_UNUSED(msg);

    if (!ntf.isValid()) {
        return false;
    }

    if (q_ptr->receivers(SIGNAL(debugNotification(Akonadi::ChangeNotification))) == 0) {
        return false;
    }
    Q_EMIT q_ptr->debugNotification(ntf);
    return true;
}

void MonitorPrivate::invalidateCaches(const Protocol::ChangeNotification &msg)
{
    // remove invalidates
    // modify removes the cache entry, as we need to re-fetch
    // And subscription modify the visibility of the collection by the collectionFetchScope.
    switch (msg.type()) {
    case Protocol::Command::CollectionChangeNotification: {
        const auto &colNtf = static_cast<const Protocol::CollectionChangeNotification &>(msg);
        switch (colNtf.operation()) {
        case Protocol::CollectionChangeNotification::Modify:
        case Protocol::CollectionChangeNotification::Move:
        case Protocol::CollectionChangeNotification::Subscribe:
            collectionCache->update(colNtf.id(), mCollectionFetchScope);
            break;
        case Protocol::CollectionChangeNotification::Remove:
            collectionCache->invalidate(colNtf.id());
            break;
        default:
            break;
        }
    } break;
    case Protocol::Command::ItemChangeNotification: {
        const auto &itemNtf = static_cast<const Protocol::ItemChangeNotification &>(msg);
        switch (itemNtf.operation()) {
        case Protocol::ItemChangeNotification::Modify:
        case Protocol::ItemChangeNotification::ModifyFlags:
        case Protocol::ItemChangeNotification::ModifyTags:
        case Protocol::ItemChangeNotification::ModifyRelations:
        case Protocol::ItemChangeNotification::Move:
            itemCache->update(itemNtf.uids(), mItemFetchScope);
            break;
        case Protocol::ItemChangeNotification::Remove:
            itemCache->invalidate(itemNtf.uids());
            break;
        default:
            break;
        }
    } break;
    case Protocol::Command::TagChangeNotification: {
        const auto &tagNtf = static_cast<const Protocol::TagChangeNotification &>(msg);
        switch (tagNtf.operation()) {
        case Protocol::TagChangeNotification::Modify:
            tagCache->update({ tagNtf.id() }, mTagFetchScope);
            break;
        case Protocol::TagChangeNotification::Remove:
            tagCache->invalidate({ tagNtf.id() });
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

// @endcond
