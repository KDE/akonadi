/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"

#include <private/protocol_p.h>

#include <QPointer>
class QTimer;

class NotificationManagerTest;
class QThreadPool;

namespace Akonadi
{
namespace Server
{
class NotificationSubscriber;
class AggregatedCollectionFetchScope;
class AggregatedItemFetchScope;
class AggregatedTagFetchScope;

class NotificationManager : public AkThread
{
    Q_OBJECT

public:
    explicit NotificationManager(StartMode startMode = AutoStart);
    ~NotificationManager() override;

    void forgetSubscriber(NotificationSubscriber *subscriber);

    AggregatedCollectionFetchScope *collectionFetchScope() const
    {
        return mCollectionFetchScope;
    }
    AggregatedItemFetchScope *itemFetchScope() const
    {
        return mItemFetchScope;
    }
    AggregatedTagFetchScope *tagFetchScope() const
    {
        return mTagFetchScope;
    }

public Q_SLOTS:
    void registerConnection(quintptr socketDescriptor);

    void emitPendingNotifications();

    void slotNotify(const Akonadi::Protocol::ChangeNotificationList &msgs);

protected:
    void init() override;
    void quit() override;

    void emitDebugNotification(const Protocol::ChangeNotificationPtr &ntf, const QVector<QByteArray> &listeners);

private:
    Protocol::ChangeNotificationList mNotifications;
    QTimer *mTimer = nullptr;

    QThreadPool *mNotifyThreadPool = nullptr;
    QVector<QPointer<NotificationSubscriber>> mSubscribers;
    int mDebugNotifications;
    AggregatedCollectionFetchScope *mCollectionFetchScope = nullptr;
    AggregatedItemFetchScope *mItemFetchScope = nullptr;
    AggregatedTagFetchScope *mTagFetchScope = nullptr;

    bool mWaiting = false;
    bool mQuitting = false;

    friend class NotificationSubscriber;
    friend class ::NotificationManagerTest;
};

} // namespace Server
} // namespace Akonadi
