/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "testattribute.h"

#include <changerecorder.h>
#include <itemfetchscope.h>
#include <itemmodifyjob.h>
#include <itemdeletejob.h>
#include <agentmanager.h>

#include <QObject>
#include <QSettings>

#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE(QSet<QByteArray>)

class ChangeRecorderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qRegisterMetaType<Akonadi::Item>();
        qRegisterMetaType<QSet<QByteArray> >();
        AkonadiTest::checkTestIsIsolated();
        AkonadiTest::setAllResourcesOffline();

        settings = new QSettings(QStringLiteral("kde.org"), QStringLiteral("akonadi-changerecordertest"), this);
    }

    // After each test
    void cleanup()
    {
        // See ChangeRecorderPrivate::notificationsFileName()
        QFile::remove(settings->fileName() + QStringLiteral("_changes.dat"));
    }

    void testChangeRecorder_data()
    {
        QTest::addColumn<QStringList>("actions");

        QTest::newRow("nothingToReplay") << (QStringList() << QStringLiteral("rn"));
        QTest::newRow("nothingOneNothing") << (QStringList() << QStringLiteral("rn") << QStringLiteral("c2") << QStringLiteral("r2") << QStringLiteral("rn"));
        QTest::newRow("multipleItems") << (QStringList() << QStringLiteral("c1") << QStringLiteral("c2") << QStringLiteral("c3") << QStringLiteral("r1") << QStringLiteral("c4") << QStringLiteral("r2") << QStringLiteral("r3") << QStringLiteral("r4") << QStringLiteral("rn"));
        QTest::newRow("reload") << (QStringList() << QStringLiteral("c1") << QStringLiteral("c1") << QStringLiteral("c3") << QStringLiteral("reload") << QStringLiteral("r1") << QStringLiteral("r1") << QStringLiteral("r3") << QStringLiteral("rn"));
        QTest::newRow("more") << (QStringList() << QStringLiteral("c1") << QStringLiteral("c2") << QStringLiteral("c3") << QStringLiteral("reload") << QStringLiteral("r1") << QStringLiteral("reload") << QStringLiteral("c4") << QStringLiteral("reload") << QStringLiteral("r2") << QStringLiteral("reload") << QStringLiteral("r3") << QStringLiteral("r4") << QStringLiteral("rn"));
        //FIXME: Due to the event compression in the server we simply expect a removal signal
        // QTest::newRow("modifyThenDelete") << (QStringList() << "c1" << "d1" << "r1" << "rn");
    }

    void testChangeRecorder()
    {
        QFETCH(QStringList, actions);
        QString lastAction;

        ChangeRecorder *rec = createChangeRecorder();
        AkonadiTest::akWaitForSignal(rec, SIGNAL(monitorReady()), 1000);
        QVERIFY(rec->isEmpty());
        for (const QString &action : qAsConst(actions)) {
            qDebug() << action;
            if (action == QStringLiteral("rn")) {
                replayNextAndExpectNothing(rec);
            } else if (action == QStringLiteral("reload")) {
                // Check saving and loading from disk
                delete rec;
                rec = createChangeRecorder();
                AkonadiTest::akWaitForSignal(rec, SIGNAL(monitorReady()), 1000);
            } else if (action.at(0) == QLatin1Char('c')) {
                // c1 = "trigger change on item 1"
                const int id = action.midRef(1).toInt();
                Q_ASSERT(id);
                triggerChange(id);
                if (action != lastAction) {
                    // enter event loop and wait for change notifications from the server
                    QVERIFY(AkonadiTest::akWaitForSignal(rec, SIGNAL(changesAdded()), 1000));
                }
            } else if (action.at(0) == QLatin1Char('d')) {
                // d1 = "delete item 1"
                const int id = action.midRef(1).toInt();
                Q_ASSERT(id);
                triggerDelete(id);
                QTest::qWait(500);
            } else if (action.at(0) == QLatin1Char('r')) {
                // r1 = "replayNext and expect to get itemChanged(1)"
                const int id = action.midRef(1).toInt();
                Q_ASSERT(id);
                replayNextAndProcess(rec, id);
            } else {
                QVERIFY2(false, qPrintable(QStringLiteral("Unsupported: ") + action));
            }
            lastAction = action;
        }
        QVERIFY(rec->isEmpty());
        delete rec;
    }

private:
    void triggerChange(Akonadi::Item::Id uid)
    {
        static int s_num = 0;
        Item item(uid);
        TestAttribute *attr = item.attribute<TestAttribute>(Item::AddIfMissing);
        attr->data = QByteArray::number(++s_num);
        ItemModifyJob *job = new ItemModifyJob(item);
        job->disableRevisionCheck();
        AKVERIFYEXEC(job);
    }

    void triggerDelete(Akonadi::Item::Id uid)
    {
        Item item(uid);
        ItemDeleteJob *job = new ItemDeleteJob(item);
        AKVERIFYEXEC(job);
    }

    void replayNextAndProcess(ChangeRecorder *rec, Akonadi::Item::Id expectedUid)
    {
        QSignalSpy nothingSpy(rec, SIGNAL(nothingToReplay()));
        QVERIFY(nothingSpy.isValid());
        QSignalSpy itemChangedSpy(rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
        QVERIFY(itemChangedSpy.isValid());

        rec->replayNext();
        if (itemChangedSpy.isEmpty()) {
            QVERIFY(AkonadiTest::akWaitForSignal(rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), 1000));
        }
        QCOMPARE(itemChangedSpy.count(), 1);
        QCOMPARE(itemChangedSpy.at(0).at(0).value<Akonadi::Item>().id(), expectedUid);

        rec->changeProcessed();

        QCOMPARE(nothingSpy.count(), 0);
    }

    void replayNextAndExpectNothing(ChangeRecorder *rec)
    {
        QSignalSpy nothingSpy(rec, SIGNAL(nothingToReplay()));
        QVERIFY(nothingSpy.isValid());
        QSignalSpy itemChangedSpy(rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
        QVERIFY(itemChangedSpy.isValid());

        rec->replayNext(); // emits nothingToReplay immediately

        QCOMPARE(itemChangedSpy.count(), 0);
        QCOMPARE(nothingSpy.count(), 1);
    }

    ChangeRecorder *createChangeRecorder() const
    {
        ChangeRecorder *rec = new ChangeRecorder();
        rec->setConfig(settings);
        rec->setAllMonitored();
        rec->itemFetchScope().fetchFullPayload();
        rec->itemFetchScope().fetchAllAttributes();
        rec->itemFetchScope().setCacheOnly(true);

        // Ensure we listen to a signal, otherwise MonitorPrivate::isLazilyIgnored will ignore notifications
        QSignalSpy *spy = new QSignalSpy(rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
        spy->setParent(rec);

        return rec;
    }

    QSettings *settings = nullptr;

};

QTEST_AKONADIMAIN(ChangeRecorderTest)

#include "changerecordertest.moc"
