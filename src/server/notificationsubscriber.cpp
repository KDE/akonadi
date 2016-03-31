/*
    Copyright (c) 2015 Daniel Vrátil <dvratil@kde.org>

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

#include "notificationsubscriber.h"
#include "akonadiserver_debug.h"
#include "notificationmanager.h"
#include "collectionreferencemanager.h"

#include <QLocalSocket>
#include <QDataStream>

#include <private/protocol_p.h>
#include <private/protocol_exception_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

QMimeDatabase NotificationSubscriber::sMimeDatabase;

NotificationSubscriber::NotificationSubscriber(NotificationManager *manager)
    : QObject()
    , mManager(manager)
    , mSocket(Q_NULLPTR)
    , mAllMonitored(false)
    , mExclusive(false)
{
}


NotificationSubscriber::NotificationSubscriber(NotificationManager *manager, quintptr socketDescriptor)
    : NotificationSubscriber(manager)
{
    mSocket = new QLocalSocket(this);
    connect(mSocket, &QLocalSocket::readyRead,
            this, &NotificationSubscriber::socketReadyRead);
    connect(mSocket, &QLocalSocket::disconnected,
            this, &NotificationSubscriber::socketDisconnected);
    mSocket->setSocketDescriptor(socketDescriptor);

    writeCommand(0, Protocol::HelloResponse(QStringLiteral("Akonadi"),
                                                        QStringLiteral("Not-really IMAP server"),
                                                        Protocol::version()));
}

NotificationSubscriber::~NotificationSubscriber()
{
}

void NotificationSubscriber::socketReadyRead()
{
    while (mSocket->bytesAvailable() > (int) sizeof(qint64)) {
        QDataStream stream(mSocket);

        // Ignored atm
        qint64 tag = -1;
        stream >> tag;

        Protocol::Command cmd;
        try {
            cmd = Protocol::deserialize(mSocket);
        } catch (const Akonadi::ProtocolException &e) {
            qDebug() << "ProtocolException:" << e.what();
            disconnectSubscriber();
            return;
        } catch (const std::exception &e) {
            qDebug() << "Unknown exception:" << e.what();
            disconnectSubscriber();
            return;
        }
        if (cmd.type() == Protocol::Command::Invalid) {
            qDebug() << "Received an invalid command: resetting connection";
            disconnectSubscriber();
            return;
        }

        switch (cmd.type()) {
        case Protocol::Command::CreateSubscription:
            registerSubscriber(cmd);
            writeCommand(tag, Protocol::CreateSubscriptionResponse());
            break;
        case Protocol::Command::ModifySubscription:
            if (mSubscriber.isEmpty()) {
                qDebug() << "Received ModifySubscription command before RegisterSubscriber";
                disconnectSubscriber();
                return;
            }
            modifySubscription(cmd);
            writeCommand(tag, Protocol::ModifySubscriptionResponse());
            break;
        case Protocol::Command::Logout:
            disconnectSubscriber();
            break;
        default:
            qDebug() << "Invalid command" << cmd.type() << "received by NotificationSubscriber" << mSubscriber;
            disconnectSubscriber();
            break;
        }
    }
}

void NotificationSubscriber::socketDisconnected()
{
    qDebug() << "Subscriber" << mSubscriber << "disconnected from us!";
    disconnectSubscriber();
}

void NotificationSubscriber::disconnectSubscriber()
{
    QMutexLocker locker(&mLock);

    if (mManager) {
        Protocol::SubscriptionChangeNotification changeNtf;
        changeNtf.setSubscriber(mSubscriber);
        changeNtf.setSessionId(mSession);
        changeNtf.setOperation(Protocol::SubscriptionChangeNotification::Remove);
        mManager->slotNotify({ changeNtf });
    }

    disconnect(mSocket, &QLocalSocket::readyRead,
               this, &NotificationSubscriber::socketReadyRead);
    disconnect(mSocket, &QLocalSocket::disconnected,
               this, &NotificationSubscriber::socketDisconnected);
    mSocket->close();
    deleteLater();
}

void NotificationSubscriber::registerSubscriber(const Protocol::CreateSubscriptionCommand &command)
{
    QMutexLocker locker(&mLock);

    qDebug() << "Subscriber" << this << "identified as" << command.subscriberName();
    mSubscriber = command.subscriberName();

    if (mManager) {
        Protocol::SubscriptionChangeNotification changeNtf;
        changeNtf.setSubscriber(mSubscriber);
        changeNtf.setSessionId(mSession);
        changeNtf.setOperation(Protocol::SubscriptionChangeNotification::Add);
        mManager->slotNotify({ changeNtf });
    }
}

void NotificationSubscriber::modifySubscription(const Protocol::ModifySubscriptionCommand &command)
{
    QMutexLocker locker(&mLock);

    const auto modifiedParts = command.modifiedParts();

    #define START_MONITORING(type) \
        (modifiedParts & Protocol::ModifySubscriptionCommand::ModifiedParts( \
            Protocol::ModifySubscriptionCommand::type | Protocol::ModifySubscriptionCommand::Add))
    #define STOP_MONITORING(type) \
        (modifiedParts & Protocol::ModifySubscriptionCommand::ModifiedParts( \
            Protocol::ModifySubscriptionCommand::type | Protocol::ModifySubscriptionCommand::Remove))

    #define APPEND(set, newItems) \
        Q_FOREACH (const auto &entity, newItems) { \
            set.insert(entity); \
        }

    #define REMOVE(set, items) \
        Q_FOREACH (const auto &entity, items) { \
            set.insert(entity); \
        }

    if (START_MONITORING(Types)) {
        APPEND(mMonitoredTypes, command.startMonitoringTypes())
    }
    if (STOP_MONITORING(Types)) {
        REMOVE(mMonitoredTypes, command.stopMonitoringTypes())
    }
    if (START_MONITORING(Collections)) {
        APPEND(mMonitoredCollections, command.startMonitoringCollections())
    }
    if (STOP_MONITORING(Collections)) {
        REMOVE(mMonitoredCollections, command.stopMonitoringCollections())
    }
    if (START_MONITORING(Items)) {
        APPEND(mMonitoredItems, command.startMonitoringItems())
    }
    if (STOP_MONITORING(Items)) {
        REMOVE(mMonitoredItems, command.stopMonitoringItems())
    }
    if (START_MONITORING(Tags)) {
        APPEND(mMonitoredTags, command.startMonitoringTags())
    }
    if (STOP_MONITORING(Tags)) {
        REMOVE(mMonitoredTags, command.stopMonitoringTags())
    }
    if (START_MONITORING(Resources)) {
        APPEND(mMonitoredResources, command.startMonitoringResources())
    }
    if (STOP_MONITORING(Resources)) {
        REMOVE(mMonitoredResources, command.stopMonitoringResources())
    }
    if (START_MONITORING(MimeTypes)) {
        APPEND(mMonitoredMimeTypes, command.startMonitoringMimeTypes())
    }
    if (STOP_MONITORING(MimeTypes)) {
        REMOVE(mMonitoredMimeTypes, command.stopMonitoringMimeTypes())
    }
    if (START_MONITORING(Sessions)) {
        APPEND(mIgnoredSessions, command.startIgnoringSessions())
    }
    if (STOP_MONITORING(Sessions)) {
        REMOVE(mIgnoredSessions, command.stopIgnoringSessions())
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::AllFlag) {
        mAllMonitored = command.allMonitored();
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::ExclusiveFlag) {
        mExclusive = command.isExclusive();
    }

    if (mManager) {
        // Did the caller just subscribed to subscription changes?
        if ((modifiedParts & Protocol::ModifySubscriptionCommand::Types)
            && command.startMonitoringTypes().contains(Protocol::ModifySubscriptionCommand::SubscriptionChanges))
        {
            // If yes, then send them list of all existing subscribers
            Protocol::ChangeNotification::List ntfs;
            Q_FOREACH (const NotificationSubscriber *subscriber, mManager->mSubscribers) {
                ntfs << subscriber->toChangeNotification();
            }
            // Send them back to caller
            notify(ntfs);
        }

        // Emit subscription change notification
        Protocol::SubscriptionChangeNotification changeNtf = toChangeNotification();
        changeNtf.setOperation(Protocol::SubscriptionChangeNotification::Modify);
        mManager->slotNotify({ changeNtf });
    }
}

Protocol::ChangeNotification NotificationSubscriber::toChangeNotification() const
{
    // Assumes mLock being locked by caller

    Protocol::SubscriptionChangeNotification ntf;
    ntf.setSessionId(mSession);
    ntf.setSubscriber(mSubscriber);
    ntf.setOperation(Protocol::SubscriptionChangeNotification::Add);
    ntf.setAddedCollections(mMonitoredCollections);
    ntf.setAddedItems(mMonitoredItems);
    ntf.setAddedTags(mMonitoredTags);
    ntf.setAddedTypes(mMonitoredTypes);
    ntf.setAddedMimeTypes(mMonitoredMimeTypes);
    ntf.setAddedResources(mMonitoredResources);
    ntf.setAddedIgnoredSessions(mIgnoredSessions);
    ntf.setAllMonitored(mAllMonitored);
    ntf.setExclusive(mExclusive);
    return ntf;
}



bool NotificationSubscriber::isCollectionMonitored(Entity::Id id) const
{
    // Assumes mLock being locked by caller

    if (id < 0) {
        return false;
    } else if (mMonitoredCollections.contains(id)) {
        return true;
    } else if (mMonitoredCollections.contains(0)) {
        return true;
    }
    return false;
}

bool NotificationSubscriber::isMimeTypeMonitored(const QString &mimeType) const
{
    // Assumes mLock being locked by caller

    const QMimeType mt = sMimeDatabase.mimeTypeForName(mimeType);
    if (mMonitoredMimeTypes.contains(mimeType)) {
        return true;
    }

    Q_FOREACH (const QString &alias, mt.aliases()) {
        if (mMonitoredMimeTypes.contains(alias)) {
            return true;
        }
    }

    return false;
}

bool NotificationSubscriber::isMoveDestinationResourceMonitored(const Protocol::ItemChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    if (msg.operation() != Protocol::ItemChangeNotification::Move) {
        return false;
    }
    return mMonitoredResources.contains(msg.destinationResource());
}

bool NotificationSubscriber::isMoveDestinationResourceMonitored(const Protocol::CollectionChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    if (msg.operation() != Protocol::CollectionChangeNotification::Move) {
        return false;
    }
    return mMonitoredResources.contains(msg.destinationResource());
}


bool NotificationSubscriber::acceptsItemNotification(const Protocol::ItemChangeNotification &notification) const
{
    // Assumes mLock being locked by caller

    if (notification.items().count() == 0) {
        return false;
    }

    if (CollectionReferenceManager::instance()->isReferenced(notification.parentCollection())) {
        //We always want notifications that affect the parent resource (like an item added to a referenced collection)
        const bool notificationForParentResource = (mSession == notification.resource());
        return (mExclusive
            || isCollectionMonitored(notification.parentCollection())
            || isMoveDestinationResourceMonitored(notification)
            || notificationForParentResource);
    }


    if (mAllMonitored) {
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::ItemChanges)) {
        return false;
    }

    // we have a resource or mimetype filter
    if (!mMonitoredResources.isEmpty() || !mMonitoredMimeTypes.isEmpty()) {
        if (mMonitoredResources.contains(notification.resource())) {
            return true;
        }

        if (isMoveDestinationResourceMonitored(notification)) {
            return true;
        }

        Q_FOREACH (const auto &item, notification.items()) {
            if (isMimeTypeMonitored(item.mimeType)) {
                return true;
            }
        }

        return false;
    }

    // we explicitly monitor that item or the collections it's in
    Q_FOREACH (const auto &item, notification.items()) {
        if (mMonitoredItems.contains(item.id)) {
            return true;
        }
    }

    return isCollectionMonitored(notification.parentCollection())
            || isCollectionMonitored(notification.parentDestCollection());
}

bool NotificationSubscriber::acceptsCollectionNotification(const Protocol::CollectionChangeNotification &notification) const
{
    // Assumes mLock being locked by caller

    if (notification.id() < 0) {
        return false;
    }

    // HACK: We need to dispatch notifications about disabled collections to SOME
    // agents (that's what we have the exclusive subscription for) - but because
    // querying each Collection from database would be expensive, we use the
    // metadata hack to transfer this information from NotificationCollector
    if (notification.metadata().contains("DISABLED")
            && (notification.operation() != Protocol::CollectionChangeNotification::Unsubscribe)
            && !notification.changedParts().contains("ENABLED")) {
        // Exclusive subscriber always gets it
        if (mExclusive) {
            return true;
        }

        //Deliver the notification if referenced from this session
        if (CollectionReferenceManager::instance()->isReferenced(notification.id(), mSession)) {
            return true;
        }

        //Exclusive subscribers still want the notification
        if (mExclusive && CollectionReferenceManager::instance()->isReferenced(notification.id())) {
            return true;
        }

        //The session belonging to this monitor referenced or dereferenced the collection. We always want this notification.
        //The referencemanager no longer holds a reference, so we have to check this way.
        if (notification.changedParts().contains("REFERENCED") && mSession == notification.sessionId()) {
            return true;
        }

        // If the collection is not referenced, monitored or the subscriber is not
        // exclusive (i.e. if we got here), then the subscriber does not care about
        // this one, so drop it
        return false;
    }

    if (mAllMonitored) {
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::CollectionChanges)) {
        return false;
    }

    // we have a resource filter
    if (!mMonitoredResources.isEmpty()) {
        const bool resourceMatches = mMonitoredResources.contains(notification.resource())
                                        || isMoveDestinationResourceMonitored(notification);

        // a bit hacky, but match the behaviour from the item case,
        // if resource is the only thing we are filtering on, stop here, and if the resource filter matched, of course
        if (mMonitoredMimeTypes.isEmpty() || resourceMatches) {
            return resourceMatches;
        }
        // else continue
    }

    // we explicitly monitor that colleciton, or all of them
    if (isCollectionMonitored(notification.id())) {
        return true;
    }

    return isCollectionMonitored(notification.parentCollection())
            || isCollectionMonitored(notification.parentDestCollection());

}

bool NotificationSubscriber::acceptsTagNotification(const Protocol::TagChangeNotification &notification) const
{
    // Assumes mLock being locked by caller

    if (notification.id() < 0) {
        return false;
    }

    // Special handling for Tag removal notifications: When a Tag is removed,
    // a notification is emitted for each Resource that owns the tag (i.e.
    // each resource that owns a Tag RID - Tag RIDs are resource-specific).
    // Additionally then we send one more notification without any RID that is
    // destined for regular applications (which don't know anything about Tag RIDs)
    if (notification.operation() == Protocol::TagChangeNotification::Remove) {
        // HACK: Since have no way to determine which resource this NotificationSource
        // belongs to, we are abusing the fact that each resource ignores it's own
        // main session, which is called the same name as the resource.

        // If there are any ignored sessions, but this notification does not have
        // a specific resource set, then we ignore it, as this notification is
        // for clients, not resources (does not have tag RID)
        if (!mIgnoredSessions.isEmpty() && notification.resource().isEmpty()) {
            return false;
        }

        // If this source ignores a session (i.e. we assume it is a resource),
        // but this notification is for another resource, then we ignore it
        if (!notification.resource().isEmpty() && !mIgnoredSessions.contains(notification.resource())) {
            return false;
        }

        // Now we got here, which means that this notification either has empty
        // resource, i.e. it is destined for a client applications, or it's
        // destined for resource that we *think* (see the hack above) this
        // NotificationSource belongs too. Which means we approve this notification,
        // but it can still be discarded in the generic Tag notification filter
        // below
    }

    if (mAllMonitored) {
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::TagChanges)) {
        return false;
    }

    if (mMonitoredTags.isEmpty()) {
        return true;
    }

    if (mMonitoredTags.contains(notification.id())) {
        return true;
    }

    return false;
}

bool NotificationSubscriber::acceptsRelationNotification(const Protocol::RelationChangeNotification &notification) const
{
    // Assumes mLock being locked by caller

    Q_UNUSED(notification);

    if (mAllMonitored) {
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::RelationChanges)) {
        return false;
    }

    return true;
}

bool NotificationSubscriber::acceptsSubscriptionNotification(const Protocol::SubscriptionChangeNotification &notification) const
{
    // Assumes mLock being locked by caller

    Q_UNUSED(notification);

    // Unlike other types, subscription notifications must be explicitly enabled
    // by caller and are excluded from "monitor all" as well
    return mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::SubscriptionChanges);
}

bool NotificationSubscriber::acceptsNotification(const Protocol::ChangeNotification &notification) const
{
    // Assumes mLock being locked

    // Uninitialized subscriber gets nothing
    if (mSubscriber.isEmpty()) {
        return false;
    }

    // session is ignored
    if (mIgnoredSessions.contains(notification.sessionId())) {
        return false;
    }

    switch (notification.type()) {
    case Protocol::Command::ItemChangeNotification:
        return acceptsItemNotification(notification);
    case Protocol::Command::CollectionChangeNotification:
        return acceptsCollectionNotification(notification);
    case Protocol::Command::TagChangeNotification:
        return acceptsTagNotification(notification);
    case Protocol::Command::RelationChangeNotification:
        return acceptsRelationNotification(notification);
    case Protocol::Command::SubscriptionChangeNotification:
        return acceptsSubscriptionNotification(notification);
    default:
        qCDebug(AKONADISERVER_LOG) << "Received invalid change notification!";
        return false;
    }
}

void NotificationSubscriber::notify(const Protocol::ChangeNotification::List &notifications)
{
    QMutexLocker locker(&mLock);

    Q_FOREACH (const auto &notification, notifications) {
        if (acceptsNotification(notification)) {
            QMetaObject::invokeMethod(this, "writeNotification", Qt::QueuedConnection,
                                      Q_ARG(Akonadi::Protocol::ChangeNotification, notification));
        }
    }
}

void NotificationSubscriber::writeNotification(const Protocol::ChangeNotification &notification)
{
    // tag chosen by fair dice roll
    writeCommand(4, notification);
}

void NotificationSubscriber::writeCommand(qint64 tag, const Protocol::Command& cmd)
{
    QDataStream stream(mSocket);
    stream << tag;
    Protocol::serialize(mSocket, cmd);
}
