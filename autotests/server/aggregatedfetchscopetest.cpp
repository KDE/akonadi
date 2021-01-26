/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "aggregatedfetchscope.h"
#include <shared/aktest.h>

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class AggregatedFetchScopeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testTagApply()
    {
        AggregatedTagFetchScope scope;

        // first subscriber, A
        scope.addSubscriber();
        Protocol::TagFetchScope oldTagScope;
        Protocol::TagFetchScope tagScopeA;
        QSet<QByteArray> attrs = {"FOO"};
        tagScopeA.setAttributes(attrs);
        tagScopeA.setFetchIdOnly(true);
        tagScopeA.setFetchAllAttributes(false);
        scope.apply(oldTagScope, tagScopeA);
        QCOMPARE(scope.attributes(), attrs);
        QVERIFY(scope.fetchIdOnly());
        QVERIFY(!scope.fetchAllAttributes());

        // second subscriber, B
        Protocol::TagFetchScope tagScopeB = tagScopeA;
        tagScopeB.setFetchIdOnly(false);
        tagScopeB.setFetchAllAttributes(true);
        scope.addSubscriber();
        scope.apply(oldTagScope, tagScopeB);
        QCOMPARE(scope.attributes(), attrs);
        QVERIFY(!scope.fetchIdOnly());
        QVERIFY(scope.fetchAllAttributes());

        // then B goes away
        scope.apply(tagScopeB, oldTagScope);
        scope.removeSubscriber();
        QCOMPARE(scope.attributes(), attrs);
        QVERIFY(scope.fetchIdOnly());
        QVERIFY(!scope.fetchAllAttributes());

        // A goes away
        scope.apply(tagScopeA, oldTagScope);
        scope.removeSubscriber();
        QCOMPARE(scope.attributes(), QSet<QByteArray>());
    }

    void testCollectionApply()
    {
        AggregatedCollectionFetchScope scope;

        // first subscriber, A
        scope.addSubscriber();
        Protocol::CollectionFetchScope oldCollectionScope;
        Protocol::CollectionFetchScope collectionScopeA;
        QSet<QByteArray> attrs = {"FOO"};
        collectionScopeA.setAttributes(attrs);
        collectionScopeA.setFetchIdOnly(true);
        scope.apply(oldCollectionScope, collectionScopeA);
        QCOMPARE(scope.attributes(), attrs);
        QVERIFY(scope.fetchIdOnly());

        // second subscriber, B
        Protocol::CollectionFetchScope collectionScopeB = collectionScopeA;
        collectionScopeB.setFetchIdOnly(false);
        scope.addSubscriber();
        scope.apply(oldCollectionScope, collectionScopeB);
        QCOMPARE(scope.attributes(), attrs);
        QVERIFY(!scope.fetchIdOnly());

        // then B goes away
        scope.apply(collectionScopeB, oldCollectionScope);
        scope.removeSubscriber();
        QCOMPARE(scope.attributes(), attrs);
        QVERIFY(scope.fetchIdOnly());

        // A goes away
        scope.apply(collectionScopeA, oldCollectionScope);
        scope.removeSubscriber();
        QCOMPARE(scope.attributes(), QSet<QByteArray>());
    }

    void testItemApply()
    {
        AggregatedItemFetchScope scope;
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::NoAncestor);

        // first subscriber, A
        scope.addSubscriber();
        Protocol::ItemFetchScope oldItemScope;
        Protocol::ItemFetchScope itemScopeA;
        QVector<QByteArray> parts = {"FOO"};
        QSet<QByteArray> partsSet = {"FOO"};
        itemScopeA.setRequestedParts(parts);
        itemScopeA.setAncestorDepth(Protocol::ItemFetchScope::ParentAncestor);
        itemScopeA.setFetch(Protocol::ItemFetchScope::CacheOnly);
        itemScopeA.setFetch(Protocol::ItemFetchScope::IgnoreErrors);
        scope.apply(oldItemScope, itemScopeA);
        QCOMPARE(scope.requestedParts(), partsSet);
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::ParentAncestor);
        QVERIFY(scope.cacheOnly());
        QVERIFY(scope.ignoreErrors());

        // second subscriber, B
        Protocol::ItemFetchScope itemScopeB = itemScopeA;
        itemScopeB.setAncestorDepth(Protocol::ItemFetchScope::AllAncestors);
        scope.addSubscriber();
        QVERIFY(!scope.cacheOnly()); // they don't agree so: false
        QVERIFY(!scope.ignoreErrors());
        scope.apply(oldItemScope, itemScopeB);
        QCOMPARE(scope.requestedParts(), partsSet);
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::AllAncestors);

        // subscriber C with ParentAncestor - but that won't make change it
        Protocol::ItemFetchScope itemScopeC = itemScopeA;
        scope.addSubscriber();
        scope.apply(oldItemScope, itemScopeC);
        QCOMPARE(scope.requestedParts(), partsSet);
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::AllAncestors); // no change

        // then C goes away
        scope.apply(itemScopeC, oldItemScope);
        scope.removeSubscriber();
        QCOMPARE(scope.requestedParts(), partsSet);
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::AllAncestors);

        // then B goes away
        scope.apply(itemScopeB, oldItemScope);
        scope.removeSubscriber();
        QCOMPARE(scope.requestedParts(), partsSet);
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::ParentAncestor);

        // A goes away
        scope.apply(itemScopeA, oldItemScope);
        scope.removeSubscriber();
        QCOMPARE(scope.requestedParts(), QSet<QByteArray>());
        QCOMPARE(scope.ancestorDepth(), Protocol::ItemFetchScope::NoAncestor);
    }
};

QTEST_MAIN(AggregatedFetchScopeTest)

#include "aggregatedfetchscopetest.moc"
