/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qtest_akonadi.h"

#include "entitydisplayattribute.h"
#include "entitytreemodel.h"
#include "entitytreemodel_p.h"
#include "fakemonitor.h"
#include "fakeserverdata.h"
#include "fakesession.h"
#include "imapparser_p.h"
#include "modelspy.h"

static const char serverContent1[] =
    // The format of these lines are first a type, either 'C' or 'I' for Item and collection.
    // The dashes show the depth in the hierarchy
    // Collections have a list of mimetypes they can contain, followed by an optional
    // displayName which is put into the EntityDisplayAttribute, followed by an optional order
    // which is the order in which the collections are returned from the job to the ETM.

    "- C (inode/directory)                  'Col 1'     4"
    "- - C (text/directory, message/rfc822) 'Col 2'     3"
    // Items just have the mimetype they contain in the payload.
    "- - - I text/directory                 'Item 1'"
    "- - - I text/directory                 'Item 2'"
    "- - - I message/rfc822                 'Item 3'"
    "- - - I message/rfc822                 'Item 4'"
    "- - C (text/directory)                 'Col 3'     3"
    "- - - C (text/directory)               'Col 4'     2"
    "- - - - C (text/directory)             'Col 5'     1" // <-- First collection to be returned
    "- - - - - I text/directory             'Item 5'"
    "- - - - - I text/directory             'Item 6'"
    "- - - - I text/directory               'Item 7'"
    "- - - I text/directory                 'Item 8'"
    "- - - I text/directory                 'Item 9'"
    "- - C (message/rfc822)                 'Col 6'     3"
    "- - - I message/rfc822                 'Item 10'"
    "- - - I message/rfc822                 'Item 11'"
    "- - C (text/directory, message/rfc822) 'Col 7'     3"
    "- - - I text/directory                 'Item 12'"
    "- - - I text/directory                 'Item 13'"
    "- - - I message/rfc822                 'Item 14'"
    "- - - I message/rfc822                 'Item 15'";

/**
 * This test verifies that the ETM reacts as expected to signals from the monitor.
 *
 * The tested ETM is only talking to fake components so the reaction of the ETM to each signal can be tested.
 *
 * WARNING: This test does no handle jobs issued by the model. It simply shortcuts (calls emitResult) them, and the connected
 * slots are never executed (because the eventloop is not run after emitResult is called).
 * This test therefore only tests the reaction of the model to signals of the monitor and not the overall behaviour.
 */
