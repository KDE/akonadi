/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searchjobtest.h"
#include "qtest_akonadi.h"

#include "collection.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionutils.h"
#include "itemfetchjob.h"
#include "persistentsearchattribute.h"
#include "searchcreatejob.h"
#include "searchquery.h"

QTEST_AKONADIMAIN(SearchJobTest)

using namespace Akonadi;

void SearchJobTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
}

void SearchJobTest::testCreateDeleteSearch()
{
    Akonadi::SearchQuery query;
    query.addTerm(Akonadi::SearchTerm(QStringLiteral("plugin"), 1));
    query.addTerm(Akonadi::SearchTerm(QStringLiteral("resource"), 2));
    query.addTerm(Akonadi::SearchTerm(QStringLiteral("plugin"), 3));
    query.addTerm(Akonadi::SearchTerm(QStringLiteral("resource"), 4));

    // Create collection
    SearchCreateJob *create = new SearchCreateJob(QStringLiteral("search123456"), query, this);
    create->setRemoteSearchEnabled(false);
    AKVERIFYEXEC(create);
    const Collection created = create->createdCollection();
    QVERIFY(created.isValid());

    // Fetch "Search" collection, check the search collection has been created
    CollectionFetchJob *list = new CollectionFetchJob(Collection(1), CollectionFetchJob::Recursive, this);
    AKVERIFYEXEC(list);
    const Collection::List cols = list->collections();
    Collection col;
    for (const auto &c : cols) {
        if (c.name() == QLatin1String("search123456")) {
            col = c;
            break;
        }
    }
    QVERIFY(col == created);
    QCOMPARE(col.parentCollection().id(), 1LL);
    QVERIFY(col.isVirtual());

    // Fetch items in the search collection, check whether they are there
    ItemFetchJob *fetch = new ItemFetchJob(created, this);
    AKVERIFYEXEC(fetch);
    const Item::List items = fetch->items();
    QCOMPARE(items.count(), 2);

    CollectionDeleteJob *delJob = new CollectionDeleteJob(col, this);
    AKVERIFYEXEC(delJob);
}

void SearchJobTest::testModifySearch()
{
    Akonadi::SearchQuery query;
    query.addTerm(Akonadi::SearchTerm(QStringLiteral("plugin"), 1));
    query.addTerm(Akonadi::SearchTerm(QStringLiteral("resource"), 2));

    // make sure there is a virtual collection
    SearchCreateJob *create = new SearchCreateJob(QStringLiteral("search123456"), query, this);
    AKVERIFYEXEC(create);
    Collection created = create->createdCollection();
    QVERIFY(created.isValid());
    QVERIFY(created.hasAttribute<PersistentSearchAttribute>());

    auto attr = created.attribute<PersistentSearchAttribute>();
    QVERIFY(!attr->isRecursive());
    QVERIFY(!attr->isRemoteSearchEnabled());
    QCOMPARE(attr->queryCollections(), QVector<qint64>{0});
    const QString oldQueryString = attr->queryString();

    // Change the attributes
    attr->setRecursive(true);
    attr->setRemoteSearchEnabled(true);
    attr->setQueryCollections(QVector<qint64>{1});
    Akonadi::SearchQuery newQuery;
    newQuery.addTerm(Akonadi::SearchTerm(QStringLiteral("plugin"), 3));
    newQuery.addTerm(Akonadi::SearchTerm(QStringLiteral("resource"), 4));
    attr->setQueryString(QString::fromUtf8(newQuery.toJSON()));

    auto modify = new CollectionModifyJob(created, this);
    AKVERIFYEXEC(modify);

    auto fetch = new CollectionFetchJob(created, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(fetch);
    QCOMPARE(fetch->collections().size(), 1);

    const auto col = fetch->collections().first();
    QVERIFY(col.hasAttribute<PersistentSearchAttribute>());
    attr = col.attribute<PersistentSearchAttribute>();

    QVERIFY(attr->isRecursive());
    QVERIFY(attr->isRemoteSearchEnabled());
    QCOMPARE(attr->queryCollections(), QVector<qint64>{1});
    QVERIFY(attr->queryString() != oldQueryString);

    auto delJob = new CollectionDeleteJob(col, this);
    AKVERIFYEXEC(delJob);
}
