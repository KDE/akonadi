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

#include <qdebug.h>


#include <iterator>

using namespace Akonadi;

Monitor::Monitor(QObject *parent)
    : QObject(parent)
    , d_ptr(new MonitorPrivate(nullptr, this))
{
    d_ptr->init();
    d_ptr->connectToNotificationManager();

    ChangeMediator::registerMonitor(this);
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
        d->pendingModification.startMonitoringCollection(collection.id());
        d->scheduleSubscriptionUpdate();
    } else if (!monitored) {
        if (d->collections.removeAll(collection)) {
            d->pendingModification.stopMonitoringCollection(collection.id());
            d->scheduleSubscriptionUpdate();
        }
    }

    emit collectionMonitored(collection, monitored);
}

void Monitor::setItemMonitored(const Item &item, bool monitored)
{
    Q_D(Monitor);
    if (!d->items.contains(item.id()) && monitored) {
        d->items.insert(item.id());
        d->pendingModification.startMonitoringItem(item.id());
        d->scheduleSubscriptionUpdate();
    } else if (!monitored) {
        if (d->items.remove(item.id())) {
            d->pendingModification.stopMonitoringItem(item.id());
            d->scheduleSubscriptionUpdate();
        }
    }

    emit itemMonitored(item,  monitored);
}

void Monitor::setResourceMonitored(const QByteArray &resource, bool monitored)
{
    Q_D(Monitor);
    if (!d->resources.contains(resource) && monitored) {
        d->resources.insert(resource);
        d->pendingModification.startMonitoringResource(resource);
        d->scheduleSubscriptionUpdate();
    } else if (!monitored) {
        if (d->resources.remove(resource)) {
            d->pendingModification.stopMonitoringResource(resource);
            d->scheduleSubscriptionUpdate();
        }
    }

    emit resourceMonitored(resource, monitored);
}

void Monitor::setMimeTypeMonitored(const QString &mimetype, bool monitored)
{
    Q_D(Monitor);
    if (!d->mimetypes.contains(mimetype) && monitored) {
        d->mimetypes.insert(mimetype);
        d->pendingModification.startMonitoringMimeType(mimetype);
        d->scheduleSubscriptionUpdate();
    } else if (!monitored) {
        if (d->mimetypes.remove(mimetype)) {
            d->pendingModification.stopMonitoringMimeType(mimetype);
            d->scheduleSubscriptionUpdate();
        }
    }

    emit mimeTypeMonitored(mimetype, monitored);
}

void Monitor::setTagMonitored(const Akonadi::Tag &tag, bool monitored)
{
    Q_D(Monitor);
    if (!d->tags.contains(tag.id()) && monitored) {
        d->tags.insert(tag.id());
        d->pendingModification.startMonitoringTag(tag.id());
        d->scheduleSubscriptionUpdate();
    } else if (!monitored) {
        if (d->tags.remove(tag.id())) {
            d->pendingModification.stopMonitoringTag(tag.id());
            d->scheduleSubscriptionUpdate();
        }
    }

    emit tagMonitored(tag, monitored);
}

void Monitor::setTypeMonitored(Monitor::Type type, bool monitored)
{
    Q_D(Monitor);
    if (!d->types.contains(type) && monitored) {
        d->types.insert(type);
        d->pendingModification.startMonitoringType(static_cast<Protocol::ModifySubscriptionCommand::ChangeType>(type));
        d->scheduleSubscriptionUpdate();
    } else if (!monitored) {
        if (d->types.remove(type)) {
            d->pendingModification.stopMonitoringType(static_cast<Protocol::ModifySubscriptionCommand::ChangeType>(type));
            d->scheduleSubscriptionUpdate();
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

    d->pendingModification.setAllMonitored(monitored);
    d->scheduleSubscriptionUpdate();

    emit allMonitored(monitored);
}

void Monitor::setExclusive(bool exclusive)
{
    Q_D(Monitor);
    d->exclusive = exclusive;
    d->pendingModification.setIsExclusive(exclusive);
    d->scheduleSubscriptionUpdate();
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
        d->pendingModification.startIgnoringSession(session->sessionId());
        d->scheduleSubscriptionUpdate();
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
    d->scheduleSubscriptionUpdate();
}

ItemFetchScope &Monitor::itemFetchScope()
{
    Q_D(Monitor);
    d->pendingModificationChanges |= Protocol::ModifySubscriptionCommand::ItemFetchScope;
    d->scheduleSubscriptionUpdate();
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

QVector<Item::Id> Monitor::itemsMonitoredEx() const
{
    Q_D(const Monitor);
    QVector<Item::Id> result;
    result.reserve(d->items.size());
    std::copy(d->items.begin(), d->items.end(), std::back_inserter(result));
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
    std::copy(d->tags.begin(), d->tags.end(), std::back_inserter(result));
    return result;
}

QVector<Monitor::Type> Monitor::typesMonitored() const
{
    Q_D(const Monitor);
    QVector<Monitor::Type> result;
    result.reserve(d->types.size());
    std::copy(d->types.begin(), d->types.end(), std::back_inserter(result));
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
    d->tagCache->setSession(d->session);

    // Reconnect with a new session
    d->connectToNotificationManager();
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