class EntityTreeModelTest : public QObject
{
    Q_OBJECT

public:
    EntityTreeModelTest()
    {
        QHashSeed::setDeterministicGlobalSeed();
    }

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testInitialFetch();
    void testCollectionMove_data();
    void testCollectionMove();
    void testCollectionAdded_data();
    void testCollectionAdded();
    void testCollectionRemoved_data();
    void testCollectionRemoved();
    void testCollectionChanged_data();
    void testCollectionChanged();
    void testItemMove_data();
    void testItemMove();
    void testItemAdded_data();
    void testItemAdded();
    void testItemRemoved_data();
    void testItemRemoved();
    void testItemChanged_data();
    void testItemChanged();
    void testRemoveCollectionOnChanged();

private:
    QPair<FakeServerData *, Akonadi::EntityTreeModel *> populateModel(const QString &serverContent, const QString &mimeType = QString())
    {
        auto const fakeMonitor = new FakeMonitor(this);

        fakeMonitor->setSession(m_fakeSession);
        fakeMonitor->setCollectionMonitored(Collection::root());
        if (!mimeType.isEmpty()) {
            fakeMonitor->setMimeTypeMonitored(mimeType);
        }
        auto const model = new EntityTreeModel(fakeMonitor, this);

        m_modelSpy = new ModelSpy{model, this};

        auto const serverData = new FakeServerData(model, m_fakeSession, fakeMonitor, this);
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

QModelIndex firstMatchedIndex(const QAbstractItemModel &model, const QString &pattern)
{
    if (pattern.isEmpty()) {
        return {};
    }
    const auto list = model.match(model.index(0, 0), Qt::DisplayRole, pattern, 1, Qt::MatchRecursive);
    Q_ASSERT(!list.isEmpty());
    return list.first();
}

void EntityTreeModelTest::initTestCase()
{
    m_sessionName = "EntityTreeModelTest fake session";
    m_fakeSession = new FakeSession(m_sessionName, FakeSession::EndJobsImmediately);
    m_fakeSession->setAsDefaultSession();

    qRegisterMetaType<QModelIndex>("QModelIndex");
}

void EntityTreeModelTest::cleanupTestCase()
{
    delete m_fakeSession;
}

void EntityTreeModelTest::testInitialFetch()
{
    auto const fakeMonitor = new FakeMonitor(this);

    fakeMonitor->setSession(m_fakeSession);
    fakeMonitor->setCollectionMonitored(Collection::root());
    auto const model = new EntityTreeModel(fakeMonitor, this);

    auto const serverData = new FakeServerData(model, m_fakeSession, fakeMonitor, this);
    serverData->setCommands(FakeJobResponse::interpret(serverData, QString::fromLatin1(serverContent1)));

    m_modelSpy = new ModelSpy(model, this);
    m_modelSpy->startSpying();

    const QList<ExpectedSignal> expectedSignals{// First the model gets a signal about the first collection to be returned, which is not a top-level collection.
                                                // It uses the parentCollection hierarchy to put placeholder collections in the model until the root is reached.
                                                // Then it inserts only one row and emits the correct signals. After that, when the other collections
                                                // arrive, dataChanged is emitted for them.
                                                {RowsAboutToBeInserted, 0, 0},
                                                {RowsInserted, 0, 0},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 4")}},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 3")}},
                                                // New collections are prepended
                                                {RowsAboutToBeInserted, 0, 0, QStringLiteral("Col 1")},
                                                {RowsInserted, 0, 0, QStringLiteral("Col 1"), QVariantList{QStringLiteral("Col 2")}},
                                                {RowsAboutToBeInserted, 0, 0, QStringLiteral("Col 1")},
                                                {RowsInserted, 0, 0, QStringLiteral("Col 1"), QVariantList{QStringLiteral("Col 6")}},
                                                {RowsAboutToBeInserted, 0, 0, QStringLiteral("Col 1")},
                                                {RowsInserted, 0, 0, QStringLiteral("Col 1"), QVariantList{QStringLiteral("Col 7")}},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 1")}},
                                                // The items in the collections are appended.
                                                {RowsAboutToBeInserted, 0, 3, QStringLiteral("Col 2")},
                                                {RowsInserted, 0, 3, QStringLiteral("Col 2")},
                                                {RowsAboutToBeInserted, 0, 1, QStringLiteral("Col 5")},
                                                {RowsInserted, 0, 1, QStringLiteral("Col 5")},
                                                {RowsAboutToBeInserted, 1, 1, QStringLiteral("Col 4")},
                                                {RowsInserted, 1, 1, QStringLiteral("Col 4")},
                                                {RowsAboutToBeInserted, 1, 2, QStringLiteral("Col 3")},
                                                {RowsInserted, 1, 2, QStringLiteral("Col 3")},
                                                {RowsAboutToBeInserted, 0, 1, QStringLiteral("Col 6")},
                                                {RowsInserted, 0, 1, QStringLiteral("Col 6")},
                                                {RowsAboutToBeInserted, 0, 3, QStringLiteral("Col 7")},
                                                {RowsInserted, 0, 3, QStringLiteral("Col 7")},
                                                {DataChanged, 3, 3, QVariantList{QStringLiteral("Col 3")}},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 1")}},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 5")}},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 4")}},
                                                {DataChanged, 2, 2, QVariantList{QStringLiteral("Col 2")}},
                                                {DataChanged, 1, 1, QVariantList{QStringLiteral("Col 7")}},
                                                {DataChanged, 0, 0, QVariantList{QStringLiteral("Col 6")}}};
    m_modelSpy->setExpectedSignals(expectedSignals);

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(10);

    // We get all the signals we expected.
    QTRY_VERIFY(m_modelSpy->expectedSignals().isEmpty());

    QTest::qWait(10);
    // We didn't get signals we didn't expect.
    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testCollectionMove_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("movedCollection");
    QTest::addColumn<QString>("targetCollection");

    QTest::newRow("move-collection01") << serverContent1 << "Col 5"
                                       << "Col 1";
    QTest::newRow("move-collection02") << serverContent1 << "Col 5"
                                       << "Col 2";
    QTest::newRow("move-collection03") << serverContent1 << "Col 5"
                                       << "Col 3";
    QTest::newRow("move-collection04") << serverContent1 << "Col 5"
                                       << "Col 6";
    QTest::newRow("move-collection05") << serverContent1 << "Col 5"
                                       << "Col 7";
    QTest::newRow("move-collection06") << serverContent1 << "Col 3"
                                       << "Col 2";
    QTest::newRow("move-collection07") << serverContent1 << "Col 3"
                                       << "Col 6";
    QTest::newRow("move-collection08") << serverContent1 << "Col 3"
                                       << "Col 7";
    QTest::newRow("move-collection09") << serverContent1 << "Col 7"
                                       << "Col 2";
    QTest::newRow("move-collection10") << serverContent1 << "Col 7"
                                       << "Col 5";
    QTest::newRow("move-collection11") << serverContent1 << "Col 7"
                                       << "Col 4";
    QTest::newRow("move-collection12") << serverContent1 << "Col 7"
                                       << "Col 3";
}

