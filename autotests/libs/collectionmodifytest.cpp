/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "entitydisplayattribute.h"
#include "qtest_akonadi.h"

using namespace Akonadi;

class CollectionModifyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testModifyCollection()
    {
        Collection col;
        col.setName(QLatin1String("test_collection"));
        col.setContentMimeTypes({Collection::mimeType()});
        col.setParentCollection(Collection(AkonadiTest::collectionIdFromPath(QLatin1String("res1"))));
        col.setRights(Collection::AllRights);

        auto cj = new CollectionCreateJob(col, this);
        AKVERIFYEXEC(cj);
        col = cj->collection();
        QVERIFY(col.isValid());

        auto attr = col.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(QStringLiteral("Test Collection"));
        col.setContentMimeTypes({Collection::mimeType(), QLatin1String("application/octet-stream")});

        auto mj = new CollectionModifyJob(col, this);
        AKVERIFYEXEC(mj);

        auto fj = new CollectionFetchJob(col, CollectionFetchJob::Base);
        AKVERIFYEXEC(fj);
        QCOMPARE(fj->collections().count(), 1);
        const Collection actual = fj->collections().at(0);

        QCOMPARE(actual.id(), col.id());
        QCOMPARE(actual.name(), col.name());
        QCOMPARE(actual.displayName(), col.displayName());
        QCOMPARE(actual.contentMimeTypes(), col.contentMimeTypes());
        QCOMPARE(actual.parentCollection(), col.parentCollection());
        QCOMPARE(actual.rights(), col.rights());

        auto dj = new CollectionDeleteJob(col, this);
        AKVERIFYEXEC(dj);
    }
};

QTEST_AKONADIMAIN(CollectionModifyTest)

#include "collectionmodifytest.moc"
