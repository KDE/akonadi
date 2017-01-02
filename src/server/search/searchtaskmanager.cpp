/*
    Copyright (c) 2013, 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "searchtaskmanager.h"
#include "agentsearchinstance.h"
#include "connection.h"
#include "storage/selectquerybuilder.h"
#include "dbusconnectionpool.h"
#include "entities.h"
#include "akonadiserver_debug.h"

#include <private/dbus_p.h>

#include <QSqlError>
#include <QTimer>
#include <QTime>

using namespace Akonadi;
using namespace Akonadi::Server;

SearchTaskManager *SearchTaskManager::sInstance = nullptr;

SearchTaskManager::SearchTaskManager()
    : AkThread()
    , mShouldStop(false)
{
    setObjectName(QStringLiteral("SearchTaskManager"));
    sInstance = this;

    QTimer::singleShot(0, this, &SearchTaskManager::searchLoop);
}

SearchTaskManager::~SearchTaskManager()
{
    QMutexLocker locker(&mLock);
    mShouldStop = true;
    mWait.wakeAll();
    locker.unlock();

    quitThread();

    mInstancesLock.lock();
    qDeleteAll(mInstances);
    mInstancesLock.unlock();
}

SearchTaskManager *SearchTaskManager::instance()
{
    Q_ASSERT(sInstance);
    return sInstance;
}

void SearchTaskManager::registerInstance(const QString &id)
{
    QMutexLocker locker(&mInstancesLock);

    qCDebug(AKONADISERVER_LOG) << "SearchManager::registerInstance(" << id << ")";

    AgentSearchInstance *instance = mInstances.value(id);
    if (instance) {
        return; // already registered
    }

    instance = new AgentSearchInstance(id);
    if (!instance->init()) {
        qCDebug(AKONADISERVER_LOG) << "Failed to initialize Search agent";
        delete instance;
        return;
    }

    qCDebug(AKONADISERVER_LOG) << "Registering search instance " << id;
    mInstances.insert(id, instance);
}

void SearchTaskManager::unregisterInstance(const QString &id)
{
    QMutexLocker locker(&mInstancesLock);

    QMap<QString, AgentSearchInstance *>::Iterator it = mInstances.find(id);
    if (it != mInstances.end()) {
        qCDebug(AKONADISERVER_LOG) << "Unregistering search instance" << id;
        it.value()->deleteLater();
        mInstances.erase(it);
    }
}

void SearchTaskManager::addTask(SearchTask *task)
{
    QueryBuilder qb(Collection::tableName());
    qb.addJoin(QueryBuilder::InnerJoin, Resource::tableName(),
               Collection::resourceIdFullColumnName(),
               Resource::idFullColumnName());
    qb.addColumn(Collection::idFullColumnName());
    qb.addColumn(Resource::nameFullColumnName());

    Q_ASSERT(!task->collections.isEmpty());
    QVariantList list;
    list.reserve(task->collections.size());
    Q_FOREACH (qint64 collection, task->collections) {
        list << collection;
    }
    qb.addValueCondition(Collection::idFullColumnName(), Query::In, list);

    if (!qb.exec()) {
        throw SearchException(qb.query().lastError().text());
    }

    QSqlQuery query = qb.query();
    if (!query.next()) {
        return;
    }

    mInstancesLock.lock();

    org::freedesktop::Akonadi::AgentManager agentManager(DBus::serviceName(DBus::Control), QStringLiteral("/AgentManager"),
                                                         DBusConnectionPool::threadConnection());
    do {
        const QString resourceId = query.value(1).toString();
        if (!mInstances.contains(resourceId)) {
            qCDebug(AKONADISERVER_LOG) << "Resource" << resourceId << "does not implement Search interface, skipping";
        } else if (!agentManager.agentInstanceOnline(resourceId)) {
            qCDebug(AKONADISERVER_LOG) << "Agent" << resourceId << "is offline, skipping";
        } else if (agentManager.agentInstanceStatus(resourceId) > 2) {     // 2 == Broken, 3 == Not Configured
            qCDebug(AKONADISERVER_LOG) << "Agent" << resourceId << "is broken or not configured";
        } else {
            const qint64 collectionId = query.value(0).toLongLong();
            qCDebug(AKONADISERVER_LOG) << "Enqueued search query (" << resourceId << ", " << collectionId << ")";
            task->queries << qMakePair(resourceId,  collectionId);
        }
    } while (query.next());
    mInstancesLock.unlock();

    QMutexLocker locker(&mLock);
    mTasklist.append(task);
    mWait.wakeAll();
}

void SearchTaskManager::pushResults(const QByteArray &searchId, const QSet<qint64> &ids,
                                    Connection *connection)
{
    Q_UNUSED(searchId);

    qCDebug(AKONADISERVER_LOG) << ids.count() << "results for search" << searchId << "pushed from" << connection->context()->resource().name();

    QMutexLocker locker(&mLock);
    ResourceTask *task = mRunningTasks.take(connection->context()->resource().name());
    if (!task) {
        qCDebug(AKONADISERVER_LOG) << "No running task for" << connection->context()->resource().name() << " - maybe it has timed out?";
        return;
    }

    if (task->parentTask->id != searchId) {
        qCDebug(AKONADISERVER_LOG) << "Received results for different search - maybe the original task has timed out?";
        qCDebug(AKONADISERVER_LOG) << "Search is" << searchId << ", but task is" << task->parentTask->id;
        return;
    }

    task->results = ids;
    mPendingResults.append(task);

    mWait.wakeAll();
}

bool SearchTaskManager::allResourceTasksCompleted(SearchTask *agentSearchTask) const
{
    // Check for queries pending to be dispatched
    if (!agentSearchTask->queries.isEmpty()) {
        return false;
    }

    // Check for running queries
    QMap<QString, ResourceTask *>::const_iterator it = mRunningTasks.begin();
    QMap<QString, ResourceTask *>::const_iterator end = mRunningTasks.end();
    for (; it != end; ++it) {
        if (it.value()->parentTask == agentSearchTask) {
            return false;
        }
    }

    return true;
}

SearchTaskManager::TasksMap::Iterator SearchTaskManager::cancelRunningTask(TasksMap::Iterator &iter)
{
    ResourceTask *task = iter.value();
    SearchTask *parentTask = task->parentTask;
    QMutexLocker locker(&parentTask->sharedLock);
    //erase the task before allResourceTasksCompleted
    SearchTaskManager::TasksMap::Iterator it = mRunningTasks.erase(iter);
    // We're not clearing the results since we don't want to clear successful results from other resources
    parentTask->complete = allResourceTasksCompleted(parentTask);
    parentTask->notifier.wakeAll();
    delete task;

    return it;
}

void SearchTaskManager::searchLoop()
{
    qint64 timeout = ULONG_MAX;

    QMutexLocker locker(&mLock);

    Q_FOREVER {
        qCDebug(AKONADISERVER_LOG) << "Search loop is waiting, will wake again in" << timeout << "ms";
        mWait.wait(&mLock, timeout);

        if (mShouldStop) {
            Q_FOREACH (SearchTask *task, mTasklist) {
                QMutexLocker locker(&task->sharedLock);
                task->queries.clear();
                task->notifier.wakeAll();
            }

            QMap<QString, ResourceTask *>::Iterator it = mRunningTasks.begin();
            for (; it != mRunningTasks.end();) {
                if (mTasklist.contains(it.value()->parentTask)) {
                    delete it.value();
                    it = mRunningTasks.erase(it);
                    continue;
                }
                it = cancelRunningTask(it);
            }

            break;
        }

        // First notify about available results
        while (!mPendingResults.isEmpty()) {
            ResourceTask *finishedTask = mPendingResults.first();
            mPendingResults.remove(0);
            qCDebug(AKONADISERVER_LOG) << "Pending results from" << finishedTask->resourceId << "for collection" << finishedTask->collectionId << "for search" << finishedTask->parentTask->id << "available!";
            SearchTask *parentTask = finishedTask->parentTask;
            QMutexLocker locker(&parentTask->sharedLock);
            // We need to append, this agent search task is shared
            parentTask->pendingResults += finishedTask->results;
            parentTask->complete = allResourceTasksCompleted(parentTask);
            parentTask->notifier.wakeAll();
            delete finishedTask;
        }

        // No check whether there are any tasks running longer than 1 minute and kill them
        QMap<QString, ResourceTask *>::Iterator it = mRunningTasks.begin();
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (; it != mRunningTasks.end();) {
            ResourceTask *task = it.value();
            if (now - task->timestamp > 60 * 1000) {
                // Remove the task - and signal to parent task that it has "finished" without results
                qCDebug(AKONADISERVER_LOG) << "Resource task" << task->resourceId << "for search" << task->parentTask->id << "timed out!";
                it = cancelRunningTask(it);
            } else {
                ++it;
            }
        }

        if (!mTasklist.isEmpty()) {
            SearchTask *task = mTasklist.first();
            qCDebug(AKONADISERVER_LOG) << "Search task" << task->id << "available!";
            if (task->queries.isEmpty()) {
                qCDebug(AKONADISERVER_LOG) << "nothing to do for task";
                QMutexLocker locker(&task->sharedLock);
                //After this the AgentSearchTask will be destroyed
                task->complete = true;
                task->notifier.wakeAll();
                mTasklist.remove(0);
                continue;
            }

            QVector<QPair<QString, qint64> >::iterator it = task->queries.begin();
            for (; it != task->queries.end();) {
                if (!mRunningTasks.contains(it->first)) {
                    qCDebug(AKONADISERVER_LOG) << "\t Sending query for collection" << it->second << "to resource" << it->first;
                    ResourceTask *rTask = new ResourceTask;
                    rTask->resourceId = it->first;
                    rTask->collectionId = it->second;
                    rTask->parentTask = task;
                    rTask->timestamp = QDateTime::currentMSecsSinceEpoch();
                    mRunningTasks.insert(it->first, rTask);

                    mInstancesLock.lock();
                    AgentSearchInstance *instance = mInstances.value(it->first);
                    if (!instance) {
                        mInstancesLock.unlock();
                        // Resource disappeared in the meanwhile
                        continue;
                    }

                    instance->search(task->id, task->query, it->second);
                    mInstancesLock.unlock();

                    task->sharedLock.lock();
                    it = task->queries.erase(it);
                    task->sharedLock.unlock();
                } else {
                    ++it;
                }
            }
            // Yay! We managed to dispatch all requests!
            if (task->queries.isEmpty()) {
                qCDebug(AKONADISERVER_LOG) << "All queries from task" << task->id << "dispatched!";
                mTasklist.remove(0);
            }

            timeout = 60 * 1000; // check whether all tasks have finished within a minute
        } else {
            if (mRunningTasks.isEmpty()) {
                timeout = ULONG_MAX;
            }
        }
    }
}
