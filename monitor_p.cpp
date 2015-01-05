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
#include "dbusconnectionpool.h"
#include "itemfetchjob.h"
#include "notificationmessagev2_p.h"
#include "notificationmanagerinterface.h"
#include "session.h"
#include "changemediator_p.h"

#include <kdebug.h>
#include <kcomponentdata.h>

using namespace Akonadi;

static const int PipelineSize = 5;

MonitorPrivate::MonitorPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_, Monitor *parent)
    : q_ptr(parent)
    , dependenciesFactory(dependenciesFactory_ ? dependenciesFactory_ : new ChangeNotificationDependenciesFactory)
    , notificationSource(0)
    , monitorAll(false)
    , exclusive(false)
    , mFetchChangedOnly(false)
    , session(Session::defaultSession())
    , collectionCache(0)
    , itemCache(0)
    , tagCache(0)
    , fetchCollection(false)
    , fetchCollectionStatistics(false)
    , collectionMoveTranslationEnabled(true)
    , useRefCounting(false)
{
}

void MonitorPrivate::init()
{
    // needs to be at least 3x pipeline size for the collection move case
    collectionCache = dependenciesFactory->createCollectionCache(3 * PipelineSize, session);
    // needs to be at least 1x pipeline size
    itemCache = dependenciesFactory->createItemListCache(PipelineSize, session);

    // 20 tags looks like a reasonable mount to keep around
    tagCache = dependenciesFactory->createTagListCache(20, session);

    QObject::connect(collectionCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()));
    QObject::connect(itemCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()));
    QObject::connect(tagCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()));
    QObject::connect(ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
                     q_ptr, SLOT(serverStateChanged(Akonadi::ServerManager::State)));

    NotificationMessageV2::registerDBusTypes();
    NotificationMessageV3::registerDBusTypes();

    statisticsCompressionTimer.setSingleShot(true);
    statisticsCompressionTimer.setInterval(500);
    QObject::connect(&statisticsCompressionTimer, SIGNAL(timeout()), q_ptr, SLOT(slotFlushRecentlyChangedCollections()));
}

bool MonitorPrivate::connectToNotificationManager()
{
    delete notificationSource;
    notificationSource = 0;

    notificationSource = dependenciesFactory->createNotificationSource(q_ptr);

    if (!notificationSource) {
        return false;
    }

    QObject::connect(notificationSource, SIGNAL(notifyV3(Akonadi::NotificationMessageV3::List)),
                     q_ptr, SLOT(slotNotify(Akonadi::NotificationMessageV3::List)));

    notificationSource->setSession(session->sessionId());

    return true;
}

void MonitorPrivate::serverStateChanged(ServerManager::State state)
{
    if (state == ServerManager::Running) {
        connectToNotificationManager();
        notificationSource->setAllMonitored(monitorAll);
        notificationSource->setSession(session->sessionId());
        Q_FOREACH (const Collection &col, collections) {
            notificationSource->setMonitoredCollection(col.id(), true);
        }
        Q_FOREACH (const Entity::Id id, items) {
            notificationSource->setMonitoredItem(id, true);
        }
        Q_FOREACH (const QByteArray &resource, resources) {
            notificationSource->setMonitoredResource(resource, true);
        }
        Q_FOREACH (const QByteArray &session, sessions) {
            notificationSource->setIgnoredSession(session, true);
        }
        Q_FOREACH (const QString &mimeType, mimetypes) {
            notificationSource->setMonitoredMimeType(mimeType, true);
        }
        Q_FOREACH (Tag::Id tagId, tags) {
            notificationSource->setMonitoredTag(tagId, true);
        }
        Q_FOREACH (Monitor::Type type, types) {
            notificationSource->setMonitoredType(static_cast<NotificationMessageV2::Type>(type), true);
        }
        notificationSource->setExclusive(exclusive);
    }
}

void MonitorPrivate::invalidateCollectionCache(qint64 id)
{
    collectionCache->update(id, mCollectionFetchScope);
}

void MonitorPrivate::invalidateItemCache(qint64 id)
{
    itemCache->update(QList<Entity::Id>() << id, mItemFetchScope);
}

void MonitorPrivate::invalidateTagCache(qint64 id)
{
    tagCache->update(QList<Tag::Id>() << id, mTagFetchScope);
}

int MonitorPrivate::pipelineSize() const
{
    return PipelineSize;
}

