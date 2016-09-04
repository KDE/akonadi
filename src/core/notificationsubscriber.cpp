/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

namespace Akonadi
{
class AKONADICORE_NO_EXPORT NotificationSubscriber::Private : public QSharedData
{
public:
    explicit Private()
        : QSharedData()
        , isAllMonitored(false)
        , isExclusive(false)
    {}

    Private(const Private &other)
        : QSharedData(other)
        , subscriber(other.subscriber)
        , sessionId(other.sessionId)
        , collections(other.collections)
        , items(other.items)
        , tags(other.tags)
        , types(other.types)
        , mimeTypes(other.mimeTypes)
        , resources(other.resources)
        , ignoredSessions(other.ignoredSessions)
        , isAllMonitored(other.isAllMonitored)
        , isExclusive(other.isExclusive)
    {}

    QByteArray subscriber;
    QByteArray sessionId;
    QSet<qint64> collections;
    QSet<qint64> items;
    QSet<qint64> tags;
    QSet<Monitor::Type> types;
    QSet<QString> mimeTypes;
    QSet<QByteArray> resources;
    QSet<QByteArray> ignoredSessions;
    bool isAllMonitored;
    bool isExclusive;
};

}

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
