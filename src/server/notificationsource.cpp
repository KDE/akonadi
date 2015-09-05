/*
    Copyright (c) 2010 Michael Jansen <kde@michael-jansen>

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

#include "notificationsource.h"

#include <shared/akdebug.h>

#include "notificationsourceadaptor.h"
#include "notificationmanager.h"
#include "collectionreferencemanager.h"
#include "connection.h"

using namespace Akonadi;
using namespace Akonadi::Server;

template<typename T>
QVector<T> setToVector(const QSet<T> &set)
{
    QVector<T> v;
    v.reserve(set.size());
    Q_FOREACH (const T &val, set) {
        v << val;
    }
    return v;
}

NotificationSource::NotificationSource(const QString &identifier, const QString &clientServiceName, NotificationManager *parent)
    : QObject(parent)
    , mManager(parent)
    , mIdentifier(identifier)
    , mDBusIdentifier(identifier)
    , mClientWatcher(0)
    , mAllMonitored(false)
    , mExclusive(false)
{
    new NotificationSourceAdaptor(this);

    // Clean up for dbus usage: any non-alphanumeric char should be turned into '_'
    const int len = mDBusIdentifier.length();
    for (int i = 0; i < len; ++i) {
        if (!mDBusIdentifier[i].isLetterOrNumber()) {
            mDBusIdentifier[i] = QLatin1Char('_');
        }
    }

    QDBusConnection::sessionBus().registerObject(
        dbusPath().path(),
        this,
        QDBusConnection::ExportAdaptors);

    mClientWatcher = new QDBusServiceWatcher(clientServiceName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, this);
    connect(mClientWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &NotificationSource::serviceUnregistered);
}

NotificationSource::~NotificationSource()
{
}

QDBusObjectPath NotificationSource::dbusPath() const
{
    return QDBusObjectPath(QLatin1String("/subscriber/") + mDBusIdentifier);
}

void NotificationSource::emitNotification(const Protocol::ChangeNotification::List &notifications)
{
    Q_FOREACH (const auto &notification, notifications) {
        Q_EMIT notify(notification);
    }
}

QString NotificationSource::identifier() const
{
    return mIdentifier;
}

void NotificationSource::unsubscribe()
{
    mManager->unsubscribe(mIdentifier);
}

bool NotificationSource::isExclusive() const
{
    return mExclusive;
}

void NotificationSource::setExclusive(bool enabled)
{
    mExclusive = enabled;
}

void NotificationSource::addClientServiceName(const QString &clientServiceName)
{
    if (mClientWatcher->watchedServices().contains(clientServiceName)) {
        return;
    }

    mClientWatcher->addWatchedService(clientServiceName);
    akDebug() << Q_FUNC_INFO << "Notification source" << mIdentifier << "now serving:" << mClientWatcher->watchedServices();
}

void NotificationSource::serviceUnregistered(const QString &serviceName)
{
    mClientWatcher->removeWatchedService(serviceName);
    akDebug() << Q_FUNC_INFO << "Notification source" << mIdentifier << "now serving:" << mClientWatcher->watchedServices();

    if (mClientWatcher->watchedServices().isEmpty()) {
        unsubscribe();
    }
}

void NotificationSource::setMonitoredCollection(Entity::Id id, bool monitored)
{
    if (id < 0) {
        return;
    }

    if (monitored && !mMonitoredCollections.contains(id)) {
        mMonitoredCollections.insert(id);
        Q_EMIT monitoredCollectionsChanged();
    } else if (!monitored) {
        mMonitoredCollections.remove(id);
        Q_EMIT monitoredCollectionsChanged();
    }
}

QVector<Entity::Id> NotificationSource::monitoredCollections() const
{
    return setToVector<Entity::Id>(mMonitoredCollections);
}

void NotificationSource::setMonitoredItem(Entity::Id id, bool monitored)
{
    if (id < 0) {
        return;
    }

    if (monitored && !mMonitoredItems.contains(id)) {
        mMonitoredItems.insert(id);
        Q_EMIT monitoredItemsChanged();
    } else if (!monitored) {
        mMonitoredItems.remove(id);
        Q_EMIT monitoredItemsChanged();
    }
}

QVector<Entity::Id> NotificationSource::monitoredItems() const
{
    return setToVector<Entity::Id>(mMonitoredItems);
}

void NotificationSource::setMonitoredTag(Entity::Id id, bool monitored)
{
    if (id < 0) {
        return;
    }

    if (monitored && !mMonitoredTags.contains(id)) {
        mMonitoredTags.insert(id);
        Q_EMIT monitoredTagsChanged();
    } else if (!monitored) {
        mMonitoredTags.remove(id);
        Q_EMIT monitoredTagsChanged();
    }
}

QVector<Entity::Id> NotificationSource::monitoredTags() const
{
    return setToVector<Entity::Id>(mMonitoredTags);
}

void NotificationSource::setMonitoredResource(const QByteArray &resource, bool monitored)
{
    if (monitored && !mMonitoredResources.contains(resource)) {
        mMonitoredResources.insert(resource);
        Q_EMIT monitoredResourcesChanged();
    } else if (!monitored) {
        mMonitoredResources.remove(resource);
        Q_EMIT monitoredResourcesChanged();
    }
}

QVector<QByteArray> NotificationSource::monitoredResources() const
{
    return setToVector<QByteArray>(mMonitoredResources);
}

void NotificationSource::setMonitoredMimeType(const QString &mimeType, bool monitored)
{
    if (mimeType.isEmpty()) {
        return;
    }

    if (monitored && !mMonitoredMimeTypes.contains(mimeType)) {
        mMonitoredMimeTypes.insert(mimeType);
        Q_EMIT monitoredMimeTypesChanged();
    } else if (!monitored) {
        mMonitoredMimeTypes.remove(mimeType);
        Q_EMIT monitoredMimeTypesChanged();
    }
}

QStringList NotificationSource::monitoredMimeTypes() const
{
    return mMonitoredMimeTypes.toList();
}

void NotificationSource::setAllMonitored(bool allMonitored)
{
    if (allMonitored && !mAllMonitored) {
        mAllMonitored = true;
        Q_EMIT isAllMonitoredChanged();
    } else if (!allMonitored) {
        mAllMonitored = false;
        Q_EMIT isAllMonitoredChanged();
    }
}

bool NotificationSource::isAllMonitored() const
{
    return mAllMonitored;
}

void NotificationSource::setSession(const QByteArray &sessionId)
{
    mSession = sessionId;
}

void NotificationSource::setIgnoredSession(const QByteArray &sessionId, bool ignored)
{
    if (ignored && !mIgnoredSessions.contains(sessionId)) {
        mIgnoredSessions.insert(sessionId);
        Q_EMIT ignoredSessionsChanged();
    } else if (!ignored) {
        mIgnoredSessions.remove(sessionId);
        Q_EMIT ignoredSessionsChanged();
    }
}

QVector<QByteArray> NotificationSource::ignoredSessions() const
{
    return setToVector<QByteArray>(mIgnoredSessions);
}

bool NotificationSource::isCollectionMonitored(Entity::Id id) const
{
    if (id < 0) {
        return false;
    } else if (mMonitoredCollections.contains(id)) {
        return true;
    } else if (mMonitoredCollections.contains(0)) {
        return true;
    }
    return false;
}

bool NotificationSource::isMimeTypeMonitored(const QString &mimeType) const
{
    return mMonitoredMimeTypes.contains(mimeType);

    // FIXME: Handle mimetype aliases
}

bool NotificationSource::isMoveDestinationResourceMonitored(const Protocol::ChangeNotification &msg) const
{
    if (msg.operation() != Protocol::ChangeNotification::Move) {
        return false;
    }
    return mMonitoredResources.contains(msg.destinationResource());
}

void NotificationSource::setMonitoredType(Protocol::ChangeNotification::Type type, bool monitored)
{
    if (monitored && !mMonitoredTypes.contains(type)) {
        mMonitoredTypes.insert(type);
        Q_EMIT monitoredTypesChanged();
    } else if (!monitored) {
        mMonitoredTypes.remove(type);
        Q_EMIT monitoredTypesChanged();
    }
}

QVector<Protocol::ChangeNotification::Type> NotificationSource::monitoredTypes() const
{
    return setToVector<Protocol::ChangeNotification::Type>(mMonitoredTypes);
}

bool NotificationSource::acceptsNotification(const Protocol::ChangeNotification &notification)
{
    // session is ignored
    if (mIgnoredSessions.contains(notification.sessionId())) {
        return false;
    }

    if (notification.entities().count() == 0 && notification.type() != Protocol::ChangeNotification::Relations) {
        return false;
    }

    //Only emit notifications for referenced collections if the subscriber is exclusive or monitors the collection
    if (notification.type() == Protocol::ChangeNotification::Collections) {
        // HACK: We need to dispatch notifications about disabled collections to SOME
        // agents (that's what we have the exclusive subscription for) - but because
        // querying each Collection from database would be expensive, we use the
        // metadata hack to transfer this information from NotificationCollector
        if (notification.metadata().contains("DISABLED") && (notification.operation() != Protocol::ChangeNotification::Unsubscribe) && !notification.itemParts().contains("ENABLED")) {
            // Exclusive subscriber always gets it
            if (mExclusive) {
                return true;
            }


            Q_FOREACH (const Protocol::ChangeNotification::Entity &entity, notification.entities()) {
                //Deliver the notification if referenced from this session
                if (CollectionReferenceManager::instance()->isReferenced(entity.id, mSession)) {
                    return true;
                }
                //Exclusive subscribers still want the notification
                if (mExclusive && CollectionReferenceManager::instance()->isReferenced(entity.id)) {
                    return true;
                }
            }

            //The session belonging to this monitor referenced or dereferenced the collection. We always want this notification.
            //The referencemanager no longer holds a reference, so we have to check this way.
            if (notification.itemParts().contains("REFERENCED") && mSession == notification.sessionId()) {
                return true;
            }

            // If the collection is not referenced, monitored or the subscriber is not
            // exclusive (i.e. if we got here), then the subscriber does not care about
            // this one, so drop it
            return false;
        }
    } else if (notification.type() == Protocol::ChangeNotification::Items) {
        if (CollectionReferenceManager::instance()->isReferenced(notification.parentCollection())) {
            return (mExclusive || isCollectionMonitored(notification.parentCollection()) || isMoveDestinationResourceMonitored(notification));
        }
    } else if (notification.type() == Protocol::ChangeNotification::Tags) {
        // Special handling for Tag removal notifications: When a Tag is removed,
        // a notification is emitted for each Resource that owns the tag (i.e.
        // each resource that owns a Tag RID - Tag RIDs are resource-specific).
        // Additionally then we send one more notification without any RID that is
        // destined for regular applications (which don't know anything about Tag RIDs)
        if (notification.operation() == Protocol::ChangeNotification::Remove) {
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
    }

    // user requested everything
    if (mAllMonitored && notification.type() != Protocol::ChangeNotification::InvalidType) {
        return true;
    }

    switch (notification.type()) {
    case Protocol::ChangeNotification::InvalidType:
        akDebug() << "Received invalid change notification!";
        return false;

    case Protocol::ChangeNotification::Items:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ChangeNotification::Items)) {
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

            Q_FOREACH (const Protocol::ChangeNotification::Entity &entity, notification.entities()) {
                if (isMimeTypeMonitored(entity.mimeType)) {
                    return true;
                }
            }

            return false;
        }

        // we explicitly monitor that item or the collections it's in
        Q_FOREACH (const Protocol::ChangeNotification::Entity &entity, notification.entities()) {
            if (mMonitoredItems.contains(entity.id)) {
                return true;
            }
        }

        return isCollectionMonitored(notification.parentCollection())
               || isCollectionMonitored(notification.parentDestCollection());

    case Protocol::ChangeNotification::Collections:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ChangeNotification::Collections)) {
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
        Q_FOREACH (const Protocol::ChangeNotification::Entity &entity, notification.entities()) {
            if (isCollectionMonitored(entity.id)) {
                return true;
            }
        }

        return isCollectionMonitored(notification.parentCollection())
               || isCollectionMonitored(notification.parentDestCollection());

    case Protocol::ChangeNotification::Tags:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ChangeNotification::Tags)) {
            return false;
        }

        if (mMonitoredTags.isEmpty()) {
            return true;
        }

        Q_FOREACH (const Protocol::ChangeNotification::Entity &entity, notification.entities()) {
            if (mMonitoredTags.contains(entity.id)) {
                return true;
            }
        }

        return false;

    case Protocol::ChangeNotification::Relations:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ChangeNotification::Relations)) {
            return false;
        }
        return true;

    }

    return false;
}
