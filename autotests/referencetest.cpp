/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "referencetest.h"

#include <sys/types.h>

#include <qtest_akonadi.h>
#include "test_utils.h"

#include "agentmanager.h"
#include "agentinstance.h"
#include "cachepolicy.h"
#include "collection.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "control.h"
#include "item.h"
#include "collectionfetchscope.h"
#include "session.h"
#include "monitor.h"

using namespace Akonadi;

QTEST_AKONADIMAIN(ReferenceTest, NoGUI)

void ReferenceTest::initTestCase()
{
    qRegisterMetaType<Akonadi::Collection::List>();
    AkonadiTest::checkTestIsIsolated();
    Control::start();
    AkonadiTest::setAllResourcesOffline();
}

static Collection::Id res1ColId = 6; // -1;

void ReferenceTest::testReference()
{
    Akonadi::Collection baseCol;
    {
        baseCol.setParentCollection(Akonadi::Collection(res1ColId));
        baseCol.setName("base");
        Akonadi::CollectionCreateJob *create = new Akonadi::CollectionCreateJob(baseCol);
        AKVERIFYEXEC(create);
        baseCol = create->collection();
    }

    {
        Akonadi::Collection col;
        col.setParentCollection(baseCol);
        col.setName("referenced");
        col.setEnabled(false);
        {
            Akonadi::CollectionCreateJob *create = new Akonadi::CollectionCreateJob(col);
            AKVERIFYEXEC(create);
            CollectionFetchJob *job = new CollectionFetchJob(create->collection(), CollectionFetchJob::Base);
            AKVERIFYEXEC(job);
            col = job->collections().first();
        }
        {
            col.setReferenced(true);
            Akonadi::CollectionModifyJob *modify = new Akonadi::CollectionModifyJob(col);
            AKVERIFYEXEC(modify);
            CollectionFetchJob *job = new CollectionFetchJob(col, CollectionFetchJob::Base);
            AKVERIFYEXEC(job);
            Akonadi::Collection result = job->collections().first();
            QCOMPARE(result.enabled(), false);
            QCOMPARE(result.referenced(), true);
        }
        {
            col.setReferenced(false);
            Akonadi::CollectionModifyJob *modify = new Akonadi::CollectionModifyJob(col);
            AKVERIFYEXEC(modify);
            CollectionFetchJob *job = new CollectionFetchJob(col, CollectionFetchJob::Base);
            AKVERIFYEXEC(job);
            Akonadi::Collection result = job->collections().first();
            QCOMPARE(result.enabled(), false);
            QCOMPARE(result.referenced(), false);
        }
    }

    //Cleanup
    CollectionDeleteJob *deleteJob = new CollectionDeleteJob(baseCol);
    AKVERIFYEXEC(deleteJob);
}

