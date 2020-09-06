/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "invalidatecachejob_p.h"

#include "collectionpathresolver.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "qtest_akonadi.h"
#include "control.h"

using namespace Akonadi;

class InvalidateCacheJobTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void shouldClearPayload();
};


void InvalidateCacheJobTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
}

void InvalidateCacheJobTest::shouldClearPayload()
{
    // Find collection by name
    Collection col(AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")));
    const int colId = col.id();
    QVERIFY(colId > 0);

    // Find item with remote id "C"
    auto *listJob = new ItemFetchJob(Collection(colId), this);
    AKVERIFYEXEC(listJob);
    const Item::List items = listJob->items();
    QVERIFY(!items.isEmpty());
    auto it = std::find_if(items.cbegin(), items.cend(), [](const Item &item) { return item.remoteId() == QLatin1Char('C'); });
    QVERIFY(it != items.cend());
    const Item::Id itemId = it->id();

    // Fetch item, from resource, with payload
    auto *fetchJob = new ItemFetchJob(Item(itemId), this);
    fetchJob->fetchScope().fetchFullPayload();
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().payload<QByteArray>(), "testmailbody2");

    auto *invCacheJob = new InvalidateCacheJob(Collection(colId), this);
    AKVERIFYEXEC(invCacheJob);

    // Fetch item from cache, should have no payload anymore
    auto *fetchFromCacheJob = new ItemFetchJob(Item(itemId), this);
    fetchFromCacheJob->fetchScope().fetchFullPayload();
    fetchFromCacheJob->fetchScope().setCacheOnly(true);
    AKVERIFYEXEC(fetchFromCacheJob);
    QVERIFY(fetchFromCacheJob->items().first().payload<QByteArray>().isEmpty());

    // Fetch item from resource again
    auto *fetchAgainJob = new ItemFetchJob(Item(itemId), this);
    fetchAgainJob->fetchScope().fetchFullPayload();
    AKVERIFYEXEC(fetchAgainJob);
    QCOMPARE(fetchAgainJob->items().first().payload<QByteArray>(), "testmailbody2");
}

QTEST_AKONADIMAIN(InvalidateCacheJobTest)

#include "invalidatecachejobtest.moc"
