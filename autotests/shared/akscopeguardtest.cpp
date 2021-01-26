/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include "shared/akscopeguard.h"

using namespace Akonadi;

class AkScopeGuardTest : public QObject
{
    Q_OBJECT

private:
    static void staticMethod()
    {
        Q_ASSERT(!mCalled);
        mCalled = true;
    }

    void regularMethod()
    {
        Q_ASSERT(!mCalled);
        mCalled = true;
    }

private Q_SLOTS:

    void testLambda()
    {
        mCalled = false;
        {
            AkScopeGuard guard([&]() {
                Q_ASSERT(!mCalled);
                mCalled = true;
            });
        }
        QVERIFY(mCalled);
    }

    void testStaticMethod()
    {
        mCalled = false;
        {
            AkScopeGuard guard(&AkScopeGuardTest::staticMethod);
        }
        QVERIFY(mCalled);
    }

    void testBindExpr()
    {
        mCalled = false;
        {
            AkScopeGuard guard(std::bind(&AkScopeGuardTest::regularMethod, this));
        }
        QVERIFY(mCalled);
    }

    void testStdFunction()
    {
        mCalled = false;
        std::function<void()> func = [&]() {
            Q_ASSERT(!mCalled);
            mCalled = true;
        };
        {
            AkScopeGuard guard(func);
        }
        QVERIFY(mCalled);
    }

private:
    static bool mCalled;
};

bool AkScopeGuardTest::mCalled = false;

QTEST_GUILESS_MAIN(AkScopeGuardTest)

#include "akscopeguardtest.moc"
