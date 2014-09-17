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

Q_DECLARE_METATYPE( KJob* );
Q_DECLARE_METATYPE( Akonadi::Job* )

using namespace Akonadi;

class FakeJob : public Job
{
  Q_OBJECT
  public:
    explicit FakeJob(QObject* parent = 0) : Job(parent) {};
    void done() { emitResult(); }
  protected:
    virtual void doStart() { emitWriteFinished(); };
};

class JobTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase()
    {
      qRegisterMetaType<KJob*>();
      qRegisterMetaType<Akonadi::Job*>();
    }

    void testTopLevelJobExecution()
    {
      FakeSession session("fakeSession", FakeSession::EndJobsManually);

      QSignalSpy sessionQueueSpy(&session, SIGNAL(jobAdded(Akonadi::Job*)));
      QVERIFY(sessionQueueSpy.isValid());

      FakeJob *job1 = new FakeJob(&session);
      QSignalSpy job1DoneSpy(job1, SIGNAL(result(KJob*)));
      QVERIFY(job1DoneSpy.isValid());

      FakeJob *job2 = new FakeJob(&session);
      QSignalSpy job2DoneSpy(job2, SIGNAL(result(KJob*)));
      QVERIFY(job2DoneSpy.isValid());

      QCOMPARE(sessionQueueSpy.size(), 2);
      QCOMPARE(job1DoneSpy.size(), 0);

      QVERIFY(AkonadiTest::akWaitForSignal(job1, SIGNAL(aboutToStart(Akonadi::Job*)), 1));
      QCOMPARE(job1DoneSpy.size(), 0);

      job1->done();
      QCOMPARE(job1DoneSpy.size(), 1);

      QVERIFY(AkonadiTest::akWaitForSignal(job2, SIGNAL(aboutToStart(Akonadi::Job*)), 1));
      QCOMPARE(job2DoneSpy.size(), 0);
      job2->done();

      QCOMPARE(job1DoneSpy.size(), 1);
      QCOMPARE(job2DoneSpy.size(), 1);
    }

    void testKillSession()
    {
      FakeSession session("fakeSession", FakeSession::EndJobsManually);

      QSignalSpy sessionQueueSpy(&session, SIGNAL(jobAdded(Akonadi::Job*)));
      QVERIFY(sessionQueueSpy.isValid());
      QSignalSpy sessionReconnectSpy(&session, SIGNAL(reconnected()));
      QVERIFY(sessionReconnectSpy.isValid());

      FakeJob *job1 = new FakeJob(&session);
      QSignalSpy job1DoneSpy(job1, SIGNAL(result(KJob*)));
      QVERIFY(job1DoneSpy.isValid());

      FakeJob *job2 = new FakeJob(&session);
      QSignalSpy job2DoneSpy(job2, SIGNAL(result(KJob*)));
      QVERIFY(job2DoneSpy.isValid());

      QCOMPARE(sessionQueueSpy.size(), 2);
      QVERIFY(AkonadiTest::akWaitForSignal(job1, SIGNAL(aboutToStart(Akonadi::Job*)), 1));

      // one job running, one queued, now kill the session
      session.clear();
      QVERIFY(AkonadiTest::akWaitForSignal(&session, SIGNAL(reconnected()), 1));

      QCOMPARE(job1DoneSpy.size(), 1);
      QCOMPARE(job2DoneSpy.size(), 1);
      QCOMPARE(sessionReconnectSpy.size(), 1); // the first one is missed as it happens directly from the ctor
    }

    void testKillQueuedJob()
    {
      FakeSession session("fakeSession", FakeSession::EndJobsManually);

      QSignalSpy sessionQueueSpy(&session, SIGNAL(jobAdded(Akonadi::Job*)));
      QVERIFY(sessionQueueSpy.isValid());
      QSignalSpy sessionReconnectSpy(&session, SIGNAL(reconnected()));
      QVERIFY(sessionReconnectSpy.isValid());

      FakeJob *job1 = new FakeJob(&session);
      QSignalSpy job1DoneSpy(job1, SIGNAL(result(KJob*)));
      QVERIFY(job1DoneSpy.isValid());

      FakeJob *job2 = new FakeJob(&session);
      QSignalSpy job2DoneSpy(job2, SIGNAL(result(KJob*)));
      QVERIFY(job2DoneSpy.isValid());

      QCOMPARE(sessionQueueSpy.size(), 2);
      QVERIFY(AkonadiTest::akWaitForSignal(job1, SIGNAL(aboutToStart(Akonadi::Job*)), 1));

      // one job running, one queued, now kill the waiting job
      QVERIFY(job2->kill(KJob::EmitResult));

      QCOMPARE(job1DoneSpy.size(), 0);
      QCOMPARE(job2DoneSpy.size(), 1);

      job1->done();
      QCOMPARE(job1DoneSpy.size(), 1);
      QCOMPARE(job2DoneSpy.size(), 1);
      QCOMPARE(sessionReconnectSpy.size(), 0); // the first one is missed as it happens directly from the ctor
    }

    void testKillRunningJob()
    {
      FakeSession session("fakeSession", FakeSession::EndJobsManually);

      QSignalSpy sessionQueueSpy(&session, SIGNAL(jobAdded(Akonadi::Job*)));
      QVERIFY(sessionQueueSpy.isValid());
      QSignalSpy sessionReconnectSpy(&session, SIGNAL(reconnected()));
      QVERIFY(sessionReconnectSpy.isValid());

      FakeJob *job1 = new FakeJob(&session);
      QSignalSpy job1DoneSpy(job1, SIGNAL(result(KJob*)));
      QVERIFY(job1DoneSpy.isValid());

      FakeJob *job2 = new FakeJob(&session);
      QSignalSpy job2DoneSpy(job2, SIGNAL(result(KJob*)));
      QVERIFY(job2DoneSpy.isValid());

      QCOMPARE(sessionQueueSpy.size(), 2);
      QVERIFY(AkonadiTest::akWaitForSignal(job1, SIGNAL(aboutToStart(Akonadi::Job*)), 1));

      // one job running, one queued, now kill the running one
      QVERIFY(job1->kill(KJob::EmitResult));

      QCOMPARE(job1DoneSpy.size(), 1);
      QCOMPARE(job2DoneSpy.size(), 0);

      // session needs to reconnect, then execute the next job
      QVERIFY(AkonadiTest::akWaitForSignal(job2, SIGNAL(aboutToStart(Akonadi::Job*)), 1));
      QCOMPARE(sessionReconnectSpy.size(), 1);
      job2->done();

      QCOMPARE(job1DoneSpy.size(), 1);
      QCOMPARE(job2DoneSpy.size(), 1);
      QCOMPARE(sessionReconnectSpy.size(), 1); // the first one is missed as it happens directly from the ctor
    }
};

QTEST_AKONADIMAIN( JobTest )

#include "jobtest.moc"