void EntityTreeModelTest::testCollectionMove()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, movedCollection);
    QFETCH(QString, targetCollection);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto movedIndex = firstMatchedIndex(*model, movedCollection);
    Q_ASSERT(movedIndex.isValid());
    const auto sourceCollection = movedIndex.parent().data().toString();
    const auto sourceRow = movedIndex.row();

    auto const moveCommand = new FakeCollectionMovedCommand(movedCollection, sourceCollection, targetCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({moveCommand});

    const QList<ExpectedSignal> expectedSignals{{RowsAboutToBeMoved, sourceRow, sourceRow, sourceCollection, 0, targetCollection, {movedCollection}},
                                                {RowsMoved, sourceRow, sourceRow, sourceCollection, 0, targetCollection, {movedCollection}}};
    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a change to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testCollectionAdded_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("addedCollection");
    QTest::addColumn<QString>("parentCollection");

    QTest::newRow("add-collection01") << serverContent1 << "new Collection"
                                      << "Col 1";
    QTest::newRow("add-collection02") << serverContent1 << "new Collection"
                                      << "Col 2";
    QTest::newRow("add-collection03") << serverContent1 << "new Collection"
                                      << "Col 3";
    QTest::newRow("add-collection04") << serverContent1 << "new Collection"
                                      << "Col 4";
    QTest::newRow("add-collection05") << serverContent1 << "new Collection"
                                      << "Col 5";
    QTest::newRow("add-collection06") << serverContent1 << "new Collection"
                                      << "Col 6";
    QTest::newRow("add-collection07") << serverContent1 << "new Collection"
                                      << "Col 7";
}

