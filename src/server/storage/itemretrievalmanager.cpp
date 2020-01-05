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
#include "dbusconnectionpool.h"
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

class ItemRetrievalJobFactory : public AbstractItemRetrievalJobFactory
{
    AbstractItemRetrievalJob *retrievalJob(ItemRetrievalRequest *request, QObject *parent) override {
        return new ItemRetrievalJob(request, parent);
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
    qDBusRegisterMetaType<QByteArrayList>();
}

ItemRetrievalManager::~ItemRetrievalManager()
{
    quitThread();
}

void ItemRetrievalManager::init()
{
    AkThread::init();

    QDBusConnection conn = DBusConnectionPool::threadConnection();
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
            DBus::agentServiceName(id, DBus::Resource), QStringLiteral("/"), DBusConnectionPool::threadConnection());
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
void ItemRetrievalManager::requestItemDelivery(ItemRetrievalRequest *req)
{
    QWriteLocker locker(&mLock);
    qCDebug(AKONADISERVER_LOG) << "ItemRetrievalManager posting retrieval request for items" << req->ids
                               << "to" <<req->resourceId << ". There are" << mPendingRequests.size() << "request queues and"
                               << mPendingRequests[req->resourceId].size() << "items mine";
    mPendingRequests[req->resourceId].append(req);
    locker.unlock();

    Q_EMIT requestAdded();
}

// called within the retrieval thread
void ItemRetrievalManager::processRequest()
{
    QVector<QPair<AbstractItemRetrievalJob *, QString> > newJobs;
    QWriteLocker locker(&mLock);
    // look for idle resources
    for (auto it = mPendingRequests.begin(); it != mPendingRequests.end();) {
        if (it.value().isEmpty()) {
            it = mPendingRequests.erase(it);
            continue;
        }
        if (!mCurrentJobs.contains(it.key()) || mCurrentJobs.value(it.key()) == nullptr) {
            // TODO: check if there is another one for the same uid with more parts requested
            ItemRetrievalRequest *req = it.value().takeFirst();
            Q_ASSERT(req->resourceId == it.key());
            AbstractItemRetrievalJob *job = mJobFactory->retrievalJob(req, this);
            connect(job, &AbstractItemRetrievalJob::requestCompleted, this, &ItemRetrievalManager::retrievalJobFinished);
            mCurrentJobs.insert(req->resourceId, job);
            // delay job execution until after we unlocked the mutex, since the job can emit the finished signal immediately in some cases
            newJobs.append(qMakePair(job, req->resourceId));
            qCDebug(AKONADISERVER_LOG) << "ItemRetrievalJob" << job << "started for request" << req;
        }
        ++it;
    }

    bool nothingGoingOn = mPendingRequests.isEmpty() && mCurrentJobs.isEmpty() && newJobs.isEmpty();
    locker.unlock();

    if (nothingGoingOn) {   // someone asked as to process requests although everything is done already, he might still be waiting
        return;
    }

    for (auto it = newJobs.constBegin(), end = newJobs.constEnd(); it != end; ++it) {
        if (ItemRetrievalJob *j = qobject_cast<ItemRetrievalJob *>((*it).first)) {
            j->setInterface(resourceInterface((*it).second));
        }
        (*it).first->start();
    }
}

void ItemRetrievalManager::retrievalJobFinished(ItemRetrievalRequest *request, const QString &errorMsg)
{
    if (errorMsg.isEmpty()) {
        qCInfo(AKONADISERVER_LOG) << "ItemRetrievalJob for request" << request << "finished";
    } else {
        qCWarning(AKONADISERVER_LOG) << "ItemRetrievalJob for request" << request << "finished with error:" << errorMsg;
    }
    QWriteLocker locker(&mLock);
    request->errorMsg = errorMsg;
    request->processed = true;
    Q_ASSERT(mCurrentJobs.contains(request->resourceId));
    mCurrentJobs.remove(request->resourceId);
    // TODO check if (*it)->parts is a subset of currentRequest->parts
    for (QList<ItemRetrievalRequest *>::Iterator it = mPendingRequests[request->resourceId].begin(); it != mPendingRequests[request->resourceId].end();) {
        if ((*it)->ids == request->ids) {
            qCDebug(AKONADISERVER_LOG) << "someone else requested item" << request->ids << "as well, marking as processed";
            (*it)->errorMsg = errorMsg;
            (*it)->processed = true;
            Q_EMIT requestFinished(*it);
            it = mPendingRequests[request->resourceId].erase(it);
        } else {
            ++it;
        }
    }
    locker.unlock();
    Q_EMIT requestFinished(request);
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
