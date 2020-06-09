/*
    Copyright (c) 2020  Daniel Vrátil <dvratil@kde.org>

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

#include "async.h"

#include <QObject>
#include <QTest>
#include <QTimer>

using namespace Akonadi::Async;

class AsyncTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSimpleTask()
    {
        Task<int> task;
        QVERIFY(!task.isFinished());
        task.setResult(42);
        QVERIFY(task.isFinished());
        QCOMPARE(task.result(), 42);
    }

    void testSimpleTaskExec()
    {
        Task<int> task;
        QTimer::singleShot(std::chrono::milliseconds{100}, [&task]() mutable {
            task.setResult(42);
        });
        task.wait();
        QVERIFY(task.isFinished());
        QCOMPARE(task.result(), 42);
    }

    void testSimpleThen()
    {
        Task<int> task;
        auto res = task.then([](int val) { return QString::number(val); });
        QVERIFY(!task.isFinished());
        QVERIFY(!res.isFinished());
        task.setResult(42);
        QVERIFY(task.isFinished());
        QVERIFY(res.isFinished());
        QCOMPARE(res.result(), QStringLiteral("42"));
    }

    void testTemporarySyncTask()
    {
        const auto func = []() {
            Task<int> task;
            task.setResult(42);
            return task;
        };

        const auto res = func().then([](int val) { return QString::number(val); });
        QVERIFY(res.isFinished());
        QCOMPARE(res.result(), QStringLiteral("42"));
    }

    void testTemporaryAsyncTask()
    {
        const auto func = []() {
            Task<int> task;
            QTimer::singleShot(std::chrono::milliseconds{100}, [task]() mutable {
                task.setResult(42);
            });
            return task;
        };

        const auto res = func().then([](int val) { return QString::number(val);} );
        QVERIFY(!res.isFinished());
        res.wait();
        QVERIFY(res.isFinished());
        QCOMPARE(res.result(), QStringLiteral("42"));
    }

    void testSyncContinuationChain()
    {
        const auto f1 = [](int val) { return QString::number(val); };
        const auto f2 = [](const QString &val) { return val.toInt() * 2; };

        Task<int> t;
        auto task = t.then(f1).then(f2);
        QVERIFY(!t.isFinished());
        QVERIFY(!task.isFinished());
        t.setResult(42);
        QVERIFY(t.isFinished());
        QVERIFY(task.isFinished());
        QCOMPARE(task.result(), 84);
    }

    void testAsyncContinuationChain()
    {
        const auto f1 = [](int val) {
            Task<QString> task;
            QTimer::singleShot(std::chrono::milliseconds{100}, [val, task]() mutable { task.setResult(QString::number(val)); });
            return task;
        };
        const auto f2 = [](const QString &val) {
            Task<int> task;
            QTimer::singleShot(std::chrono::milliseconds{100}, [val, task]() mutable { task.setResult(val.toInt() * 2); });
            return task;
        };

        Task<int> task;
        Task<QString> = task.then(f1);
        Task<int> result = task.then(f1).then(f2);
        QTimer::singleShot(std::chrono::milliseconds{100}, [task]() mutable { task.setResult(42); });
        QVERIFY(!task.isFinished());
        QVERIFY(!result.isFinished());
        task.wait();
        QVERIFY(task.isFinished());
        QVERIFY(!result.isFinished());
        result.wait();
        QVERIFY(result.isFinished());
        QCOMPARE(result.result(), 84);
    }
};

QTEST_GUILESS_MAIN(AsyncTest)

#include "asynctest.moc"