bool MonitorPrivate::isLazilyIgnored(const NotificationMessageV3 &msg, bool allowModifyFlagsConversion) const
{
    NotificationMessageV2::Operation op = msg.operation();

    if (msg.type() == NotificationMessageV2::Tags
        && ((op == NotificationMessageV2::Add && q_ptr->receivers(SIGNAL(tagAdded(Akonadi::Tag))) == 0)
            || (op == NotificationMessageV2::Modify && q_ptr->receivers(SIGNAL(tagChanged(Akonadi::Tag))) == 0)
            || (op == NotificationMessageV2::Remove && q_ptr->receivers(SIGNAL(tagRemoved(Akonadi::Tag))) == 0))) {
        return true;
    }

    if (!fetchCollectionStatistics
        && (msg.type() == NotificationMessageV2::Items)
        && ((op == NotificationMessageV2::Add && q_ptr->receivers(SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection))) == 0)
             || (op == NotificationMessageV2::Remove && q_ptr->receivers(SIGNAL(itemRemoved(Akonadi::Item))) == 0
                 && q_ptr->receivers(SIGNAL(itemsRemoved(Akonadi::Item::List))) == 0)
             || (op == NotificationMessageV2::Modify && q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) == 0)
             || (op == NotificationMessageV2::ModifyFlags
                 && (q_ptr->receivers(SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>))) == 0
                     // Newly delivered ModifyFlags notifications will be converted to
                     // itemChanged(item, "FLAGS") for legacy clients.
                     && (!allowModifyFlagsConversion || q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) == 0)))
             || (op == NotificationMessageV2::ModifyTags && q_ptr->receivers(SIGNAL(itemsTagsChanged(Akonadi::Item::List,QSet<Akonadi::Tag>,QSet<Akonadi::Tag>))) == 0)
             || (op == NotificationMessageV2::Move && q_ptr->receivers(SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection))) == 0
                 && q_ptr->receivers(SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection))) == 0)
             || (op == NotificationMessageV2::Link && q_ptr->receivers(SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection))) == 0
                 && q_ptr->receivers(SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection))) == 0)
             || (op == NotificationMessageV2::Unlink && q_ptr->receivers(SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection))) == 0
                 && q_ptr->receivers(SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection))) == 0))) {
        return true;
    }

    if (!useRefCounting) {
        return false;
    }

    if (msg.type() == NotificationMessageV2::Collections) {
        // Lazy fetching can only affects items.
        return false;
    }

    Collection::Id parentCollectionId = msg.parentCollection();

    if ((op == NotificationMessageV2::Add)
        || (op == NotificationMessageV2::Remove)
        || (op == NotificationMessageV2::Modify)
        || (op == NotificationMessageV2::ModifyFlags)
        || (op == NotificationMessageV2::ModifyTags)
        || (op == NotificationMessageV2::Link)
        || (op == NotificationMessageV2::Unlink)) {
        if (isMonitored(parentCollectionId)) {
            return false;
        }
    }

    if (op == NotificationMessageV2::Move) {
        if (!isMonitored(parentCollectionId) && !isMonitored(msg.parentDestCollection())) {
            return true;
        }
        // We can't ignore the move. It must be transformed later into a removal or insertion.
        return false;
    }
    return true;
}

void MonitorPrivate::checkBatchSupport(const NotificationMessageV3 &msg, bool &needsSplit, bool &batchSupported) const
{
    const bool isBatch = (msg.entities().count() > 1);

    if (msg.type() == NotificationMessageV2::Items) {
        switch (msg.operation()) {
        case NotificationMessageV2::Add:
            needsSplit = isBatch;
            batchSupported = false;
            return;
        case NotificationMessageV2::Modify:
            needsSplit = isBatch;
            batchSupported = false;
            return;
        case NotificationMessageV2::ModifyFlags:
            batchSupported = q_ptr->receivers(SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>))) > 0;
            needsSplit = isBatch && !batchSupported && q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) > 0;
            return;
        case NotificationMessageV2::ModifyTags:
            // Tags were added after batch notifications, so they are always supported
            batchSupported = true;
            needsSplit = false;
            return;
        case NotificationMessageV2::ModifyRelations:
            // Relations were added after batch notifications, so they are always supported
            batchSupported = true;
            needsSplit = false;
            return;
        case NotificationMessageV2::Move:
            needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection))) > 0;
            batchSupported = q_ptr->receivers(SIGNAL(itemsMoved(Akonadi::Item::List,Akonadi::Collection,Akonadi::Collection))) > 0;
            return;
        case NotificationMessageV2::Remove:
            needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemRemoved(Akonadi::Item))) > 0;
            batchSupported = q_ptr->receivers(SIGNAL(itemsRemoved(Akonadi::Item::List))) > 0;
            return;
        case NotificationMessageV2::Link:
            needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection))) > 0;
            batchSupported = q_ptr->receivers(SIGNAL(itemsLinked(Akonadi::Item::List,Akonadi::Collection))) > 0;
            return;
        case NotificationMessageV2::Unlink:
            needsSplit = isBatch && q_ptr->receivers(SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection))) > 0;
            batchSupported = q_ptr->receivers(SIGNAL(itemsUnlinked(Akonadi::Item::List,Akonadi::Collection))) > 0;
            return;
        default:
            needsSplit = isBatch;
            batchSupported = false;
            kDebug() << "Unknown operation type" << msg.operation() << "in item change notification";
            return;
        }
    } else if (msg.type() == NotificationMessageV2::Collections) {
        needsSplit = isBatch;
        batchSupported = false;
    } else if (msg.type() == NotificationMessageV2::Tags) {
        needsSplit = isBatch;
        batchSupported = false;
    } else if (msg.type() == NotificationMessageV2::Relations) {
        needsSplit = isBatch;
        batchSupported = false;
    }
}

