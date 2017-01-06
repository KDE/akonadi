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

#ifndef AKONADI_SEARCHTASKMANAGER_H
#define AKONADI_SEARCHTASKMANAGER_H

#include "akthread.h"

#include <QMap>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QMutex>
#include <QWaitCondition>
#include "exception.h"
#include "agentmanagerinterface.h"

namespace Akonadi
{
namespace Server
{

class AkonadiServer;
class Connection;
class AgentSearchInstance;

class SearchTask
{
public:
    QByteArray id;
    QString query;
    QStringList mimeTypes;
    QVector<qint64> collections;
    bool complete;

    QMutex sharedLock;
    QWaitCondition notifier;

    QVector<QPair<QString /* resource */, qint64 /* collection */> > queries;
    QSet<qint64> pendingResults;
};

class SearchTaskManager : public AkThread
{
    Q_OBJECT

public:
    static SearchTaskManager *instance();

    ~SearchTaskManager();

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

    typedef QMap<QString /* resource */, ResourceTask *>  TasksMap;

    static SearchTaskManager *sInstance;

    explicit SearchTaskManager();
    bool mShouldStop;

    TasksMap::Iterator cancelRunningTask(TasksMap::Iterator &iter);
    bool allResourceTasksCompleted(SearchTask *agentSearchTask) const;

    QMap<QString, AgentSearchInstance *> mInstances;
    QMutex mInstancesLock;

    QWaitCondition mWait;
    QMutex mLock;

    QVector<SearchTask *> mTasklist;

    QMap<QString /* resource */, ResourceTask *> mRunningTasks;
    QVector<ResourceTask *> mPendingResults;

    friend class AkonadiServer;
};

AKONADI_EXCEPTION_MAKE_INSTANCE(SearchException);

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SEARCHTASKMANAGER_H
