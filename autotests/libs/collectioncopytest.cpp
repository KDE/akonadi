/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "agentmanager.h"
#include "collectioncopyjob.h"
#include "collectionfetchjob.h"
#include "control.h"
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
        const Akonadi::AgentInstance::List agents = Akonadi::AgentManager::self()->instances();
        for (Akonadi::AgentInstance agent : agents) {
            if (agent.identifier() == QLatin1StringView("akonadi_knut_resource_2")) {
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
        auto fetch = new CollectionFetchJob(source, CollectionFetchJob::Base);
        AKVERIFYEXEC(fetch);
        const auto collections = fetch->collections();
        QCOMPARE(collections.count(), 1);
        source = collections.first();
        QVERIFY(source.isValid());

        fetch = new CollectionFetchJob(source, CollectionFetchJob::Recursive);
        AKVERIFYEXEC(fetch);
        QMap<Collection, Item::List> referenceData;
        Collection::List cols = fetch->collections();
        cols << source;
        for (const Collection &c : std::as_const(cols)) {
            auto job = new ItemFetchJob(c, this);
            AKVERIFYEXEC(job);
            referenceData.insert(c, job->items());
        }

        // actually copy the collection
        auto copy = new CollectionCopyJob(source, target);
        AKVERIFYEXEC(copy);

        // list destination and check if everything has arrived
        auto list = new CollectionFetchJob(target, CollectionFetchJob::Recursive);
        AKVERIFYEXEC(list);
        cols = list->collections();
        QCOMPARE(cols.count(), referenceData.count());
        for (QMap<Collection, Item::List>::ConstIterator it = referenceData.constBegin(), end = referenceData.constEnd(); it != end; ++it) {
            QVERIFY(!cols.contains(it.key()));
            Collection col;
            for (const Collection &c : std::as_const(cols)) {
                if (it.key().name() == c.name()) {
                    col = c;
                }
            }

            QVERIFY(col.isValid());
            QCOMPARE(col.resource(), QStringLiteral("akonadi_knut_resource_2"));
            QVERIFY(col.remoteId().isEmpty());
            auto job = new ItemFetchJob(col, this);
            job->fetchScope().fetchFullPayload();
            job->fetchScope().setCacheOnly(true);
            AKVERIFYEXEC(job);
            QCOMPARE(job->items().count(), it.value().count());
            const Item::List items = job->items();
            for (const Item &item : items) {
                QVERIFY(!it.value().contains(item));
                QVERIFY(item.remoteId().isEmpty());
                QVERIFY(item.hasPayload());
            }
        }
    }

    void testIlleagalCopy()
    {
        // invalid source
        auto copy = new CollectionCopyJob(Collection(), Collection(1));
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

QTEST_AKONADI_CORE_MAIN(CollectionCopyTest)

#include "collectioncopytest.moc"
