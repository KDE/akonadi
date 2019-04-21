/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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
    explicit FakeJob(QObject *parent = nullptr) : Job(parent) {}
    void done()
    {
        emitResult();
    }
protected:
    void doStart() override { emitWriteFinished(); }
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

        FakeJob *job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        FakeJob *job2 = new FakeJob(&session);
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

        FakeJob *job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        FakeJob *job2 = new FakeJob(&session);
        QSignalSpy job2DoneSpy(job2, &KJob::result);
        QVERIFY(job2DoneSpy.isValid());

        QCOMPARE(sessionQueueSpy.size(), 2);
        QSignalSpy job1AboutToStartSpy(job1, &Job::aboutToStart);
        QVERIFY(job1AboutToStartSpy.wait());

        // one job running, one queued, now kill the session
        session.clear();
        QVERIFY(sessionReconnectSpy.wait());

        QCOMPARE(job1DoneSpy.size(), 1);
        QCOMPARE(job2DoneSpy.size(), 1);
        QCOMPARE(sessionReconnectSpy.size(), 2);
    }

    void testKillQueuedJob()
    {
        FakeSession session("fakeSession", FakeSession::EndJobsManually);

        QSignalSpy sessionQueueSpy(&session, &FakeSession::jobAdded);
        QVERIFY(sessionQueueSpy.isValid());
        QSignalSpy sessionReconnectSpy(&session, &Session::reconnected);
        QVERIFY(sessionReconnectSpy.isValid());

        FakeJob *job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        FakeJob *job2 = new FakeJob(&session);
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

        FakeJob *job1 = new FakeJob(&session);
        QSignalSpy job1DoneSpy(job1, &KJob::result);
        QVERIFY(job1DoneSpy.isValid());

        FakeJob *job2 = new FakeJob(&session);
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

        FakeJob *parentJob = new FakeJob(&session);
        parentJob->setObjectName(QStringLiteral("parentJob"));
        QSignalSpy parentJobDoneSpy(parentJob, &KJob::result);

        FakeJob *subjob = new FakeJob(parentJob);
        subjob->setObjectName(QStringLiteral("subjob"));
        QSignalSpy subjobDoneSpy(subjob, &KJob::result);

        FakeJob *subjob2 = new FakeJob(parentJob);
        subjob2->setObjectName(QStringLiteral("subjob2"));
        QSignalSpy subjob2DoneSpy(subjob2, &KJob::result);

        FakeJob *nextJob = new FakeJob(&session);
        nextJob->setObjectName(QStringLiteral("nextJob"));
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
