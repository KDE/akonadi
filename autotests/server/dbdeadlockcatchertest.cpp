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

#include <QObject>
#include <QTest>
#include <QSqlQuery>

#include "storage/dbdeadlockcatcher.h"

#include <aktest.h>

using namespace Akonadi::Server;

class DbDeadlockCatcherTest : public QObject
{
    Q_OBJECT

private:
    int m_myFuncCalled = 0;
    void myFunc(int maxRecursion)
    {
        ++m_myFuncCalled;
        if (m_myFuncCalled <= maxRecursion)
            throw DbDeadlockException(QSqlQuery());
    }

private Q_SLOTS:
    void testRecurseOnce()
    {
        m_myFuncCalled = 0;
        DbDeadlockCatcher catcher([this](){ myFunc(1); });
        QCOMPARE(m_myFuncCalled, 2);
    }

    void testRecurseTwice()
    {
        m_myFuncCalled = 0;
        DbDeadlockCatcher catcher([this](){ myFunc(2); });
        QCOMPARE(m_myFuncCalled, 3);
    }

    void testHitRecursionLimit()
    {
        m_myFuncCalled = 0;
        QVERIFY_EXCEPTION_THROWN(
                    DbDeadlockCatcher catcher([this](){ myFunc(10); }),
                    DbDeadlockException);
        QCOMPARE(m_myFuncCalled, 6);
    }
};

AKTEST_MAIN(DbDeadlockCatcherTest)

#include "dbdeadlockcatchertest.moc"
