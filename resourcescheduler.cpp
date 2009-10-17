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
  if ( !mTaskList.isEmpty() && ( mTaskList.last() == t || mCurrentTask == t ) )
    return;
  mTaskList << t;
  signalTaskToTracker( t, "SyncAll" );
  scheduleNext();
}

void ResourceScheduler::scheduleCollectionTreeSync()
{
  Task t;
  t.type = SyncCollectionTree;
  if ( !mTaskList.isEmpty() && ( mTaskList.last() == t || mCurrentTask == t ) )
    return;
  mTaskList << t;
  signalTaskToTracker( t, "SyncCollectionTree" );
  scheduleNext();
}

void ResourceScheduler::scheduleSync(const Collection & col)
{
  Task t;
  t.type = SyncCollection;
  t.collection = col;
  if ( !mTaskList.isEmpty() && ( mTaskList.last() == t || mCurrentTask == t ) )
    return;
  mTaskList << t;
  signalTaskToTracker( t, "SyncCollection" );
  scheduleNext();
}

void ResourceScheduler::scheduleItemFetch(const Item & item, const QSet<QByteArray> &parts, const QDBusMessage & msg)
{
  Task t;
  t.type = FetchItem;
  t.item = item;
  t.itemParts = parts;
  t.dbusMsg = msg;
  if ( !mTaskList.isEmpty() && ( mTaskList.last() == t || mCurrentTask == t ) )
    return;
  mTaskList << t;
  signalTaskToTracker( t, "FetchItem" );
  scheduleNext();
}

void ResourceScheduler::scheduleResourceCollectionDeletion()
{
  Task t;
  t.type = DeleteResourceCollection;
  if ( !mTaskList.isEmpty() && ( mTaskList.last() == t || mCurrentTask == t ) )
    return;
  mTaskList << t;
  signalTaskToTracker( t, "DeleteResourceCollection" );
  scheduleNext();
}

void ResourceScheduler::scheduleChangeReplay()
{
  Task t;
  t.type = ChangeReplay;
  if ( mTaskList.contains( t ) )
    return;
  // change replays have to happen before we pull changes from the backend, otherwise
  // we will overwrite our still unsaved local changes if the backend can't do
  // incremental retrieval
  mTaskList.prepend( t );
  signalTaskToTracker( t, "ChangeReplay" );
  scheduleNext();
}

void Akonadi::ResourceScheduler::scheduleFullSyncCompletion()
{
  Task t;
  t.type = SyncAllDone;
  mTaskList << t;
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
  if ( mTaskList.contains( t ) )
    return;

  switch (priority) {
  case ResourceBase::Prepend:
    mTaskList.prepend(t);
    break;
  case ResourceBase::AfterChangeReplay:
    {
      QMutableListIterator<Task> it(mTaskList);
      bool inserted = false;
      while (it.hasNext() && !inserted) {
        if (it.next().type != ChangeReplay) {
          it.previous(); // next returns the item *and* advances the iterator.
          it.insert(t);
          inserted = true;
        }
      }
      if (!inserted)
        mTaskList.append(t);
    }
    break;
  default:
    mTaskList.append(t);
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
  mTaskList << t;
  signalTaskToTracker( t, "DeferedTask" );

  scheduleNext();
}

bool ResourceScheduler::isEmpty()
{
  return mTaskList.isEmpty();
}

void ResourceScheduler::scheduleNext()
{
  if ( mCurrentTask.type != Invalid || mTaskList.isEmpty() || !mOnline )
    return;
  QTimer::singleShot( 0, this, SLOT(executeNext()) );
}

void ResourceScheduler::executeNext()
{
  if( mCurrentTask.type != Invalid || mTaskList.isEmpty() )
    return;

  mCurrentTask = mTaskList.takeFirst();

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
      const bool success = QMetaObject::invokeMethod( mCurrentTask.receiver, mCurrentTask.methodName, Q_ARG(QVariant, mCurrentTask.argument) );
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
    mTaskList.prepend( mCurrentTask );
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
  for ( QList<Task>::iterator it = mTaskList.begin(); it != mTaskList.end(); ) {
    if ( (*it).type == SyncCollection && (*it).collection == collection ) {
      it = mTaskList.erase( it );
      kDebug() << " erasing";
    } else
      ++it;
  }
}

//@endcond

#include "resourcescheduler_p.moc"