NotificationMessageV3::List MonitorPrivate::splitMessage(const NotificationMessageV3 &msg, bool legacy) const
{
    NotificationMessageV3::List list;

    NotificationMessageV3 baseMsg;
    baseMsg.setSessionId(msg.sessionId());
    baseMsg.setType(msg.type());
    if (legacy && msg.operation() == NotificationMessageV2::ModifyFlags) {
        baseMsg.setOperation(NotificationMessageV2::Modify);
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

    Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
        NotificationMessageV3 copy = baseMsg;
        copy.addEntity(entity.id, entity.remoteId, entity.remoteRevision, entity.mimeType);

        list << copy;
    }

    return list;
}

bool MonitorPrivate::acceptNotification(const Akonadi::NotificationMessageV3 &msg) const
{
    // session is ignored
    if (sessions.contains(msg.sessionId())) {
        return false;
    }

    if (msg.entities().count() == 0 && msg.type() != NotificationMessageV2::Relations) {
        return false;
    }

    // user requested everything
    if (monitorAll && msg.type() != NotificationMessageV2::InvalidType) {
        return true;
    }

    // Types are monitored, but not this one
    if (!types.isEmpty() && !types.contains(static_cast<Monitor::Type>(msg.type()))) {
        return false;
    }

    switch (msg.type()) {
    case NotificationMessageV2::InvalidType:
        kWarning() << "Received invalid change notification!";
        return false;

    case NotificationMessageV2::Items:
        // we have a resource or mimetype filter
        if (!resources.isEmpty() || !mimetypes.isEmpty()) {
            if (resources.contains(msg.resource()) || isMoveDestinationResourceMonitored(msg)) {
                return true;
            }

            Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
                if (isMimeTypeMonitored(entity.mimeType)) {
                    return true;
                }
            }
            return false;
        }

        // we explicitly monitor that item or the collections it's in
        Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
            if (items.contains(entity.id)) {
                return true;
            }
        }

        return isCollectionMonitored(msg.parentCollection())
               || isCollectionMonitored(msg.parentDestCollection());

    case NotificationMessageV2::Collections:
        // we have a resource filter
        if (!resources.isEmpty()) {
            const bool resourceMatches = resources.contains(msg.resource()) || isMoveDestinationResourceMonitored(msg);
            // a bit hacky, but match the behaviour from the item case,
            // if resource is the only thing we are filtering on, stop here, and if the resource filter matched, of course
            if (mimetypes.isEmpty() || resourceMatches) {
                return resourceMatches;
            }
            // else continue
        }

        // we explicitly monitor that colleciton, or all of them
        Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
            if (isCollectionMonitored(entity.id)) {
                return true;
            }
        }
        return isCollectionMonitored(msg.parentCollection())
               || isCollectionMonitored(msg.parentDestCollection());

    case NotificationMessageV2::Tags:
        if (!tags.isEmpty()) {
            Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
                if (tags.contains(entity.id)) {
                    return true;
                }
            }
            return false;
        }
        return true;
    case NotificationMessageV2::Relations:
        return true;
    }
    Q_ASSERT(false);
    return false;
}

void MonitorPrivate::cleanOldNotifications()
{
    bool erased = false;
    for (QQueue<NotificationMessageV3>::iterator it = pipeline.begin(); it != pipeline.end();) {
        if (!acceptNotification(*it) || isLazilyIgnored(*it)) {
            it = pipeline.erase(it);
            erased = true;
        } else {
            ++it;
        }
    }

    for (QQueue<NotificationMessageV3>::iterator it = pendingNotifications.begin(); it != pendingNotifications.end();) {
        if (!acceptNotification(*it) || isLazilyIgnored(*it)) {
            it = pendingNotifications.erase(it);
            erased = true;
        } else {
            ++it;
        }
    }
    if (erased) {
        notificationsErased();
    }
}

bool MonitorPrivate::fetchCollections() const
{
    return fetchCollection;
}

bool MonitorPrivate::fetchItems() const
{
    return !mItemFetchScope.isEmpty();
}

