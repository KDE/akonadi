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
    QPair<FakeServerData *, Akonadi::TagModel *> populateModel(const QString &serverContent)
    {
        const auto fakeMonitor = new FakeMonitor(this);

        fakeMonitor->setSession(m_fakeSession);
        fakeMonitor->setCollectionMonitored(Collection::root());
        fakeMonitor->setTypeMonitored(Akonadi::Monitor::Tags);

        const auto model = new TagModel(fakeMonitor, this);

        m_modelSpy = new ModelSpy{model, this};

        const auto serverData = new FakeServerData(model, m_fakeSession, fakeMonitor);
        serverData->setCommands(FakeJobResponse::interpret(serverData, serverContent));

        // Give the model a chance to populate
        QTest::qWait(100);
        return qMakePair(serverData, model);
    }

private:
    ModelSpy *m_modelSpy = nullptr;
    FakeSession *m_fakeSession = nullptr;
    QByteArray m_sessionName;
};

QModelIndex firstMatchedIndex(const QAbstractItemModel &model, const QString pattern)
{
    if (pattern.isEmpty()) {
        return {};
    }
    const auto list = model.match(model.index(0, 0), Qt::DisplayRole, pattern, 1, Qt::MatchRecursive);
    Q_ASSERT(!list.isEmpty());
    return list.first();
}

void TagModelTest::initTestCase()
{
    m_sessionName = "TagModelTest fake session";
    m_fakeSession = new FakeSession(m_sessionName, FakeSession::EndJobsImmediately);
    m_fakeSession->setAsDefaultSession();

    qRegisterMetaType<QModelIndex>("QModelIndex");
}

void TagModelTest::testInitialFetch()
{
    const auto fakeMonitor = new FakeMonitor(this);

    fakeMonitor->setSession(m_fakeSession);
    fakeMonitor->setCollectionMonitored(Collection::root());
    const auto model = new TagModel(fakeMonitor, this);

    const auto serverData = new FakeServerData(model, m_fakeSession, fakeMonitor);
    const auto initialFetchResponse =  FakeJobResponse::interpret(serverData, serverContent1);
    serverData->setCommands(initialFetchResponse);

    m_modelSpy = new ModelSpy{model, this};
    m_modelSpy->startSpying();

    const QList<ExpectedSignal> expectedSignals {
        {RowsAboutToBeInserted, 0, 0},
        {RowsInserted, 0, 0},
        {RowsAboutToBeInserted, 0, 0, QStringLiteral("Tag 1")},
        {RowsInserted, 0, 0, QStringLiteral("Tag 1")},
        {RowsAboutToBeInserted, 1, 1, QStringLiteral("Tag 1")},
        {RowsInserted, 1, 1, QStringLiteral("Tag 1")},
        {RowsAboutToBeInserted, 0, 0, QStringLiteral("Tag 2")},
        {RowsInserted, 0, 0, QStringLiteral("Tag 2")},
        {RowsAboutToBeInserted, 1, 1},
        {RowsInserted, 1, 1}
    };
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

    const auto testDrivers = populateModel(serverContent);
    const auto serverData = testDrivers.first;
    const auto model = testDrivers.second;

    const auto parentIndex = firstMatchedIndex(*model, parentTag);
    const auto newRow = model->rowCount(parentIndex);

    const auto addCommand = new FakeTagAddedCommand(addedTag, parentTag, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({addCommand});


    const QList<ExpectedSignal> expectedSignals {
        {RowsAboutToBeInserted, newRow, newRow, parentTag, QVariantList{addedTag}},
        {RowsInserted, newRow, newRow, parentTag, QVariantList{addedTag}}
    };
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

    const auto testDrivers = populateModel(serverContent);
    const auto serverData = testDrivers.first;
    const auto model = testDrivers.second;

    const auto changedIndex = firstMatchedIndex(*model, tagName);
    const auto parentTag = changedIndex.parent().data().toString();
    const auto changedRow = changedIndex.row();

    const auto changeCommand = new FakeTagChangedCommand(tagName, parentTag, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands(QList<FakeAkonadiServerCommand *>() << changeCommand);

    const QList<ExpectedSignal> expectedSignals {
        {DataChanged, changedRow, changedRow, parentTag, QVariantList{tagName}}
    };
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

    const auto testDrivers = populateModel(serverContent);
    const auto serverData = testDrivers.first;
    const auto model = testDrivers.second;

    const auto removedIndex = firstMatchedIndex(*model, removedTag);
    const auto parentTag = removedIndex.parent().data().toString();
    const auto sourceRow = removedIndex.row();

    const auto removeCommand = new FakeTagRemovedCommand(removedTag, parentTag, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({removeCommand});

    const auto removedTagList = QVariantList{removedTag};
    const auto parentTagVariant = parentTag.isEmpty() ? QVariant{} : parentTag;
    const QList<ExpectedSignal> expectedSignals {
        ExpectedSignal{RowsAboutToBeRemoved, sourceRow, sourceRow, parentTagVariant, QVariantList{removedTag}},
        ExpectedSignal{RowsRemoved, sourceRow, sourceRow, parentTagVariant, QVariantList{removedTag}}
    };

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

    const auto testDrivers = populateModel(serverContent);
    const auto serverData = testDrivers.first;
    const auto model = testDrivers.second;

    const auto changedIndex = firstMatchedIndex(*model, changedTag);
    const auto parentTag = changedIndex.parent().data().toString();
    const auto sourceRow = changedIndex.row();

    QModelIndex newParentIndex = firstMatchedIndex(*model, newParent);
    const auto destRow = model->rowCount(newParentIndex);

    const auto moveCommand = new FakeTagMovedCommand(changedTag, parentTag, newParent, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({moveCommand});

    const auto parentTagVariant = parentTag.isEmpty() ? QVariant{} : parentTag;
    const auto newParentVariant = newParent.isEmpty() ? QVariant{} : newParent;
    const QList<ExpectedSignal> expectedSignals {
        {RowsAboutToBeMoved, sourceRow, sourceRow, parentTagVariant, destRow, newParentVariant, QVariantList{changedTag}},
        {RowsMoved, sourceRow, sourceRow, parentTagVariant, destRow, newParentVariant, QVariantList{changedTag}}
    };

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a change to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

#include "tagmodeltest.moc"

QTEST_MAIN(TagModelTest)
