/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "fakeakonadiserver.h"
#include "fakeintervalcheck.h"
#include <shared/aktest.h>

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;
using TimePoint = CollectionScheduler::TimePoint;

using namespace std::literals::chrono_literals;

class CollectionSchedulerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;
private Q_SLOTS:
    void initTestCase()
    {
        mAkonadi.init();
    }

    void shouldInitializeSyncIntervals()
    {
        // WHEN
        auto sched = AkThread::create<FakeIntervalCheck>(mAkonadi.itemRetrievalManager());
        sched->waitForInit();
        const TimePoint now(std::chrono::steady_clock::now());
        // THEN
        // Collections root (1), ColA (2), ColB (3), ColD (5), virtual (6) and virtual2 (7)
        // should have a check scheduled in 5 minutes (default value)
        for (qint64 collectionId : {1, 2, 3, 5, 6, 7}) {
            QVERIFY2(sched->nextScheduledTime(collectionId) > now + 4min, qPrintable(QString::number(collectionId)));
            QVERIFY(sched->nextScheduledTime(collectionId) < now + 6min);
        }
        QCOMPARE(sched->nextScheduledTime(4).time_since_epoch(), TimePoint::duration::zero()); // ColC is skipped because syncPref=false
        QCOMPARE(sched->nextScheduledTime(314).time_since_epoch(), TimePoint::duration::zero()); // no such collection
        QVERIFY(sched->currentTimerInterval() > 4min);
        QVERIFY(sched->currentTimerInterval() < 6min);
    }

    // (not that this feature is really used right now, it defaults to 5 and CacheCleaner sets it to 5)
    void shouldObeyMinimumInterval()
    {
        // GIVEN
        auto sched = AkThread::create<FakeIntervalCheck>(mAkonadi.itemRetrievalManager());
        // WHEN
        sched->setMinimumInterval(10);
        sched->waitForInit();
        // THEN
        const TimePoint now(std::chrono::steady_clock::now());
        QTRY_VERIFY(sched->nextScheduledTime(2).time_since_epoch() > TimePoint::duration::zero());
        QVERIFY(sched->nextScheduledTime(2) > now + 9min);
        QVERIFY(sched->nextScheduledTime(2) < now + 11min);
        QVERIFY(sched->currentTimerInterval() > 9min);
        QVERIFY(sched->currentTimerInterval() < 11min);
    }

    void shouldRemoveAndAddCollectionFromSchedule()
    {
        // GIVEN
        auto sched = AkThread::create<FakeIntervalCheck>(mAkonadi.itemRetrievalManager());
        sched->waitForInit();
        const auto timeForRoot = sched->nextScheduledTime(1);
        const auto timeForColB = sched->nextScheduledTime(3);
        QVERIFY(sched->nextScheduledTime(2) <= timeForColB);
        // WHEN
        sched->collectionRemoved(2);
        // THEN
        QTRY_COMPARE(sched->nextScheduledTime(2).time_since_epoch(), TimePoint::duration::zero());
        QCOMPARE(sched->nextScheduledTime(1), timeForRoot); // unchanged
        QCOMPARE(sched->nextScheduledTime(3), timeForColB); // unchanged
        QVERIFY(sched->currentTimerInterval() > 4min); // unchanged
        QVERIFY(sched->currentTimerInterval() < 6min); // unchanged

        // AND WHEN re-adding the collection
        QTest::qWait(1000); // we only have precision to the second...
        sched->collectionAdded(2);
        // THEN
        QTRY_VERIFY(sched->nextScheduledTime(2).time_since_epoch() > TimePoint::duration::zero());
        // This is unchanged, even though it would normally have been 1s later. See "minor optimization" in scheduler.
        QCOMPARE(sched->nextScheduledTime(2), timeForColB);
        QCOMPARE(sched->nextScheduledTime(1), timeForRoot); // unchanged
        QCOMPARE(sched->nextScheduledTime(3), timeForColB); // unchanged
        QVERIFY(sched->currentTimerInterval() > 4min); // unchanged
        QVERIFY(sched->currentTimerInterval() < 6min); // unchanged
    }

    void shouldHonourIntervalChange()
    {
        // GIVEN
        auto sched = AkThread::create<FakeIntervalCheck>(mAkonadi.itemRetrievalManager());
        sched->waitForInit();
        const auto timeForColB = sched->nextScheduledTime(3);
        Collection colA = Collection::retrieveByName(QStringLiteral("Collection A"));
        QCOMPARE(colA.id(), 2);
        QVERIFY(sched->nextScheduledTime(2) <= timeForColB);
        // WHEN
        colA.setCachePolicyInherit(false);
        colA.setCachePolicyCheckInterval(20); // in minutes
        QVERIFY(colA.update());
        sched->collectionChanged(2);
        // THEN
        // "in 20 minutes" is 15 minutes later than "in 5 minutes"
        QTRY_VERIFY(sched->nextScheduledTime(2) >= timeForColB + 14min);
        QVERIFY(sched->nextScheduledTime(2) <= timeForColB + 16min);
        QVERIFY(sched->currentTimerInterval() > 4min); // unchanged
        QVERIFY(sched->currentTimerInterval() < 6min); // unchanged
    }
};

AKTEST_FAKESERVER_MAIN(CollectionSchedulerTest)

#include "collectionschedulertest.moc"
