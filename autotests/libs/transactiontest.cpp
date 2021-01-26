/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transactiontest.h"

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "control.h"
#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "session.h"
#include "transactionjobs.h"

#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN(TransactionTest)

void TransactionTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
}

void TransactionTest::testTransaction()
{
    Collection basisCollection;

    CollectionFetchJob *listJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
    AKVERIFYEXEC(listJob);
    Collection::List list = listJob->collections();
    foreach (const Collection &col, list)
        if (col.name() == QLatin1String("res3")) {
            basisCollection = col;
        }

    Collection testCollection;
    testCollection.setParentCollection(basisCollection);
    testCollection.setName(QStringLiteral("transactionTest"));
    testCollection.setRemoteId(QStringLiteral("transactionTestRemoteId"));
    auto *job = new CollectionCreateJob(testCollection, Session::defaultSession());

    AKVERIFYEXEC(job);

    testCollection = job->collection();

    auto *beginTransaction1 = new TransactionBeginJob(Session::defaultSession());
    AKVERIFYEXEC(beginTransaction1);

    auto *beginTransaction2 = new TransactionBeginJob(Session::defaultSession());
    AKVERIFYEXEC(beginTransaction2);

    auto *commitTransaction2 = new TransactionCommitJob(Session::defaultSession());
    AKVERIFYEXEC(commitTransaction2);

    auto *commitTransaction1 = new TransactionCommitJob(Session::defaultSession());
    AKVERIFYEXEC(commitTransaction1);

    auto *commitTransactionX = new TransactionCommitJob(Session::defaultSession());
    QVERIFY(commitTransactionX->exec() == false);

    auto *beginTransaction3 = new TransactionBeginJob(Session::defaultSession());
    AKVERIFYEXEC(beginTransaction3);

    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setPayload<QByteArray>("body data");
    auto *appendJob = new ItemCreateJob(item, testCollection, Session::defaultSession());
    AKVERIFYEXEC(appendJob);

    auto *rollbackTransaction3 = new TransactionRollbackJob(Session::defaultSession());
    AKVERIFYEXEC(rollbackTransaction3);

    auto *fetchJob = new ItemFetchJob(testCollection, Session::defaultSession());
    AKVERIFYEXEC(fetchJob);

    QVERIFY(fetchJob->items().isEmpty());

    auto *deleteJob = new CollectionDeleteJob(testCollection, Session::defaultSession());
    AKVERIFYEXEC(deleteJob);
}
