/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "itemretrievalmanager.h"
#include "itemretrievalrequest.h"
#include "itemretrievaljob.h"
#include "akonadiserver_debug.h"

#include "resourceinterface.h"

#include <private/dbus_p.h>

#include <QReadWriteLock>
#include <QScopedPointer>
#include <QWaitCondition>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::ItemRetrievalResult)

class ItemRetrievalJobFactory : public AbstractItemRetrievalJobFactory
{
    AbstractItemRetrievalJob *retrievalJob(ItemRetrievalRequest request, QObject *parent) override {
        return new ItemRetrievalJob(std::move(request), parent);
    }
};

ItemRetrievalManager::ItemRetrievalManager(QObject *parent)
    : ItemRetrievalManager(std::make_unique<ItemRetrievalJobFactory>(), parent)
{
}

ItemRetrievalManager::ItemRetrievalManager(std::unique_ptr<AbstractItemRetrievalJobFactory> factory, QObject *parent)
    : AkThread(QStringLiteral("ItemRetrievalManager"), QThread::HighPriority, parent)
    , mJobFactory(std::move(factory))
{
    qRegisterMetaType<ItemRetrievalResult>("Akonadi::Server::ItemRetrievalResult");
    qDBusRegisterMetaType<QByteArrayList>();
}

ItemRetrievalManager::~ItemRetrievalManager()
{
    quitThread();
}

void ItemRetrievalManager::init()
{
    AkThread::init();

    QDBusConnection conn = QDBusConnection::sessionBus();
    connect(conn.interface(), &QDBusConnectionInterface::serviceOwnerChanged,
            this, &ItemRetrievalManager::serviceOwnerChanged);
    connect(this, &ItemRetrievalManager::requestAdded,
            this, &ItemRetrievalManager::processRequest,
            Qt::QueuedConnection);
}

// called within the retrieval thread
void ItemRetrievalManager::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(newOwner);
    if (oldOwner.isEmpty()) {
        return;
    }
    const auto service = DBus::parseAgentServiceName(serviceName);
    if (!service.has_value() || service->agentType != DBus::Resource) {
        return;
    }
    qCDebug(AKONADISERVER_LOG) << "ItemRetrievalManager lost connection to resource" << serviceName << ", discarding cached interface";
    mResourceInterfaces.erase(service->identifier);
}

// called within the retrieval thread
org::freedesktop::Akonadi::Resource *ItemRetrievalManager::resourceInterface(const QString &id)
{
    if (id.isEmpty()) {
        return nullptr;
    }

    auto ifaceIt = mResourceInterfaces.find(id);
    if (ifaceIt != mResourceInterfaces.cend() && ifaceIt->second->isValid()) {
        return ifaceIt->second.get();
    }

    auto iface = std::make_unique<org::freedesktop::Akonadi::Resource>(
            DBus::agentServiceName(id, DBus::Resource), QStringLiteral("/"), QDBusConnection::sessionBus());
    if (!iface->isValid()) {
        qCCritical(AKONADISERVER_LOG, "Cannot connect to agent instance with identifier '%s', error message: '%s'",
                   qUtf8Printable(id), qUtf8Printable(iface ? iface->lastError().message() : QString()));
        return nullptr;
    }
    // DBus calls can take some time to reply -- e.g. if a huge local mbox has to be parsed first.
    iface->setTimeout(5 * 60 * 1000);   // 5 minutes, rather than 25 seconds
    std::tie(ifaceIt, std::ignore) = mResourceInterfaces.emplace(id, std::move(iface));
    return ifaceIt->second.get();
}

// called from any thread
void ItemRetrievalManager::requestItemDelivery(ItemRetrievalRequest req)
{
    QWriteLocker locker(&mLock);
    qCDebug(AKONADISERVER_LOG) << "ItemRetrievalManager posting retrieval request for items" << req.ids
                               << "to" <<req.resourceId << ". There are" << mPendingRequests.size() << "request queues and"
                               << mPendingRequests[req.resourceId].size() << "items mine";
    mPendingRequests[req.resourceId].emplace_back(std::move(req));
    locker.unlock();

    Q_EMIT requestAdded();
}

