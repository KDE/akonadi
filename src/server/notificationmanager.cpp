/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Michael Jansen <kde@michael-jansen>

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

#include "notificationmanager.h"
#include "notificationsubscriber.h"
#include "storage/notificationcollector.h"
#include "tracer.h"
#include "akonadiserver_debug.h"
#include "aggregatedfetchscope.h"
#include "storage/collectionstatistics.h"
#include "handlerhelper.h"

#include <private/standarddirs_p.h>
#include <private/scope_p.h>

#include <QSettings>
#include <QThreadPool>
#include <QPointer>
#include <QDateTime>
#include <QEventLoop>
#include <QLocalSocket>

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationManager::NotificationManager()
    : AkThread(QStringLiteral("NotificationManager"))
    , mTimer(nullptr)
    , mNotifyThreadPool(nullptr)
    , mDebugNotifications(0)
{
}

NotificationManager::~NotificationManager()
{
    quitThread();
}

void NotificationManager::init()
{
    AkThread::init();

    const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
    QSettings settings(serverConfigFile, QSettings::IniFormat);

    mTimer = new QTimer(this);
    mTimer->setInterval(settings.value(QStringLiteral("NotificationManager/Interval"), 50).toInt());
    mTimer->setSingleShot(true);
    connect(mTimer, &QTimer::timeout,
            this, &NotificationManager::emitPendingNotifications);

    mNotifyThreadPool = new QThreadPool(this);
    mNotifyThreadPool->setMaxThreadCount(5);

    mCollectionFetchScope = new AggregatedCollectionFetchScope();
    mItemFetchScope = new AggregatedItemFetchScope();
    mTagFetchScope = new AggregatedTagFetchScope();
}

void NotificationManager::quit()
{
    mQuitting = true;
    if (mEventLoop) {
        mEventLoop->quit();
        return;
    }

    mTimer->stop();
    delete mTimer;

    mNotifyThreadPool->clear();
    mNotifyThreadPool->waitForDone();
    delete mNotifyThreadPool;

    qDeleteAll(mSubscribers);

    delete mCollectionFetchScope;
    delete mItemFetchScope;
    delete mTagFetchScope;

    AkThread::quit();
}

void NotificationManager::registerConnection(quintptr socketDescriptor)
{
    Q_ASSERT(thread() == QThread::currentThread());

    NotificationSubscriber *subscriber = new NotificationSubscriber(this, socketDescriptor);
    qCInfo(AKONADISERVER_LOG) << "New notification connection (registered as" << subscriber << ")";
    connect(subscriber, &NotificationSubscriber::notificationDebuggingChanged,
            this, [this](bool enabled) {
                if (enabled) {
                    ++mDebugNotifications;
                } else {
                    --mDebugNotifications;
                }
                Q_ASSERT(mDebugNotifications >= 0);
                Q_ASSERT(mDebugNotifications <= mSubscribers.count());
            });

    mSubscribers.push_back(subscriber);
}
void NotificationManager::forgetSubscriber(NotificationSubscriber *subscriber)
{
    Q_ASSERT(QThread::currentThread() == thread());
    mSubscribers.removeAll(subscriber);
}

void NotificationManager::slotNotify(const Protocol::ChangeNotificationList &msgs)
{
    Q_ASSERT(QThread::currentThread() == thread());
    for (const auto &msg : msgs) {
        switch (msg->type()) {
        case Protocol::Command::CollectionChangeNotification:
            Protocol::CollectionChangeNotification::appendAndCompress(mNotifications, msg);
            continue;
        case Protocol::Command::ItemChangeNotification:
        case Protocol::Command::TagChangeNotification:
        case Protocol::Command::RelationChangeNotification:
        case Protocol::Command::SubscriptionChangeNotification:
        case Protocol::Command::DebugChangeNotification:
            mNotifications.push_back(msg);
            continue;

        default:
            Q_ASSERT_X(false, "slotNotify", "Invalid notification type!");
            continue;
        }
    }

    if (!mTimer->isActive()) {
        mTimer->start();
    }
}

class NotifyRunnable : public QRunnable
{
public:
    explicit NotifyRunnable(NotificationSubscriber *subscriber,
                            const Protocol::ChangeNotificationList &notifications)
        : mSubscriber(subscriber)
        , mNotifications(notifications)
    {
    }

    ~NotifyRunnable()
    {
    }

    void run() override {
        for (const auto &ntf : qAsConst(mNotifications))
        {
            if (mSubscriber) {
                mSubscriber->notify(ntf);
            } else {
                break;
            }
        }
    }

private:
    QPointer<NotificationSubscriber> mSubscriber;
    Protocol::ChangeNotificationList mNotifications;
};

void NotificationManager::emitPendingNotifications()
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (mNotifications.isEmpty()) {
        return;
    }

    if (mDebugNotifications == 0) {
        for (NotificationSubscriber *subscriber : qAsConst(mSubscribers)) {
            if (subscriber) {
                mNotifyThreadPool->start(new NotifyRunnable(subscriber, mNotifications));
            }
        }
    } else {
        // When debugging notification we have to use a non-threaded approach
        // so that we can work with return value of notify()
        for (const auto &notification : qAsConst(mNotifications)) {
            QVector<QByteArray> listeners;
            for (NotificationSubscriber *subscriber : qAsConst(mSubscribers)) {
                if (subscriber && subscriber->notify(notification)) {
                    listeners.push_back(subscriber->subscriber());
                }
            }

            emitDebugNotification(notification, listeners);
        }
    }

    mNotifications.clear();
}

void NotificationManager::emitDebugNotification(const Protocol::ChangeNotificationPtr &ntf,
        const QVector<QByteArray> &listeners)
{
    auto debugNtf = Protocol::DebugChangeNotificationPtr::create();
    debugNtf->setNotification(ntf);
    debugNtf->setListeners(listeners);
    debugNtf->setTimestamp(QDateTime::currentMSecsSinceEpoch());
    for (NotificationSubscriber *subscriber : qAsConst(mSubscribers)) {
        if (subscriber) {
            mNotifyThreadPool->start(new NotifyRunnable(subscriber, { debugNtf }));
        }
    }
}
