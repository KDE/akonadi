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

#include "resourcescheduler.h"

#include <kdebug.h>

#include <QTimer>

using namespace Akonadi;

ResourceScheduler::ResourceScheduler( QObject *parent ) :
    QObject( parent ),
    mOnline( false )
{
}

void ResourceScheduler::scheduleFullSync()
{
  Task t;
  t.type = SyncAll;
  mTaskList << t;
  scheduleNext();
}

void ResourceScheduler::scheduleSync(const Collection & col)
{
  Task t;
  t.type = SyncCollection;
  t.collection = col;
  mTaskList << t;
  scheduleNext();
}

void ResourceScheduler::scheduleItemFetch(const DataReference & ref, const QStringList &parts, const QDBusMessage & msg)
{
  Task t;
  t.type = FetchItem;
  t.itemRef = ref;
  t.itemParts = parts;
  t.dbusMsg = msg;
  mTaskList << t;
  scheduleNext();
}

void ResourceScheduler::scheduleChangeReplay()
{
  Task t;
  t.type = ChangeReplay;
  if ( mTaskList.contains( t ) )
    return;
  mTaskList << t;
  scheduleNext();
}

void ResourceScheduler::taskDone()
{
  mCurrentTask = Task();
  scheduleNext();
}

void ResourceScheduler::scheduleNext()
{
  if ( mCurrentTask.type != Invalid || mTaskList.isEmpty() || !mOnline )
    return;
  QTimer::singleShot( 0, this, SLOT(executeNext()) );
}

void ResourceScheduler::executeNext()
{
  mCurrentTask = mTaskList.takeFirst();
  switch ( mCurrentTask.type ) {
    case SyncAll:
      emit executeFullSync();
      break;
    case SyncCollection:
      emit executeCollectionSync( mCurrentTask.collection );
      break;
    case FetchItem:
      emit executeItemFetch( mCurrentTask.itemRef, mCurrentTask.itemParts, mCurrentTask.dbusMsg );
      break;
    case ChangeReplay:
      emit executeChangeReplay();
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

#include "resourcescheduler.moc"
