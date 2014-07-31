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
#include <akdebug.h>

#include "notificationsourceadaptor.h"
#include "notificationmanager.h"
#include "collectionreferencemanager.h"

using namespace Akonadi;
using namespace Akonadi::Server;

template<typename T>
QVector<T> setToVector(const QSet<T> &set)
{
    QVector<T> v(set.size());
    std::copy(set.constBegin(), set.constEnd(), v.begin());
    return v;
}

NotificationSource::NotificationSource(const QString &identifier, const QString &clientServiceName, NotificationManager *parent)
    : QObject(parent)
    , mManager(parent)
    , mIdentifier(identifier)
    , mDBusIdentifier(identifier)
    , mClientWatcher(0)
    , mServerSideMonitorEnabled(false)
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
    connect(mClientWatcher, SIGNAL(serviceUnregistered(QString)), SLOT(serviceUnregistered(QString)));
}

NotificationSource::~NotificationSource()
{
}

QDBusObjectPath NotificationSource::dbusPath() const
{
    return QDBusObjectPath(QLatin1String("/subscriber/") + mDBusIdentifier);
}

void NotificationSource::emitNotification(const NotificationMessage::List &notifications)
{
    Q_EMIT notify(notifications);
}

void NotificationSource::emitNotification(const NotificationMessageV2::List &notifications)
{
    Q_EMIT notifyV2(notifications);
}

void NotificationSource::emitNotification(const NotificationMessageV3::List &notifications)
{
    Q_EMIT notifyV3(notifications);
}

QString NotificationSource::identifier() const
{
    return mIdentifier;
}

void NotificationSource::unsubscribe()
{
    mManager->unsubscribe(mIdentifier);
}

bool NotificationSource::isServerSideMonitorEnabled() const
{
    return mServerSideMonitorEnabled;
}

void NotificationSource::setServerSideMonitorEnabled(bool enabled)
{
    mServerSideMonitorEnabled = enabled;
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
    if (id < 0 || !isServerSideMonitorEnabled()) {
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
    if (id < 0 || !isServerSideMonitorEnabled()) {
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
    if (id < 0 || !isServerSideMonitorEnabled()) {
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
    if (!isServerSideMonitorEnabled()) {
        return;
    }

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
    if (mimeType.isEmpty() || !isServerSideMonitorEnabled()) {
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
    if (!isServerSideMonitorEnabled()) {
        return;
    }

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

void NotificationSource::setIgnoredSession(const QByteArray &sessionId, bool ignored)
{
    if (!isServerSideMonitorEnabled()) {
        return;
    }

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

bool NotificationSource::isMoveDestinationResourceMonitored(const NotificationMessageV3 &msg) const
{
    if (msg.operation() != NotificationMessageV2::Move) {
        return false;
    }
    return mMonitoredResources.contains(msg.destinationResource());
}

void NotificationSource::setMonitoredType(NotificationMessageV2::Type type, bool monitored)
{
    if (!isServerSideMonitorEnabled()) {
        return;
    }

    if (monitored && !mMonitoredTypes.contains(type)) {
        mMonitoredTypes.insert(type);
        Q_EMIT monitoredTypesChanged();
    } else if (!monitored) {
        mMonitoredTypes.remove(type);
        Q_EMIT monitoredTypesChanged();
    }
}

QVector<NotificationMessageV2::Type> NotificationSource::monitoredTypes() const
{
    return setToVector<NotificationMessageV2::Type>(mMonitoredTypes);
}

bool NotificationSource::acceptsNotification(const NotificationMessageV3 &notification)
{
    // session is ignored
    if (mIgnoredSessions.contains(notification.sessionId())) {
        return false;
    }

    if (notification.entities().count() == 0) {
        return false;
    }

    //Only emit notifications for referenced collections if the subscriber is exclusive or monitors the collection
    if (notification.type() == NotificationMessageV2::Collections) {
        Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
            if (CollectionReferenceManager::instance()->isReferenced(entity.id)) {
                return (mExclusive || isCollectionMonitored(entity.id));
            }
        }
    } else if (notification.type() == NotificationMessageV2::Items) {
        if (CollectionReferenceManager::instance()->isReferenced(notification.parentCollection())) {
            return (mExclusive || isCollectionMonitored(notification.parentCollection()) || isMoveDestinationResourceMonitored(notification));
        }
    }

    // user requested everything
    if (mAllMonitored && notification.type() != NotificationMessageV2::InvalidType) {
        return true;
    }

    switch (notification.type()) {
    case NotificationMessageV2::InvalidType:
        akDebug() << "Received invalid change notification!";
        return false;

    case NotificationMessageV2::Items:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(NotificationMessageV2::Items)) {
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

            Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
                if (isMimeTypeMonitored(entity.mimeType)) {
                    return true;
                }
            }

            return false;
        }

        // we explicitly monitor that item or the collections it's in
        Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
            if (mMonitoredItems.contains(entity.id)) {
                return true;
            }
        }

        return isCollectionMonitored(notification.parentCollection())
               || isCollectionMonitored(notification.parentDestCollection());

    case NotificationMessageV2::Collections:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(NotificationMessageV2::Collections)) {
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
        Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
            if (isCollectionMonitored(entity.id)) {
                return true;
            }
        }

        return isCollectionMonitored(notification.parentCollection())
               || isCollectionMonitored(notification.parentDestCollection());

    case NotificationMessageV2::Tags:
        if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(NotificationMessageV2::Tags)) {
            return false;
        }

        if (mMonitoredTags.isEmpty()) {
            return true;
        }

        Q_FOREACH (const NotificationMessageV2::Entity &entity, notification.entities()) {
            if (mMonitoredTags.contains(entity.id)) {
                return true;
            }
        }

        return false;
    }

    return false;
}
