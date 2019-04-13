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
#include <QTimer>

using namespace std::literals::chrono_literals;

namespace Akonadi
{
namespace Server
{

/**
 * @warning: QTimer's methods are not virtual, so it's necessary to always call
 * methods on pointer to PauseableTimer!
 */
class PauseableTimer : public QTimer
{
    Q_OBJECT

public:
    PauseableTimer(QObject *parent = nullptr)
        : QTimer(parent)
    {
    }

    void start(int interval)
    {
        mStarted = QDateTime::currentDateTimeUtc();
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
        if (!isActive() || isPaused()) {
            return;
        }

        mPaused = QDateTime::currentDateTimeUtc();
        QTimer::stop();
    }

    Q_INVOKABLE void resume()
    {
        if (!isPaused()) {
            return;
        }

        const int remainder = interval() - (mStarted.secsTo(mPaused) * 1000);
        start(qMax(0, remainder));
        mPaused = QDateTime();
        // Update mStarted so that pause() can be called repeatedly
        mStarted = QDateTime::currentDateTimeUtc();
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

CollectionScheduler::CollectionScheduler(const QString &threadName, QThread::Priority priority, QObject *parent)
    : AkThread(threadName, priority, parent)
{
}

CollectionScheduler::~CollectionScheduler()
{
}

// Called in secondary thread
void CollectionScheduler::quit()
{
    delete mScheduler;
    mScheduler = nullptr;

    AkThread::quit();
}

void CollectionScheduler::inhibit(bool inhibit)
{
    if (inhibit) {
        const bool success = QMetaObject::invokeMethod(mScheduler, &PauseableTimer::pause, Qt::QueuedConnection);
        Q_ASSERT(success); Q_UNUSED(success);
    } else {
        const bool success = QMetaObject::invokeMethod(mScheduler, &PauseableTimer::resume, Qt::QueuedConnection);
        Q_ASSERT(success); Q_UNUSED(success);
    }
}

int CollectionScheduler::minimumInterval() const
{
    return mMinInterval;
}

CollectionScheduler::TimePoint CollectionScheduler::nextScheduledTime(qint64 collectionId) const
{
    QMutexLocker locker(&mScheduleLock);
    const auto i = constFind(collectionId);
    if (i != mSchedule.cend()) {
        return i.key();
    }
    return {};
}

std::chrono::milliseconds CollectionScheduler::currentTimerInterval() const
{
    return std::chrono::milliseconds(mScheduler->isActive() ? mScheduler->interval() : 0);
}

void CollectionScheduler::setMinimumInterval(int intervalMinutes)
{
    // No mutex -- you can only call this before starting the thread
    mMinInterval = intervalMinutes;
}

void CollectionScheduler::collectionAdded(qint64 collectionId)
{
    Collection collection = Collection::retrieveById(collectionId);
    DataStore::self()->activeCachePolicy(collection);
    if (shouldScheduleCollection(collection)) {
        QMetaObject::invokeMethod(this, [this, collection]() {scheduleCollection(collection);}, Qt::QueuedConnection);
    }
}

void CollectionScheduler::collectionChanged(qint64 collectionId)
{
    QMutexLocker locker(&mScheduleLock);
    const auto it = constFind(collectionId);
    if (it != mSchedule.cend()) {
        const Collection oldCollection = it.value();
        Collection changed = Collection::retrieveById(collectionId);
        DataStore::self()->activeCachePolicy(changed);
        if (hasChanged(oldCollection, changed)) {
            if (shouldScheduleCollection(changed)) {
                locker.unlock();
                // Scheduling the changed collection will automatically remove the old one
                QMetaObject::invokeMethod(this, [this, changed]() {scheduleCollection(changed);}, Qt::QueuedConnection);
            } else {
                locker.unlock();
                // If the collection should no longer be scheduled then remove it
                collectionRemoved(collectionId);
            }
        }
    } else {
        // We don't know the collection yet, but maybe now it can be scheduled
        collectionAdded(collectionId);
    }
}

void CollectionScheduler::collectionRemoved(qint64 collectionId)
{
    QMutexLocker locker(&mScheduleLock);
    auto it = find(collectionId);
    if (it != mSchedule.end()) {
        const bool reschedule = it == mSchedule.begin();
        mSchedule.erase(it);

        // If we just remove currently scheduled collection, schedule the next one
        if (reschedule) {
            QMetaObject::invokeMethod(this, &CollectionScheduler::startScheduler, Qt::QueuedConnection);
        }
    }
}

// Called in secondary thread
void CollectionScheduler::startScheduler()
{
    QMutexLocker locker(&mScheduleLock);
    // Don't restart timer if we are paused.
    if (mScheduler->isPaused()) {
        return;
    }

    if (mSchedule.isEmpty()) {
        // Stop the timer. It will be started again once some collection is scheduled
        mScheduler->stop();
        return;
    }

    // Get next collection to expire and start the timer
    const auto next = mSchedule.constBegin().key();
    // TimePoint uses a signed representation internally (int64_t), so we get negative result when next is in the past
    const auto delayUntilNext = next - std::chrono::steady_clock::now();
    mScheduler->start(qMax<qint64>(0, std::chrono::duration_cast<std::chrono::milliseconds>(delayUntilNext).count()));
}

// Called in secondary thread
void CollectionScheduler::scheduleCollection(Collection collection, bool shouldStartScheduler)
{
    QMutexLocker locker(&mScheduleLock);
    auto i = find(collection.id());
    if (i != mSchedule.end()) {
        mSchedule.erase(i);
    }

    DataStore::self()->activeCachePolicy(collection);

    if (!shouldScheduleCollection(collection)) {
        return;
    }

    const int expireMinutes = qMax(mMinInterval, collectionScheduleInterval(collection));
    TimePoint nextCheck(std::chrono::steady_clock::now() + std::chrono::minutes(expireMinutes));

    // Check whether there's another check scheduled within a minute after this one.
    // If yes, then delay this check so that it's scheduled together with the others
    // This is a minor optimization to reduce wakeups and SQL queries
    auto it = constLowerBound(nextCheck);
    if (it != mSchedule.cend() && it.key() - nextCheck < 1min) {
        nextCheck = it.key();

        // Also check whether there's another checked scheduled within a minute before
        // this one.
    } else if (it != mSchedule.cbegin()) {
        --it;
        if (nextCheck - it.key() < 1min) {
            nextCheck = it.key();
        }
    }

    mSchedule.insert(nextCheck, collection);
    if (shouldStartScheduler && !mScheduler->isActive()) {
        locker.unlock();
        startScheduler();
    }
}

CollectionScheduler::ScheduleMap::const_iterator CollectionScheduler::constFind(qint64 collectionId) const
{
    return std::find_if(mSchedule.cbegin(), mSchedule.cend(), [collectionId](const Collection &c) { return c.id() == collectionId; });
}

CollectionScheduler::ScheduleMap::iterator CollectionScheduler::find(qint64 collectionId)
{
    return std::find_if(mSchedule.begin(), mSchedule.end(), [collectionId](const Collection &c) { return c.id() == collectionId; });
}

// separate method so we call the const version of QMap::lowerBound
CollectionScheduler::ScheduleMap::const_iterator CollectionScheduler::constLowerBound(TimePoint timestamp) const
{
    return mSchedule.lowerBound(timestamp);
}

// Called in secondary thread
void CollectionScheduler::init()
{
    AkThread::init();

    mScheduler = new PauseableTimer();
    mScheduler->setSingleShot(true);
    connect(mScheduler, &QTimer::timeout,
            this, &CollectionScheduler::schedulerTimeout);

    // Only retrieve enabled collections and referenced collections, we don't care
    // about anything else
    SelectQueryBuilder<Collection> qb;
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to query initial collections for scheduler!";
        qCWarning(AKONADISERVER_LOG) << "Not a fatal error, no collections will be scheduled for sync or cache expiration!";
    }

    const Collection::List collections = qb.result();
    for (const Collection &collection : collections) {
        scheduleCollection(collection);
    }

    startScheduler();
}

// Called in secondary thread
void CollectionScheduler::schedulerTimeout()
{
    QMutexLocker locker(&mScheduleLock);

    // Call stop() explicitly to reset the timer
    mScheduler->stop();

    const auto timestamp = mSchedule.constBegin().key();
    const QList<Collection> collections = mSchedule.values(timestamp);
    mSchedule.remove(timestamp);
    locker.unlock();

    for (const Collection &collection : collections) {
        collectionExpired(collection);
        scheduleCollection(collection, false);
    }

    startScheduler();
}

#include "collectionscheduler.moc"