void EntityTreeModelTest::testCollectionAdded()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, addedCollection);
    QFETCH(QString, parentCollection);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;

    auto const addCommand = new FakeCollectionAddedCommand(addedCollection, parentCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({addCommand});

    const QList<ExpectedSignal> expectedSignals{
        {RowsAboutToBeInserted, 0, 0, parentCollection, QVariantList{addedCollection}},
        {RowsInserted, 0, 0, parentCollection, QVariantList{addedCollection}},
        // The data changed signal comes from the item fetch job that is triggered because we have ImmediatePopulation enabled
        {DataChanged, 0, 0, parentCollection, QVariantList{addedCollection}}};

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testCollectionRemoved_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("removedCollection");

    // The test suite doesn't handle this case yet.
    //   QTest::newRow("remove-collection01") << serverContent1 << "Col 1";
    QTest::newRow("remove-collection02") << serverContent1 << "Col 2";
    QTest::newRow("remove-collection03") << serverContent1 << "Col 3";
    QTest::newRow("remove-collection04") << serverContent1 << "Col 4";
    QTest::newRow("remove-collection05") << serverContent1 << "Col 5";
    QTest::newRow("remove-collection06") << serverContent1 << "Col 6";
    QTest::newRow("remove-collection07") << serverContent1 << "Col 7";
}

void EntityTreeModelTest::testCollectionRemoved()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, removedCollection);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto removedIndex = firstMatchedIndex(*model, removedCollection);
    const auto parentCollection = removedIndex.parent().data().toString();
    const auto sourceRow = removedIndex.row();

    auto const removeCommand = new FakeCollectionRemovedCommand(removedCollection, parentCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({removeCommand});

    const QList<ExpectedSignal> expectedSignals{{RowsAboutToBeRemoved, sourceRow, sourceRow, parentCollection, QVariantList{removedCollection}},
                                                {RowsRemoved, sourceRow, sourceRow, parentCollection, QVariantList{removedCollection}}};

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testCollectionChanged_data()
{
    QTest::addColumn<QString>("serverContent");

    QTest::newRow("change-collection01") << serverContent1 << "Col 1" << QString();
    QTest::newRow("change-collection02") << serverContent1 << "Col 2" << QString();
    QTest::newRow("change-collection03") << serverContent1 << "Col 3" << QString();
    QTest::newRow("change-collection04") << serverContent1 << "Col 4" << QString();
    QTest::newRow("change-collection05") << serverContent1 << "Col 5" << QString();
    QTest::newRow("change-collection06") << serverContent1 << "Col 6" << QString();
    QTest::newRow("change-collection07") << serverContent1 << "Col 7" << QString();
    // Don't remove the parent due to a missing mimetype
    QTest::newRow("change-collection08") << serverContent1 << "Col 1" << QStringLiteral("message/rfc822");
}

void EntityTreeModelTest::testCollectionChanged()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, collectionName);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto changedIndex = firstMatchedIndex(*model, collectionName);
    const auto parentCollection = changedIndex.parent().data().toString();
    const auto changedRow = changedIndex.row();

    auto const changeCommand = new FakeCollectionChangedCommand(collectionName, parentCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({changeCommand});

    const QList<ExpectedSignal> expectedSignals{{DataChanged, changedRow, changedRow, parentCollection, QVariantList{collectionName}}};

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testItemMove_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("movedItem");
    QTest::addColumn<QString>("targetCollection");

    QTest::newRow("move-item01") << serverContent1 << "Item 1"
                                 << "Col 7";
    QTest::newRow("move-item02") << serverContent1 << "Item 5"
                                 << "Col 4"; // Move item to grandparent.
    QTest::newRow("move-item03") << serverContent1 << "Item 7"
                                 << "Col 5"; // Move item to sibling.
    QTest::newRow("move-item04") << serverContent1 << "Item 8"
                                 << "Col 5"; // Move item to nephew
    QTest::newRow("move-item05") << serverContent1 << "Item 8"
                                 << "Col 6"; // Move item to uncle
    QTest::newRow("move-item02") << serverContent1 << "Item 5"
                                 << "Col 3"; // Move item to great-grandparent.
}

