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

#include "task.h"

#include <QObject>
#include <QTest>
#include <QTimer>

using namespace Akonadi;

static_assert(traits::is_task_v<Task<QString>>);
static_assert(!traits::is_task_v<QString>);


class TaskTest : public QObject
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

    void testAsyncTask()
    {
        Task<int> task;
        QVERIFY(!task.isFinished());
        QTimer::singleShot(std::chrono::milliseconds{100}, [task]() mutable { task.setResult(42); });
        QVERIFY(!task.isFinished());
        task.wait();
        QVERIFY(task.isFinished());
        QCOMPARE(task.result(), 42);
    }

    void testSyncThenContinuation()
    {
        Task<int> task;
        Task<QString> result = task.then([](int val) {
                Task<QString> inner;
                inner.setResult(QString::number(val));
                return inner;
            });
        QVERIFY(!task.isFinished());
        QVERIFY(!result.isFinished());
        task.setResult(42);
        QVERIFY(task.isFinished());
        QVERIFY(result.isFinished());
        QCOMPARE(result.result(), QStringLiteral("42"));
    }

    void testAsyncThenContinuation()
    {
        Task<int> task;
        Task<QString> result = task.then([](int val) {
                Task<QString> inner;
                QTimer::singleShot(std::chrono::milliseconds{100}, [inner, val]() mutable {
                    inner.setResult(QString::number(val));
                });
                return inner;
            });
        QVERIFY(!task.isFinished());
        QVERIFY(!result.isFinished());
        task.setResult(42);
        result.wait();
        QVERIFY(task.isFinished());
        QVERIFY(result.isFinished());
        QCOMPARE(result.result(), QStringLiteral("42"));
    }

    void testVoidTask()
    {
        Task<void> task;
        QVERIFY(!task.isFinished());
        task.setResult();
        QVERIFY(task.isFinished());
    }

    void testAsyncVoidTask()
    {
        Task<void> task;
        QTimer::singleShot(std::chrono::milliseconds{100}, [task]() mutable {
            task.setResult();
        });
        QVERIFY(!task.isFinished());
        task.wait();
        QVERIFY(task.isFinished());
    }

    void testError()
    {
        Task<int> task;
        QVERIFY(!task.isFinished());
        QVERIFY(!task.hasError());
        task.setError(1);
        QVERIFY(task.isFinished());
        QVERIFY(task.hasError());
        QCOMPARE(task.error().code(), 1);
    }

    void testErrorNotPropagatingThroughThen()
    {
        bool thenCalled = false;
        bool errorCalled = false;

        Task<int> task;
        task.then(
            [&thenCalled](int val) mutable {
                thenCalled = true;
                Q_UNUSED(val);
            },
            [&errorCalled](const Error &error) mutable {
                errorCalled = true;
                Q_UNUSED(error);
            });
        QVERIFY(!task.isFinished());
        task.setError(42);
        QVERIFY(task.isFinished());
        QVERIFY(task.hasError());
        QVERIFY(!thenCalled);
        QVERIFY(errorCalled);
        QCOMPARE(task.error().code(), 42);
    }

    void testScope()
    {
        const auto func = []() {
            Task<int> task;
            QTimer::singleShot(std::chrono::milliseconds{100}, [task]() mutable {
                task.setResult(42);
            });
            return task;
        };

        QEventLoop loop;
        bool allDone = false;

        {
            func().then([&loop, &allDone](int val) mutable {
                QCOMPARE(val, 42);
                allDone = true;
                loop.quit();
            });
        }

        loop.exec();
        QVERIFY(allDone);
    }

    void testForEach()
    {
        QVector<int> items = {1, 2, 3, 4, 5};

        auto task = taskForEach(items, [](int item) {
            Task<int> t;
            QTimer::singleShot(std::chrono::milliseconds{10}, [t, item]() mutable {
                t.setResult(item);
            });
            return t;
        });
        QVERIFY(!task.isFinished());
        task.wait();
        QVERIFY(task.isFinished());
    }

    void testForEachError()
    {
        QVector<int> items = {1, 2, 3, 4, 5};

        auto task = taskForEach(items, [](int item) {
            Task<int> t;
            QTimer::singleShot(std::chrono::milliseconds{10}, [t, item]() mutable {
                if (item == 3) {
                    t.setError(1, QStringLiteral("Oppsie"));
                } else {
                    t.setResult(item);
                }
            });
            return t;
        });
        QVERIFY(!task.isFinished());
        task.wait();
        QVERIFY(task.hasError());
        QCOMPARE(task.error().code(), 1);
    }

    void testCollectionTasks()
    {
        QVector<int> items{1, 2, 3, 4, 5};

        auto task = collectTasks(items, [](int item) {
            Task<int> t;
            QTimer::singleShot(std::chrono::milliseconds{10}, [t, item]() mutable {
                t.setResult(item);
            });
            return t;
        });
        QVERIFY(!task.isFinished());
        task.wait();
        QVERIFY(task.isFinished());
        QVERIFY(!task.hasError());
        QCOMPARE(task.result(), items);
    }

    void testCollectionTasksWithCollection()
    {
        QVector<int> items{1, 2, 3, 4, 5};

        auto task = collectTasks(items, [](int item) {
            Task<QVector<int>> t;
            QTimer::singleShot(std::chrono::milliseconds{10}, [t, item]() mutable {
                t.setResult({item, item, item});
            });
            return t;
        });
        QVERIFY(!task.isFinished());
        task.wait();
        QVERIFY(task.isFinished());
        QVERIFY(!task.hasError());
        QCOMPARE(task.result(), (QVector<int>{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5}));
    }
};

QTEST_GUILESS_MAIN(TaskTest)

#include "tasktest.moc"
