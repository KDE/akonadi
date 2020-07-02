/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notificationsubscriber.h"
#include "itemfetchscope.h"
#include "collectionfetchscope.h"
#include "tagfetchscope.h"

namespace Akonadi
{
class AKONADICORE_NO_EXPORT NotificationSubscriber::Private : public QSharedData
{
public:
    QByteArray subscriber;
    QByteArray sessionId;
    QSet<qint64> collections;
    QSet<qint64> items;
    QSet<qint64> tags;
    QSet<Monitor::Type> types;
    QSet<QString> mimeTypes;
    QSet<QByteArray> resources;
    QSet<QByteArray> ignoredSessions;
    ItemFetchScope itemFetchScope;
    CollectionFetchScope collectionFetchScope;
    TagFetchScope tagFetchScope;
    bool isAllMonitored = false;
    bool isExclusive = false;
};

} // namespace Akonadi

using namespace Akonadi;

NotificationSubscriber::NotificationSubscriber()
    : d(new Private)
{
}

NotificationSubscriber::NotificationSubscriber(const NotificationSubscriber &other)
    : d(other.d)
{
}

NotificationSubscriber::~NotificationSubscriber()
{
}

NotificationSubscriber &NotificationSubscriber::operator=(const NotificationSubscriber &other)
{
    d = other.d;
    return *this;
}

bool NotificationSubscriber::isValid() const
{
    return !d->subscriber.isEmpty();
}

QByteArray NotificationSubscriber::subscriber() const
{
    return d->subscriber;
}

void NotificationSubscriber::setSubscriber(const QByteArray &subscriber)
{
    d->subscriber = subscriber;
}

QByteArray NotificationSubscriber::sessionId() const
{
    return d->sessionId;
}

void NotificationSubscriber::setSessionId(const QByteArray &sessionId)
{
    d->sessionId = sessionId;
}

QSet<qint64> NotificationSubscriber::monitoredCollections() const
{
    return d->collections;
}

void NotificationSubscriber::setMonitoredCollections(const QSet<qint64> &collections)
{
    d->collections = collections;
}

QSet<qint64> NotificationSubscriber::monitoredItems() const
{
    return d->items;
}

void NotificationSubscriber::setMonitoredItems(const QSet<qint64> &items)
{
    d->items = items;
}

QSet<qint64> NotificationSubscriber::monitoredTags() const
{
    return d->tags;
}

void NotificationSubscriber::setMonitoredTags(const QSet<qint64> &tags)
{
    d->tags = tags;
}

QSet<Monitor::Type> NotificationSubscriber::monitoredTypes() const
{
    return d->types;
}

void NotificationSubscriber::setMonitoredTypes(const QSet<Monitor::Type> &types)
{
    d->types = types;
}

QSet<QString> NotificationSubscriber::monitoredMimeTypes() const
{
    return d->mimeTypes;
}

void NotificationSubscriber::setMonitoredMimeTypes(const QSet<QString> &mimeTypes)
{
    d->mimeTypes = mimeTypes;
}

QSet<QByteArray> NotificationSubscriber::monitoredResources() const
{
    return d->resources;
}

void NotificationSubscriber::setMonitoredResources(const QSet<QByteArray> &resources)
{
    d->resources = resources;
}

QSet<QByteArray> NotificationSubscriber::ignoredSessions() const
{
    return d->ignoredSessions;
}

void NotificationSubscriber::setIgnoredSessions(const QSet<QByteArray> &ignoredSessions)
{
    d->ignoredSessions = ignoredSessions;
}

bool NotificationSubscriber::isAllMonitored() const
{
    return d->isAllMonitored;
}

void NotificationSubscriber::setIsAllMonitored(bool isAllMonitored)
{
    d->isAllMonitored = isAllMonitored;
}

bool NotificationSubscriber::isExclusive() const
{
    return d->isExclusive;
}

void NotificationSubscriber::setIsExclusive(bool isExclusive)
{
    d->isExclusive = isExclusive;
}

ItemFetchScope NotificationSubscriber::itemFetchScope() const
{
    return d->itemFetchScope;
}

void NotificationSubscriber::setItemFetchScope(const ItemFetchScope &itemFetchScope)
{
    d->itemFetchScope = itemFetchScope;
}

CollectionFetchScope NotificationSubscriber::collectionFetchScope() const
{
    return d->collectionFetchScope;
}

void NotificationSubscriber::setCollectionFetchScope(const CollectionFetchScope &fetchScope)
{
    d->collectionFetchScope = fetchScope;
}

TagFetchScope NotificationSubscriber::tagFetchScope() const
{
    return d->tagFetchScope;
}

void NotificationSubscriber::setTagFetchScope(const TagFetchScope &tagFetchScope)
{
    d->tagFetchScope = tagFetchScope;
}
