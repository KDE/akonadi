/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "resourcescheduler_p.h"

#include <kdebug.h>

#include <QtCore/QTimer>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnectionInterface>
#include <boost/graph/graph_concepts.hpp>

using namespace Akonadi;

qint64 ResourceScheduler::Task::latestSerial = 0;
static QDBusAbstractInterface *s_resourcetracker = 0;

//@cond PRIVATE

ResourceScheduler::ResourceScheduler( QObject *parent ) :
    QObject( parent ),
    mOnline( false )
{
}

void ResourceScheduler::scheduleFullSync()
{
  Task t;
  t.type = SyncAll;
  TaskList& queue = queueForTaskType( t.type );
  if ( queue.contains( t ) || mCurrentTask == t )
    return;
  queue << t;
  signalTaskToTracker( t, "SyncAll" );
  scheduleNext();
}

void ResourceScheduler::scheduleCollectionTreeSync()
{
  Task t;
  t.type = SyncCollectionTree;
  TaskList& queue = queueForTaskType( t.type );
  if ( queue.contains( t ) || mCurrentTask == t )
    return;
  queue << t;
  signalTaskToTracker( t, "SyncCollectionTree" );
  scheduleNext();
}

void ResourceScheduler::scheduleSync(const Collection & col)
{
  Task t;
  t.type = SyncCollection;
  t.collection = col;
  TaskList& queue = queueForTaskType( t.type );
  if ( queue.contains( t ) || mCurrentTask == t )
    return;
  queue << t;
  signalTaskToTracker( t, "SyncCollection" );
  scheduleNext();
}

void ResourceScheduler::scheduleItemFetch(const Item & item, const QSet<QByteArray> &parts, const QDBusMessage & msg)
{
  Task t;
  t.type = FetchItem;
  t.item = item;
  t.itemParts = parts;

  // if the current task does already fetch the requested item, break here but
  // keep the dbus message, so we can send the reply later on
  if ( mCurrentTask == t ) {
    mCurrentTask.dbusMsgs << msg;
    return;
  }

  // If this task is already in the queue, merge with it.
  TaskList& queue = queueForTaskType( t.type );
  const int idx = queue.indexOf( t );
  if ( idx != -1 ) {
    queue[ idx ].dbusMsgs << msg;
    return;
  }

  t.dbusMsgs << msg;
  queue << t;
  signalTaskToTracker( t, "FetchItem" );
  scheduleNext();
}

void ResourceScheduler::scheduleResourceCollectionDeletion()
{
  Task t;
  t.type = DeleteResourceCollection;
  TaskList& queue = queueForTaskType( t.type );
  if ( queue.contains( t ) || mCurrentTask == t )
    return;
  queue << t;
  signalTaskToTracker( t, "DeleteResourceCollection" );
  scheduleNext();
}

void ResourceScheduler::scheduleChangeReplay()
{
  Task t;
  t.type = ChangeReplay;
  TaskList& queue = queueForTaskType( t.type );
  if ( queue.contains( t ) || mCurrentTask == t )
    return;
  queue << t;
  signalTaskToTracker( t, "ChangeReplay" );
  scheduleNext();
}

void Akonadi::ResourceScheduler::scheduleFullSyncCompletion()
{
  Task t;
  t.type = SyncAllDone;
  TaskList& queue = queueForTaskType( t.type );
  if ( queue.contains( t ) || mCurrentTask == t )
    return;
  queue << t;
  signalTaskToTracker( t, "SyncAllDone" );
  scheduleNext();
}

void Akonadi::ResourceScheduler::scheduleCustomTask( QObject *receiver, const char* methodName, const QVariant &argument, ResourceBase::SchedulePriority priority )
{
  Task t;
  t.type = Custom;
  t.receiver = receiver;
  t.methodName = methodName;
  t.argument = argument;
  QueueType queueType = GenericTaskQueue;
  if ( priority == ResourceBase::AfterChangeReplay )
    queueType = AfterChangeReplayQueue;
  TaskList& queue = mTaskList[ queueType ];

  if ( queue.contains( t ) )
    return;

  switch (priority) {
  case ResourceBase::Prepend:
    queue.prepend( t );
    break;
  default:
    queue.append(t);
    break;
  }

  signalTaskToTracker( t, "Custom-" + t.methodName );
  scheduleNext();
}

void ResourceScheduler::taskDone()
{
  if ( isEmpty() )
    emit status( AgentBase::Idle );

  if ( s_resourcetracker ) {
    QList<QVariant> argumentList;
    argumentList << QString::number( mCurrentTask.serial )
                 << QString();
    s_resourcetracker->asyncCallWithArgumentList(QLatin1String("jobEnded"), argumentList);
  }

  mCurrentTask = Task();
  scheduleNext();
}

void ResourceScheduler::deferTask()
{
  if ( s_resourcetracker ) {
    QList<QVariant> argumentList;
    argumentList << QString::number( mCurrentTask.serial )
                 << QString();
    s_resourcetracker->asyncCallWithArgumentList(QLatin1String("jobEnded"), argumentList);
  }

  Task t = mCurrentTask;
  mCurrentTask = Task();
  mTaskList[GenericTaskQueue] << t;
  signalTaskToTracker( t, "DeferedTask" );

  scheduleNext();
}

bool ResourceScheduler::isEmpty()
{
  for ( int i = 0; i < NQueueCount; ++i ) {
    if ( !mTaskList[i].isEmpty() )
      return false;
  }
  return true;
}

void ResourceScheduler::scheduleNext()
{
  if ( mCurrentTask.type != Invalid || isEmpty() || !mOnline )
    return;
  QTimer::singleShot( 0, this, SLOT(executeNext()) );
}

