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
#include "storage/selectquerybuilder.h"
#include "akonadiserver_debug.h"

#include <private/tristate_p.h>

#include <QDateTime>
#include <QCoreApplication>

namespace Akonadi {
namespace Server {

/**
 * @warning: QTimer's methods are not virtual, so it's necessary to always call
 * methods on pointer to PauseableTimer!
 */
class PauseableTimer : public QTimer
{
    Q_OBJECT

public:
    PauseableTimer(QObject *parent = 0)
        : QTimer(parent)
    {
    }

    void start(int interval)
    {
        mStarted = QDateTime::currentDateTime();
        mPaused = QDateTime();
        setInterval(interval);
        QTimer::start(interval);
    }

    void start()
    {
        start(interval());
    }

    void stop()
    {
        mStarted = QDateTime();
        mPaused = QDateTime();
        QTimer::stop();
    }

    Q_INVOKABLE void pause()
    {
        if (!isActive()) {
            akError() << "Cannot pause an inactive timer";
            return;
        }
        if (isPaused()) {
            akError() << "Cannot pause an already paused timer";
            return;
        }

        mPaused = QDateTime::currentDateTime();
        QTimer::stop();
    }

    Q_INVOKABLE void resume()
    {
        if (!isPaused()) {
            akError() << "Cannot resume a timer that is not paused.";
            return;
        }

        const int remainder = interval() - (mStarted.secsTo(mPaused) * 1000);
        start(qMax(0, remainder));
        mPaused = QDateTime();
        // Update mStarted so that pause() can be called repeatedly
        mStarted = QDateTime::currentDateTime();
    }

    bool isPaused() const
    {
        return mPaused.isValid();
    }

private:
    QDateTime mStarted;
    QDateTime mPaused;
};

} // namespace Server
} // namespace Akonadi

using namespace Akonadi::Server;

CollectionScheduler::CollectionScheduler(QObject *parent)
    : QObject(parent)
    , mMinInterval(5)
{
    // make sure we are created from the main thread, ie. before all other threads start to potentially use us
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    QMetaObject::invokeMethod(this, "initScheduler", Qt::QueuedConnection);
}

CollectionScheduler::~CollectionScheduler()
{
    delete mScheduler;
}

void CollectionScheduler::inhibit(bool inhibit)
{
    if (inhibit && mScheduler->isActive() && !mScheduler->isPaused()) {
        const bool success = QMetaObject::invokeMethod(mScheduler, "pause", Qt::QueuedConnection);
        Q_ASSERT(success);
        Q_UNUSED(success);
    } else if (!inhibit && mScheduler->isPaused()) {
        const bool success = QMetaObject::invokeMethod(mScheduler, "resume", Qt::QueuedConnection);
        Q_ASSERT(success);
        Q_UNUSED(success);
    }
}

int CollectionScheduler::minimumInterval() const
{
    return mMinInterval;
}

void CollectionScheduler::setMinimumInterval(int intervalMinutes)
{
    mMinInterval = intervalMinutes;
}

void CollectionScheduler::collectionAdded(qint64 collectionId)
{
    Collection collection = Collection::retrieveById(collectionId);
    DataStore::self()->activeCachePolicy(collection);
    if (shouldScheduleCollection(collection)) {
        QMetaObject::invokeMethod(this, "scheduleCollection",
                                  Qt::QueuedConnection,
                                  Q_ARG(Collection, collection));
    }
}

void CollectionScheduler::collectionChanged(qint64 collectionId)
{
    QMutexLocker locker(&mScheduleLock);
    Q_FOREACH (const Collection &collection, mSchedule) {
        if (collection.id() == collectionId) {
            Collection changed = Collection::retrieveById(collectionId);
            DataStore::self()->activeCachePolicy(changed);
            if (hasChanged(collection, changed)) {
                if (shouldScheduleCollection(changed)) {
                    locker.unlock();
                    // Scheduling the changed collection will automatically remove the old one
                    scheduleCollection(changed);
                } else {
                    locker.unlock();
                    // If the collection should no longer be scheduled then remove it
                    collectionRemoved(collectionId);
                }
            }

            return;
        }
    }

    // We don't know the collection yet, but maybe now it can be scheduled
    collectionAdded(collectionId);
}

