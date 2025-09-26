/*
    SPDX-FileCopyrightText: 2006, 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionmovejob.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "qtest_akonadi.h"

#include <QHash>

using namespace Akonadi;

class CollectionMoveTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }
    void testIllegalMove_data()
    {
        QTest::addColumn<Collection>("source");
        QTest::addColumn<Collection>("destination");

        const Collection res1(AkonadiTest::collectionIdFromPath(QStringLiteral("res1")));
        const Collection res1foo(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        const Collection res1bla(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo/bar/bla")));

        QTest::newRow("non-existing-target") << res1 << Collection(INT_MAX);
        QTest::newRow("root") << Collection::root() << res1;
        QTest::newRow("move-into-child") << res1 << res1foo;
        QTest::newRow("same-name-in-target") << res1bla << res1foo;
        QTest::newRow("non-existing-source") << Collection(INT_MAX) << res1;
    }

    void testIllegalMove()
    {
        QFETCH(Collection, source);
        QFETCH(Collection, destination);
        QVERIFY(source.isValid());
        QVERIFY(destination.isValid());

        auto mod = new CollectionMoveJob(source, destination, this);
        QVERIFY(!mod->exec());
    }

    void testMove_data()
    {
        QTest::addColumn<Collection>("source");
        QTest::addColumn<Collection>("destination");
        QTest::addColumn<bool>("crossResource");

        QTest::newRow("inter-resource") << Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res1")))
                                        << Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res2"))) << true;
        QTest::newRow("intra-resource") << Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo/bla")))
                                        << Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo/bar/bla"))) << false;
    }

    // TODO: test signals
    void testMove()
    {
        QFETCH(Collection, source);
        QFETCH(Collection, destination);
        QFETCH(bool, crossResource);
        QVERIFY(source.isValid());
        QVERIFY(destination.isValid());

        auto fetch = new CollectionFetchJob(source, CollectionFetchJob::Base, this);
        fetch->fetchScope().setAncestorRetrieval(CollectionFetchScope::Parent);
        AKVERIFYEXEC(fetch);
        auto collections = fetch->collections();
        QCOMPARE(collections.count(), 1);
        source = collections.first();

        // obtain reference listing
        fetch = new CollectionFetchJob(source, CollectionFetchJob::Recursive);
        AKVERIFYEXEC(fetch);
        QHash<Collection, Item::List> referenceData;
        collections = fetch->collections();
        for (const Collection &c : collections) {
            auto job = new ItemFetchJob(c, this);
            AKVERIFYEXEC(job);
            referenceData.insert(c, job->items());
        }

        // move collection
        auto mod = new CollectionMoveJob(source, destination, this);
        AKVERIFYEXEC(mod);

        // check if source was modified correctly
        auto ljob = new CollectionFetchJob(source, CollectionFetchJob::Base);
        AKVERIFYEXEC(ljob);
        Collection::List list = ljob->collections();

        QCOMPARE(list.count(), 1);
        Collection col = list.first();
        QCOMPARE(col.name(), source.name());
        QCOMPARE(col.parentCollection(), destination);

        // list destination and check if everything is still there
        ljob = new CollectionFetchJob(destination, CollectionFetchJob::Recursive);
        AKVERIFYEXEC(ljob);
        list = ljob->collections();

        QVERIFY(list.count() >= referenceData.count());
        for (QHash<Collection, Item::List>::ConstIterator it = referenceData.constBegin(); it != referenceData.constEnd(); ++it) {
            QVERIFY(list.contains(it.key()));
            if (crossResource) {
                QVERIFY(list[list.indexOf(it.key())].resource() != it.key().resource());
            } else {
                QCOMPARE(list[list.indexOf(it.key())].resource(), it.key().resource());
            }
            auto job = new ItemFetchJob(it.key(), this);
            job->fetchScope().fetchFullPayload();
            AKVERIFYEXEC(job);
            QCOMPARE(job->items().count(), it.value().count());
            const Item::List items = job->items();
            for (const Item &item : items) {
                QVERIFY(it.value().contains(item));
                QVERIFY(item.hasPayload());
            }
        }

        // cleanup
        mod = new CollectionMoveJob(col, source.parentCollection(), this);
        AKVERIFYEXEC(mod);
    }
};

QTEST_AKONADI_CORE_MAIN(CollectionMoveTest)

#include "collectionmovetest.moc"
