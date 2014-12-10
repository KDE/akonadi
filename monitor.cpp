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

#include "monitor.h"
#include "monitor_p.h"

#include "changemediator_p.h"
#include "collectionfetchscope.h"
#include "itemfetchjob.h"
#include "session.h"

#include <kdebug.h>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <iterator>

using namespace Akonadi;

Monitor::Monitor(QObject *parent)
    : QObject(parent)
    , d_ptr(new MonitorPrivate(0, this))
{
    d_ptr->init();
    d_ptr->connectToNotificationManager();
}

//@cond PRIVATE
Monitor::Monitor(MonitorPrivate *d, QObject *parent)
    : QObject(parent)
    , d_ptr(d)
{
    d_ptr->init();
    d_ptr->connectToNotificationManager();

    ChangeMediator::registerMonitor(this);
}
//@endcond

Monitor::~Monitor()
{
    ChangeMediator::unregisterMonitor(this);

    delete d_ptr;
}

void Monitor::setCollectionMonitored(const Collection &collection, bool monitored)
{
    Q_D(Monitor);
    if (!d->collections.contains(collection) && monitored) {
        d->collections << collection;
        if (d->notificationSource) {
            d->notificationSource->setMonitoredCollection(collection.id(), true);
        }
    } else if (!monitored) {
        if (d->collections.removeAll(collection)) {
            d->cleanOldNotifications();
            if (d->notificationSource) {
                d->notificationSource->setMonitoredCollection(collection.id(), false);
            }
        }
    }

    emit collectionMonitored(collection, monitored);
}

void Monitor::setItemMonitored(const Item &item, bool monitored)
{
    Q_D(Monitor);
    if (!d->items.contains(item.id()) && monitored) {
        d->items.insert(item.id());
        if (d->notificationSource) {
            d->notificationSource->setMonitoredItem(item.id(), true);
        }
    } else if (!monitored) {
        if (d->items.remove(item.id())) {
            d->cleanOldNotifications();
            if (d->notificationSource) {
                d->notificationSource->setMonitoredItem(item.id(), false);
            }
        }
    }

    emit itemMonitored(item,  monitored);
}

void Monitor::setResourceMonitored(const QByteArray &resource, bool monitored)
{
    Q_D(Monitor);
    if (!d->resources.contains(resource) && monitored) {
        d->resources.insert(resource);
        if (d->notificationSource) {
            d->notificationSource->setMonitoredResource(resource, true);
        }
    } else if (!monitored) {
        if (d->resources.remove(resource)) {
            d->cleanOldNotifications();
            if (d->notificationSource) {
                d->notificationSource->setMonitoredResource(resource, false);
            }
        }
    }

    emit resourceMonitored(resource, monitored);
}

void Monitor::setMimeTypeMonitored(const QString &mimetype, bool monitored)
{
    Q_D(Monitor);
    if (!d->mimetypes.contains(mimetype) && monitored) {
        d->mimetypes.insert(mimetype);
        if (d->notificationSource) {
            d->notificationSource->setMonitoredMimeType(mimetype, true);
        }
    } else if (!monitored) {
        if (d->mimetypes.remove(mimetype)) {
            d->cleanOldNotifications();
            if (d->notificationSource) {
                d->notificationSource->setMonitoredMimeType(mimetype, false);
            }
        }
    }

    emit mimeTypeMonitored(mimetype, monitored);
}

void Monitor::setTagMonitored(const Akonadi::Tag &tag, bool monitored)
{
    Q_D(Monitor);
    if (!d->tags.contains(tag.id()) && monitored) {
        d->tags.insert(tag.id());
        if (d->notificationSource) {
            d->notificationSource->setMonitoredTag(tag.id(), true);
        }
    } else if (!monitored) {
        if (d->tags.remove(tag.id())) {
            d->cleanOldNotifications();
            if (d->notificationSource) {
                d->notificationSource->setMonitoredTag(tag.id(), false);
            }
        }
    }

    emit tagMonitored(tag, monitored);
}

void Monitor::setTypeMonitored(Monitor::Type type, bool monitored)
{
    Q_D(Monitor);
    if (!d->types.contains(type) && monitored) {
        d->types.insert(type);
        if (d->notificationSource) {
            d->notificationSource->setMonitoredType(static_cast<NotificationMessageV2::Type>(type), true);
        }
    } else if (!monitored) {
        if (d->types.remove(type)) {
            d->cleanOldNotifications();
            if (d->notificationSource) {
                d->notificationSource->setMonitoredType(static_cast<NotificationMessageV2::Type>(type), false);
            }
        }
    }

    emit typeMonitored(type, monitored);
}

