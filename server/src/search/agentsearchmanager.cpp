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

using namespace Akonadi;

AgentSearchManager *AgentSearchManager::sInstance = 0;

AgentSearchManager::AgentSearchManager()
  : QObject()
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
    delete it.value();
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
      task->queries << qMakePair( resourceId, query.value( 0 ).toLongLong() );
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

  mLock.lock();
  ResourceTask *task = mRunningTasks.take( connection->resourceContext().name() );
  Q_ASSERT( task );
  Q_ASSERT( task->parentTask->id == searchId );

  task->results = ids;
  mPendingResults.append( task );
  mLock.unlock();

  mWait.wakeAll();
}

void AgentSearchManager::searchLoop()
{
  Q_FOREVER {
    mWait.wait( &mLock );

    mLock.lock();
    // First notify about available results
    while( !mPendingResults.isEmpty() ) {
      ResourceTask *finishedTask = mPendingResults.first();
      AgentSearchTask *parentTask = finishedTask->parentTask;
      parentTask->sharedLock.lock();
      parentTask->pendingResults = finishedTask->results;
      parentTask->sharedLock.unlock();
      parentTask->notifier.wakeAll();

      mPendingResults.remove( 0 );
      delete finishedTask;
    }

    if ( !mTasklist.isEmpty() ) {
      AgentSearchTask *task = mTasklist.first();
      QVector<QPair<QString,qint64> >::iterator it = task->queries.begin();
      for ( ; it != task->queries.end(); ) {
        if ( !mRunningTasks.contains( it->first ) ) {
          ResourceTask *rTask = new ResourceTask;
          rTask->resourceId = it->first;
          rTask->collectionId = it->second;
          rTask->parentTask = task;
          mRunningTasks.insert( it->first, rTask );

          mInstancesLock.lock();
          AgentSearchInstance *instance = mInstances.value( it->first );
          mInstancesLock.unlock();
          Q_ASSERT ( instance );

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
        mTasklist.remove( 0 );
      }
    }
    mLock.unlock();
  }
}
