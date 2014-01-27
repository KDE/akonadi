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

#include "agentsearchmanager.h"
#include "agentsearchinstance.h"
#include "akdebug.h"
#include "akonadiconnection.h"
#include "storage/selectquerybuilder.h"
#include <entities.h>

#include <QSqlError>
#include <QTimer>
#include <QTime>

using namespace Akonadi;

AgentSearchManager *AgentSearchManager::sInstance = 0;

AgentSearchManager::AgentSearchManager()
  : QObject()
  , mShouldStop( false )
{
  sInstance = this;

  QTimer::singleShot(0, this, SLOT(searchLoop()) );
}

AgentSearchManager::~AgentSearchManager()
{
  mInstancesLock.lock();
  qDeleteAll( mInstances );
  mInstancesLock.unlock();
}

AgentSearchManager* AgentSearchManager::instance()
{
  Q_ASSERT( sInstance );
  return sInstance;
}

void AgentSearchManager::stop()
{
  mLock.lock();
  mShouldStop = true;
  mWait.wakeAll();
  mLock.unlock();
}

void AgentSearchManager::registerInstance( const QString &id )
{
  QMutexLocker locker( &mInstancesLock );

  akDebug() << "SearchManager::registerInstance(" << id << ")";

  AgentSearchInstance *instance = mInstances.value( id );
  if ( instance ) {
    return; // already registered
  }

  instance = new AgentSearchInstance( id );
  if ( !instance->init() ) {
    akDebug() << "Failed to initialize Search agent";
    delete instance;
    return;
  }

  akDebug() << "Registering search instance " << id;
  mInstances.insert( id, instance );
}

void AgentSearchManager::unregisterInstance( const QString &id )
{
  QMutexLocker locker( &mInstancesLock );

  QMap<QString, AgentSearchInstance*>::Iterator it = mInstances.find( id );
  if ( it != mInstances.end() ) {
    akDebug() << "Unregistering search instance" << id;
    it.value()->deleteLater();
    mInstances.erase( it );
  }
}

void AgentSearchManager::addTask( AgentSearchTask *task )
{
  QueryBuilder qb( Collection::tableName() );
  qb.addJoin( QueryBuilder::InnerJoin, Resource::tableName(),
              Collection::resourceIdFullColumnName(),
              Resource::idFullColumnName());
  qb.addColumn( Collection::idFullColumnName() );
  qb.addColumn( Resource::nameFullColumnName() );

  QVariantList list;
  Q_FOREACH ( qint64 collection, task->collections ) {
    list << collection;
  }
  qb.addValueCondition( Collection::idFullColumnName(), Query::In, list );

  if ( !qb.exec() ) {
    throw AgentSearchException( qb.query().lastError().text() );
  }

  QSqlQuery query = qb.query();
  if ( !query.next() ) {
    return;
  }

  mInstancesLock.lock();
  do {
    const QString resourceId = query.value( 1 ).toString();
    if ( !mInstances.contains( resourceId ) ) {
      akDebug() << "Resource" << resourceId << "does not implement Search interface, skipping";
    } else {
      const qint64 collectionId = query.value( 0 ).toLongLong();
      akDebug() << "Enqueued search query (" << resourceId << ", " << collectionId << ")";
      task->queries << qMakePair( resourceId,  collectionId );
    }
  } while ( query.next() );
  mInstancesLock.unlock();

  mLock.lock();
  mTasklist.append( task );
  mLock.unlock();

  mWait.wakeAll();
}


void AgentSearchManager::pushResults( const QByteArray &searchId, const QSet<qint64> &ids,
                                      AkonadiConnection* connection )
{
  Q_UNUSED( searchId );

  akDebug() << ids.count() << "results for search" << searchId << "pushed from" << connection->resourceContext().name();

  QMutexLocker locker( &mLock );
  ResourceTask *task = mRunningTasks.take( connection->resourceContext().name() );
  if ( !task ) {
    akDebug() << "No running task for" << connection->resourceContext().name() << " - maybe it has timed out?";
    return;
  }

  if ( task->parentTask->id != searchId ) {
    akDebug() << "Received results for different search - maybe the original task has timed out?";
    akDebug() << "Search is" << searchId << ", but task is" << task->parentTask->id;
    return;
  }

  task->results = ids;
  mPendingResults.append( task );

  mWait.wakeAll();
}