void Akonadi::Monitor::setAllMonitored(bool monitored)
{
    Q_D(Monitor);
    if (d->monitorAll == monitored) {
        return;
    }

    d->monitorAll = monitored;

    if (!monitored) {
        d->cleanOldNotifications();
    }

    if (d->notificationSource) {
        d->notificationSource->setAllMonitored(monitored);
    }

    emit allMonitored(monitored);
}

void Monitor::setExclusive(bool exclusive)
{
    Q_D(Monitor);
    d->exclusive = exclusive;
    if (d->notificationSource) {
        d->notificationSource->setExclusive(exclusive);
    }
}

bool Monitor::exclusive() const
{
    Q_D(const Monitor);
    return d->exclusive;
}

void Monitor::ignoreSession(Session *session)
{
    Q_D(Monitor);

    if (!d->sessions.contains(session->sessionId())) {
        d->sessions << session->sessionId();
        connect(session, SIGNAL(destroyed(QObject*)), this, SLOT(slotSessionDestroyed(QObject*)));
        if (d->notificationSource) {
            d->notificationSource->setIgnoredSession(session->sessionId(), true);
        }
    }
}

void Monitor::fetchCollection(bool enable)
{
    Q_D(Monitor);
    d->fetchCollection = enable;
}

void Monitor::fetchCollectionStatistics(bool enable)
{
    Q_D(Monitor);
    d->fetchCollectionStatistics = enable;
}

void Monitor::setItemFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(Monitor);
    d->mItemFetchScope = fetchScope;
}

ItemFetchScope &Monitor::itemFetchScope()
{
    Q_D(Monitor);
    return d->mItemFetchScope;
}

void Monitor::fetchChangedOnly(bool enable)
{
    Q_D(Monitor);
    d->mFetchChangedOnly = enable;
}

void Monitor::setCollectionFetchScope(const CollectionFetchScope &fetchScope)
{
    Q_D(Monitor);
    d->mCollectionFetchScope = fetchScope;
}

CollectionFetchScope &Monitor::collectionFetchScope()
{
    Q_D(Monitor);
    return d->mCollectionFetchScope;
}

void Monitor::setTagFetchScope(const TagFetchScope &fetchScope)
{
    Q_D(Monitor);
    d->mTagFetchScope = fetchScope;
}

TagFetchScope &Monitor::tagFetchScope()
{
    Q_D(Monitor);
    return d->mTagFetchScope;
}

Akonadi::Collection::List Monitor::collectionsMonitored() const
{
    Q_D(const Monitor);
    return d->collections;
}

QList<Item::Id> Monitor::itemsMonitored() const
{
    Q_D(const Monitor);
    return d->items.toList();
}

QVector<Item::Id> Monitor::itemsMonitoredEx() const
{
    Q_D(const Monitor);
    QVector<Item::Id> result;
    result.reserve(d->items.size());
    qCopy(d->items.begin(), d->items.end(), std::back_inserter(result));
    return result;
}

int Monitor::numItemsMonitored() const
{
    Q_D(const Monitor);
    return d->items.size();
}

QVector<Tag::Id> Monitor::tagsMonitored() const
{
    Q_D(const Monitor);
    QVector<Tag::Id> result;
    result.reserve(d->tags.size());
    qCopy(d->tags.begin(), d->tags.end(), std::back_inserter(result));
    return result;
}

QVector<Monitor::Type> Monitor::typesMonitored() const
{
    Q_D(const Monitor);
    QVector<Monitor::Type> result;
    result.reserve(d->types.size());
    qCopy(d->types.begin(), d->types.end(), std::back_inserter(result));
    return result;
}

QStringList Monitor::mimeTypesMonitored() const
{
    Q_D(const Monitor);
    return d->mimetypes.toList();
}

int Monitor::numMimeTypesMonitored() const
{
    Q_D(const Monitor);
    return d->mimetypes.count();
}

QList<QByteArray> Monitor::resourcesMonitored() const
{
    Q_D(const Monitor);
    return d->resources.toList();
}

int Monitor::numResourcesMonitored() const
{
    Q_D(const Monitor);
    return d->resources.count();
}

bool Monitor::isAllMonitored() const
{
    Q_D(const Monitor);
    return d->monitorAll;
}

void Monitor::setSession(Akonadi::Session *session)
{
    Q_D(Monitor);
    if (session == d->session) {
        return;
    }

    if (!session) {
        d->session = Session::defaultSession();
    } else {
        d->session = session;
    }

    d->itemCache->setSession(d->session);
    d->collectionCache->setSession(d->session);
    if (d->notificationSource) {
        d->notificationSource->setSession(d->session->sessionId());
    }
}

Session *Monitor::session() const
{
    Q_D(const Monitor);
    return d->session;
}

void Monitor::setCollectionMoveTranslationEnabled(bool enabled)
{
    Q_D(Monitor);
    d->collectionMoveTranslationEnabled = enabled;
}

#include "moc_monitor.cpp"
