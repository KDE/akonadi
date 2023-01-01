/*
    SPDX-FileCopyrightText: 2017-2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cachepolicytest.h"
#include "cachepolicy.h"
#include <QTest>

CachePolicyTest::CachePolicyTest(QObject *parent)
    : QObject(parent)
{
}

CachePolicyTest::~CachePolicyTest()
{
}

void CachePolicyTest::shouldHaveDefaultValue()
{
    Akonadi::CachePolicy c;
    QVERIFY(c.inheritFromParent());
    QCOMPARE(c.intervalCheckTime(), -1);
    QCOMPARE(c.cacheTimeout(), -1);
    QVERIFY(!c.syncOnDemand());
    QVERIFY(c.localParts().isEmpty());
}

QTEST_MAIN(CachePolicyTest)