void EntityTreeModelTest::testItemMove()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, movedItem);
    QFETCH(QString, targetCollection);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto movedIndex = firstMatchedIndex(*model, movedItem);
    const auto sourceCollection = movedIndex.parent().data().toString();
    const auto sourceRow = movedIndex.row();

    const auto targetIndex = firstMatchedIndex(*model, targetCollection);
    const auto targetRow = model->rowCount(targetIndex);

    auto const moveCommand = new FakeItemMovedCommand(movedItem, sourceCollection, targetCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({moveCommand});

    const QList<ExpectedSignal> expectedSignals{
        // Currently moves are implemented as remove + insert in the ETM.
        {RowsAboutToBeRemoved, sourceRow, sourceRow, sourceCollection, QVariantList{movedItem}},
        {RowsRemoved, sourceRow, sourceRow, sourceCollection, QVariantList{movedItem}},
        {RowsAboutToBeInserted, targetRow, targetRow, targetCollection, QVariantList{movedItem}},
        {RowsInserted, targetRow, targetRow, targetCollection, QVariantList{movedItem}},
        // {RowsAboutToBeMoved, sourceRow, sourceRow, sourceCollection, targetRow, targetCollection, QVariantList{movedItem}},
        // {RowsMoved, sourceRow, sourceRow, sourceCollection, targetRow, targetCollection, QVariantList{movedItem}},
    };

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testItemAdded_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("addedItem");
    QTest::addColumn<QString>("parentCollection");

    QTest::newRow("add-item01") << serverContent1 << "new Item"
                                << "Col 1";
    QTest::newRow("add-item02") << serverContent1 << "new Item"
                                << "Col 2";
    QTest::newRow("add-item03") << serverContent1 << "new Item"
                                << "Col 3";
    QTest::newRow("add-item04") << serverContent1 << "new Item"
                                << "Col 4";
    QTest::newRow("add-item05") << serverContent1 << "new Item"
                                << "Col 5";
    QTest::newRow("add-item06") << serverContent1 << "new Item"
                                << "Col 6";
    QTest::newRow("add-item07") << serverContent1 << "new Item"
                                << "Col 7";
}

void EntityTreeModelTest::testItemAdded()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, addedItem);
    QFETCH(QString, parentCollection);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto parentIndex = firstMatchedIndex(*model, parentCollection);
    const auto targetRow = model->rowCount(parentIndex);

    auto const addedCommand = new FakeItemAddedCommand(addedItem, parentCollection, serverData);

    m_modelSpy->startSpying();

    serverData->setCommands({addedCommand});

    const QList<ExpectedSignal> expectedSignals{{RowsAboutToBeInserted, targetRow, targetRow, parentCollection, QVariantList{addedItem}},
                                                {RowsInserted, targetRow, targetRow, parentCollection, QVariantList{addedItem}}};
    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testItemRemoved_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("removedItem");

    QTest::newRow("remove-item01") << serverContent1 << "Item 1";
    QTest::newRow("remove-item02") << serverContent1 << "Item 2";
    QTest::newRow("remove-item03") << serverContent1 << "Item 3";
    QTest::newRow("remove-item04") << serverContent1 << "Item 4";
    QTest::newRow("remove-item05") << serverContent1 << "Item 5";
    QTest::newRow("remove-item06") << serverContent1 << "Item 6";
    QTest::newRow("remove-item07") << serverContent1 << "Item 7";
    QTest::newRow("remove-item08") << serverContent1 << "Item 8";
    QTest::newRow("remove-item09") << serverContent1 << "Item 9";
    QTest::newRow("remove-item10") << serverContent1 << "Item 10";
    QTest::newRow("remove-item11") << serverContent1 << "Item 11";
    QTest::newRow("remove-item12") << serverContent1 << "Item 12";
    QTest::newRow("remove-item13") << serverContent1 << "Item 13";
    QTest::newRow("remove-item14") << serverContent1 << "Item 14";
    QTest::newRow("remove-item15") << serverContent1 << "Item 15";
}