bool MonitorPrivate::ensureDataAvailable(const NotificationMessageV3 &msg)
{
    if (msg.type() == NotificationMessageV2::Tags) {
        Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
            if (!tagCache->ensureCached(QList<Tag::Id>() << entity.id, mTagFetchScope)) {
                return false;
            }
        }
        return true;
    }
    if (msg.type() == NotificationMessageV2::Relations) {
        return true;
    }

    if (msg.operation() == NotificationMessageV2::Remove && msg.type() == NotificationMessageV2::Collections) {
        //For collection removals the collection is gone anyways, so we can't fetch it. Rid will be set later on instead.
        return true;
    }

    bool allCached = true;
    if (fetchCollections()) {
        if (!collectionCache->ensureCached(msg.parentCollection(), mCollectionFetchScope)) {
            allCached = false;
        }
        if (msg.operation() == NotificationMessageV2::Move && !collectionCache->ensureCached(msg.parentDestCollection(), mCollectionFetchScope)) {
            allCached = false;
        }
    }
    if (msg.operation() == NotificationMessageV2::Remove) {
        return allCached; // the actual object is gone already, nothing to fetch there
    }

    if (msg.type() == NotificationMessageV2::Items && fetchItems()) {
        ItemFetchScope scope(mItemFetchScope);
        if (mFetchChangedOnly && (msg.operation() == NotificationMessageV2::Modify || msg.operation() == NotificationMessageV2::ModifyFlags)) {
            bool fullPayloadWasRequested = scope.fullPayload();
            scope.fetchFullPayload(false);
            QSet<QByteArray> requestedPayloadParts = scope.payloadParts();
            Q_FOREACH (const QByteArray &part, requestedPayloadParts) {
                scope.fetchPayloadPart(part, false);
            }

            bool allAttributesWereRequested = scope.allAttributes();
            QSet<QByteArray> requestedAttrParts = scope.attributes();
            Q_FOREACH (const QByteArray &part, requestedAttrParts) {
                scope.fetchAttribute(part, false);
            }

            QSet<QByteArray> changedParts = msg.itemParts();
            Q_FOREACH (const QByteArray &part, changedParts)  {
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
        if (!itemCache->ensureCached(msg.uids(), scope)) {
            allCached = false;

        }

        // Make sure all tags for ModifyTags operation are in cache too
        if (msg.operation() == NotificationMessageV2::ModifyTags) {
            if (!tagCache->ensureCached((msg.addedTags() + msg.removedTags()).toList(), mTagFetchScope)) {
                allCached = false;
            }
        }

    } else if (msg.type() == NotificationMessageV2::Collections && fetchCollections()) {
        Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
            if (!collectionCache->ensureCached(entity.id, mCollectionFetchScope)) {
                allCached = false;
                break;
            }
        }
    }
    return allCached;
}

bool MonitorPrivate::emitNotification(const NotificationMessageV3 &msg)
{
    bool someoneWasListening = false;
    bool shouldCleanOldNotifications = false;
    if (msg.type() == NotificationMessageV2::Tags) {
        //In case of a Remove notification this will return a list of invalid entities (we'll deal later with them)
        const Tag::List tags = tagCache->retrieve(msg.uids());
        someoneWasListening = emitTagsNotification(msg, tags);
        shouldCleanOldNotifications = !someoneWasListening;
    } else if (msg.type() == NotificationMessageV2::Relations) {
        Relation rel;
        Q_FOREACH (const QByteArray & part, msg.itemParts()) {
            QList<QByteArray> splitPart = part.split(' ');
            Q_ASSERT(splitPart.size() == 2);
            if (splitPart.first() == "LEFT") {
                rel.setLeft(Akonadi::Item(splitPart.at(1).toLongLong()));
            } else if (splitPart.first() == "RIGHT") {
                rel.setRight(Akonadi::Item(splitPart.at(1).toLongLong()));
            } else if (splitPart.first() == "TYPE") {
                rel.setType(splitPart.at(1));
            } else if (splitPart.first() == "RID") {
                rel.setRemoteId(splitPart.at(1));
            }
        }
        someoneWasListening = emitRelationsNotification(msg, Relation::List() << rel);
        shouldCleanOldNotifications = !someoneWasListening;
    } else {
        const Collection parent = collectionCache->retrieve(msg.parentCollection());
        Collection destParent;
        if (msg.operation() == NotificationMessageV2::Move) {
            destParent = collectionCache->retrieve(msg.parentDestCollection());
        }

        if (msg.type() == NotificationMessageV2::Collections) {
            Collection col;
            Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
                //For removals this will retrieve an invalid collection. We'll deal with that in emitCollectionNotification
                col = collectionCache->retrieve(entity.id);
                //It is possible that the retrieval fails also in the non-removal case (e.g. because the item was meanwhile removed while
                //the changerecorder stored the notification or the notification was in the queue). In order to drop such invalid notifications we have to ignore them.
                if (col.isValid() || msg.operation() == NotificationMessageV2::Remove || !fetchCollections()) {
                    someoneWasListening = emitCollectionNotification(msg, col, parent, destParent);
                    shouldCleanOldNotifications = !someoneWasListening;
                }
            }
        } else if (msg.type() == NotificationMessageV2::Items) {
            //For removals this will retrieve an empty set. We'll deal with that in emitItemNotification
            const Item::List items = itemCache->retrieve(msg.uids());
            //It is possible that the retrieval fails also in the non-removal case (e.g. because the item was meanwhile removed while
            //the changerecorder stored the notification or the notification was in the queue). In order to drop such invalid notifications we have to ignore them.
            if (!items.isEmpty() || msg.operation() == NotificationMessageV2::Remove || !fetchItems()) {
                someoneWasListening = emitItemsNotification(msg, items, parent, destParent);
                shouldCleanOldNotifications = !someoneWasListening;
            }
        }
    }

    if (shouldCleanOldNotifications) {
        cleanOldNotifications(); // probably someone disconnected a signal in the meantime, get rid of the no longer interesting stuff
    }

    return someoneWasListening;
}

