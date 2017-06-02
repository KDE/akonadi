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
#include "storage/selectquerybuilder.h"
#include "handlerhelper.h"

#include <private/standarddirs_p.h>

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
    , mCollectionFetchScope(nullptr)
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
    delete mTagFetchScope;

    AkThread::quit();
}

void NotificationManager::registerConnection(quintptr socketDescriptor)
{
    Q_ASSERT(thread() == QThread::currentThread());

    NotificationSubscriber *subscriber = new NotificationSubscriber(this, socketDescriptor);
    qCDebug(AKONADISERVER_LOG) << "New notification connection (registered as" << subscriber << ")";
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
    if (mEventLoop) {
        mEventLoop->quit();
    }
    if (!mWaiting) {
        mWaiting = true;
        QTimer::singleShot(0, this, &NotificationManager::waitForSocketData);
    }
}

void NotificationManager::waitForSocketData()
{
    mWaiting = true;
    while (!mSubscribers.isEmpty()) {
        QEventLoop loop;
        mEventLoop = &loop;
        for (const auto sub : qAsConst(mSubscribers)) {
            if (sub) {
                connect(sub->socket(), &QLocalSocket::readyRead, &loop, &QEventLoop::quit);
                connect(sub->socket(), &QLocalSocket::disconnected, &loop, &QEventLoop::quit);
            }
        }
        loop.exec();
        mEventLoop = nullptr;

        if (mQuitting) {
            QTimer::singleShot(0, this, &NotificationManager::quit);
            break;
        }

        for (const auto sub : qAsConst(mSubscribers)) {
            if (sub) {
                sub->handleIncomingData();
            }
        }
    }
    mWaiting = false;
}

void NotificationManager::forgetSubscriber(NotificationSubscriber *subscriber)
{
    Q_ASSERT(QThread::currentThread() == thread());
    mSubscribers.removeAll(subscriber);
    if (mEventLoop) {
        mEventLoop->quit();
    }
}

void NotificationManager::connectNotificationCollector(NotificationCollector *collector)
{
    connect(collector, &NotificationCollector::notify,
            this, &NotificationManager::slotNotify);
}

void NotificationManager::slotNotify(const Protocol::ChangeNotificationList &msgs)
{
    Q_ASSERT(QThread::currentThread() == thread());

    for (const auto &msg : msgs) {
        if (msg->type() == Protocol::Command::CollectionChangeNotification) {
            auto &cmsg = Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg);
            auto msgCollection = cmsg.collection();

            // Make sure we have all the data
            if (!mCollectionFetchScope->fetchIdOnly() && msgCollection->name().isEmpty()) {
                const auto col = Collection::retrieveById(msgCollection->id());
                const auto mts = col.mimeTypes();
                QStringList mimeTypes;
                mimeTypes.reserve(mts.size());
                for (const auto &mt : mts) {
                    mimeTypes.push_back(mt.name());
                }
                msgCollection = HandlerHelper::fetchCollectionsResponse(col, {}, false, 0, {}, {}, false, mimeTypes);
            }
            // Get up-to-date statistics
            if (mCollectionFetchScope->fetchStatistics()) {
                Collection col;
                col.setId(msgCollection->id());
                const auto stats = CollectionStatistics::self()->statistics(col);
                msgCollection->setStatistics(Protocol::FetchCollectionStatsResponse(stats.count, stats.count - stats.read, stats.size));
            }
            // Get attributes
            const auto requestedAttrs = mCollectionFetchScope->attributes();
            auto msgColAttrs = msgCollection->attributes();
            // TODO: This assumes that we have either none or all attributes in msgCollection
            if (msgColAttrs.isEmpty() && !requestedAttrs.isEmpty()) {
                SelectQueryBuilder<CollectionAttribute> qb;
                qb.addColumn(CollectionAttribute::typeFullColumnName());
                qb.addColumn(CollectionAttribute::valueFullColumnName());
                qb.addValueCondition(CollectionAttribute::collectionIdFullColumnName(),
                                     Query::Equals, msgCollection->id());
                Query::Condition cond(Query::Or);
                for (const auto &attr : requestedAttrs) {
                    cond.addValueCondition(CollectionAttribute::typeFullColumnName(), Query::Equals, attr);
                }
                qb.addCondition(cond);
                if (!qb.exec()) {
                    qCWarning(AKONADISERVER_LOG) << "Failed to obtain collection attributes!";
                }
                const auto attrs = qb.result();
                for (const auto &attr : attrs)  {
                    msgColAttrs.insert(attr.type(), attr.value());
                }
                msgCollection->setAttributes(msgColAttrs);
            }

            Protocol::CollectionChangeNotification::appendAndCompress(mNotifications, msg);
        } else if (msg->type() == Protocol::Command::TagChangeNotification) {
            auto &tmsg = Protocol::cmdCast<Protocol::TagChangeNotification>(msg);
            auto msgTag = tmsg.tag();

            if (!mTagFetchScope->fetchIdOnly() && msgTag->gid().isEmpty()) {
                msgTag = HandlerHelper::fetchTagsResponse(Tag::retrieveById(msgTag->id()), false);
            }

            const auto requestedAttrs = mTagFetchScope->attributes();
            auto msgTagAttrs = msgTag->attributes();
            if (msgTagAttrs.isEmpty() && !requestedAttrs.isEmpty()) {
                SelectQueryBuilder<TagAttribute> qb;
                qb.addColumn(TagAttribute::typeFullColumnName());
                qb.addColumn(TagAttribute::valueFullColumnName());
                qb.addValueCondition(TagAttribute::tagIdFullColumnName(), Query::Equals, msgTag->id());
                Query::Condition cond(Query::Or);
                for (const auto &attr : requestedAttrs) {
                    cond.addValueCondition(TagAttribute::typeFullColumnName(), Query::Equals, attr);
                }
                qb.addCondition(cond);
                if (!qb.exec()) {
                    qCWarning(AKONADISERVER_LOG) << "Failed to obtain tag attributes!";
                }
                const auto attrs = qb.result();
                for (const auto &attr : attrs) {
                    msgTagAttrs.insert(attr.type(), attr.value());
                }
                msgTag->setAttributes(msgTagAttrs);
            }
            mNotifications.push_back(msg);

        } else {
            mNotifications.push_back(msg);
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
            mNotifyThreadPool->start(new NotifyRunnable(subscriber, mNotifications));
        }
    } else {
        // When debugging notification we have to use a non-threaded approach
        // so that we can work with return value of notify()
        for (const auto &notification : qAsConst(mNotifications)) {
            QVector<QByteArray> listeners;
            for (NotificationSubscriber *subscriber : qAsConst(mSubscribers)) {
                if (subscriber->notify(notification)) {
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
        mNotifyThreadPool->start(new NotifyRunnable(subscriber, { debugNtf }));
    }
}
