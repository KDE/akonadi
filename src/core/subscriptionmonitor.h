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

#ifndef AKONADI_SUBSCRIPTIONMONITOR
#define AKONADI_SUBSCRIPTIONMONITOR

#include "monitor.h"
#include "akonadicore_export.h"


namespace Akonadi
{

class SubscriptionMonitorPrivate;
class SubscriberPrivate;
class AKONADICORE_EXPORT Subscriber
{
public:
    enum Type {
        InvalidType,
        Collection,
        Item,
        Tag,
        Relation,
        Subscription
    };

    explicit Subscriber();
    Subscriber(const Subscriber &other);
    ~Subscriber();

    Subscriber &operator=(const Subscriber &other);
    bool operator==(const Subscriber &other) const;

    QByteArray name() const;
    QByteArray sessionId() const;

    QSet<Collection::Id> monitoredCollections() const;
    QSet<Item::Id> monitoredItems() const;
    QSet<Tag::Id> monitoredTags() const;
    QSet<Type> monitoredTypes() const;
    QSet<QString> monitoredMimeTypes() const;
    QSet<QByteArray> monitoredResources() const;
    QSet<QByteArray> ignoredSessions() const;
    bool monitorsAll() const;
    bool isExclusive() const;

private:
    QSharedDataPointer<SubscriberPrivate> d;

    friend class SubscriptionMonitorPrivate;
};

class SubscriptionMonitorPrivate;
/**
 * @short Monitors change notification subscription changes
 *
 * The SubscriptionMonitor is a specialized Monitor which monitors only for
 * changes in notification subscriptions.
 *
 * This class is only useful for developer tools like Akonadi Console which
 * want to show and introspect the notification system and should not ever be
 * user in normal applications.
 *
 * @since 16.04
 */
class AKONADICORE_EXPORT SubscriptionMonitor : protected Monitor
{
    Q_OBJECT

public:
    explicit SubscriptionMonitor(QObject *parent = Q_NULLPTR);

    ~SubscriptionMonitor();

    void setSession(Session *session);

    Session *session() const;

Q_SIGNALS:
    /**
     * This signal is emitted when a new Monitor subscribes to notifications.
     * Once this monitor is set up and registered to Akonadi it will also be
     * emitted once for each existing subscriber so that applications can
     * initially populate their list of subscribers.
     */
    void subscriptionAdded(const Subscriber &subscription);

    /**
     * This signal is emitted when an existing subscriber changes its subscription
     * settings.
     */
    void subscriptionChanged(const Subscriber &subscription);

    /**
     * This signal is emitted when an existing subscriber unsubscribes from the
     * server, i.e. when a Monitor is destroyed.
     */
    void subscriptionRemoved(const QByteArray &subscriber);

private:
    friend class SubscriptionMonitorPrivate;
};

}

#endif
