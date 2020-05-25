/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include <QObject>

#include "control.h"
#include "relationcreatejob.h"
#include "relationfetchjob.h"
#include "relationdeletejob.h"
#include "tagmodifyjob.h"
#include "resourceselectjob_p.h"
#include "qtest_akonadi.h"
#include "item.h"
#include "itemcreatejob.h"
#include "itemmodifyjob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "monitor.h"
#include "attributefactory.h"

using namespace Akonadi;

class RelationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testCreateFetch();
    void testMonitor();
    void testEqualRelation();
};

void RelationTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    AkonadiTest::setAllResourcesOffline();
    qRegisterMetaType<Akonadi::Relation>();
    qRegisterMetaType<QSet<Akonadi::Relation> >();
    qRegisterMetaType<Akonadi::Item::List>();
}

void RelationTest::testCreateFetch()
{
    const Collection res3{AkonadiTest::collectionIdFromPath(QStringLiteral("res3"))};
    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }
    Item item2;
    {
        item2.setMimeType(QStringLiteral("application/octet-stream"));
        ItemCreateJob *append = new ItemCreateJob(item2, res3, this);
        AKVERIFYEXEC(append);
        item2 = append->item();
    }

    Relation rel(Relation::GENERIC, item1, item2);
    RelationCreateJob *createjob = new RelationCreateJob(rel, this);
    AKVERIFYEXEC(createjob);

    //Test fetch & create
    {
        RelationFetchJob *fetchJob = new RelationFetchJob(QVector<QByteArray>(), this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->relations().size(), 1);
        QCOMPARE(fetchJob->relations().first().type(), QByteArray(Relation::GENERIC));
    }

    //Test item fetch
    {
        ItemFetchJob *fetchJob = new ItemFetchJob(item1);
        fetchJob->fetchScope().setFetchRelations(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->items().first().relations().size(), 1);
    }

    {
        ItemFetchJob *fetchJob = new ItemFetchJob(item2);
        fetchJob->fetchScope().setFetchRelations(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->items().first().relations().size(), 1);
    }

    //Test delete
    {
        RelationDeleteJob *deleteJob = new RelationDeleteJob(rel, this);
        AKVERIFYEXEC(deleteJob);

        RelationFetchJob *fetchJob = new RelationFetchJob(QVector<QByteArray>(), this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->relations().size(), 0);
    }
}

void RelationTest::testMonitor()
{
    Akonadi::Monitor monitor;
    monitor.setTypeMonitored(Akonadi::Monitor::Relations);
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    const Collection res3{AkonadiTest::collectionIdFromPath(QStringLiteral("res3"))};
    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }
    Item item2;
    {
        item2.setMimeType(QStringLiteral("application/octet-stream"));
        ItemCreateJob *append = new ItemCreateJob(item2, res3, this);
        AKVERIFYEXEC(append);
        item2 = append->item();
    }

    Relation rel(Relation::GENERIC, item1, item2);

    {
        QSignalSpy addedSpy(&monitor, &Monitor::relationAdded);

        RelationCreateJob *createjob = new RelationCreateJob(rel, this);
        AKVERIFYEXEC(createjob);

        //We usually pick up signals from the previous tests as well (due to server-side notification caching)
        QTRY_VERIFY(addedSpy.count() >= 1);
        QTRY_COMPARE(addedSpy.last().first().value<Akonadi::Relation>(), rel);
    }

    {
        QSignalSpy removedSpy(&monitor, &Monitor::relationRemoved);
        QVERIFY(removedSpy.isValid());
        RelationDeleteJob *deleteJob = new RelationDeleteJob(rel, this);
        AKVERIFYEXEC(deleteJob);
        QTRY_VERIFY(removedSpy.count() >= 1);
        QTRY_COMPARE(removedSpy.last().first().value<Akonadi::Relation>(), rel);
    }
}

void RelationTest::testEqualRelation()
{
    Relation r1;
    Item it1(45);
    Item it2(46);
    r1.setLeft(it1);
    r1.setRight(it2);
    r1.setRemoteId(QByteArrayLiteral("foo"));
    r1.setType(QByteArrayLiteral("foo1"));

    Relation r2 = r1;
    QCOMPARE(r1, r2);
}

QTEST_AKONADIMAIN(RelationTest)

#include "relationtest.moc"