void MonitorPrivate::updatePendingStatistics(const NotificationMessageV3 &msg)
{
    if (msg.type() == NotificationMessageV2::Items) {
        notifyCollectionStatisticsWatchers(msg.parentCollection(), msg.resource());
        // FIXME use the proper resource of the target collection, for cross resource moves
        notifyCollectionStatisticsWatchers(msg.parentDestCollection(), msg.destinationResource());
    } else if (msg.type() == NotificationMessageV2::Collections && msg.operation() == NotificationMessageV2::Remove) {
        // no need for statistics updates anymore
        Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
            recentlyChangedCollections.remove(entity.id);
        }
    }
}

void MonitorPrivate::slotSessionDestroyed(QObject *object)
{
    Session *objectSession = qobject_cast<Session *>(object);
    if (objectSession) {
        sessions.removeAll(objectSession->sessionId());
        if (notificationSource) {
            notificationSource->setIgnoredSession(objectSession->sessionId(), false);
        }
    }
}

void MonitorPrivate::slotStatisticsChangedFinished(KJob *job)
{
    if (job->error()) {
        kWarning() << "Error on fetching collection statistics: " << job->errorText();
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

int MonitorPrivate::translateAndCompress(QQueue< NotificationMessageV3 > &notificationQueue, const NotificationMessageV3 &msg)
{
    // We have to split moves into insert or remove if the source or destination
    // is not monitored.
    if (msg.operation() != NotificationMessageV2::Move) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    // Always handle tags
    if (msg.type() == NotificationMessageV2::Tags) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    bool sourceWatched = false;
    bool destWatched = false;

    if (useRefCounting && msg.type() == NotificationMessageV2::Items) {
        sourceWatched = isMonitored(msg.parentCollection());
        destWatched = isMonitored(msg.parentDestCollection());
    } else {
        if (!resources.isEmpty()) {
            sourceWatched = resources.contains(msg.resource());
            destWatched = isMoveDestinationResourceMonitored(msg);
        }
        if (!sourceWatched) {
            sourceWatched = isCollectionMonitored(msg.parentCollection());
        }
        if (!destWatched) {
            destWatched = isCollectionMonitored(msg.parentDestCollection());
        }
    }

    if (!sourceWatched && !destWatched) {
        return 0;
    }

    if ((sourceWatched && destWatched) || (!collectionMoveTranslationEnabled && msg.type() == NotificationMessageV2::Collections)) {
        notificationQueue.enqueue(msg);
        return 1;
    }

    if (sourceWatched) {
        // Transform into a removal
        NotificationMessageV3 removalMessage = msg;
        removalMessage.setOperation(NotificationMessageV2::Remove);
        removalMessage.setParentDestCollection(-1);
        notificationQueue.enqueue(removalMessage);
        return 1;
    }

    // Transform into an insertion
    NotificationMessageV3 insertionMessage = msg;
    insertionMessage.setOperation(NotificationMessageV2::Add);
    insertionMessage.setParentCollection(msg.parentDestCollection());
    insertionMessage.setParentDestCollection(-1);
    // We don't support batch insertion, so we have to do it one by one
    const NotificationMessageV3::List split = splitMessage(insertionMessage, false);
    Q_FOREACH (const NotificationMessageV3 &insertion, split) {
        notificationQueue.enqueue(insertion);
    }
    return split.count();
}

/*

  server notification --> ?accepted --> pendingNotifications --> ?dataAvailable --> emit
                                  |                                           |
                                  x --> discard                               x --> pipeline

  fetchJobDone --> pipeline ?dataAvailable --> emit
 */

void MonitorPrivate::slotNotify(const NotificationMessageV3::List &msgs)
{
    int appendedMessages = 0;
    int modifiedMessages = 0;
    int erasedMessages = 0;
    Q_FOREACH (const NotificationMessageV3 &msg, msgs) {
        invalidateCaches(msg);
        updatePendingStatistics(msg);
        bool needsSplit = true;
        bool supportsBatch = false;

        if (isLazilyIgnored(msg, true)) {
            continue;
        }

        checkBatchSupport(msg, needsSplit, supportsBatch);

        if (supportsBatch
            || (!needsSplit && !supportsBatch && msg.operation() != NotificationMessageV2::ModifyFlags)
            || msg.type() == NotificationMessageV2::Collections) {
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
            itemCache->ensureCached(msg.uids(), mItemFetchScope);
        }

        // if the message contains more items, but we need to emit single-item notification,
        // split the message into one message per item and queue them
        // if the message contains only one item, but batches are not supported
        // (and thus neither is flagsModified), splitMessage() will convert the
        // notification to regular Modify with "FLAGS" part changed
        if (needsSplit || (!needsSplit && !supportsBatch && msg.operation() == Akonadi::NotificationMessageV2::ModifyFlags)) {
            const NotificationMessageV3::List split = splitMessage(msg, !supportsBatch);
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
        const NotificationMessageV3 msg = pipeline.head();
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
        const NotificationMessageV3 msg = pendingNotifications.dequeue();
        if (ensureDataAvailable(msg) && pipeline.isEmpty()) {
            emitNotification(msg);
        } else {
            pipeline.enqueue(msg);
        }
    }
}

static Relation::List extractRelations(QSet<QByteArray> &flags)
{
    Relation::List relations;
    Q_FOREACH (const QByteArray &flag, flags) {
        if (flag.startsWith("RELATION")) {
            flags.remove(flag);
            const QList<QByteArray> parts = flag.split(' ');
            Q_ASSERT(parts.size() == 4);
            relations << Relation(parts[1], Item(parts[2].toLongLong()), Item(parts[3].toLongLong()));
        }
    }
    return relations;
}

bool MonitorPrivate::emitItemsNotification(const NotificationMessageV3 &msg_, const Item::List &items, const Collection &collection, const Collection &collectionDest)
{
    NotificationMessageV3 msg = msg_;
    Q_ASSERT(msg.type() == NotificationMessageV2::Items);
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
            colDest.setResource(QString::fromLatin1(*(msg.itemParts().begin())));
        }
    }

    Relation::List addedRelations, removedRelations;
    if (msg.operation() == NotificationMessageV2::ModifyRelations) {
        QSet<QByteArray> addedFlags = msg.addedFlags();
        addedRelations = extractRelations(addedFlags);
        msg.setAddedFlags(addedFlags);

        QSet<QByteArray> removedFlags = msg.removedFlags();
        removedRelations = extractRelations(removedFlags);
        msg.setRemovedFlags(removedFlags);
    }

    Tag::List addedTags, removedTags;
    if (msg.operation() == NotificationMessageV2::ModifyTags) {
        addedTags = tagCache->retrieve(msg.addedTags().toList());
        removedTags = tagCache->retrieve(msg.removedTags().toList());
    }

    QMap<NotificationMessageV2::Id, NotificationMessageV2::Entity> msgEntities = msg.entities();
    Item::List its = items;
    QMutableListIterator<Item> iter(its);
    while (iter.hasNext()) {
        Item it = iter.next();
        if (it.isValid()) {
            const NotificationMessageV2::Entity msgEntity = msgEntities[it.id()];
            if (msg.operation() == NotificationMessageV2::Remove) {
                it.setRemoteId(msgEntity.remoteId);
                it.setRemoteRevision(msgEntity.remoteRevision);
                it.setMimeType(msgEntity.mimeType);
            }

            if (!it.parentCollection().isValid()) {
                if (msg.operation() == NotificationMessageV2::Move) {
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
                    if (msg.operation() != NotificationMessageV2::Move) {
                        it.setParentCollection(col);
                    }
                }
            }
            iter.setValue(it);
            msgEntities.remove(it.id());
        } else {
            // remove the invalid item
            iter.remove();
        }
    }

    // Now reconstruct any items there were left in msgItems
    Q_FOREACH (const NotificationMessageV2::Entity &msgItem, msgEntities) {
        Item it(msgItem.id);
        it.setRemoteId(msgItem.remoteId);
        it.setRemoteRevision(msgItem.remoteRevision);
        it.setMimeType(msgItem.mimeType);
        if (msg.operation() == NotificationMessageV2::Move) {
            it.setParentCollection(colDest);
        } else {
            it.setParentCollection(col);
        }
        its << it;
    }

    bool handled = false;
    switch (msg.operation()) {
    case NotificationMessageV2::Add:
        if (q_ptr->receivers(SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemAdded(its.first(), col);
            return true;
        }
        return false;
    case NotificationMessageV2::Modify:
        if (q_ptr->receivers(SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>))) > 0) {
            Q_ASSERT(its.count() == 1);
            emit q_ptr->itemChanged(its.first(), msg.itemParts());
            return true;
        }
        return false;
    case NotificationMessageV2::ModifyFlags:
        if (q_ptr->receivers(SIGNAL(itemsFlagsChanged(Akonadi::Item::List,QSet<QByteArray>,QSet<QByteArray>))) > 0) {
            emit q_ptr->itemsFlagsChanged(its, msg.addedFlags(), msg.removedFlags());
            handled = true;
        }
        return handled;
    case NotificationMessageV2::Move:
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
    case NotificationMessageV2::Remove:
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
    case NotificationMessageV2::Link:
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
    case NotificationMessageV2::Unlink:
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
    case NotificationMessageV2::ModifyTags:
        if (q_ptr->receivers(SIGNAL(itemsTagsChanged(Akonadi::Item::List,QSet<Akonadi::Tag>,QSet<Akonadi::Tag>))) > 0) {
            emit q_ptr->itemsTagsChanged(its, addedTags.toSet(), removedTags.toSet());
            return true;
        }
        return false;
    case NotificationMessageV2::ModifyRelations:
        if (q_ptr->receivers(SIGNAL(itemsRelationsChanged(Akonadi::Item::List,Akonadi::Relation::List,Akonadi::Relation::List))) > 0) {
            emit q_ptr->itemsRelationsChanged(its, addedRelations, removedRelations);
            return true;
        }
        return false;
    default:
        kDebug() << "Unknown operation type" << msg.operation() << "in item change notification";
    }

    return false;
}

