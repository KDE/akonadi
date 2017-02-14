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
    virtual ~CollectionScheduler();

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
    int minimumInterval() const;

protected:
    virtual void init() Q_DECL_OVERRIDE;
    virtual void quit() Q_DECL_OVERRIDE;

    virtual bool shouldScheduleCollection(const Collection &collection) = 0;
    virtual bool hasChanged(const Collection &collection, const Collection &changed) = 0;
    /**
     * @return Return cache timeout in minutes
     */
    virtual int collectionScheduleInterval(const Collection &collection) = 0;
    virtual void collectionExpired(const Collection &collection) = 0;

    void inhibit(bool inhibit = true);

protected Q_SLOTS:
    void schedulerTimeout();
    void startScheduler();
    void scheduleCollection(/*sic!*/ Collection collection, bool shouldStartScheduler = true);

protected:
    QMutex mScheduleLock;
    QMultiMap<uint /*timestamp*/, Collection> mSchedule;
    PauseableTimer *mScheduler;
    int mMinInterval;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_COLLECTIONSCHEDULER_H
