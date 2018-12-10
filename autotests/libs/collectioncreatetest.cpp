/*
 * Copyright 2017  Daniel Vrátil <dvratil@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "test_utils.h"
#include "collectioncreatejob.h"
#include "collectionfetchjob.h"
#include "collectiondeletejob.h"
#include "entitydisplayattribute.h"

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
        auto monitor = getTestMonitor();
        QSignalSpy spy(monitor, &Monitor::collectionAdded);

        Collection col;
        col.setName(QLatin1String("test_collection"));
        col.setContentMimeTypes({ Collection::mimeType() });
        col.setParentCollection(Collection(collectionIdFromPath(QLatin1String("res1"))));
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