void EntityTreeModelTest::testItemRemoved()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, removedItem);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto removedIndex = firstMatchedIndex(*model, removedItem);
    const auto sourceCollection = removedIndex.parent().data().toString();
    const auto sourceRow = removedIndex.row();

    auto const removeCommand = new FakeItemRemovedCommand(removedItem, sourceCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({removeCommand});

    const QList<ExpectedSignal> expectedSignals{{RowsAboutToBeRemoved, sourceRow, sourceRow, sourceCollection, QVariantList{removedItem}},
                                                {RowsRemoved, sourceRow, sourceRow, sourceCollection, QVariantList{removedItem}}};

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testItemChanged_data()
{
    QTest::addColumn<QString>("serverContent");
    QTest::addColumn<QString>("changedItem");

    QTest::newRow("change-item01") << serverContent1 << "Item 1";
    QTest::newRow("change-item02") << serverContent1 << "Item 2";
    QTest::newRow("change-item03") << serverContent1 << "Item 3";
    QTest::newRow("change-item04") << serverContent1 << "Item 4";
    QTest::newRow("change-item05") << serverContent1 << "Item 5";
    QTest::newRow("change-item06") << serverContent1 << "Item 6";
    QTest::newRow("change-item07") << serverContent1 << "Item 7";
    QTest::newRow("change-item08") << serverContent1 << "Item 8";
    QTest::newRow("change-item09") << serverContent1 << "Item 9";
    QTest::newRow("change-item10") << serverContent1 << "Item 10";
    QTest::newRow("change-item11") << serverContent1 << "Item 11";
    QTest::newRow("change-item12") << serverContent1 << "Item 12";
    QTest::newRow("change-item13") << serverContent1 << "Item 13";
    QTest::newRow("change-item14") << serverContent1 << "Item 14";
    QTest::newRow("change-item15") << serverContent1 << "Item 15";
}

void EntityTreeModelTest::testItemChanged()
{
    QFETCH(QString, serverContent);
    QFETCH(QString, changedItem);

    const auto testDrivers = populateModel(serverContent);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto changedIndex = firstMatchedIndex(*model, changedItem);
    const auto parentCollection = changedIndex.parent().data().toString();
    const auto sourceRow = changedIndex.row();

    auto const changeCommand = new FakeItemChangedCommand(changedItem, parentCollection, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({changeCommand});

    const QList<ExpectedSignal> expectedSignals{{DataChanged, sourceRow, sourceRow, QVariantList{changedItem}}};

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

void EntityTreeModelTest::testRemoveCollectionOnChanged()
{
    const auto serverContent = QStringLiteral(
        "- C (inode/directory, text/directory)  'Col 1'     2"
        "- - C (text/directory)                 'Col 2'     1"
        "- - - I text/directory                 'Item 1'");
    const auto collectionName = QStringLiteral("Col 2");
    const auto monitoredMimeType = QStringLiteral("text/directory");

    const auto testDrivers = populateModel(serverContent, monitoredMimeType);
    auto const serverData = testDrivers.first;
    auto const model = testDrivers.second;

    const auto changedIndex = firstMatchedIndex(*model, collectionName);
    auto changedCol = changedIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    changedCol.setContentMimeTypes({QStringLiteral("foobar")});
    const auto parentCollection = changedIndex.parent().data().toString();

    auto const changeCommand = new FakeCollectionChangedCommand(changedCol, serverData);

    m_modelSpy->startSpying();
    serverData->setCommands({changeCommand});

    const QList<ExpectedSignal> expectedSignals{
        {RowsAboutToBeRemoved, changedIndex.row(), changedIndex.row(), parentCollection, QVariantList{collectionName}},
        {RowsRemoved, changedIndex.row(), changedIndex.row(), parentCollection, QVariantList{collectionName}},
    };

    m_modelSpy->setExpectedSignals(expectedSignals);
    serverData->processNotifications();

    // Give the model a chance to run the event loop to process the signals.
    QTest::qWait(0);

    QVERIFY(m_modelSpy->isEmpty());
}

#include "entitytreemodeltest.moc"

QTEST_MAIN(EntityTreeModelTest)