QVector<AbstractItemRetrievalJob *> ItemRetrievalManager::scheduleJobsForIdleResourcesLocked()
{
    QVector<AbstractItemRetrievalJob *> newJobs;
    for (auto it = mPendingRequests.begin(); it != mPendingRequests.end();) {
        if (it->second.empty()) {
            it = mPendingRequests.erase(it);
            continue;
        }

        if (!mCurrentJobs.contains(it->first) || mCurrentJobs.value(it->first) == nullptr) {
            // TODO: check if there is another one for the same uid with more parts requested
            auto req = std::move(it->second.front());
            it->second.pop_front();
            Q_ASSERT(req.resourceId == it->first);
            auto job = mJobFactory->retrievalJob(std::move(req), this);
            connect(job, &AbstractItemRetrievalJob::requestCompleted, this, &ItemRetrievalManager::retrievalJobFinished);
            mCurrentJobs.insert(job->request().resourceId, job);
            // delay job execution until after we unlocked the mutex, since the job can emit the finished signal immediately in some cases
            newJobs.append(job);
            qCDebug(AKONADISERVER_LOG) << "ItemRetrievalJob" << job << "started for request" << job->request().id;
        }
        ++it;
    }

    return newJobs;
}

// called within the retrieval thread
void ItemRetrievalManager::processRequest()
{
    QWriteLocker locker(&mLock);
    // look for idle resources
    auto newJobs = scheduleJobsForIdleResourcesLocked();
    // someone asked as to process requests although everything is done already, he might still be waiting
    if (mPendingRequests.empty() && mCurrentJobs.isEmpty() && newJobs.isEmpty()) {
        return;
    }
    locker.unlock();

    // Start the jobs
    for (auto *job : newJobs) {
        if (ItemRetrievalJob *j = qobject_cast<ItemRetrievalJob *>(job)) {
            j->setInterface(resourceInterface(j->request().resourceId));
        }
        job->start();
    }
}

namespace {

bool isSubsetOf(const QByteArrayList &superset, const QByteArrayList &subset)
{
    // For very small lists like these, this is faster than copy, sort and std::include
    return std::all_of(subset.cbegin(), subset.cend(),
                       [&superset](const auto &val) { return superset.contains(val); });
}

}

void ItemRetrievalManager::retrievalJobFinished(AbstractItemRetrievalJob *job)
{
    const auto &request = job->request();
    const auto &result = job->result();

    if (result.errorMsg.has_value()) {
        qCWarning(AKONADISERVER_LOG) << "ItemRetrievalJob for request" << request.id << "finished with error:" << *result.errorMsg;
    } else {
        qCInfo(AKONADISERVER_LOG) << "ItemRetrievalJob for request" << request.id << "finished";
    }

    QWriteLocker locker(&mLock);
    Q_ASSERT(mCurrentJobs.contains(request.resourceId));
    mCurrentJobs.remove(request.resourceId);
    // Check if there are any pending requests that are satisfied by this retrieval job
    auto &requests = mPendingRequests[request.resourceId];
    for (auto it = requests.begin(); it != requests.end();) {
        // TODO: also complete requests that are subset of the completed one
        if (it->ids == request.ids && isSubsetOf(request.parts, it->parts)) {
            qCDebug(AKONADISERVER_LOG) << "Someone else requested items " << request.ids << "as well, marking as processed.";
            ItemRetrievalResult otherResult{std::move(*it)};
            otherResult.errorMsg = result.errorMsg;
            Q_EMIT requestFinished(otherResult);
            it = requests.erase(it);
        } else {
            ++it;
        }
    }
    locker.unlock();

    Q_EMIT requestFinished(result);
    Q_EMIT requestAdded(); // trigger processRequest() again, in case there is more in the queues
}

// Can be called from any thread
void ItemRetrievalManager::triggerCollectionSync(const QString &resource, qint64 colId)
{
    QTimer::singleShot(0, this, [this, resource, colId]() {
        if (auto *interface = resourceInterface(resource)) {
            interface->synchronizeCollection(colId);
        }
    });
}

void ItemRetrievalManager::triggerCollectionTreeSync(const QString &resource)
{
    QTimer::singleShot(0, this, [this, resource]() {
        if (auto *interface = resourceInterface(resource)) {
            interface->synchronizeCollectionTree();
        }
    });
}
