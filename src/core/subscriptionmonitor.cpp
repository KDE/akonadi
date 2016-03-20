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

#include "subscriptionmonitor.h"
#include "monitor_p.h"

using namespace Akonadi;

namespace Akonadi
{

class SubscriberPrivate : public QSharedData
{
public:
    SubscriberPrivate()
        : QSharedData()
        , allMonitored(false)
        , isExclusive(false)
    {}

    SubscriberPrivate(const SubscriberPrivate &other)
        : QSharedData(other)
        , name(other.name)
        , sessionId(other.sessionId)
        , collections(other.collections)
        , items(other.items)
        , tags(other.tags)
        , types(other.types)
        , mimeTypes(other.mimeTypes)
        , resources(other.resources)
        , ignoredSessions(other.ignoredSessions)
        , allMonitored(other.allMonitored)
        , isExclusive(other.isExclusive)
    {}

    QByteArray name;
    QByteArray sessionId;
    QSet<Collection::Id> collections;
    QSet<Item::Id> items;
    QSet<Tag::Id> tags;
    QSet<Subscriber::Type> types;
    QSet<QString> mimeTypes;
    QSet<QByteArray> resources;
    QSet<QByteArray> ignoredSessions;
    bool allMonitored;
    bool isExclusive;
};

class SubscriptionMonitorPrivate : public MonitorPrivate
{
public:
    SubscriptionMonitorPrivate(SubscriptionMonitor *qq)
        : MonitorPrivate(Q_NULLPTR, qq)
    {
        pendingModification.startMonitoringType(Protocol::ModifySubscriptionCommand::SubscriptionChanges);
    }

    void slotNotify(const Protocol::ChangeNotification &msg) Q_DECL_OVERRIDE
    {
        if (msg.type() == Protocol::Command::SubscriptionChangeNotification) {
            emitSubscriptionNotification(msg);
        } else {
            MonitorPrivate::slotNotify(msg);
        }
    }

    void emitSubscriptionNotification(const Protocol::SubscriptionChangeNotification &msg)
    {
        Q_Q(SubscriptionMonitor);

        Subscriber subscriber;
        subscriber.d->name = msg.subscriber();
        if (!msg.isRemove()) {
            // NOTE: I have slightly over-designed the change notifications to
            // have the "added" and "removed" API without realizing we don't really
            // need that in the client API. The server does not use the "removed"
            // API either but if we need that in the future, we can easily change
            // it to use it (i.e. to send incremental changes rather than current
            // full state of subscription) and adjust the signal APIs in this class
            subscriber.d->sessionId = msg.sessionId();
            subscriber.d->collections = msg.addedCollections();
            subscriber.d->items = msg.addedItems();
            subscriber.d->tags = msg.addedTags();
            Q_FOREACH (Protocol::ModifySubscriptionCommand::ChangeType type, msg.addedTypes()) {
                subscriber.d->types.insert(static_cast<Subscriber::Type>(type));
            }
            subscriber.d->mimeTypes = msg.addedMimeTypes();
            subscriber.d->resources = msg.addedResources();
            subscriber.d->ignoredSessions = msg.addedIgnoredSessions();
            subscriber.d->allMonitored = msg.isAllMonitored();
            subscriber.d->isExclusive = msg.isExclusive();
        }

        switch (msg.operation()) {
        case Protocol::SubscriptionChangeNotification::Add:
            Q_EMIT q->subscriptionAdded(subscriber);
            break;
        case Protocol::SubscriptionChangeNotification::Modify:
            Q_EMIT q->subscriptionChanged(subscriber);
            break;
        case Protocol::SubscriptionChangeNotification::Remove:
            Q_EMIT q->subscriptionRemoved(subscriber.name());
            break;
        default:
            Q_ASSERT_X(false, __FUNCTION__, "Unknown operation type");
            break;
        }
    }

private:
    Q_DECLARE_PUBLIC(SubscriptionMonitor)
};

}

Subscriber::Subscriber()
    : d(new SubscriberPrivate)
{
}

Subscriber::Subscriber(const Subscriber &other)
    : d(other.d)
{
}

Subscriber::~Subscriber()
{
}

Subscriber &Subscriber::operator=(const Subscriber &other)
{
    if (*this == other) {
        return *this;
    }
    d = other.d;
    return *this;
}

bool Subscriber::operator==(const Subscriber &other) const
{
    return d->name == other.d->name
            && d->sessionId == other.d->sessionId
            && d->collections == other.d->collections
            && d->items == other.d->items
            && d->tags == other.d->tags
            && d->types == other.d->types
            && d->mimeTypes == other.d->mimeTypes
            && d->resources == other.d->resources
            && d->ignoredSessions == other.d->ignoredSessions
            && d->allMonitored == other.d->allMonitored
            && d->isExclusive == other.d->isExclusive;
}

QByteArray Subscriber::name() const
{
    return d->name;
}

QByteArray Subscriber::sessionId() const
{
    return d->sessionId;
}

QSet<Collection::Id> Subscriber::monitoredCollections() const
{
    return d->collections;
}

QSet<Item::Id> Subscriber::monitoredItems() const
{
    return d->items;
}

QSet<Tag::Id> Subscriber::monitoredTags() const
{
    return d->tags;
}

QSet<Subscriber::Type> Subscriber::monitoredTypes() const
{
    return d->types;
}

QSet<QString> Subscriber::monitoredMimeTypes() const
{
    return d->mimeTypes;
}

QSet<QByteArray> Subscriber::monitoredResources() const
{
    return d->resources;
}

QSet<QByteArray> Subscriber::ignoredSessions() const
{
    return d->ignoredSessions;
}

bool Subscriber::monitorsAll() const
{
    return d->allMonitored;
}

bool Subscriber::isExclusive() const
{
    return d->isExclusive;
}




SubscriptionMonitor::SubscriptionMonitor(QObject *parent)
    : Monitor(new SubscriptionMonitorPrivate(this), parent)
{
}

SubscriptionMonitor::~SubscriptionMonitor()
{
}

void SubscriptionMonitor::setSession(Akonadi::Session *session)
{
    Monitor::setSession(session);
}

Session *SubscriptionMonitor::session() const
{
    return Monitor::session();
}