AgentSearchManager::TasksMap::Iterator AgentSearchManager::cancelRunningTask( TasksMap::Iterator &iter )
{
  ResourceTask *task = iter.value();
  AgentSearchTask *parentTask = task->parentTask;
  parentTask->sharedLock.lock();
  parentTask->pendingResults.clear();
  parentTask->complete = true;
  parentTask->sharedLock.unlock();
  parentTask->notifier.wakeAll();
  delete task;

  return mRunningTasks.erase( iter );
}

void AgentSearchManager::searchLoop()
{
  qint64 timeout = ULONG_MAX;

  mLock.lock();
  Q_FOREVER {
    akDebug() << "Search loop is waiting, will wake again in" << timeout << "ms";
    mWait.wait( &mLock, timeout ); // wait for a minute

    if ( mShouldStop ) {
      Q_FOREACH (AgentSearchTask *task, mTasklist ) {
        task->sharedLock.lock();
        task->queries.clear();
        task->sharedLock.unlock();
        task->notifier.wakeAll();
      }

      QMap<QString,ResourceTask*>::Iterator it = mRunningTasks.begin();
      for ( ; it != mRunningTasks.end(); ) {
        if ( mTasklist.contains( it.value()->parentTask ) ) {
          delete it.value();
          it = mRunningTasks.erase( it );
          continue;
        }
        it = cancelRunningTask( it );
      }

      break;
    }

    // First notify about available results
    while( !mPendingResults.isEmpty() ) {
      ResourceTask *finishedTask = mPendingResults.first();
      mPendingResults.remove( 0 );
      akDebug() << "Pending results for search" << finishedTask->parentTask->id << "available!";
      AgentSearchTask *parentTask = finishedTask->parentTask;
      parentTask->sharedLock.lock();
      parentTask->pendingResults = finishedTask->results;
      parentTask->complete = true;
      parentTask->sharedLock.unlock();
      parentTask->notifier.wakeAll();
      delete finishedTask;
    }

    // No check whether there are any tasks running longer than 1 minute and kill them
    QMap<QString,ResourceTask*>::Iterator it = mRunningTasks.begin();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for ( ; it != mRunningTasks.end(); ) {
      ResourceTask *task = it.value();
      if ( now - task->timestamp > 60 * 1000 ) {
        // Remove the task - and signal to parent task that it has "finished" without results
        akDebug() << "Resource task" << task->resourceId << "for search" << task->parentTask->id << "timed out!";
        it = cancelRunningTask( it );
      } else {
        ++it;
      }
    }

    if ( !mTasklist.isEmpty() ) {
      AgentSearchTask *task = mTasklist.first();
      akDebug() << "Search task" << task->id << "available!";
      if ( task->queries.isEmpty() ) {
        akDebug() << "nothing to do for task";
        task->sharedLock.lock();
        //After this the AgentSearchTask will be destroyed
        task->complete = true;
        task->sharedLock.unlock();

        mTasklist.remove( 0 );
        continue;
      }

      QVector<QPair<QString,qint64> >::iterator it = task->queries.begin();
      for ( ; it != task->queries.end(); ) {
        if ( !mRunningTasks.contains( it->first ) ) {
          akDebug() << "\t Sending query to resource" << it->first;
          ResourceTask *rTask = new ResourceTask;
          rTask->resourceId = it->first;
          rTask->collectionId = it->second;
          rTask->parentTask = task;
          rTask->timestamp = QDateTime::currentMSecsSinceEpoch();
          mRunningTasks.insert( it->first, rTask );

          mInstancesLock.lock();
          AgentSearchInstance *instance = mInstances.value( it->first );
          mInstancesLock.unlock();
          if ( !instance ) {
            // Resource disappeared in the meanwhile
            continue;
          }

          instance->search( task->id, task->query, it->second );

          task->sharedLock.lock();
          it = task->queries.erase( it );
          task->sharedLock.unlock();
        } else {
          ++it;
        }
      }
      // Yay! We managed to dispatch all requests!
      if ( task->queries.isEmpty() ) {
        akDebug() << "All queries from task" << task->id << "dispatched!";
        mTasklist.remove( 0 );
      }

      timeout = 60 * 1000; // check whether all tasks have finished within a minute
    } else {
      if ( mRunningTasks.isEmpty() ) {
        timeout = ULONG_MAX;
      }
    }
  }
  mLock.unlock();
}
