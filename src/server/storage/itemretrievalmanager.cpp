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

#include "resourceinterface.h"

#include <shared/akdbus.h>
#include <shared/akdebug.h>

#include <QCoreApplication>
#include <QReadWriteLock>
#include <QScopedPointer>
#include <QWaitCondition>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi::Server;

ItemRetrievalManager *ItemRetrievalManager::sInstance = 0;

ItemRetrievalManager::ItemRetrievalManager(QObject *parent)
    : QObject(parent)
    , mDBusConnection(DBusConnectionPool::threadConnection())
{
    qDBusRegisterMetaType<QByteArrayList>();

    // make sure we are created from the retrieval thread and only once
    Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());
    Q_ASSERT(sInstance == 0);
    sInstance = this;

    mLock = new QReadWriteLock();
    mWaitCondition = new QWaitCondition();

    connect(mDBusConnection.interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));
    connect(this, SIGNAL(requestAdded()), this, SLOT(processRequest()),Qt::QueuedConnection);
}

ItemRetrievalManager::~ItemRetrievalManager()
{
    delete mWaitCondition;
    delete mLock;
}

ItemRetrievalManager *ItemRetrievalManager::instance()
{
    Q_ASSERT(sInstance);
    return sInstance;
}

// called within the retrieval thread
void ItemRetrievalManager::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(newOwner);
    if (oldOwner.isEmpty()) {
        return;
    }
    AkDBus::AgentType type = AkDBus::Unknown;
    const QString resourceId = AkDBus::parseAgentServiceName(serviceName, type);
    if (resourceId.isEmpty() || type != AkDBus::Resource) {
        return;
    }
    akDebug() << "Lost connection to resource" << serviceName << ", discarding cached interface";
    mResourceInterfaces.remove(resourceId);
}

// called within the retrieval thread
org::freedesktop::Akonadi::Resource *ItemRetrievalManager::resourceInterface(const QString &id)
{
    if (id.isEmpty()) {
        return 0;
    }

    org::freedesktop::Akonadi::Resource *iface = mResourceInterfaces.value(id);
    if (iface && iface->isValid()) {
        return iface;
    }

    delete iface;
    iface = new org::freedesktop::Akonadi::Resource(AkDBus::agentServiceName(id, AkDBus::Resource),
                                                    QStringLiteral("/"), mDBusConnection, this);
    if (!iface || !iface->isValid()) {
        akError() << QString::fromLatin1("Cannot connect to agent instance with identifier '%1', error message: '%2'")
                  .arg(id, iface ? iface->lastError().message() : QString());
        delete iface;
        return 0;
    }
#if QT_VERSION >= 0x040800
    // DBus calls can take some time to reply -- e.g. if a huge local mbox has to be parsed first.
    iface->setTimeout(5 * 60 * 1000);   // 5 minutes, rather than 25 seconds
#endif
    mResourceInterfaces.insert(id, iface);
    return iface;
}

// called from any thread
void ItemRetrievalManager::requestItemDelivery(qint64 uid, const QByteArray &remoteId, const QByteArray &mimeType,
                                               const QString &resource, const QVector<QByteArray> &parts)
{
    ItemRetrievalRequest *req = new ItemRetrievalRequest();
    req->id = uid;
    req->remoteId = QString::fromUtf8(remoteId);
    req->mimeType = QString::fromUtf8(mimeType);
    req->resourceId = resource;
    req->parts = parts.toList();

    requestItemDelivery(req);
}

