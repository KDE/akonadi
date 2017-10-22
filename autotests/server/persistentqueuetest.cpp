/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.org>

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
#include <QTemporaryFile>
#include <QTest>

#include "server/indexer/persistentqueue.h"
#include "server/indexer/indextask.h"

using namespace Akonadi::Server;


class InspectablePersistentQueue : public PersistentQueue
{
public:
    using PersistentQueue::PersistentQueue;

    bool rewrite() override
    {
        mWasRewritten = true;
        return PersistentQueue::rewrite();
    }

    quint32 offset() const
    {
        return mOffset;
    }

    bool wasRewritten()
    {
        const bool was = mWasRewritten;
        mWasRewritten = false;
        return was;
    }

private:
    bool mWasRewritten = false;
};


class PersistentQueueTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSavingAndLoading()
    {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        QVERIFY(tempFile.open());

        IndexTask task1{ 11, 1, QStringLiteral("message/rfc822"), "ABCD Payload" };
        IndexTask task2{ 12, 2, QStringLiteral("application/x-vnd-akonadi.event"), "EVENT" };
        IndexTask task3{ 13, 3, QStringLiteral("message/rfc822"), "MIME MESSAGE" };

        // Empty queue, enqueue two tasks
        {
            PersistentQueue queue;
            QVERIFY(queue.isEmpty());
            QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
            QVERIFY(queue.isEmpty());

            queue.enqueue(task1);
            QCOMPARE(queue.peekHead(), task1);
            queue.enqueue(task2);
            QCOMPARE(queue.peekTail(), task2);
        }

        // Load queue from file, should load 2 tasks. Dequeue one task.
        {
            PersistentQueue queue;
            QVERIFY(queue.isEmpty());
            QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
            QCOMPARE(queue.size(), 2);

            QCOMPARE(queue.dequeue(), task1);
            QCOMPARE(queue.peekHead(), task2);
        }

        // Load queue from file, should load 1 task. Enqueue one task.
        {
            PersistentQueue queue;
            QVERIFY(queue.isEmpty());
            QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
            QCOMPARE(queue.size(), 1);

            queue.enqueue(task3);
            QCOMPARE(queue.size(), 2);
        }

        // Load queue from file, should load 2 tasks. Dequeue one task
        {
            PersistentQueue queue;
            QVERIFY(queue.isEmpty());
            QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
            QCOMPARE(queue.size(), 2);

            QCOMPARE(queue.dequeue(), task2);
            QCOMPARE(queue.peekHead(), task3);
            QCOMPARE(queue.size(), 1);
        }

        // Load queue from file, should load 1 task, Dequeue one task.
        {
            PersistentQueue queue;
            QVERIFY(queue.isEmpty());
            QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
            QCOMPARE(queue.size(), 1);

            QCOMPARE(queue.dequeue(), task3);
        }

        // Load queue from file, should load no tasks.
        {
            PersistentQueue queue;
            QVERIFY(queue.isEmpty());
            QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
            QVERIFY(queue.isEmpty());
        }
    }

    void testFileRewriting()
    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());

        InspectablePersistentQueue queue;
        QVERIFY(queue.setPersistenceFile(tempFile.fileName()));
        QVERIFY(queue.isEmpty());
        QVERIFY(!queue.wasRewritten());

        for (int i = 0; i < 4; ++i) {
            queue.enqueue({ i, i, QStringLiteral("message/rfc822"),
                            QByteArray("MIME MESSAGE PAYLOAD ") + QByteArray::number(i) });
        }
        QCOMPARE(queue.size(), 4);

        queue.dequeue();
        QVERIFY(!queue.wasRewritten()); // should not trigger rewrite
        queue.dequeue();
        QVERIFY(!queue.wasRewritten()); // should not trigger rewrite
        queue.dequeue();
        QVERIFY(queue.wasRewritten());  // should have triggered rewrite
        queue.dequeue();
        QVERIFY(queue.wasRewritten());  // should have triggered rewrite
    }
};

QTEST_MAIN(PersistentQueueTest)

#include "persistentqueuetest.moc"
