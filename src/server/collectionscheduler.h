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

#ifndef AKONADI_SERVER_COLLECTIONSCHEDULER_H
#define AKONADI_SERVER_COLLECTIONSCHEDULER_H

#include <QThread>
#include <QMultiMap>
#include <QMutex>

#include "entities.h"
#include "akthread.h"

namespace Akonadi
{
namespace Server
{

class Collection;
class PauseableTimer;

class CollectionScheduler : public AkThread
{
    Q_OBJECT

public:
    explicit CollectionScheduler(const QString &threadName, QThread::Priority priority, QObject *parent = nullptr);
    ~CollectionScheduler() override;

    void collectionChanged(qint64 collectionId);
    void collectionRemoved(qint64 collectionId);
    void collectionAdded(qint64 collectionId);

    /**
     * Sets the minimum timeout interval.
     *
     * Default value is 5.
     *
     * @p intervalMinutes Minimum timeout interval in minutes.
     */
    void setMinimumInterval(int intervalMinutes);
    Q_REQUIRED_RESULT int minimumInterval() const;

    using TimePoint = std::chrono::steady_clock::time_point;

    /**
     * @return the timestamp (in seconds since epoch) when collectionExpired
     * will next be called on the given collection, or 0 if we don't know about the collection.
     * Only used by the unittest.
     */
    TimePoint nextScheduledTime(qint64 collectionId) const;

    /**
     * @return the next timeout
     */
    std::chrono::milliseconds currentTimerInterval() const;

protected:
    void init() override;
    void quit() override;

    virtual bool shouldScheduleCollection(const Collection &collection) = 0;
    virtual bool hasChanged(const Collection &collection, const Collection &changed) = 0;
    /**
     * @return Return cache timeout in minutes
     */
    virtual int collectionScheduleInterval(const Collection &collection) = 0;
    /**
     * Called when it's time to do something on that collection.
     * Notice: this method is called in the secondary thread
     */
    virtual void collectionExpired(const Collection &collection) = 0;

    void inhibit(bool inhibit = true);

private Q_SLOTS:
    void schedulerTimeout();
    void startScheduler();
    void scheduleCollection(/*sic!*/ Akonadi::Server::Collection collection, bool shouldStartScheduler = true);

private:
    using ScheduleMap = QMultiMap<TimePoint /*timestamp*/, Collection>;
    ScheduleMap::const_iterator constFind(qint64 collectionId) const;
    ScheduleMap::iterator find(qint64 collectionId);
    ScheduleMap::const_iterator constLowerBound(TimePoint timestamp) const;

    mutable QMutex mScheduleLock;
    ScheduleMap mSchedule;
    PauseableTimer *mScheduler = nullptr;
    int mMinInterval = 5;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_COLLECTIONSCHEDULER_H
