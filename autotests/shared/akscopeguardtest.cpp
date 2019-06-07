/*
    Copyright (C) 2019  Daniel Vrátil <dvratil@kde.org>

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

#include <QTest>
#include <QObject>

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

