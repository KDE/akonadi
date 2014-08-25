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

#include <QtCore/QObject>
#include <QtCore/QSettings>

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

        settings = new QSettings(QLatin1String("kde.org"), QLatin1String("akonadi-changerecordertest"), this);
    }

    // After each test
    void cleanup()
    {
        // See ChangeRecorderPrivate::notificationsFileName()
        QFile::remove(settings->fileName() + QLatin1String("_changes.dat"));
    }

    void testChangeRecorder_data()
    {
        QTest::addColumn<QStringList>("actions");

        QTest::newRow("nothingToReplay") << (QStringList() << QLatin1String("rn"));
        QTest::newRow("nothingOneNothing") << (QStringList() << QLatin1String("rn") << QLatin1String("c2") << QLatin1String("r2") << QLatin1String("rn"));
        QTest::newRow("multipleItems") << (QStringList() << QLatin1String("c1") << QLatin1String("c2") << QLatin1String("c3") << QLatin1String("r1") << QLatin1String("c4") << QLatin1String("r2") << QLatin1String("r3") << QLatin1String("r4") << QLatin1String("rn"));
        QTest::newRow("reload") << (QStringList() << QLatin1String("c1") << QLatin1String("c1") << QLatin1String("c3") << QLatin1String("reload") << QLatin1String("r1") << QLatin1String("r3") << QLatin1String("rn"));
        QTest::newRow("more") << (QStringList() << QLatin1String("c1") << QLatin1String("c2") << QLatin1String("c3") << QLatin1String("reload") << QLatin1String("r1") << QLatin1String("reload") << QLatin1String("c4") << QLatin1String("reload") << QLatin1String("r2") << QLatin1String("reload") << QLatin1String("r3") << QLatin1String("r4") << QLatin1String("rn"));
        QTest::newRow("modifyThenDelete") << (QStringList() << QLatin1String("c1") << QLatin1String("d1") << QLatin1String("r1") << QLatin1String("rn"));
    }

    void testChangeRecorder()
    {
        QFETCH(QStringList, actions);
        QString lastAction;

        ChangeRecorder *rec = createChangeRecorder();
        QVERIFY(rec->isEmpty());
        Q_FOREACH (const QString &action, actions) {
            qDebug() << action;
            if (action == QLatin1String("rn")) {
                replayNextAndExpectNothing(rec);
            } else if (action == QLatin1String("reload")) {
                // Check saving and loading from disk
                delete rec;
                rec = createChangeRecorder();
            } else if (action.at(0) == QLatin1Char('c')) {
                // c1 = "trigger change on item 1"
                const int id = action.mid(1).toInt();
                Q_ASSERT(id);
                triggerChange(id);
                if (action != lastAction) {
                    // enter event loop and wait for change notifications from the server
                    QVERIFY(QTest::kWaitForSignal(rec, SIGNAL(changesAdded()), 1000));
                }
            } else if (action.at(0) == QLatin1Char('d')) {
                // d1 = "delete item 1"
                const int id = action.mid(1).toInt();
                Q_ASSERT(id);
                triggerDelete(id);
                QTest::qWait(500);
            } else if (action.at(0) == QLatin1Char('r')) {
                // r1 = "replayNext and expect to get itemChanged(1)"
                const int id = action.mid(1).toInt();
                Q_ASSERT(id);
                replayNextAndProcess(rec, id);
            } else {
                QVERIFY2(false, qPrintable(QLatin1String("Unsupported: ") + action));
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
        qDebug();

        QSignalSpy nothingSpy(rec, SIGNAL(nothingToReplay()));
        QVERIFY(nothingSpy.isValid());
        QSignalSpy itemChangedSpy(rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)));
        QVERIFY(itemChangedSpy.isValid());

        rec->replayNext();
        if (itemChangedSpy.isEmpty()) {
            QVERIFY(QTest::kWaitForSignal(rec, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), 1000));
        }
        QCOMPARE(itemChangedSpy.count(), 1);
        QCOMPARE(itemChangedSpy.at(0).at(0).value<Akonadi::Item>().id(), expectedUid);

        rec->changeProcessed();

        QCOMPARE(nothingSpy.count(), 0);
    }

    void replayNextAndExpectNothing(ChangeRecorder *rec)
    {
        qDebug();

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

    QSettings *settings;

};

QTEST_AKONADIMAIN(ChangeRecorderTest)

#include "changerecordertest.moc"
