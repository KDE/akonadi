/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "changerecorder_p.h"
#include "collectioncreatejob.h"
#include "control.h"
#include "entitytreemodel.h"
#include "entitytreemodel_p.h"
#include "itemcreatejob.h"
#include "monitor_p.h"
#include "qtest_akonadi.h"

using namespace Akonadi;

class InspectableETM : public EntityTreeModel
{
public:
    explicit InspectableETM(ChangeRecorder *monitor, QObject *parent = nullptr)
        : EntityTreeModel(monitor, parent)
    {
    }
    EntityTreeModelPrivate *etmPrivate()
    {
        return d_ptr.get();
    }
};

/**
 * This is a test for the LazyPopulation of the ETM and the associated refcounting in the Monitor.
 */
class LazyPopulationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    /**
     * Test a complete scenario that checks:
     * * loading on referencing
     * * buffering after referencing
     * * purging after the collection leaves the buffer
     * * not-fetching when a collection is not buffered and not referenced
     * * reloading after a collection becomes referenced again
     */
    void testItemAdded();
    /*
     * Test what happens if we
     * * Create an item
     * * Reference before item added signal arrives
     * * Try fetching rest of items
     */
    void testItemAddedBeforeFetch();

    /*
     * We purge an empty collection and make sure it can be fetched later on.
     * * reference collection to remember empty status
     * * purge collection
     * * add item (it should not be added since not monitored)
     * * reference collection and make sure items are added
     */
    void testPurgeEmptyCollection();

private:
    Collection res3;
    static const int numberOfRootCollections = 4;
    static const int bufferSize;
};

const int LazyPopulationTest::bufferSize = MonitorPrivate::PurgeBuffer::buffersize();

void LazyPopulationTest::initTestCase()
{
    qRegisterMetaType<Akonadi::Collection::Id>("Akonadi::Collection::Id");
    AkonadiTest::checkTestIsIsolated();
    AkonadiTest::setAllResourcesOffline();

    res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));

    // Set up a bunch of collections that we can select to purge a collection from the buffer

    // Number of buffered collections in the monitor
    const int bufferSize = MonitorPrivate::PurgeBuffer::buffersize();
    for (int i = 0; i < bufferSize; i++) {
        Collection col;
        col.setParentCollection(res3);
        col.setName(QStringLiteral("col%1").arg(i));
        auto create = new CollectionCreateJob(col, this);
        AKVERIFYEXEC(create);
    }
}

QModelIndex getIndex(const QString &string, EntityTreeModel *model)
{
    QModelIndexList list = model->match(model->index(0, 0), Qt::DisplayRole, string, 1, Qt::MatchRecursive);
    if (list.isEmpty()) {
        return QModelIndex();
    }
    return list.first();
}

/**
 * Since we have no sensible way to figure out if the model is fully populated,
 * we use the brute force approach.
 */
bool waitForPopulation(const QModelIndex &idx, EntityTreeModel *model, int count)
{
    for (int i = 0; i < 500; i++) {
        if (model->rowCount(idx) >= count) {
            return true;
        }
        QTest::qWait(10);
    }
    return false;
}

void referenceCollection(EntityTreeModel *model, int index)
{
    QModelIndex idx = getIndex(QStringLiteral("col%1").arg(index), model);
    QVERIFY(idx.isValid());
    model->setData(idx, QVariant(), EntityTreeModel::CollectionRefRole);
    model->setData(idx, QVariant(), EntityTreeModel::CollectionDerefRole);
}

void referenceCollections(EntityTreeModel *model, int count)
{
    for (int i = 0; i < count; i++) {
        referenceCollection(model, i);
    }
}

void LazyPopulationTest::testItemAdded()
{
    const int bufferSize = MonitorPrivate::PurgeBuffer::buffersize();
    const QString mainCollectionName(QStringLiteral("main"));
    Collection monitorCol;
    {
        monitorCol.setParentCollection(res3);
        monitorCol.setName(mainCollectionName);
        auto create = new CollectionCreateJob(monitorCol, this);
        AKVERIFYEXEC(create);
        monitorCol = create->collection();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, monitorCol, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    auto changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(Collection::root());
    QVERIFY(AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady));
    auto model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    // Wait for initial listing to complete
    QVERIFY(waitForPopulation(QModelIndex(), model, numberOfRootCollections));

    const QModelIndex res3Index = getIndex(QStringLiteral("res3"), model);
    QVERIFY(waitForPopulation(res3Index, model, bufferSize + 1));

    QModelIndex monitorIndex = getIndex(mainCollectionName, model);
    QVERIFY(monitorIndex.isValid());

    // Start

    //---Check that the item is present after the collection was referenced
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionRefRole);
    model->fetchMore(monitorIndex);
    // Wait for collection to be fetched
    QVERIFY(waitForPopulation(monitorIndex, model, 1));

    //---ensure we cannot fetchMore again
    QVERIFY(!model->etmPrivate()->canFetchMore(monitorIndex));

    // The item should now be present
    QCOMPARE(model->index(0, 0, monitorIndex).data(Akonadi::EntityTreeModel::ItemIdRole).value<Akonadi::Item::Id>(), item1.id());

    //---ensure item1 is still available after no longer being referenced due to buffering
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionDerefRole);
    QCOMPARE(model->index(0, 0, monitorIndex).data(Akonadi::EntityTreeModel::ItemIdRole).value<Akonadi::Item::Id>(), item1.id());

    //---ensure item1 gets purged after the collection is no longer buffered
    // Get the monitorCol out of the buffer
    referenceCollections(model, bufferSize - 1);
    // The collection is still in the buffer...
    QCOMPARE(model->rowCount(monitorIndex), 1);
    referenceCollection(model, bufferSize - 1);
    //...and now purged
    QCOMPARE(model->rowCount(monitorIndex), 0);
    QVERIFY(model->etmPrivate()->canFetchMore(monitorIndex));

    //---ensure item2 added to unbuffered and unreferenced collection is not added to the model
    Item item2;
    {
        item2.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item2, monitorCol, this);
        AKVERIFYEXEC(append);
        item2 = append->item();
    }
    QCOMPARE(model->rowCount(monitorIndex), 0);

    //---ensure all items are loaded after re-referencing the collection
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionRefRole);
    model->fetchMore(monitorIndex);
    // Wait for collection to be fetched
    QVERIFY(waitForPopulation(monitorIndex, model, 2));
    QCOMPARE(model->rowCount(monitorIndex), 2);

    QVERIFY(!model->etmPrivate()->canFetchMore(monitorIndex));

    // purge collection again
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionDerefRole);
    referenceCollections(model, bufferSize);
    QCOMPARE(model->rowCount(monitorIndex), 0);
    // fetch when not monitored
    QVERIFY(model->etmPrivate()->canFetchMore(monitorIndex));
    model->fetchMore(monitorIndex);
    QVERIFY(waitForPopulation(monitorIndex, model, 2));
    // ensure we cannot refetch
    QVERIFY(!model->etmPrivate()->canFetchMore(monitorIndex));
}

