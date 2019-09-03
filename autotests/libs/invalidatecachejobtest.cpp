/*
    Copyright (c) 2019 David Faure <faure@kde.org>

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

#include "invalidatecachejob_p.h"

#include <collectionpathresolver.h>
#include <itemfetchjob.h>
#include <itemfetchscope.h>
#include <qtest_akonadi.h>
#include "test_utils.h"
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
    Collection col(collectionIdFromPath(QStringLiteral("res1/foo")));
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
    QCOMPARE(fetchJob->items().first().payloadData(), "testmailbody2");

    // Invalidate cache
    auto *invCacheJob = new InvalidateCacheJob(Collection(colId), this);
    AKVERIFYEXEC(invCacheJob);

    // Fetch item from cache, should have no payload anymore
    auto *fetchFromCacheJob = new ItemFetchJob(Item(itemId), this);
    fetchFromCacheJob->fetchScope().fetchFullPayload();
    fetchFromCacheJob->fetchScope().setCacheOnly(true);
    AKVERIFYEXEC(fetchFromCacheJob);
    QVERIFY(fetchFromCacheJob->items().first().payloadData().isEmpty());

    // Fetch item from resource again
    auto *fetchAgainJob = new ItemFetchJob(Item(itemId), this);
    fetchAgainJob->fetchScope().fetchFullPayload();
    AKVERIFYEXEC(fetchAgainJob);
    QCOMPARE(fetchAgainJob->items().first().payloadData(), "testmailbody2");
}

QTEST_AKONADIMAIN(InvalidateCacheJobTest)

#include "invalidatecachejobtest.moc"
