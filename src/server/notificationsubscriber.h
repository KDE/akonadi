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

#include <private/protocol_p.h>
#include "entities.h"

class QLocalSocket;

namespace Akonadi {
namespace Server {


class NotificationSubscriber : public QObject
{
    Q_OBJECT

public:
    explicit NotificationSubscriber(quintptr socketDescriptor);
    ~NotificationSubscriber();

    void notify(const Protocol::ChangeNotification::List &notifications);

private Q_SLOTS:
    void socketReadyRead();
    void socketDisconnected();

protected:
    void registerSubscriber(const Protocol::CreateSubscriptionCommand &command);
    void modifySubscription(const Protocol::ModifySubscriptionCommand &command);
    void disconnectSubscriber();

private:
    bool acceptsNotification(const Protocol::ChangeNotification &notification);
    bool isCollectionMonitored(Entity::Id id) const;
    bool isMimeTypeMonitored(const QString &mimeType) const;
    bool isMoveDestinationResourceMonitored(const Protocol::ChangeNotification &msg) const;

protected:
    explicit NotificationSubscriber();

    virtual void writeNotification(const Protocol::ChangeNotification &notification);
    void writeCommand(qint64 tag, const Protocol::Command &cmd);

    QLocalSocket *mSocket;
    QByteArray mSubscriber;
    QSet<Entity::Id> mMonitoredCollections;
    QSet<Entity::Id> mMonitoredItems;
    QSet<Entity::Id> mMonitoredTags;
    // TODO: Make this a bitflag
    QSet<Protocol::ChangeNotification::Type> mMonitoredTypes;
    QSet<QString> mMonitoredMimeTypes;
    QSet<QByteArray> mMonitoredResources;
    QSet<QByteArray> mIgnoredSessions;
    QByteArray mSession;
    bool mAllMonitored;
    bool mExclusive;

    static QMimeDatabase sMimeDatabase;
};

} // namespace Server
} // namespace Akonadi


#endif
