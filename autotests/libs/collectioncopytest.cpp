/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "agentmanager.h"
#include "control.h"
#include "collectioncopyjob.h"
#include "collectionfetchjob.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "qtest_akonadi.h"

#include <QObject>

using namespace Akonadi;

class CollectionCopyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();

        Control::start();
        // switch target resources offline to reduce interference from them
        foreach (Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances()) {   //krazy:exclude=foreach
            if (agent.identifier() == QLatin1String("akonadi_knut_resource_2")) {
                agent.setIsOnline(false);
            }
        }
    }

    void testCopy()
    {
        const Collection target(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        Collection source(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
        QVERIFY(target.isValid());
        QVERIFY(source.isValid());

        // obtain reference listing
        CollectionFetchJob *fetch = new CollectionFetchJob(source, CollectionFetchJob::Base);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->collections().count(), 1);
        source = fetch->collections().first();
        QVERIFY(source.isValid());

        fetch = new CollectionFetchJob(source, CollectionFetchJob::Recursive);
        AKVERIFYEXEC(fetch);
        QMap<Collection, Item::List> referenceData;
        Collection::List cols = fetch->collections();
        cols << source;
        for (const Collection &c : qAsConst(cols)) {
            ItemFetchJob *job = new ItemFetchJob(c, this);
            AKVERIFYEXEC(job);
            referenceData.insert(c, job->items());
        }

        // actually copy the collection
        CollectionCopyJob *copy = new CollectionCopyJob(source, target);
        AKVERIFYEXEC(copy);

        // list destination and check if everything has arrived
        CollectionFetchJob *list = new CollectionFetchJob(target, CollectionFetchJob::Recursive);
        AKVERIFYEXEC(list);
        cols = list->collections();
        QCOMPARE(cols.count(), referenceData.count());
        for (QMap<Collection, Item::List>::ConstIterator it = referenceData.constBegin(), end = referenceData.constEnd(); it != end; ++it) {
            QVERIFY(!cols.contains(it.key()));
            Collection col;
            for (const Collection &c : qAsConst(cols)) {
                if (it.key().name() == c.name()) {
                    col = c;
                }
            }

            QVERIFY(col.isValid());
            QCOMPARE(col.resource(), QStringLiteral("akonadi_knut_resource_2"));
            QVERIFY(col.remoteId().isEmpty());
            ItemFetchJob *job = new ItemFetchJob(col, this);
            job->fetchScope().fetchFullPayload();
            job->fetchScope().setCacheOnly(true);
            AKVERIFYEXEC(job);
            QCOMPARE(job->items().count(), it.value().count());
            foreach (const Item &item, job->items()) {
                QVERIFY(!it.value().contains(item));
                QVERIFY(item.remoteId().isEmpty());
                QVERIFY(item.hasPayload());
            }
        }
    }

    void testIlleagalCopy()
    {
        // invalid source
        CollectionCopyJob *copy = new CollectionCopyJob(Collection(), Collection(1));
        QVERIFY(!copy->exec());

        // non-existing source
        copy = new CollectionCopyJob(Collection(INT_MAX), Collection(1));
        QVERIFY(!copy->exec());

        // invalid target
        copy = new CollectionCopyJob(Collection(1), Collection());
        QVERIFY(!copy->exec());

        // non-existing target
        copy = new CollectionCopyJob(Collection(1), Collection(INT_MAX));
        QVERIFY(!copy->exec());
    }

};

QTEST_AKONADIMAIN(CollectionCopyTest)

#include "collectioncopytest.moc"
