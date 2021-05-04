/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "agentmanager.h"
#include "collection.h"
#include "collectionstatistics.h"
#include "control.h"
#include "itemcopyjob.h"
#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include "qtest_akonadi.h"

#include <QObject>
#include <QTemporaryFile>

using namespace Akonadi;

class ItemCopyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        Control::start();
        // switch target resources offline to reduce interference from them
        foreach (Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances()) { // krazy:exclude=foreach
            if (agent.identifier() == QLatin1String("akonadi_knut_resource_2")) {
                agent.setIsOnline(false);
            }
        }
    }

    void testCopy()
    {
        const Collection target(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        QVERIFY(target.isValid());

        auto copy = new ItemCopyJob(Item(1), target);
        AKVERIFYEXEC(copy);

        Item source(1);
        auto sourceFetch = new ItemFetchJob(source);
        AKVERIFYEXEC(sourceFetch);
        source = sourceFetch->items().first();

        auto fetch = new ItemFetchJob(target);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().fetchAllAttributes();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);

        Item item = fetch->items().first();
        QVERIFY(item.hasPayload());
        QVERIFY(source.size() > 0);
        QVERIFY(item.size() > 0);
        QCOMPARE(item.size(), source.size());
        QCOMPARE(item.attributes().count(), 1);
        QVERIFY(item.remoteId().isEmpty());
        QEXPECT_FAIL("", "statistics are not properly updated after copy", Abort);
        QCOMPARE(target.statistics().count(), 1LL);
    }

    void testIlleagalCopy()
    {
        // empty item list
        auto copy = new ItemCopyJob(Item::List(), Collection::root());
        QVERIFY(!copy->exec());

        // non-existing target
        copy = new ItemCopyJob(Item(1), Collection(INT_MAX));
        QVERIFY(!copy->exec());

        // non-existing source
        copy = new ItemCopyJob(Item(INT_MAX), Collection::root());
        QVERIFY(!copy->exec());
    }

    void testCopyForeign()
    {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write("123456789");
        file.close();

        const Collection source(AkonadiTest::collectionIdFromPath(QStringLiteral("res2")));

        Item item(QStringLiteral("application/octet-stream"));
        item.setPayloadPath(file.fileName());

        auto create = new ItemCreateJob(item, source, this);
        AKVERIFYEXEC(create);
        item = create->item();

        const Collection target(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
        auto copy = new ItemCopyJob(item, target, this);
        AKVERIFYEXEC(copy);

        auto fetch = new ItemFetchJob(target, this);
        fetch->fetchScope().fetchFullPayload(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().size(), 1);
        auto copiedItem = fetch->items().at(0);

        // Copied payload should be completely stored inside Akonadi
        QVERIFY(copiedItem.payloadPath().isEmpty());
        QVERIFY(copiedItem.hasPayload<QByteArray>());
        QCOMPARE(copiedItem.payload<QByteArray>(), item.payload<QByteArray>());
    }
};

QTEST_AKONADIMAIN(ItemCopyTest)

#include "itemcopytest.moc"