void ItemRetrievalManager::requestItemDelivery(ItemRetrievalRequest *req)
{
    mLock->lockForWrite();
    akDebug() << "posting retrieval request for item" << req->id << " there are "
              << mPendingRequests.size() << " queues and "
              << mPendingRequests[req->resourceId].size() << " items in mine";
    mPendingRequests[req->resourceId].append(req);
    mLock->unlock();

    Q_EMIT requestAdded();

    mLock->lockForRead();
    Q_FOREVER {
        //akDebug() << "checking if request for item" << req->id << "has been processed...";
        if (req->processed) {
            QScopedPointer<ItemRetrievalRequest> reqDeleter(req);
            Q_ASSERT(!mPendingRequests[req->resourceId].contains(req));
            const QString errorMsg = req->errorMsg;
            mLock->unlock();
            if (errorMsg.isEmpty()) {
                akDebug() << "request for item" << req->id << "succeeded";
                return;
            } else {
                akDebug() << "request for item" << req->id << req->remoteId << "failed:" << errorMsg;
                throw ItemRetrieverException(errorMsg);
            }
        } else {
            akDebug() << "request for item" << req->id << "still pending - waiting";
            mWaitCondition->wait(mLock);
            akDebug() << "continuing";
        }
    }

    throw ItemRetrieverException("WTF?");
}

// called within the retrieval thread
void ItemRetrievalManager::processRequest()
{
    QVector<QPair<ItemRetrievalJob *, QString> > newJobs;

    mLock->lockForWrite();
    // look for idle resources
    for (QHash< QString, QList< ItemRetrievalRequest *> >::iterator it = mPendingRequests.begin(); it != mPendingRequests.end();) {
        if (it.value().isEmpty()) {
            it = mPendingRequests.erase(it);
            continue;
        }
        if (!mCurrentJobs.contains(it.key()) || mCurrentJobs.value(it.key()) == 0) {
            // TODO: check if there is another one for the same uid with more parts requested
            ItemRetrievalRequest *req = it.value().takeFirst();
            Q_ASSERT(req->resourceId == it.key());
            ItemRetrievalJob *job = new ItemRetrievalJob(req, this);
            connect(job, SIGNAL(requestCompleted(ItemRetrievalRequest*,QString)), SLOT(retrievalJobFinished(ItemRetrievalRequest*,QString)));
            mCurrentJobs.insert(req->resourceId, job);
            // delay job execution until after we unlocked the mutex, since the job can emit the finished signal immediately in some cases
            newJobs.append(qMakePair(job, req->resourceId));
        }
        ++it;
    }

    bool nothingGoingOn = mPendingRequests.isEmpty() && mCurrentJobs.isEmpty() && newJobs.isEmpty();
    mLock->unlock();

    if (nothingGoingOn) {   // someone asked as to process requests although everything is done already, he might still be waiting
        mWaitCondition->wakeAll();
        return;
    }

    for (QVector<QPair<ItemRetrievalJob *, QString> >::const_iterator it = newJobs.constBegin(); it != newJobs.constEnd(); ++it) {
        (*it).first->start(resourceInterface((*it).second));
    }
}

void ItemRetrievalManager::retrievalJobFinished(ItemRetrievalRequest *request, const QString &errorMsg)
{
    mLock->lockForWrite();
    request->errorMsg = errorMsg;
    request->processed = true;
    Q_ASSERT(mCurrentJobs.contains(request->resourceId));
    mCurrentJobs.remove(request->resourceId);
    // TODO check if (*it)->parts is a subset of currentRequest->parts
    for (QList<ItemRetrievalRequest *>::Iterator it = mPendingRequests[request->resourceId].begin(); it != mPendingRequests[request->resourceId].end();) {
        if ((*it)->id == request->id) {
            akDebug() << "someone else requested item" << request->id << "as well, marking as processed";
            (*it)->errorMsg = errorMsg;
            (*it)->processed = true;
            it = mPendingRequests[request->resourceId].erase(it);
        } else {
            ++it;
        }
    }
    mWaitCondition->wakeAll();
    mLock->unlock();
    Q_EMIT requestAdded(); // trigger processRequest() again, in case there is more in the queues
}

void ItemRetrievalManager::triggerCollectionSync(const QString &resource, qint64 colId)
{
    org::freedesktop::Akonadi::Resource *interface = resourceInterface(resource);
    if (interface) {
        interface->synchronizeCollection(colId);
    }
}

void ItemRetrievalManager::triggerCollectionTreeSync(const QString &resource)
{
    org::freedesktop::Akonadi::Resource *interface = resourceInterface(resource);
    if (interface) {
        interface->synchronizeCollectionTree();
    }
}
