/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef NOTIFICATIONSUBSCRIBER_H
#define NOTIFICATIONSUBSCRIBER_H

#include <QObject>
#include <QByteArray>
#include <QMimeDatabase>
#include <QMutex>

#include <private/protocol_p.h>
#include "entities.h"

class QLocalSocket;

namespace Akonadi
{
namespace Server
{

class NotificationManager;

class NotificationSubscriber : public QObject
{
    Q_OBJECT

public:
    explicit NotificationSubscriber(NotificationManager *manager, quintptr socketDescriptor);
    ~NotificationSubscriber();

    Q_REQUIRED_RESULT inline QByteArray subscriber() const
    {
        return mSubscriber;
    }

    Q_REQUIRED_RESULT QLocalSocket *socket() const
    {
        return mSocket;
    }

    void handleIncomingData();

public Q_SLOTS:
    bool notify(const Akonadi::Protocol::ChangeNotificationPtr &notification);

private Q_SLOTS:
    void socketDisconnected();

Q_SIGNALS:
    void notificationDebuggingChanged(bool enabled);

protected:
    void registerSubscriber(const Protocol::CreateSubscriptionCommand &command);
    void modifySubscription(const Protocol::ModifySubscriptionCommand &command);
    void disconnectSubscriber();

private:
    bool acceptsNotification(const Protocol::ChangeNotification &notification) const;
    bool acceptsItemNotification(const Protocol::ItemChangeNotification &notification) const;
    bool acceptsCollectionNotification(const Protocol::CollectionChangeNotification &notification) const;
    bool acceptsTagNotification(const Protocol::TagChangeNotification &notification) const;
    bool acceptsRelationNotification(const Protocol::RelationChangeNotification &notification) const;
    bool acceptsSubscriptionNotification(const Protocol::SubscriptionChangeNotification &notification) const;
    bool acceptsDebugChangeNotification(const Protocol::DebugChangeNotification &notification) const;

    bool isCollectionMonitored(Entity::Id id) const;
    bool isMimeTypeMonitored(const QString &mimeType) const;
    bool isMoveDestinationResourceMonitored(const Protocol::ItemChangeNotification &msg) const;
    bool isMoveDestinationResourceMonitored(const Protocol::CollectionChangeNotification &msg) const;

    Protocol::CollectionChangeNotificationPtr customizeCollection(const Protocol::CollectionChangeNotificationPtr &msg);
    Protocol::SubscriptionChangeNotificationPtr toChangeNotification() const;

protected Q_SLOTS:
    virtual void writeNotification(const Akonadi::Protocol::ChangeNotificationPtr &notification);

protected:
    explicit NotificationSubscriber(NotificationManager *manager = nullptr);

    void writeCommand(qint64 tag, const Protocol::CommandPtr &cmd);

    mutable QMutex mLock;
    NotificationManager *mManager = nullptr;
    QLocalSocket *mSocket = nullptr;
    QByteArray mSubscriber;
    QSet<Entity::Id> mMonitoredCollections;
    QSet<Entity::Id> mMonitoredItems;
    QSet<Entity::Id> mMonitoredTags;
    QSet<Protocol::ModifySubscriptionCommand::ChangeType> mMonitoredTypes;
    QSet<QString> mMonitoredMimeTypes;
    QSet<QByteArray> mMonitoredResources;
    QSet<QByteArray> mIgnoredSessions;
    QByteArray mSession;
    Protocol::ItemFetchScope mItemFetchScope;
    Protocol::CollectionFetchScope mCollectionFetchScope;
    Protocol::TagFetchScope mTagFetchScope;
    bool mAllMonitored;
    bool mExclusive;
    bool mNotificationDebugging;

    static QMimeDatabase sMimeDatabase;
};

} // namespace Server
} // namespace Akonadi

#endif