void LazyPopulationTest::testItemAddedBeforeFetch()
{
    const QString mainCollectionName(QStringLiteral("main2"));
    Collection monitorCol;
    {
        monitorCol.setParentCollection(res3);
        monitorCol.setName(mainCollectionName);
        auto create = new CollectionCreateJob(monitorCol, this);
        AKVERIFYEXEC(create);
        monitorCol = create->collection();
    }

    auto changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(Collection::root());
    QVERIFY(AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady));
    auto model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    // Wait for initial listing to complete
    QVERIFY(waitForPopulation(QModelIndex(), model, numberOfRootCollections));

    const QModelIndex res3Index = getIndex(QStringLiteral("res3"), model);
    QVERIFY(waitForPopulation(res3Index, model, bufferSize + 1));

    QModelIndex monitorIndex = getIndex(mainCollectionName, model);
    QVERIFY(monitorIndex.isValid());

    // Create a first item before referencing, it should not show up in the ETM
    {
        Item item1;
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, monitorCol, this);
        AKVERIFYEXEC(append);
    }

    // Before referenced or fetchMore is called, the collection should be empty
    QTest::qWait(500);
    QCOMPARE(model->rowCount(monitorIndex), 0);

    // Reference the collection
    QVERIFY(!model->etmPrivate()->isMonitored(monitorCol.id()));
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionRefRole);
    QVERIFY(model->etmPrivate()->isMonitored(monitorCol.id()));

    // Create another item, it should not be added to the ETM although the signal is emitted from the monitor, but we should be able to fetchMore
    {
        QSignalSpy addedSpy(changeRecorder, &Monitor::itemAdded);
        QVERIFY(addedSpy.isValid());
        Item item2;
        item2.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item2, monitorCol, this);
        AKVERIFYEXEC(append);
        QTRY_VERIFY(addedSpy.count() >= 1);
    }

    QVERIFY(model->etmPrivate()->canFetchMore(monitorIndex));

    model->fetchMore(monitorIndex);
    // Wait for collection to be fetched
    QVERIFY(waitForPopulation(monitorIndex, model, 2));
}

void LazyPopulationTest::testPurgeEmptyCollection()
{
    const QString mainCollectionName(QStringLiteral("main3"));
    Collection monitorCol;
    {
        monitorCol.setParentCollection(res3);
        monitorCol.setName(mainCollectionName);
        auto create = new CollectionCreateJob(monitorCol, this);
        AKVERIFYEXEC(create);
        monitorCol = create->collection();
    }
    // Monitor without referencing so we get all signals
    auto monitor = new Monitor(this);
    monitor->setCollectionMonitored(Collection::root());
    QVERIFY(AkonadiTest::akWaitForSignal(monitor, &Monitor::monitorReady));

    auto changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(Collection::root());
    QVERIFY(AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady));
    auto model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    // Wait for initial listing to complete
    QVERIFY(waitForPopulation(QModelIndex(), model, numberOfRootCollections));

    const QModelIndex res3Index = getIndex(QStringLiteral("res3"), model);
    QVERIFY(waitForPopulation(res3Index, model, bufferSize + 1));

    QModelIndex monitorIndex = getIndex(mainCollectionName, model);
    QVERIFY(monitorIndex.isValid());

    // fetch the collection so we remember the empty status
    QSignalSpy populatedSpy(model, &EntityTreeModel::collectionPopulated);
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionRefRole);
    model->fetchMore(monitorIndex);
    // Wait for collection to be fetched
    QTRY_VERIFY(populatedSpy.count() >= 1);

    // get the collection purged
    model->setData(monitorIndex, QVariant(), EntityTreeModel::CollectionDerefRole);
    referenceCollections(model, bufferSize);

    // create an item
    {
        QSignalSpy addedSpy(monitor, &Monitor::itemAdded);
        QVERIFY(addedSpy.isValid());
        Item item1;
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, monitorCol, this);
        AKVERIFYEXEC(append);
        QTRY_VERIFY(addedSpy.count() >= 1);
    }

    // ensure it's not in the model
    // fetch the collection
    QVERIFY(model->etmPrivate()->canFetchMore(monitorIndex));

    model->fetchMore(monitorIndex);
    // Wait for collection to be fetched
    QVERIFY(waitForPopulation(monitorIndex, model, 1));
}

#include "lazypopulationtest.moc"

QTEST_AKONADIMAIN(LazyPopulationTest)