bool MonitorPrivate::emitCollectionNotification(const NotificationMessageV3 &msg, const Collection &col, const Collection &par, const Collection &dest)
{
    Q_ASSERT(msg.type() == NotificationMessageV2::Collections);
    Collection parent = par;
    if (!parent.isValid()) {
        parent = Collection(msg.parentCollection());
    }
    Collection destination = dest;
    if (!destination.isValid()) {
        destination = Collection(msg.parentDestCollection());
    }

    Collection collection = col;
    NotificationMessageV2::Entity msgEntities = msg.entities().values().first();
    if (!collection.isValid() || msg.operation() == NotificationMessageV2::Remove) {
        collection = Collection(msgEntities.id);
        collection.setResource(QString::fromUtf8(msg.resource()));
        collection.setRemoteId(msgEntities.remoteId);
    }

    if (!collection.parentCollection().isValid()) {
        if (msg.operation() == NotificationMessageV2::Move) {
            collection.setParentCollection(destination);
        } else {
            collection.setParentCollection(parent);
        }
    }

    switch (msg.operation()) {
    case NotificationMessageV2::Add:
        if (q_ptr->receivers(SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection))) == 0) {
            return false;
        }
        emit q_ptr->collectionAdded(collection, parent);
        return true;
    case NotificationMessageV2::Modify:
        if (q_ptr->receivers(SIGNAL(collectionChanged(Akonadi::Collection))) == 0
            && q_ptr->receivers(SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>))) == 0) {
            return false;
        }
        emit q_ptr->collectionChanged(collection);
        emit q_ptr->collectionChanged(collection, msg.itemParts());
        return true;
    case NotificationMessageV2::Move:
        if (q_ptr->receivers(SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection))) == 0) {
            return false;
        }
        emit q_ptr->collectionMoved(collection, parent, destination);
        return true;
    case NotificationMessageV2::Remove:
        if (q_ptr->receivers(SIGNAL(collectionRemoved(Akonadi::Collection))) == 0) {
            return false;
        }
        emit q_ptr->collectionRemoved(collection);
        return true;
    case NotificationMessageV2::Subscribe:
        if (q_ptr->receivers(SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection))) == 0) {
            return false;
        }
        if (!monitorAll) {  // ### why??
            emit q_ptr->collectionSubscribed(collection, parent);
        }
        return true;
    case NotificationMessageV2::Unsubscribe:
        if (q_ptr->receivers(SIGNAL(collectionUnsubscribed(Akonadi::Collection))) == 0) {
            return false;
        }
        if (!monitorAll) {  // ### why??
            emit q_ptr->collectionUnsubscribed(collection);
        }
        return true;
    default:
        kDebug() << "Unknown operation type" << msg.operation() << "in collection change notification";
    }

    return false;
}

