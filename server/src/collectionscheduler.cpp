/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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


#include "collectionscheduler.h"
#include "storage/datastore.h"

#include <QDateTime>
#include <QCoreApplication>

using namespace Akonadi::Server;

CollectionScheduler::CollectionScheduler( QObject *parent )
  : QThread( parent )
  , mMinInterval( 5 )
{
  // make sure we are created from the main thread, ie. before all other threads start to potentially use us
  Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );

  mScheduler = new QTimer( this );
  mScheduler->setSingleShot( true );
  connect( mScheduler, SIGNAL(timeout()),
           this, SLOT(schedulerTimeout()) );
}

CollectionScheduler::~CollectionScheduler()
{
}

void CollectionScheduler::run()
{
  DataStore::self();

  QTimer::singleShot( 0, this, SLOT(initScheduler()) );
  exec();

  DataStore::self()->close();
}

int CollectionScheduler::minimumInterval() const
{
  return mMinInterval;
}

void CollectionScheduler::setMinimumInterval( int intervalMinutes )
{
  mMinInterval = intervalMinutes;
}

void CollectionScheduler::collectionAdded( qint64 collectionId )
{
  Collection collection = Collection::retrieveById( collectionId );
  DataStore::self()->activeCachePolicy( collection );
  if ( shouldScheduleCollection( collection ) ) {
    QMetaObject::invokeMethod( this, "scheduleCollection",
                               Qt::QueuedConnection,
                               Q_ARG( Collection, collection ) );
  }
}

void CollectionScheduler::collectionChanged( qint64 collectionId )
{
  QMutexLocker locker( &mScheduleLock );
  Q_FOREACH ( const Collection &collection, mSchedule ) {
    if ( collection.id() == collectionId ) {
      Collection changed = Collection::retrieveById( collectionId );
      DataStore::self()->activeCachePolicy( changed );
      if ( hasChanged( collection, changed ) ) {
        if ( shouldScheduleCollection( changed ) ) {
          locker.unlock();
          // Scheduling the changed collection will automatically remove the old one
          scheduleCollection( changed );
        } else {
          locker.unlock();
          // If the collection should no longer be scheduled then remove it
          collectionRemoved( collectionId );
        }
      }

      return;
    }
  }
}

void CollectionScheduler::collectionRemoved( qint64 collectionId )
{
  QMutexLocker locker( &mScheduleLock );
  Q_FOREACH ( const Collection &collection, mSchedule ) {
    if ( collection.id() == collectionId ) {
      const uint key = mSchedule.key( collection );
      const bool reschedule = ( key == mSchedule.constBegin().key() );
      mSchedule.remove( key );
      locker.unlock();

      // If we just remove currently scheduled collection, schedule the next one
      if ( reschedule ) {
        startScheduler();
      }

      return;
    }
  }
}

void CollectionScheduler::startScheduler()
{
  QMutexLocker locker( &mScheduleLock );
  if ( mSchedule.isEmpty() ) {
    // Stop the timer. It will be started again once some collection is scheduled
    mScheduler->stop();
    return;
  }

  // Get next collection to expire and start the timer
  const uint next = mSchedule.constBegin().key();
  mScheduler->start( ( next - QDateTime::currentDateTime().toTime_t() ) * 1000 );
}

void CollectionScheduler::scheduleCollection( Collection collection )
{
  QMutexLocker locker( &mScheduleLock );
  if ( mSchedule.values().contains( collection ) ) {
    const uint key = mSchedule.key( collection );
    mSchedule.remove( key, collection );
  }

  DataStore::self()->activeCachePolicy( collection );

  if ( !shouldScheduleCollection( collection ) ) {
    return;
  }

  const int expireMinutes = qMax( mMinInterval, collectionScheduleInterval( collection ) );
  uint nextCheck = QDateTime::currentDateTime().toTime_t() + ( expireMinutes * 60 );

  // Check whether there's another check scheduled within a minute after this one.
  // If yes, then delay this check so that it's scheduled together with the others
  // This is a minor optimization to reduce wakeups and SQL queries
  QMap<uint, Collection>::iterator it = mSchedule.lowerBound( nextCheck );
  if ( it != mSchedule.end() && it.key() - nextCheck < 60 ) {
    nextCheck = it.key();

  // Also check whether there's another checked scheduled within a minute before
  // this one.
  } else if ( it != mSchedule.begin() ) {
    --it;
    if ( nextCheck - it.key() < 60 ) {
      nextCheck = it.key();
    }
  }

  mSchedule.insert( nextCheck, collection );
  if ( !mScheduler->isActive() ) {
    mScheduleLock.unlock();
    startScheduler();
  }
}

void CollectionScheduler::initScheduler()
{
  const QVector<Collection> collections = Collection::retrieveAll();
  Q_FOREACH ( /*sic!*/ Collection collection, collections ) {
    scheduleCollection( collection );
  }

  startScheduler();
}

void CollectionScheduler::schedulerTimeout()
{
  mScheduleLock.lock();
  const uint timestamp = mSchedule.constBegin().key();
  const QList<Collection> collections = mSchedule.values( timestamp );
  mSchedule.remove( timestamp );
  mScheduleLock.unlock();

  const QDateTime now = QDateTime::currentDateTime();

  Q_FOREACH ( const Collection &collection, collections ) {
    collectionExpired( collection );
    scheduleCollection( collection );
  }

  startScheduler();
}
