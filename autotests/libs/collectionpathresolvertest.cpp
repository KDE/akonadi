/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionpathresolvertest.h"
#include "collectionpathresolver.h"
#include "control.h"

#include "qtest_akonadi.h"

using namespace Akonadi;

QTEST_AKONADIMAIN(CollectionPathResolverTest)

void CollectionPathResolverTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
}

void CollectionPathResolverTest::testPathResolver()
{
    CollectionPathResolver *resolver = new CollectionPathResolver(QStringLiteral("/res1/foo/bar/bla"), this);
    AKVERIFYEXEC(resolver);
    int col = resolver->collection();
    QVERIFY(col > 0);

    resolver = new CollectionPathResolver(Collection(col), this);
    AKVERIFYEXEC(resolver);
    QCOMPARE(resolver->path(), QStringLiteral("res1/foo/bar/bla"));
}

void CollectionPathResolverTest::testRoot()
{
    CollectionPathResolver *resolver = new CollectionPathResolver(CollectionPathResolver::pathDelimiter(), this);
    AKVERIFYEXEC(resolver);
    QCOMPARE(resolver->collection(), Collection::root().id());

    resolver = new CollectionPathResolver(Collection::root(), this);
    AKVERIFYEXEC(resolver);
    QVERIFY(resolver->path().isEmpty());
}

void CollectionPathResolverTest::testFailure()
{
    CollectionPathResolver *resolver = new CollectionPathResolver(QStringLiteral("/I/do not/exist"), this);
    QVERIFY(!resolver->exec());

    resolver = new CollectionPathResolver(Collection(INT_MAX), this);
    QVERIFY(!resolver->exec());
}
