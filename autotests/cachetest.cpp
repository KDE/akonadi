/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "test_utils.h"

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "agentmanager.h"
#include "agentinstance.h"
#include "itemcopyjob.h"

#include <QHash>

using namespace Akonadi;

class CacheTest : public QObject
{
    Q_OBJECT
private:
    void enableAgent(const QString &id, bool enable)
    {
        foreach (AgentInstance agent, Akonadi::AgentManager::self()->instances()) {   //krazy:exclude=foreach
            if (agent.identifier() == id) {
                agent.setIsOnline(enable);
            }
        }
    }

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }
    void testRetrievalErrorBurst() // caused rare server crashs with old item retrieval code
    {
        Collection col(collectionIdFromPath(QLatin1String("res1/foo")));
        QVERIFY(col.isValid());

        enableAgent(QLatin1String("akonadi_knut_resource_0"), false);

        ItemFetchJob *fetch = new ItemFetchJob(col, this);
        fetch->fetchScope().fetchFullPayload(true);
        QVERIFY(!fetch->exec());
    }

    void testResourceRetrievalOnFetch_data()
    {
        QTest::addColumn<Item>("item");
        QTest::addColumn<bool>("resourceEnabled");

        QTest::newRow("resource online") << Item(1) << true;
        QTest::newRow("resource offline") << Item(2) << false;
    }

    void testResourceRetrievalOnFetch()
    {
        QFETCH(Item, item);
        QFETCH(bool, resourceEnabled);

        ItemFetchJob *fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QVERIFY(!item.hasPayload());

        enableAgent(QLatin1String("akonadi_knut_resource_0"), resourceEnabled);

        fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        QCOMPARE(fetch->exec(), resourceEnabled);
        if (resourceEnabled) {
            QCOMPARE(fetch->items().count(), 1);
            item = fetch->items().first();
            QVERIFY(item.isValid());
            QVERIFY(item.hasPayload());
            QVERIFY(item.revision() > 0);   // was changed by the resource delivering the payload
        }

        fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QCOMPARE(item.hasPayload(), resourceEnabled);
    }

    void testResourceRetrievalOnCopy_data()
    {
        QTest::addColumn<Item>("item");
        QTest::addColumn<bool>("resourceEnabled");

        QTest::newRow("online") << Item(3) << true;
        QTest::newRow("offline") << Item(4) << false;
    }

    void testResourceRetrievalOnCopy()
    {
        QFETCH(Item, item);
        QFETCH(bool, resourceEnabled);

        ItemFetchJob *fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QVERIFY(!item.hasPayload());

        enableAgent(QLatin1String("akonadi_knut_resource_0"), resourceEnabled);

        Collection dest(collectionIdFromPath(QLatin1String("res3")));
        QVERIFY(dest.isValid());

        ItemCopyJob *copy = new ItemCopyJob(item, dest, this);
        QCOMPARE(copy->exec(), resourceEnabled);

        fetch = new ItemFetchJob(item, this);
        fetch->fetchScope().fetchFullPayload();
        fetch->fetchScope().setCacheOnly(true);
        AKVERIFYEXEC(fetch);
        QCOMPARE(fetch->items().count(), 1);
        item = fetch->items().first();
        QVERIFY(item.isValid());
        QCOMPARE(item.hasPayload(), resourceEnabled);
    }

};

QTEST_AKONADIMAIN(CacheTest)

#include "cachetest.moc"
