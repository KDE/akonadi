/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changerecorder.h"
#include "agentmanager.h"
#include "itemdeletejob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "testattribute.h"

#include "qtest_akonadi.h"

#include <QObject>
#include <QSettings>

using namespace Akonadi;

Q_DECLARE_METATYPE(QSet<QByteArray>)

class ChangeRecorderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qRegisterMetaType<Akonadi::Item>();
        qRegisterMetaType<QSet<QByteArray>>();
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
        QTest::newRow("multipleItems") << (QStringList() << QStringLiteral("c1") << QStringLiteral("c2") << QStringLiteral("c3") << QStringLiteral("r1")
                                                         << QStringLiteral("c4") << QStringLiteral("r2") << QStringLiteral("r3") << QStringLiteral("r4")
                                                         << QStringLiteral("rn"));
        QTest::newRow("reload") << (QStringList() << QStringLiteral("c1") << QStringLiteral("c1") << QStringLiteral("c3") << QStringLiteral("reload")
                                                  << QStringLiteral("r1") << QStringLiteral("r1") << QStringLiteral("r3") << QStringLiteral("rn"));
        QTest::newRow("more") << (QStringList() << QStringLiteral("c1") << QStringLiteral("c2") << QStringLiteral("c3") << QStringLiteral("reload")
                                                << QStringLiteral("r1") << QStringLiteral("reload") << QStringLiteral("c4") << QStringLiteral("reload")
                                                << QStringLiteral("r2") << QStringLiteral("reload") << QStringLiteral("r3") << QStringLiteral("r4")
                                                << QStringLiteral("rn"));
        // FIXME: Due to the event compression in the server we simply expect a removal signal
        // QTest::newRow("modifyThenDelete") << (QStringList() << "c1" << "d1" << "r1" << "rn");
    }

    void testChangeRecorder()
    {
        QFETCH(QStringList, actions);
        QString lastAction;

        auto rec = createChangeRecorder();
        QVERIFY(rec->isEmpty());
        for (const QString &action : qAsConst(actions)) {
            qDebug() << action;
            if (action == QLatin1String("rn")) {
                replayNextAndExpectNothing(rec.get());
            } else if (action == QLatin1String("reload")) {
                // Check saving and loading from disk
                rec = createChangeRecorder();
            } else if (action.at(0) == QLatin1Char('c')) {
                // c1 = "trigger change on item 1"
                const int id = action.midRef(1).toInt();
                Q_ASSERT(id);
                triggerChange(id);
                if (action != lastAction) {
                    // enter event loop and wait for change notifications from the server
                    QVERIFY(AkonadiTest::akWaitForSignal(rec.get(), &ChangeRecorder::changesAdded, 1000));
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
                replayNextAndProcess(rec.get(), id);
            } else {
                QVERIFY2(false, qPrintable(QStringLiteral("Unsupported: ") + action));
            }
            lastAction = action;
        }
        QVERIFY(rec->isEmpty());
    }

private:
    void triggerChange(Akonadi::Item::Id uid)
    {
        static int s_num = 0;
        Item item(uid);
        auto attr = item.attribute<TestAttribute>(Item::AddIfMissing);
        attr->data = QByteArray::number(++s_num);
        auto job = new ItemModifyJob(item);
        job->disableRevisionCheck();
        AKVERIFYEXEC(job);
    }

    void triggerDelete(Akonadi::Item::Id uid)
    {
        Item item(uid);
        auto job = new ItemDeleteJob(item);
        AKVERIFYEXEC(job);
    }

    void replayNextAndProcess(ChangeRecorder *rec, Akonadi::Item::Id expectedUid)
    {
        QSignalSpy nothingSpy(rec, &ChangeRecorder::nothingToReplay);
        QVERIFY(nothingSpy.isValid());
        QSignalSpy itemChangedSpy(rec, &Monitor::itemChanged);
        QVERIFY(itemChangedSpy.isValid());

        rec->replayNext();
        if (itemChangedSpy.isEmpty()) {
            QVERIFY(AkonadiTest::akWaitForSignal(rec, &Monitor::itemChanged, 1000));
        }
        QCOMPARE(itemChangedSpy.count(), 1);
        QCOMPARE(itemChangedSpy.at(0).at(0).value<Akonadi::Item>().id(), expectedUid);

        rec->changeProcessed();

        QCOMPARE(nothingSpy.count(), 0);
    }

    void replayNextAndExpectNothing(ChangeRecorder *rec)
    {
        QSignalSpy nothingSpy(rec, &ChangeRecorder::nothingToReplay);
        QVERIFY(nothingSpy.isValid());
        QSignalSpy itemChangedSpy(rec, &Monitor::itemChanged);
        QVERIFY(itemChangedSpy.isValid());

        rec->replayNext(); // emits nothingToReplay immediately

        QCOMPARE(itemChangedSpy.count(), 0);
        QCOMPARE(nothingSpy.count(), 1);
    }

    std::unique_ptr<ChangeRecorder> createChangeRecorder() const
    {
        auto rec = std::make_unique<ChangeRecorder>();
        rec->setConfig(settings);
        rec->setAllMonitored();
        rec->itemFetchScope().fetchFullPayload();
        rec->itemFetchScope().fetchAllAttributes();
        rec->itemFetchScope().setCacheOnly(true);

        // Ensure we listen to a signal, otherwise MonitorPrivate::isLazilyIgnored will ignore notifications
        auto spy = new QSignalSpy(rec.get(), &Monitor::itemChanged);
        spy->setParent(rec.get());

        QSignalSpy readySpy(rec.get(), &Monitor::monitorReady);
        if (!readySpy.wait()) {
            QTest::qFail("Failed to wait for Monitor", __FILE__, __LINE__);
            return nullptr;
        }

        return rec;
    }

    QSettings *settings = nullptr;
};

QTEST_AKONADIMAIN(ChangeRecorderTest)

#include "changerecordertest.moc"
