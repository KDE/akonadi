/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QSqlQuery>
#include <QTest>

#include "storage/dbdeadlockcatcher.h"

#include "aktest.h"

using namespace Akonadi::Server;

class DbDeadlockCatcherTest : public QObject
{
    Q_OBJECT

private:
    int m_myFuncCalled = 0;
    void myFunc(int maxRecursion)
    {
        ++m_myFuncCalled;
        if (m_myFuncCalled <= maxRecursion) {
            throw DbDeadlockException(QSqlQuery());
        }
    }

private Q_SLOTS:
    void testRecurseOnce()
    {
        m_myFuncCalled = 0;
        DbDeadlockCatcher catcher([this]() {
            myFunc(1);
        });
        QCOMPARE(m_myFuncCalled, 2);
    }

    void testRecurseTwice()
    {
        m_myFuncCalled = 0;
        DbDeadlockCatcher catcher([this]() {
            myFunc(2);
        });
        QCOMPARE(m_myFuncCalled, 3);
    }

    void testHitRecursionLimit()
    {
        m_myFuncCalled = 0;
        QVERIFY_EXCEPTION_THROWN(DbDeadlockCatcher catcher([this]() {
                                     myFunc(10);
                                 }),
                                 DbDeadlockException);
        QCOMPARE(m_myFuncCalled, 6);
    }
};

AKTEST_MAIN(DbDeadlockCatcherTest)

#include "dbdeadlockcatchertest.moc"