void CollectionScheduler::collectionRemoved(qint64 collectionId)
{
    QMutexLocker locker(&mScheduleLock);
    Q_FOREACH (const Collection &collection, mSchedule) {
        if (collection.id() == collectionId) {
            const uint key = mSchedule.key(collection);
            const bool reschedule = (key == mSchedule.constBegin().key());
            mSchedule.remove(key);
            locker.unlock();

            // If we just remove currently scheduled collection, schedule the next one
            if (reschedule) {
                startScheduler();
            }

            return;
        }
    }
}

void CollectionScheduler::startScheduler()
{
    // Don't restart timer if we are paused.
    if (mScheduler->isPaused()) {
        return;
    }

    QMutexLocker locker(&mScheduleLock);
    if (mSchedule.isEmpty()) {
        // Stop the timer. It will be started again once some collection is scheduled
        mScheduler->stop();
        return;
    }

    // Get next collection to expire and start the timer
    const uint next = mSchedule.constBegin().key();
    // cast next - now() to int, so that we get negative result when next is in the past
    mScheduler->start(qMax(0, (int)(next - QDateTime::currentDateTime().toTime_t()) * 1000));
}

void CollectionScheduler::scheduleCollection(Collection collection, bool shouldStartScheduler)
{
    QMutexLocker locker(&mScheduleLock);
    auto i = std::find(mSchedule.cbegin(), mSchedule.cend(), collection);
    if (i != mSchedule.cend()) {
        mSchedule.remove(i.key(), i.value());
    }

    DataStore::self()->activeCachePolicy(collection);

    if (!shouldScheduleCollection(collection)) {
        return;
    }

    const int expireMinutes = qMax(mMinInterval, collectionScheduleInterval(collection));
    uint nextCheck = QDateTime::currentDateTime().toTime_t() + (expireMinutes * 60);

    // Check whether there's another check scheduled within a minute after this one.
    // If yes, then delay this check so that it's scheduled together with the others
    // This is a minor optimization to reduce wakeups and SQL queries
    QMap<uint, Collection>::iterator it = mSchedule.lowerBound(nextCheck);
    if (it != mSchedule.end() && it.key() - nextCheck < 60) {
        nextCheck = it.key();

        // Also check whether there's another checked scheduled within a minute before
        // this one.
    } else if (it != mSchedule.begin()) {
        --it;
        if (nextCheck - it.key() < 60) {
            nextCheck = it.key();
        }
    }

    mSchedule.insert(nextCheck, collection);
    if (shouldStartScheduler && !mScheduler->isActive()) {
        locker.unlock();
        startScheduler();
    }
}

void CollectionScheduler::initScheduler()
{
    mScheduler = new PauseableTimer();
    mScheduler->setSingleShot(true);
    connect(mScheduler, SIGNAL(timeout()),
            this, SLOT(schedulerTimeout()));

    // Only retrieve enabled collections and referenced collections, we don't care
    // about anything else
    SelectQueryBuilder<Collection> qb;
    Query::Condition orCondition(Query::Or);
    orCondition.addValueCondition(Collection::syncPrefFullColumnName(), Query::Equals, (int)Akonadi::Tristate::True);
    Query::Condition andCondition(Query::And);
    andCondition.addValueCondition(Collection::syncPrefFullColumnName(), Query::Equals, (int)Akonadi::Tristate::Undefined);
    andCondition.addValueCondition(Collection::enabledFullColumnName(), Query::Equals, true);
    orCondition.addCondition(andCondition);
    orCondition.addValueCondition(Collection::referencedFullColumnName(), Query::Equals, true);
    qb.addCondition(orCondition);
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to query initial collections for scheduler!";
        qCWarning(AKONADISERVER_LOG) << "Not a fatal error, no collections will be scheduled for sync or cache expiration!";
    }

    const Collection::List collections = qb.result();
    Q_FOREACH (const Collection &collection, collections) {
        scheduleCollection(collection);
    }

    startScheduler();
}

void CollectionScheduler::schedulerTimeout()
{
    // Call stop() explicitly to reset the timer
    mScheduler->stop();

    mScheduleLock.lock();
    const uint timestamp = mSchedule.constBegin().key();
    const QList<Collection> collections = mSchedule.values(timestamp);
    mSchedule.remove(timestamp);
    mScheduleLock.unlock();

    Q_FOREACH (const Collection &collection, collections) {
        collectionExpired(collection);
        scheduleCollection(collection, false);
    }

    startScheduler();
}

#include "collectionscheduler.moc"
