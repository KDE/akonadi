/*
 * Copyright 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <qtest_akonadi.h>

#include "fakeserverdata.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "modelspy.h"

#include "tagmodel.h"
#include "tagmodel_p.h"


static const QString serverContent1 = QStringLiteral(
        "- T PLAIN                              'Tag 1'     4"
        "- - T PLAIN                            'Tag 2'     3"
        "- - - T PLAIN                          'Tag 4'     1"
        "- - T PLAIN                            'Tag 3'     2"
        "- T PLAIN                              'Tag 5'     5");


class TagModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void testInitialFetch();

    void testTagAdded_data();
    void testTagAdded();

    void testTagChanged_data();
    void testTagChanged();

    void testTagRemoved_data();
    void testTagRemoved();

    void testTagMoved_data();
    void testTagMoved();

private:
    ExpectedSignal getExpectedSignal(SignalType type, int start, int end, const QVariantList newData)
    {
        return getExpectedSignal(type, start, end, QVariant(), newData);
    }

    ExpectedSignal getExpectedSignal(SignalType type, int start, int end, const QVariant &parentData = QVariant(), const QVariantList newData = QVariantList())
    {
        ExpectedSignal expectedSignal;
        expectedSignal.signalType = type;
        expectedSignal.startRow = start;
        expectedSignal.endRow = end;
        expectedSignal.parentData = parentData;
        expectedSignal.newData = newData;
        return expectedSignal;
    }

    ExpectedSignal getExpectedSignal(SignalType type, int start, int end, const QVariant &sourceParentData, int destRow, const QVariant &destParentData, const QVariantList newData)
    {
        ExpectedSignal expectedSignal;
        expectedSignal.signalType = type;
        expectedSignal.startRow = start;
        expectedSignal.endRow = end;
        expectedSignal.sourceParentData = sourceParentData;
        expectedSignal.destRow = destRow;
        expectedSignal.parentData = destParentData;
        expectedSignal.newData = newData;
        return expectedSignal;
    }

    QPair<FakeServerData *, Akonadi::TagModel *> populateModel(const QString &serverContent)
    {
        FakeMonitor *fakeMonitor = new FakeMonitor(this);

        fakeMonitor->setSession(m_fakeSession);
        fakeMonitor->setCollectionMonitored(Collection::root());
        fakeMonitor->setTypeMonitored(Akonadi::Monitor::Tags);

        TagModel *model = new TagModel(fakeMonitor, this);

        m_modelSpy = new ModelSpy(this);
        m_modelSpy->setModel(model);

        FakeServerData *serverData = new FakeServerData(model, m_fakeSession, fakeMonitor);
        QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret(serverData, serverContent);
        serverData->setCommands(initialFetchResponse);

        // Give the model a chance to populate
        QTest::qWait(100);
        return qMakePair(serverData, model);
    }

private:
    ModelSpy *m_modelSpy;
    FakeSession *m_fakeSession;
    QByteArray m_sessionName;
};

void TagModelTest::initTestCase()
{
    m_sessionName = "TagModelTest fake session";
    m_fakeSession = new FakeSession(m_sessionName, FakeSession::EndJobsImmediately);
    m_fakeSession->setAsDefaultSession();

    qRegisterMetaType<QModelIndex>("QModelIndex");
}

void TagModelTest::testInitialFetch()
{
    FakeMonitor *fakeMonitor = new FakeMonitor(this);

    fakeMonitor->setSession(m_fakeSession);
    fakeMonitor->setCollectionMonitored(Collection::root());
    TagModel *model = new TagModel(fakeMonitor, this);

    FakeServerData *serverData = new FakeServerData(model, m_fakeSession, fakeMonitor);
    QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret(serverData, serverContent1);
    serverData->setCommands(initialFetchResponse);

    m_modelSpy = new ModelSpy(this);
    m_modelSpy->setModel(model);
    m_modelSpy->startSpying();

    QList<ExpectedSignal> expectedSignals;

    expectedSignals << getExpectedSignal(RowsAboutToBeInserted, 0, 0);
    expectedSignals << getExpectedSignal(RowsInserted, 0, 0);
    expectedSignals << getExpectedSignal(RowsAboutToBeInserted, 0, 0, QStringLiteral("Tag 1"));
    expectedSignals << getExpectedSignal(RowsInserted, 0, 0, QStringLiteral("Tag 1"));
    expectedSignals << getExpectedSignal(RowsAboutToBeInserted, 1, 1, QStringLiteral("Tag 1"));
    expectedSignals << getExpectedSignal(RowsInserted, 1, 1, QStringLiteral("Tag 1"));
    expectedSignals << getExpectedSignal(RowsAboutToBeInserted, 0, 0, QStringLiteral("Tag 2"));
    expectedSignals << getExpectedSignal(RowsInserted, 0, 0, QStringLiteral("Tag 2"));
    expectedSignals << getExpectedSignal(RowsAboutToBeInserted, 1, 1);
    expectedSignals << getExpectedSignal(RowsInserted, 1, 1);

    m_modelSpy->setExpectedSignals(expectedSignals);

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(10);

    // We get all the signals we expected.
    QTRY_VERIFY(m_modelSpy->expectedSignals().isEmpty());

    QTest::qWait(10);
    // We didn't get signals we didn't expect.
    QVERIFY(m_modelSpy->isEmpty());
}


void TagModelTest::testTagAdded_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("addedTag");
    QTest::addColumn<QString>("parentTag");

    QTest::newRow("add-tag01") << serverContent1 << "new Tag" << "Tag 1";
    QTest::newRow("add-tag02") << serverContent1 << "new Tag" << "Tag 2";
    QTest::newRow("add-tag03") << serverContent1 << "new Tag" << "Tag 3";
    QTest::newRow("add-tag04") << serverContent1 << "new Tag" << "Tag 4";
    QTest::newRow("add-tag05") << serverContent1 << "new Tag" << "Tag 5";
}

void TagModelTest::testTagAdded()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, addedTag);
    QFETCH(QString, parentTag);

    QPair<FakeServerData *, Akonadi::TagModel *> testDrivers = populateModel(serverContent);
    FakeServerData *serverData = testDrivers.first;
    Akonadi::TagModel *model = testDrivers.second;

    const QModelIndexList list = model->match(model->index(0, 0), Qt::DisplayRole, parentTag, 1, Qt::MatchRecursive);
    QVERIFY(!list.isEmpty());
    const QModelIndex parentIndex = list.first();
    const int newRow = model->rowCount(parentIndex);


    FakeTagAddedCommand *addCommand = new FakeTagAddedCommand(addedTag, parentTag, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands(QList<FakeAkonadiServerCommand *>() << addCommand);

    QList<ExpectedSignal> expectedSignals;

    expectedSignals << getExpectedSignal(RowsAboutToBeInserted, newRow, newRow, parentTag, QVariantList() << addedTag);
    expectedSignals << getExpectedSignal(RowsInserted, newRow, newRow, parentTag, QVariantList() << addedTag);

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a change to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void TagModelTest::testTagChanged_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("tagName");

    QTest::newRow("change-tag01") << serverContent1 << "Tag 1";
    QTest::newRow("change-tag02") << serverContent1 << "Tag 2";
    QTest::newRow("change-tag03") << serverContent1 << "Tag 3";
    QTest::newRow("change-tag04") << serverContent1 << "Tag 4";
    QTest::newRow("change-tag05") << serverContent1 << "Tag 5";
}

void TagModelTest::testTagChanged()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, tagName);

    const QPair<FakeServerData *, Akonadi::TagModel *> testDrivers = populateModel(serverContent);
    FakeServerData *serverData = testDrivers.first;
    Akonadi::TagModel *model = testDrivers.second;

    const QModelIndexList list = model->match(model->index(0, 0), Qt::DisplayRole, tagName, 1, Qt::MatchRecursive);
    QVERIFY(!list.isEmpty());
    const QModelIndex changedIndex = list.first();
    const QString parentTag = changedIndex.parent().data().toString();
    const int changedRow = changedIndex.row();

    FakeTagChangedCommand *changeCommand = new FakeTagChangedCommand(tagName, parentTag, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands(QList<FakeAkonadiServerCommand *>() << changeCommand);

    QList<ExpectedSignal> expectedSignals;

    expectedSignals << getExpectedSignal(DataChanged, changedRow, changedRow, parentTag, QVariantList() << tagName);

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a change to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void TagModelTest::testTagRemoved_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("removedTag");

    QTest::newRow("remove-tag01") << serverContent1 << "Tag 1";
    QTest::newRow("remove-tag02") << serverContent1 << "Tag 2";
    QTest::newRow("remove-tag03") << serverContent1 << "Tag 3";
    QTest::newRow("remove-tag04") << serverContent1 << "Tag 4";
    QTest::newRow("remove-tag05") << serverContent1 << "Tag 5";
}


void TagModelTest::testTagRemoved()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, removedTag);

    const QPair<FakeServerData *, Akonadi::TagModel *> testDrivers = populateModel(serverContent);
    FakeServerData *serverData = testDrivers.first;
    Akonadi::TagModel *model = testDrivers.second;

    const QModelIndexList list = model->match(model->index(0, 0), Qt::DisplayRole, removedTag, 1, Qt::MatchRecursive);
    QVERIFY(!list.isEmpty());
    const QModelIndex removedIndex = list.first();
    const QString parentTag = removedIndex.parent().data().toString();
    const int sourceRow = removedIndex.row();


    FakeTagRemovedCommand *removeCommand = new FakeTagRemovedCommand(removedTag, parentTag, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands(QList<FakeAkonadiServerCommand *>() << removeCommand);

    QList<ExpectedSignal> expectedSignals;

    expectedSignals << getExpectedSignal(RowsAboutToBeRemoved, sourceRow, sourceRow,
                                         parentTag.isEmpty() ? QVariant() : parentTag,
                                         QVariantList() << removedTag);
    expectedSignals << getExpectedSignal(RowsRemoved, sourceRow, sourceRow,
                                         parentTag.isEmpty() ? QVariant() : parentTag,
                                         QVariantList() << removedTag);

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a change to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void TagModelTest::testTagMoved_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("changedTag");
    QTest::addColumn<QString>("newParent");

    QTest::newRow("move-tag01") << serverContent1 << "Tag 1" << "Tag 5";
    QTest::newRow("move-tag02") << serverContent1 << "Tag 2" << "Tag 5";
    QTest::newRow("move-tag03") << serverContent1 << "Tag 3" << "Tag 4";
    QTest::newRow("move-tag04") << serverContent1 << "Tag 4" << "Tag 1";
    QTest::newRow("move-tag05") << serverContent1 << "Tag 5" << "Tag 2";
    QTest::newRow("move-tag06") << serverContent1 << "Tag 3" << QString();
    QTest::newRow("move-tag07") << serverContent1 << "Tag 2" << QString();
}

void TagModelTest::testTagMoved()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, changedTag);
    QFETCH(QString, newParent);

    const QPair<FakeServerData *, Akonadi::TagModel *> testDrivers = populateModel(serverContent);
    FakeServerData *serverData = testDrivers.first;
    Akonadi::TagModel *model = testDrivers.second;

    QModelIndexList list = model->match(model->index(0, 0), Qt::DisplayRole, changedTag, 1, Qt::MatchRecursive);
    QVERIFY(!list.isEmpty());
    const QModelIndex changedIndex = list.first();
    const QString parentTag = changedIndex.parent().data().toString();
    const int sourceRow = changedIndex.row();

    QModelIndex newParentIndex;
    if (!newParent.isEmpty()) {
        list = model->match(model->index(0, 0), Qt::DisplayRole, newParent, 1, Qt::MatchRecursive);
        QVERIFY(!list.isEmpty());
        newParentIndex = list.first();
    }
    const int destRow = model->rowCount(newParentIndex);

    FakeTagMovedCommand *moveCommand = new FakeTagMovedCommand(changedTag, parentTag, newParent, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands(QList<FakeAkonadiServerCommand *>() << moveCommand);

    QList<ExpectedSignal> expectedSignals;
    expectedSignals << getExpectedSignal(RowsAboutToBeMoved,
                                         sourceRow, sourceRow, parentTag.isEmpty() ? QVariant() : parentTag,
                                         destRow, newParent.isEmpty() ? QVariant() : newParent,
                                         QVariantList() << changedTag);
    expectedSignals << getExpectedSignal(RowsMoved,
                                         sourceRow, sourceRow, parentTag.isEmpty() ? QVariant() : parentTag,
                                         destRow, newParent.isEmpty() ? QVariant() : newParent,
                                         QVariantList() << changedTag);

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a change to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}


#include "tagmodeltest.moc"

QTEST_MAIN(TagModelTest)
