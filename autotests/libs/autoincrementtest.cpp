/*
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "autoincrementtest.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "control.h"
#include "item.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"

#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN(AutoIncrementTest)

void AutoIncrementTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
    AkonadiTest::setAllResourcesOffline();

    itemTargetCollection = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/space folder")));
    QVERIFY(itemTargetCollection.isValid());

    collectionTargetCollection = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    QVERIFY(collectionTargetCollection.isValid());
}

Akonadi::ItemCreateJob *AutoIncrementTest::createItemCreateJob()
{
    QByteArray payload("Hello world");
    Item item(-1);
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload(payload);
    return new ItemCreateJob(item, itemTargetCollection);
}

Akonadi::CollectionCreateJob *AutoIncrementTest::createCollectionCreateJob(int number)
{
    Collection collection;
    collection.setParentCollection(collectionTargetCollection);
    collection.setName(QStringLiteral("testCollection") + QString::number(number));
    return new CollectionCreateJob(collection);
}

void AutoIncrementTest::testItemAutoIncrement()
{
    QList<Item> itemsToDelete;
    Item::Id lastId = -1;

    // Create 20 test items
    for (int i = 0; i < 20; i++) {
        ItemCreateJob *job = createItemCreateJob();
        AKVERIFYEXEC(job);
        Item newItem = job->item();
        QVERIFY(newItem.id() > lastId);
        lastId = newItem.id();
        itemsToDelete.append(newItem);
    }

    // Delete the 20 items
    for (const Item &item : qAsConst(itemsToDelete)) {
        auto *job = new ItemDeleteJob(item);
        AKVERIFYEXEC(job);
    }

    // Restart the server, then test item creation again. The new id of the item
    // should be higher than all ids before.
    AkonadiTest::restartAkonadiServer();
    ItemCreateJob *job = createItemCreateJob();
    AKVERIFYEXEC(job);
    Item newItem = job->item();

    QVERIFY(newItem.id() > lastId);
}

void AutoIncrementTest::testCollectionAutoIncrement()
{
    Collection::List collectionsToDelete;
    Collection::Id lastId = -1;

    // Create 20 test collections
    for (int i = 0; i < 20; i++) {
        CollectionCreateJob *job = createCollectionCreateJob(i);
        AKVERIFYEXEC(job);
        Collection newCollection = job->collection();
        QVERIFY(newCollection.id() > lastId);
        lastId = newCollection.id();
        collectionsToDelete.append(newCollection);
    }

    // Delete the 20 collections
    foreach (const Collection &collection, collectionsToDelete) {
        auto *job = new CollectionDeleteJob(collection);
        AKVERIFYEXEC(job);
    }

    // Restart the server, then test collection creation again. The new id of the collection
    // should be higher than all ids before.
    AkonadiTest::restartAkonadiServer();

    CollectionCreateJob *job = createCollectionCreateJob(0);
    AKVERIFYEXEC(job);
    Collection newCollection = job->collection();

    QVERIFY(newCollection.id() > lastId);
}
