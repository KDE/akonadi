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
  mTaskList << t;
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

//@endcond

#include "resourcescheduler_p.moc"