void ReferenceTest::testReferenceFromMultiSession()
{
    Akonadi::Collection baseCol;
    {
        baseCol.setParentCollection(Akonadi::Collection(res1ColId));
        baseCol.setName("base");
        baseCol.setEnabled(false);
        Akonadi::CollectionCreateJob *create = new Akonadi::CollectionCreateJob(baseCol);
        AKVERIFYEXEC(create);
        baseCol = create->collection();
    }

    {
        Akonadi::Collection col;
        col.setParentCollection(baseCol);
        col.setName("referenced");
        col.setEnabled(false);
        {
        Akonadi::CollectionCreateJob *create = new Akonadi::CollectionCreateJob(col);
        AKVERIFYEXEC(create);
        CollectionFetchJob *job = new CollectionFetchJob(create->collection(), CollectionFetchJob::Base);
        AKVERIFYEXEC(job);
        col = job->collections().first();
        }

        Akonadi::Session *session1 = new Akonadi::Session("session1");
        Akonadi::Monitor *monitor1 = new Akonadi::Monitor();
        monitor1->setSession(session1);
        monitor1->setCollectionMonitored(Collection::root());

        Akonadi::Session *session2 = new Akonadi::Session("session2");
        Akonadi::Monitor *monitor2 = new Akonadi::Monitor();
        monitor2->setSession(session2);
        monitor2->setCollectionMonitored(Collection::root());

        //Reference in first session and ensure second session is not affected
        {
            QSignalSpy cmodspy1(monitor1, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));
            QSignalSpy cmodspy2(monitor2, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));

            col.setReferenced(true);
            Akonadi::CollectionModifyJob *modify = new Akonadi::CollectionModifyJob(col, session1);
            AKVERIFYEXEC(modify);

            //We want a signal only in the session that referenced the collection
            QVERIFY(QTest::kWaitForSignal(monitor1, SIGNAL(collectionChanged(Akonadi::Collection)), 1000));
            QTest::qWait(100);
            QCOMPARE(cmodspy1.count(), 1);
            QCOMPARE(cmodspy2.count(), 0);

            {
                CollectionFetchJob *job = new CollectionFetchJob(baseCol, CollectionFetchJob::Recursive, session1);
                job->fetchScope().setListFilter(CollectionFetchScope::Display);
                AKVERIFYEXEC(job);
                QCOMPARE(job->collections().size(), 1);
            }

            {
                CollectionFetchJob *job = new CollectionFetchJob(baseCol, CollectionFetchJob::Recursive, session2);
                job->fetchScope().setListFilter(CollectionFetchScope::Display);
                AKVERIFYEXEC(job);
                QCOMPARE(job->collections().size(), 0);
            }
        }
        //Reference in second session and ensure first session is not affected
        {
            QSignalSpy cmodspy1(monitor1, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));
            QSignalSpy cmodspy2(monitor2, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));

            col.setReferenced(true);
            Akonadi::CollectionModifyJob *modify = new Akonadi::CollectionModifyJob(col, session2);
            AKVERIFYEXEC(modify);

            //We want a signal only in the session that referenced the collection
            QVERIFY(QTest::kWaitForSignal(monitor2, SIGNAL(collectionChanged(Akonadi::Collection)), 1000));
            QTest::qWait(100);
            //FIXME The first session still gets the notification since it has the session referenced
            QCOMPARE(cmodspy1.count(), 1);
            QCOMPARE(cmodspy2.count(), 1);

            {
                CollectionFetchJob *job = new CollectionFetchJob(baseCol, CollectionFetchJob::Recursive, session1);
                job->fetchScope().setListFilter(CollectionFetchScope::Display);
                AKVERIFYEXEC(job);
                QCOMPARE(job->collections().size(), 1);
            }

            {
                CollectionFetchJob *job = new CollectionFetchJob(baseCol, CollectionFetchJob::Recursive, session2);
                job->fetchScope().setListFilter(CollectionFetchScope::Display);
                AKVERIFYEXEC(job);
                QCOMPARE(job->collections().size(), 1);
            }
        }
        {
            QSignalSpy cmodspy1(monitor1, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));
            QSignalSpy cmodspy2(monitor2, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));
            col.setReferenced(false);
            Akonadi::CollectionModifyJob *modify = new Akonadi::CollectionModifyJob(col, session1);
            AKVERIFYEXEC(modify);

            //We want a signal only in the session that referenced the collection
            QVERIFY(QTest::kWaitForSignal(monitor1, SIGNAL(collectionChanged(Akonadi::Collection)), 1000));
            QTest::qWait(100);
            QCOMPARE(cmodspy1.count(), 1);
            //FIXME here we still get a notification for dereferenced because we don't filter correctly
            QCOMPARE(cmodspy2.count(), 1);

            {
                CollectionFetchJob *job = new CollectionFetchJob(baseCol, CollectionFetchJob::Recursive, session1);
                job->fetchScope().setListFilter(CollectionFetchScope::Display);
                AKVERIFYEXEC(job);
                QCOMPARE(job->collections().size(), 0);
            }

            {
                CollectionFetchJob *job = new CollectionFetchJob(baseCol, CollectionFetchJob::Recursive, session2);
                job->fetchScope().setListFilter(CollectionFetchScope::Display);
                AKVERIFYEXEC(job);
                QCOMPARE(job->collections().size(), 1);
            }
        }
    }

    //Cleanup
    CollectionDeleteJob *deleteJob = new CollectionDeleteJob(baseCol);
    AKVERIFYEXEC(deleteJob);
}

#include "referencetest.moc"
