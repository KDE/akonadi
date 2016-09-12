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

    inline QByteArray subscriber() const
    {
        return mSubscriber;
    }

public Q_SLOTS:
    bool notify(const Akonadi::Protocol::ChangeNotificationPtr &notification);

private Q_SLOTS:
    void socketReadyRead();
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

    Protocol::SubscriptionChangeNotificationPtr toChangeNotification() const;

protected Q_SLOTS:
    virtual void writeNotification(const Akonadi::Protocol::ChangeNotificationPtr &notification);

protected:
    explicit NotificationSubscriber(NotificationManager *manager = nullptr);

    void writeCommand(qint64 tag, const Protocol::CommandPtr &cmd);

    mutable QMutex mLock;
    NotificationManager *mManager;
    QLocalSocket *mSocket;
    QByteArray mSubscriber;
    QSet<Entity::Id> mMonitoredCollections;
    QSet<Entity::Id> mMonitoredItems;
    QSet<Entity::Id> mMonitoredTags;
    QSet<Protocol::ModifySubscriptionCommand::ChangeType> mMonitoredTypes;
    QSet<QString> mMonitoredMimeTypes;
    QSet<QByteArray> mMonitoredResources;
    QSet<QByteArray> mIgnoredSessions;
    QByteArray mSession;
    bool mAllMonitored;
    bool mExclusive;
    bool mNotificationDebugging;

    static QMimeDatabase sMimeDatabase;
};

} // namespace Server
} // namespace Akonadi

#endif