bool MonitorPrivate::emitTagsNotification(const NotificationMessageV3 &msg, const Tag::List &tags)
{
    Q_ASSERT(msg.type() == NotificationMessageV2::Tags);

    Tag::List validTags;
    if (msg.operation() == NotificationMessageV2::Remove) {
        //In case of a removed signal the cache entry was already invalidated, and we therefore received an empty list of tags
        Q_FOREACH (const Entity::Id &uid, msg.uids()) {
            Tag tag(uid);
            tag.setRemoteId(msg.entity(uid).remoteId.toLatin1());
            validTags << Tag(uid);
        }
    } else {
        validTags = tags;
    }

    if (validTags.isEmpty()) {
        return false;
    }

    switch (msg.operation()) {
    case NotificationMessageV2::Add:
        if (q_ptr->receivers(SIGNAL(tagAdded(Akonadi::Tag))) == 0) {
            return false;
        }
        Q_FOREACH (const Tag &tag, validTags) {
            Q_EMIT q_ptr->tagAdded(tag);
        }
        return true;
    case NotificationMessageV2::Modify:
        if (q_ptr->receivers(SIGNAL(tagChanged(Akonadi::Tag))) == 0) {
            return false;
        }
        Q_FOREACH (const Tag &tag, validTags) {
            Q_EMIT q_ptr->tagChanged(tag);
        }
        return true;
    case NotificationMessageV2::Remove:
        if (q_ptr->receivers(SIGNAL(tagRemoved(Akonadi::Tag))) == 0) {
            return false;
        }
        Q_FOREACH (const Tag &tag, validTags) {
            Q_EMIT q_ptr->tagRemoved(tag);
        }
        return true;
    default:
        kDebug() << "Unknown operation type" << msg.operation() << "in tag change notification";
    }

    return false;
}

