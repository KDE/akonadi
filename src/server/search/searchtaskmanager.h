/*
    SPDX-FileCopyrightText: 2013, 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"

#include "agentmanagerinterface.h"
#include "exception.h"
#include <QList>
#include <QMap>
#include <QMutex>
#include <QSet>
#include <QStringList>
#include <QWaitCondition>

namespace Akonadi
{
namespace Server
{
class Connection;
class AgentSearchInstance;

class SearchTask
{
public:
    QByteArray id;
    QString query;
    QStringList mimeTypes;
    QList<qint64> collections;
    bool complete;

    QMutex sharedLock;
    QWaitCondition notifier;

    QList<QPair<QString /* resource */, qint64 /* collection */>> queries;
    QSet<qint64> pendingResults;
};

class SearchTaskManager : public AkThread
{
    Q_OBJECT

protected:
    /**
     * Use AkThread::create() to create and start a new SearchTaskManager thread.
     */
    explicit SearchTaskManager();

public:
    ~SearchTaskManager() override;

    void registerInstance(const QString &id);
    void unregisterInstance(const QString &id);

    void addTask(SearchTask *task);

    void pushResults(const QByteArray &searchId, const QSet<qint64> &ids, Connection *connection);

private Q_SLOTS:
    void searchLoop();

private:
    class ResourceTask
    {
    public:
        QString resourceId;
        qint64 collectionId;
        SearchTask *parentTask;
        QSet<qint64> results;

        qint64 timestamp;
    };

    using TasksMap = QMap<QString, ResourceTask *>;

    bool mShouldStop;

    TasksMap::Iterator cancelRunningTask(TasksMap::Iterator &iter);
    bool allResourceTasksCompleted(SearchTask *agentSearchTask) const;

    QMap<QString, AgentSearchInstance *> mInstances;
    QMutex mInstancesLock;

    QWaitCondition mWait;
    QMutex mLock;

    QList<SearchTask *> mTasklist;

    QMap<QString /* resource */, ResourceTask *> mRunningTasks;
    QList<ResourceTask *> mPendingResults;
};

AKONADI_EXCEPTION_MAKE_INSTANCE(SearchException);

} // namespace Server
} // namespace Akonadi
