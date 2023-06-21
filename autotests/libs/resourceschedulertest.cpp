/*
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "resourceschedulertest.h"

#include "../src/agentbase/resourcescheduler_p.h"

#include <QSignalSpy>
#include <QTest>

using namespace Akonadi;

QTEST_MAIN(ResourceSchedulerTest)

Q_DECLARE_METATYPE(QSet<QByteArray>)

ResourceSchedulerTest::ResourceSchedulerTest(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QSet<QByteArray>>();
}

void ResourceSchedulerTest::testTaskComparison()
{
    ResourceScheduler::Task t1;
    t1.type = ResourceScheduler::ChangeReplay;
    ResourceScheduler::Task t2;
    t2.type = ResourceScheduler::ChangeReplay;
    QCOMPARE(t1, t2);
    QList<ResourceScheduler::Task> taskList;
    taskList.append(t1);
    QVERIFY(taskList.contains(t2));

    ResourceScheduler::Task t3;
    t3.type = ResourceScheduler::DeleteResourceCollection;
    QVERIFY(!(t2 == t3));
    QVERIFY(!taskList.contains(t3));

    ResourceScheduler::Task t4;
    t4.type = ResourceScheduler::Custom;
    t4.receiver = this;
    t4.methodName = "customTask";
    t4.argument = QStringLiteral("call1");

    ResourceScheduler::Task t5(t4);
    QVERIFY(t4 == t5);

    t5.argument = QStringLiteral("call2");
    QVERIFY(!(t4 == t5));
}

void ResourceSchedulerTest::testChangeReplaySchedule()
{
    ResourceScheduler scheduler;
    scheduler.setOnline(true);
    qRegisterMetaType<Akonadi::Collection>("Akonadi::Collection");
    QSignalSpy changeReplaySpy(&scheduler, SIGNAL(executeChangeReplay()));
    QSignalSpy collectionTreeSyncSpy(&scheduler, SIGNAL(executeCollectionTreeSync()));
    QSignalSpy syncSpy(&scheduler, SIGNAL(executeCollectionSync(Akonadi::Collection)));
    QVERIFY(changeReplaySpy.isValid());
    QVERIFY(collectionTreeSyncSpy.isValid());
    QVERIFY(syncSpy.isValid());

    // Schedule a change replay, it should be executed first thing when we enter the
    // event loop, but not before
    QVERIFY(scheduler.isEmpty());
    scheduler.scheduleChangeReplay();
    QVERIFY(!scheduler.isEmpty());
    QVERIFY(changeReplaySpy.isEmpty());
    QTest::qWait(1);
    QCOMPARE(changeReplaySpy.count(), 1);
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(changeReplaySpy.count(), 1);

    // Schedule two change replays. The duplicate one should not be executed.
    changeReplaySpy.clear();
    scheduler.scheduleChangeReplay();
    scheduler.scheduleChangeReplay();
    QVERIFY(changeReplaySpy.isEmpty());
    QTest::qWait(1);
    QCOMPARE(changeReplaySpy.count(), 1);
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(changeReplaySpy.count(), 1);

    // Schedule a second change replay while one is in progress, should give as two signal emissions
    changeReplaySpy.clear();
    scheduler.scheduleChangeReplay();
    QVERIFY(changeReplaySpy.isEmpty());
    QTest::qWait(1);
    QCOMPARE(changeReplaySpy.count(), 1);
    scheduler.scheduleChangeReplay();
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(changeReplaySpy.count(), 2);
    scheduler.taskDone();

    //
    // Schedule various stuff.
    //
    Collection collection(42);
    changeReplaySpy.clear();
    scheduler.scheduleCollectionTreeSync();
    scheduler.scheduleChangeReplay();
    scheduler.scheduleSync(collection);
    scheduler.scheduleChangeReplay();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 0);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);

    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);

    // Omit a taskDone() here, there shouldn't be a new signal
    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);

    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 1);

    // At this point, we're done, check that nothing else is emitted
    scheduler.taskDone();
    QVERIFY(scheduler.isEmpty());
    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 1);
}

void ResourceSchedulerTest::customTaskNoArg()
{
    ++mCustomCallCount;
}

void ResourceSchedulerTest::customTask(const QVariant &argument)
{
    ++mCustomCallCount;
    mLastArgument = argument;
}

void ResourceSchedulerTest::testCustomTask()
{
    ResourceScheduler scheduler;
    scheduler.setOnline(true);
    mCustomCallCount = 0;

    scheduler.scheduleCustomTask(this, "customTask", QStringLiteral("call1"));
    scheduler.scheduleCustomTask(this, "customTask", QStringLiteral("call1"));
    scheduler.scheduleCustomTask(this, "customTask", QStringLiteral("call2"));
    scheduler.scheduleCustomTask(this, "customTaskNoArg", QVariant());

    QCOMPARE(mCustomCallCount, 0);

    QTest::qWait(1);
    QCOMPARE(mCustomCallCount, 1);
    QCOMPARE(mLastArgument.toString(), QStringLiteral("call1"));

    scheduler.taskDone();
    QVERIFY(!scheduler.isEmpty());
    QTest::qWait(1);
    QCOMPARE(mCustomCallCount, 2);
    QCOMPARE(mLastArgument.toString(), QStringLiteral("call2"));

    scheduler.taskDone();
    QVERIFY(!scheduler.isEmpty());
    QTest::qWait(1);
    QCOMPARE(mCustomCallCount, 3);

    scheduler.taskDone();
    QVERIFY(scheduler.isEmpty());
}

void ResourceSchedulerTest::testCompression()
{
    ResourceScheduler scheduler;
    scheduler.setOnline(true);
    qRegisterMetaType<Akonadi::Collection>("Akonadi::Collection");
    qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
    QSignalSpy fullSyncSpy(&scheduler, SIGNAL(executeFullSync()));
    QSignalSpy collectionTreeSyncSpy(&scheduler, SIGNAL(executeCollectionTreeSync()));
    QSignalSpy syncSpy(&scheduler, SIGNAL(executeCollectionSync(Akonadi::Collection)));
    QSignalSpy fetchSpy(&scheduler, SIGNAL(executeItemFetch(Akonadi::Item, QSet<QByteArray>)));
    QVERIFY(fullSyncSpy.isValid());
    QVERIFY(collectionTreeSyncSpy.isValid());
    QVERIFY(syncSpy.isValid());
    QVERIFY(fetchSpy.isValid());

    // full sync
    QVERIFY(scheduler.isEmpty());
    scheduler.scheduleFullSync();
    scheduler.scheduleFullSync();
    QTest::qWait(1); // start execution
    QCOMPARE(fullSyncSpy.count(), 1);
    scheduler.scheduleCollectionTreeSync();
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(fullSyncSpy.count(), 1);
    QVERIFY(scheduler.isEmpty());

    // collection tree sync
    QVERIFY(scheduler.isEmpty());
    scheduler.scheduleCollectionTreeSync();
    scheduler.scheduleCollectionTreeSync();
    QTest::qWait(1); // start execution
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    scheduler.scheduleCollectionTreeSync();
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QVERIFY(scheduler.isEmpty());

    // sync collection
    scheduler.scheduleSync(Akonadi::Collection(42));
    scheduler.scheduleSync(Akonadi::Collection(42));
    QTest::qWait(1); // start execution
    QCOMPARE(syncSpy.count(), 1);
    scheduler.scheduleSync(Akonadi::Collection(43));
    scheduler.scheduleSync(Akonadi::Collection(42));
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(syncSpy.count(), 2);
    scheduler.taskDone();
    QTest::qWait(2);
    QVERIFY(scheduler.isEmpty());

    // sync collection
    scheduler.scheduleItemFetch(Akonadi::Item(42), QSet<QByteArray>(), QDBusMessage());
    scheduler.scheduleItemFetch(Akonadi::Item(42), QSet<QByteArray>(), QDBusMessage());
    QTest::qWait(1); // start execution
    QCOMPARE(fetchSpy.count(), 1);
    scheduler.scheduleItemFetch(Akonadi::Item(43), QSet<QByteArray>(), QDBusMessage());
    scheduler.scheduleItemFetch(Akonadi::Item(42), QSet<QByteArray>(), QDBusMessage());
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(fetchSpy.count(), 2);
    scheduler.taskDone();
    QTest::qWait(2);
    QVERIFY(scheduler.isEmpty());
}

void ResourceSchedulerTest::testSyncCompletion()
{
    ResourceScheduler scheduler;
    scheduler.setOnline(true);
    QSignalSpy completionSpy(&scheduler, SIGNAL(fullSyncComplete()));
    QVERIFY(completionSpy.isValid());

    // sync completion does not do compression
    QVERIFY(scheduler.isEmpty());
    scheduler.scheduleFullSyncCompletion();
    scheduler.scheduleFullSyncCompletion();
    QTest::qWait(1); // start execution
    QCOMPARE(completionSpy.count(), 1);
    scheduler.scheduleFullSyncCompletion();
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(completionSpy.count(), 2);
    scheduler.taskDone();
    QTest::qWait(1);
    QCOMPARE(completionSpy.count(), 3);
    scheduler.taskDone();
    QVERIFY(scheduler.isEmpty());
}

void ResourceSchedulerTest::testPriorities()
{
    ResourceScheduler scheduler;
    scheduler.setOnline(true);
    qRegisterMetaType<Akonadi::Collection>("Akonadi::Collection");
    qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
    QSignalSpy changeReplaySpy(&scheduler, SIGNAL(executeChangeReplay()));
    QSignalSpy fullSyncSpy(&scheduler, SIGNAL(executeFullSync()));
    QSignalSpy collectionTreeSyncSpy(&scheduler, SIGNAL(executeCollectionTreeSync()));
    QSignalSpy syncSpy(&scheduler, SIGNAL(executeCollectionSync(Akonadi::Collection)));
    QSignalSpy fetchSpy(&scheduler, SIGNAL(executeItemFetch(Akonadi::Item, QSet<QByteArray>)));
    QSignalSpy attributesSyncSpy(&scheduler, SIGNAL(executeCollectionAttributesSync(Akonadi::Collection)));
    QVERIFY(changeReplaySpy.isValid());
    QVERIFY(fullSyncSpy.isValid());
    QVERIFY(collectionTreeSyncSpy.isValid());
    QVERIFY(syncSpy.isValid());
    QVERIFY(fetchSpy.isValid());
    QVERIFY(attributesSyncSpy.isValid());

    scheduler.scheduleCollectionTreeSync();
    scheduler.scheduleChangeReplay();
    scheduler.scheduleSync(Akonadi::Collection(42));
    scheduler.scheduleItemFetch(Akonadi::Item(42), QSet<QByteArray>(), QDBusMessage());
    scheduler.scheduleAttributesSync(Akonadi::Collection(42));
    scheduler.scheduleFullSync();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 0);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);
    QCOMPARE(fullSyncSpy.count(), 0);
    QCOMPARE(fetchSpy.count(), 0);
    QCOMPARE(attributesSyncSpy.count(), 0);
    scheduler.taskDone();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 0);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);
    QCOMPARE(fullSyncSpy.count(), 0);
    QCOMPARE(fetchSpy.count(), 1);
    QCOMPARE(attributesSyncSpy.count(), 0);
    scheduler.taskDone();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 0);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);
    QCOMPARE(fullSyncSpy.count(), 0);
    QCOMPARE(fetchSpy.count(), 1);
    QCOMPARE(attributesSyncSpy.count(), 1);
    scheduler.taskDone();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 0);
    QCOMPARE(fullSyncSpy.count(), 0);
    QCOMPARE(fetchSpy.count(), 1);
    QCOMPARE(attributesSyncSpy.count(), 1);
    scheduler.taskDone();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 1);
    QCOMPARE(fullSyncSpy.count(), 0);
    QCOMPARE(fetchSpy.count(), 1);
    QCOMPARE(attributesSyncSpy.count(), 1);
    scheduler.taskDone();

    QTest::qWait(1);
    QCOMPARE(collectionTreeSyncSpy.count(), 1);
    QCOMPARE(changeReplaySpy.count(), 1);
    QCOMPARE(syncSpy.count(), 1);
    QCOMPARE(fullSyncSpy.count(), 1);
    QCOMPARE(fetchSpy.count(), 1);
    QCOMPARE(attributesSyncSpy.count(), 1);
    scheduler.taskDone();

    QVERIFY(scheduler.isEmpty());
}

#include "moc_resourceschedulertest.cpp"