bool MonitorPrivate::emitRelationsNotification(const NotificationMessageV3 &msg, const Relation::List &relations)
{
    Q_ASSERT(msg.type() == NotificationMessageV2::Relations);

    if (relations.isEmpty()) {
        return false;
    }

    switch (msg.operation()) {
    case NotificationMessageV2::Add:
        if (q_ptr->receivers(SIGNAL(relationAdded(Akonadi::Relation))) == 0) {
            return false;
        }
        Q_FOREACH (const Relation &relation, relations) {
            Q_EMIT q_ptr->relationAdded(relation);
        }
        return true;
    case NotificationMessageV2::Remove:
        if (q_ptr->receivers(SIGNAL(relationRemoved(Akonadi::Relation))) == 0) {
            return false;
        }
        Q_FOREACH (const Relation &relation, relations) {
            Q_EMIT q_ptr->relationRemoved(relation);
        }
        return true;
    default:
        kDebug() << "Unknown operation type" << msg.operation() << "in tag change notification";
    }

    return false;
}

void MonitorPrivate::invalidateCaches(const NotificationMessageV3 &msg)
{
    // remove invalidates
    if (msg.operation() == NotificationMessageV2::Remove) {
        if (msg.type() == NotificationMessageV2::Collections) {
            Q_FOREACH (qint64 uid, msg.uids())
            collectionCache->invalidate(uid);
        } else if (msg.type() == NotificationMessageV2::Items) {
            itemCache->invalidate(msg.uids());
        } else if (msg.type() == NotificationMessageV2::Tags) {
            tagCache->invalidate(msg.uids());
        }
    }

    // modify removes the cache entry, as we need to re-fetch
    // And subscription modify the visibility of the collection by the collectionFetchScope.
    if (msg.operation() == NotificationMessageV2::Modify
        || msg.operation() == NotificationMessageV2::ModifyFlags
        || msg.operation() == NotificationMessageV3::ModifyTags
        || msg.operation() == NotificationMessageV2::Move
        || msg.operation() == NotificationMessageV2::Subscribe) {
        if (msg.type() == NotificationMessageV2::Collections) {
            Q_FOREACH (quint64 uid, msg.uids())
            collectionCache->update(uid, mCollectionFetchScope);
        } else if (msg.type() == NotificationMessageV2::Items) {
            itemCache->update(msg.uids(), mItemFetchScope);
        } else if (msg.type() == NotificationMessageV2::Tags) {
            tagCache->update(msg.uids(), mTagFetchScope);
        }
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

bool MonitorPrivate::isMonitored(Entity::Id colId) const
{
    if (!useRefCounting) {
        return true;
    }
    return refCountMap.contains(colId) || m_buffer.isBuffered(colId);
}

void MonitorPrivate::notifyCollectionStatisticsWatchers(Entity::Id collection, const QByteArray &resource) {
    if (collection > 0 && (monitorAll || isCollectionMonitored(collection) || resources.contains(resource))) {
        recentlyChangedCollections.insert(collection);
        if (!statisticsCompressionTimer.isActive()) {
            statisticsCompressionTimer.start();
        }
    }
}

// @endcond
