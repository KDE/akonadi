/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "intervalcheck.h"
#include "storage/datastore.h"
#include "storage/itemretrievalmanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

using namespace Akonadi;

static int MINIMUM_AUTOSYNC_INTERVAL = 5; // minutes
static int MINIMUM_COLTREESYNC_INTERVAL = 5; // minutes

IntervalCheck* IntervalCheck::s_instance = 0;

IntervalCheck::IntervalCheck(QObject * parent) :
    QThread( parent )
{
  // make sure we are created from the main thread, ie. before all other threads start to potentially use us
  Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );

  Q_ASSERT( s_instance == 0 );
  s_instance = this;
}

IntervalCheck::~ IntervalCheck()
{
}

void IntervalCheck::run()
{
  DataStore::self();
  QTimer::singleShot( 60 * 1000, this, SLOT(doIntervalCheck()) );
  exec();
  DataStore::self()->close();
}

IntervalCheck* IntervalCheck::instance()
{
  Q_ASSERT( s_instance );
  return s_instance;
}

void IntervalCheck::doIntervalCheck()
{
  // cycle over all collections
  const QDateTime now = QDateTime::currentDateTime();
  const QVector<Collection> collections = Collection::retrieveAll();
  Q_FOREACH ( /*sic!*/ Collection collection, collections ) {
    // determine active cache policy
    DataStore::self()->activeCachePolicy( collection );

    // check if there is something to sync at all
    if ( collection.cachePolicyCheckInterval() <= 0 || !collection.subscribed() )
      continue;
    requestCollectionSync( collection );
  }

  QTimer::singleShot( 60 * 1000, this, SLOT(doIntervalCheck()) );
}

void IntervalCheck::requestCollectionSync(const Akonadi::Collection& collection)
{
  const QDateTime now = QDateTime::currentDateTime();

  // if the collection is a resource collection we trigger a synchronization
  // of the collection hierarchy as well
  if ( collection.parentId() == 0 ) {
    const QString resourceName = collection.resource().name();
    QMutexLocker locker( &m_lastSyncMutex );
    const QDateTime lastExpectedCheck = now.addSecs( MINIMUM_COLTREESYNC_INTERVAL * - 60 );
    if ( !mLastCollectionTreeSyncs.contains( resourceName ) || mLastCollectionTreeSyncs.value( resourceName ) < lastExpectedCheck ) {
      mLastCollectionTreeSyncs.insert( resourceName, now );
      locker.unlock();
      QMetaObject::invokeMethod( ItemRetrievalManager::instance(), "triggerCollectionTreeSync", Qt::QueuedConnection, Q_ARG( QString, resourceName ) );
    }
  }

  // now on to the actual collection syncing
  int minInterval = MINIMUM_AUTOSYNC_INTERVAL;
  if ( collection.cachePolicyCheckInterval() > 0 )
    minInterval = collection.cachePolicyCheckInterval();

  const QDateTime lastExpectedCheck = now.addSecs( minInterval * -60 );
  QMutexLocker locker( &m_lastSyncMutex );
  if ( mLastChecks.contains( collection.id() ) && mLastChecks.value( collection.id() ) > lastExpectedCheck )
    return;
  mLastChecks.insert( collection.id(), now );
  locker.unlock();
  QMetaObject::invokeMethod( ItemRetrievalManager::instance(), "triggerCollectionSync", Qt::QueuedConnection,
                             Q_ARG( QString, collection.resource().name() ), Q_ARG( qint64, collection.id() ) );
}

#include "intervalcheck.moc"
