/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_AGENTSEARCHMANAGER_H
#define AKONADI_AGENTSEARCHMANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QMutex>
#include <QWaitCondition>
#include "exception.h"

namespace Akonadi {

class AkonadiConnection;
class AgentSearchRequest;
class AgentSearchInstance;

class SearchResultsRetriever;

class AgentSearchTask
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

class AgentSearchManager : public QObject
{
  Q_OBJECT

  public:
    static AgentSearchManager *instance();

    ~AgentSearchManager();

    void registerInstance( const QString &id );
    void unregisterInstance( const QString &id );

    void addTask( AgentSearchTask *task );

    void pushResults( const QByteArray &searchId, const QSet<qint64> &ids,
                      AkonadiConnection *connection );


  private Q_SLOTS:
    void searchLoop();

  private:
    class ResourceTask {
      public:
        QString resourceId;
        qint64 collectionId;
        AgentSearchTask *parentTask;
        QSet<qint64> results;

        qint64 timestamp;
    };

    typedef QMap<QString /* resource */, ResourceTask *>  TasksMap;

    static AgentSearchManager *sInstance;
    AgentSearchManager();
    void stop();
    bool mShouldStop;

    TasksMap::Iterator cancelRunningTask( TasksMap::Iterator &iter );

    QMap<QString, AgentSearchInstance* > mInstances;
    QMutex mInstancesLock;

    QWaitCondition mWait;
    QMutex mLock;

    QVector<AgentSearchTask*> mTasklist;

    QMap<QString /* resource */, ResourceTask *> mRunningTasks;
    QVector<ResourceTask *> mPendingResults;

    friend class AgentSearchManagerThread;
};

AKONADI_EXCEPTION_MAKE_INSTANCE( AgentSearchException );

}

#endif // AKONADI_AGENTSEARCHMANAGER_H
