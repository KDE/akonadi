/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <qtest_akonadi.h>

#include "fakesession.h"
#include "job.h"

Q_DECLARE_METATYPE(KJob *)
Q_DECLARE_METATYPE(Akonadi::Job *)

using namespace Akonadi;

class FakeJob : public Job
{
    Q_OBJECT
public:
    explicit FakeJob(QObject *parent = nullptr)
        : Job(parent)
    {
    }
    void done()
    {
        emitResult();
    }

protected:
    void doStart() override
    {
        emitWriteFinished();
    }
};

class JobTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        qRegisterMetaType<KJob *>();
        qRegisterMetaType<Akonadi::Job *>();
    }

    void testTopLevelJobExecution()
    {
        FakeSession session("fakeSession", FakeSession::EndJobsManually);

        QSignalSpy sessionQueueSpy(&session, &FakeSession::jobAdded);
        QVERIFY(sessionQueueSpy.isValid());

        auto job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        auto job2 = new FakeJob(&session);
        QSignalSpy job2DoneSpy(job2, &KJob::result);
        QVERIFY(job2DoneSpy.isValid());

        QCOMPARE(sessionQueueSpy.size(), 2);
        QCOMPARE(job1DoneSpy.size(), 0);

        QSignalSpy job1AboutToStartSpy(job1, &Job::aboutToStart);
        QVERIFY(job1AboutToStartSpy.wait());

        QCOMPARE(job1DoneSpy.size(), 0);

        job1->done();
        QCOMPARE(job1DoneSpy.size(), 1);

        QSignalSpy job2AboutToStartSpy(job2, &Job::aboutToStart);
        QVERIFY(job2AboutToStartSpy.wait());
        QCOMPARE(job2DoneSpy.size(), 0);
        job2->done();

        QCOMPARE(job1DoneSpy.size(), 1);
        QCOMPARE(job2DoneSpy.size(), 1);
    }

    void testKillSession()
    {
        FakeSession session("fakeSession", FakeSession::EndJobsManually);

        QSignalSpy sessionQueueSpy(&session, &FakeSession::jobAdded);
        QVERIFY(sessionQueueSpy.isValid());
        QSignalSpy sessionReconnectSpy(&session, &Session::reconnected);
        QVERIFY(sessionReconnectSpy.isValid());

        auto job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        auto job2 = new FakeJob(&session);
        QSignalSpy job2DoneSpy(job2, &KJob::result);
        QVERIFY(job2DoneSpy.isValid());

        auto job3 = new FakeJob(&session);
        QSignalSpy job3DoneSpy(job3, &KJob::result);
        QVERIFY(job3DoneSpy.isValid());

        auto job4 = new FakeJob(&session);
        QSignalSpy job4DoneSpy(job4, &KJob::result);
        QVERIFY(job4DoneSpy.isValid());

        QCOMPARE(sessionQueueSpy.size(), 4);
        QSignalSpy job1AboutToStartSpy(job1, &Job::aboutToStart);
        QVERIFY(job1AboutToStartSpy.wait());

        // one job running, 3 queued, now kill the session
        session.clear();
        QVERIFY(sessionReconnectSpy.wait());

        QCOMPARE(job1DoneSpy.size(), 1);
        QCOMPARE(job2DoneSpy.size(), 1);
        QCOMPARE(job3DoneSpy.size(), 1);
        QCOMPARE(job4DoneSpy.size(), 1);
        QCOMPARE(sessionReconnectSpy.size(), 2);
    }

    void testKillQueuedJob()
    {
        FakeSession session("fakeSession", FakeSession::EndJobsManually);

        QSignalSpy sessionQueueSpy(&session, &FakeSession::jobAdded);
        QVERIFY(sessionQueueSpy.isValid());
        QSignalSpy sessionReconnectSpy(&session, &Session::reconnected);
        QVERIFY(sessionReconnectSpy.isValid());

        auto job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        auto job2 = new FakeJob(&session);
        QSignalSpy job2DoneSpy(job2, &KJob::result);
        QVERIFY(job2DoneSpy.isValid());

        QCOMPARE(sessionQueueSpy.size(), 2);
        QSignalSpy job1AboutToStartSpy(job1, &Job::aboutToStart);
        QVERIFY(job1AboutToStartSpy.wait());

        // one job running, one queued, now kill the waiting job
        QVERIFY(job2->kill(KJob::EmitResult));

        QCOMPARE(job1DoneSpy.size(), 0);
        QCOMPARE(job2DoneSpy.size(), 1);

        job1->done();
        QCOMPARE(job1DoneSpy.size(), 1);
        QCOMPARE(job2DoneSpy.size(), 1);
        QCOMPARE(sessionReconnectSpy.size(), 1);
    }

    void testKillRunningJob()
    {
        FakeSession session("fakeSession", FakeSession::EndJobsManually);

        QSignalSpy sessionQueueSpy(&session, &FakeSession::jobAdded);
        QVERIFY(sessionQueueSpy.isValid());
        QSignalSpy sessionReconnectSpy(&session, &Session::reconnected);
        QVERIFY(sessionReconnectSpy.isValid());

        auto job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        auto job2 = new FakeJob(&session);
        QSignalSpy job2DoneSpy(job2, &KJob::result);
        QVERIFY(job2DoneSpy.isValid());

        QCOMPARE(sessionQueueSpy.size(), 2);
        QSignalSpy job1AboutToStartSpy(job1, &Job::aboutToStart);
        QVERIFY(job1AboutToStartSpy.wait());

        // one job running, one queued, now kill the running one
        QVERIFY(job1->kill(KJob::EmitResult));

        QCOMPARE(job1DoneSpy.size(), 1);
        QCOMPARE(job2DoneSpy.size(), 0);

        // session needs to reconnect, then execute the next job
        QSignalSpy job2AboutToStartSpy(job2, &Job::aboutToStart);
        QVERIFY(job2AboutToStartSpy.wait());
        QCOMPARE(sessionReconnectSpy.size(), 2);
        job2->done();

        QCOMPARE(job1DoneSpy.size(), 1);
        QCOMPARE(job2DoneSpy.size(), 1);
        QCOMPARE(sessionReconnectSpy.size(), 2);
    }

    void testKillRunningSubjob()
    {
        FakeSession session("fakeSession", FakeSession::EndJobsManually);

        QSignalSpy sessionQueueSpy(&session, &FakeSession::jobAdded);
        QSignalSpy sessionReconnectSpy(&session, &Session::reconnected);

        auto parentJob = new FakeJob(&session);
        parentJob->setObjectName(QLatin1StringView("parentJob"));
        QSignalSpy parentJobDoneSpy(parentJob, &KJob::result);

        auto subjob = new FakeJob(parentJob);
        subjob->setObjectName(QLatin1StringView("subjob"));
        QSignalSpy subjobDoneSpy(subjob, &KJob::result);

        auto subjob2 = new FakeJob(parentJob);
        subjob2->setObjectName(QLatin1StringView("subjob2"));
        QSignalSpy subjob2DoneSpy(subjob2, &KJob::result);

        auto nextJob = new FakeJob(&session);
        nextJob->setObjectName(QLatin1StringView("nextJob"));
        QSignalSpy nextJobDoneSpy(nextJob, &KJob::result);

        QCOMPARE(sessionQueueSpy.size(), 2);
        QSignalSpy parentJobAboutToStart(parentJob, &Job::aboutToStart);
        QVERIFY(parentJobAboutToStart.wait());

        QSignalSpy subjobAboutToStart(subjob, &Job::aboutToStart);
        QVERIFY(subjobAboutToStart.wait());

        // one parent job, one subjob running (another one waiting), now kill the running subjob
        QVERIFY(subjob->kill(KJob::EmitResult));

        QCOMPARE(subjobDoneSpy.size(), 1);
        QCOMPARE(subjob2DoneSpy.size(), 0);

        // Note that killing a subjob aborts the whole parent job
        // Since the session only knows about the parent
        QCOMPARE(parentJobDoneSpy.size(), 1);

        // session needs to reconnect, then execute the next job
        QSignalSpy nextJobAboutToStartSpy(nextJob, &Job::aboutToStart);
        QVERIFY(nextJobAboutToStartSpy.wait());
        QCOMPARE(sessionReconnectSpy.size(), 2);
        nextJob->done();

        QCOMPARE(subjob2DoneSpy.size(), 0);
        QCOMPARE(nextJobDoneSpy.size(), 1);
    }
};

QTEST_AKONADIMAIN(JobTest)

#include "jobtest.moc"