void ResourceScheduler::executeNext()
{
  if( mCurrentTask.type != Invalid || isEmpty() )
    return;

  for ( int i = 0; i < NQueueCount; ++i ) {
    if ( !mTaskList[ i ].isEmpty() ) {
      mCurrentTask = mTaskList[ i ].takeFirst();
      break;
    }
  }

  if ( s_resourcetracker ) {
    QList<QVariant> argumentList;
    argumentList << QString::number( mCurrentTask.serial );
    s_resourcetracker->asyncCallWithArgumentList(QLatin1String("jobStarted"), argumentList);
  }

  switch ( mCurrentTask.type ) {
    case SyncAll:
      emit executeFullSync();
      break;
    case SyncCollectionTree:
      emit executeCollectionTreeSync();
      break;
    case SyncCollection:
      emit executeCollectionSync( mCurrentTask.collection );
      break;
    case FetchItem:
      emit executeItemFetch( mCurrentTask.item, mCurrentTask.itemParts );
      break;
    case DeleteResourceCollection:
      emit executeResourceCollectionDeletion();
      break;
    case ChangeReplay:
      emit executeChangeReplay();
      break;
    case SyncAllDone:
      emit fullSyncComplete();
      break;
    case Custom:
    {
      bool success = QMetaObject::invokeMethod( mCurrentTask.receiver, mCurrentTask.methodName, Q_ARG(QVariant, mCurrentTask.argument) );;
      if ( !success )
        success = QMetaObject::invokeMethod( mCurrentTask.receiver, mCurrentTask.methodName );

      if ( !success )
        kError() << "Could not invoke slot" << mCurrentTask.methodName << "on" << mCurrentTask.receiver << "with argument" << mCurrentTask.argument;
      break;
    }
    default:
      Q_ASSERT( false );
  }
}

ResourceScheduler::Task ResourceScheduler::currentTask() const
{
  return mCurrentTask;
}

void ResourceScheduler::setOnline(bool state)
{
  if ( mOnline == state )
    return;
  mOnline = state;
  if ( mOnline ) {
    scheduleNext();
  } else if ( mCurrentTask.type != Invalid ) {
    // abort running task
    queueForTaskType( mCurrentTask.type ).prepend( mCurrentTask );
    mCurrentTask = Task();
  }
}

void ResourceScheduler::signalTaskToTracker( const Task &task, const QByteArray &taskType )
{
  // if there's a job tracer running, tell it about the new job
  if ( !s_resourcetracker && QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String("org.kde.akonadiconsole") ) ) {
    s_resourcetracker = new QDBusInterface( QLatin1String("org.kde.akonadiconsole"),
                                       QLatin1String("/resourcesJobtracker"),
                                       QLatin1String("org.freedesktop.Akonadi.JobTracker"),
                                       QDBusConnection::sessionBus(), 0 );
  }

  if ( s_resourcetracker ) {
    QList<QVariant> argumentList;
    argumentList << static_cast<AgentBase*>(  parent() )->identifier()
                 << QString::number( task.serial )
                 << QString()
                 << QString::fromLatin1( taskType );
    s_resourcetracker->asyncCallWithArgumentList(QLatin1String("jobCreated"), argumentList);
  }
}

void ResourceScheduler::collectionRemoved( const Akonadi::Collection &collection )
{
  if ( !collection.isValid() ) // should not happen, but you never know...
    return;
  TaskList& queue = queueForTaskType( SyncCollection );
  for ( QList<Task>::iterator it = queue.begin(); it != queue.end(); ) {
    if ( (*it).type == SyncCollection && (*it).collection == collection ) {
      it = queue.erase( it );
      kDebug() << " erasing";
    } else
      ++it;
  }
}

void ResourceScheduler::Task::sendDBusReplies( bool success )
{
  Q_FOREACH( const QDBusMessage &msg, dbusMsgs ) {
    QDBusMessage reply( msg );
    reply << success;
    QDBusConnection::sessionBus().send( reply );
  }
}

ResourceScheduler::QueueType ResourceScheduler::queueTypeForTaskType( TaskType type )
{
  switch( type ) {
  case ChangeReplay:
    return ChangeReplayQueue;
  case FetchItem:
    return ItemFetchQueue;
  default:
    return GenericTaskQueue;
  }
}

ResourceScheduler::TaskList& ResourceScheduler::queueForTaskType( TaskType type )
{
  const QueueType qt = queueTypeForTaskType( type );
  return mTaskList[ qt ];
}

void ResourceScheduler::dump()
{
  kDebug() << "ResourceScheduler: Online:" << mOnline;
  kDebug() << " current task:" << mCurrentTask;
  for ( int i = 0; i < NQueueCount; ++i ) {
    const TaskList& queue = mTaskList[i];
    kDebug() << " queue" << i << queue.size() << "tasks:";
    for ( QList<Task>::const_iterator it = queue.begin(); it != queue.end(); ++it ) {
      kDebug() << "  " << (*it);
    }
  }
}

static const char s_taskTypes[][25] = {
      "Invalid",
      "SyncAll",
      "SyncCollectionTree",
      "SyncCollection",
      "FetchItem",
      "ChangeReplay",
      "DeleteResourceCollection",
      "SyncAllDone",
      "Custom"
};

QDebug Akonadi::operator<<( QDebug d, const ResourceScheduler::Task& task )
{
  d << task.serial << s_taskTypes[task.type] << "collection" << task.collection.id() << "item" << task.item.id();
  if ( !task.methodName.isEmpty() )
    d << task.methodName << task.argument;
  return d;
}

//@endcond

#include "resourcescheduler_p.moc"
