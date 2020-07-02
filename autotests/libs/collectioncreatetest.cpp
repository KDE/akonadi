/*
 * SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "collectioncreatejob.h"
#include "collectionfetchjob.h"
#include "collectiondeletejob.h"
#include "entitydisplayattribute.h"
#include "qtest_akonadi.h"

using namespace Akonadi;

class CollectionCreateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testCreateCollection()
    {
        auto monitor = AkonadiTest::getTestMonitor();
        QSignalSpy spy(monitor.get(), &Monitor::collectionAdded);

        Collection col;
        col.setName(QLatin1String("test_collection"));
        col.setContentMimeTypes({ Collection::mimeType() });
        col.setParentCollection(Collection(AkonadiTest::collectionIdFromPath(QLatin1String("res1"))));
        col.setRights(Collection::AllRights);

        CollectionCreateJob *cj = new CollectionCreateJob(col, this);
        AKVERIFYEXEC(cj);
        col = cj->collection();
        QVERIFY(col.isValid());

        QTRY_COMPARE(spy.count(), 1);
        auto ntfCol = spy.at(0).at(0).value<Collection>();
        QCOMPARE(col, ntfCol);

        CollectionDeleteJob *dj = new CollectionDeleteJob(col, this);
        AKVERIFYEXEC(dj);
    }
};

QTEST_AKONADIMAIN(CollectionCreateTest)

#include "collectioncreatetest.moc"
