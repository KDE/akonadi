/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changerecorder_p.h"
#include "collectioncreatejob.h"
#include "control.h"
#include "entitytreemodel.h"
#include "entitytreemodel_p.h"
#include "favoritecollectionsmodel.h"
#include "itemcreatejob.h"
#include "monitor_p.h"
#include "qtest_akonadi.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QObject>
#include <QSortFilterProxyModel>

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
    void reset()
    {
        beginResetModel();
        endResetModel();
    }
};

class FavoriteProxyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testItemAdded();
    void testLoadConfig();
    void testInsertAfterModelCreation();

private:
    InspectableETM *createETM();
};

void FavoriteProxyTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Akonadi::Control::start();
    AkonadiTest::setAllResourcesOffline();
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

InspectableETM *FavoriteProxyTest::createETM()
{
    auto changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(Collection::root());
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    auto model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);
    return model;
}

/**
 * Tests that the item is being referenced when added to the favorite proxy, and dereferenced when removed.
 */
void FavoriteProxyTest::testItemAdded()
{
    Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));

    InspectableETM *model = createETM();

    KConfigGroup configGroup(KSharedConfig::openConfig(), QLatin1String("favoritecollectionsmodeltest"));

    auto favoriteModel = new FavoriteCollectionsModel(model, configGroup, this);

    const int numberOfRootCollections = 4;
    // Wait for initial listing to complete
    QVERIFY(waitForPopulation(QModelIndex(), model, numberOfRootCollections));

    const QModelIndex res3Index = getIndex(QStringLiteral("res3"), model);
    QVERIFY(res3Index.isValid());

    const auto favoriteCollection = res3Index.data(EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    QVERIFY(favoriteCollection.isValid());

    QVERIFY(!model->etmPrivate()->isMonitored(favoriteCollection.id()));

    // Ensure the collection is reference counted after being added to the favorite model
    {
        favoriteModel->addCollection(favoriteCollection);
        // the collection is in the favorites model
        QTRY_COMPARE(favoriteModel->rowCount(QModelIndex()), 1);
        QTRY_COMPARE(favoriteModel->data(favoriteModel->index(0, 0, QModelIndex()), EntityTreeModel::CollectionIdRole).value<Akonadi::Collection::Id>(),
                     favoriteCollection.id());
        // the collection got referenced
        QTRY_VERIFY(model->etmPrivate()->isMonitored(favoriteCollection.id()));
        // the collection is not yet buffered though
        QTRY_VERIFY(!model->etmPrivate()->isBuffered(favoriteCollection.id()));
    }

    // Survive a reset
    {
        QSignalSpy resetSpy(model, &QAbstractItemModel::modelReset);
        model->reset();
        QTRY_COMPARE(resetSpy.count(), 1);
        // the collection is in the favorites model
        QTRY_COMPARE(favoriteModel->rowCount(QModelIndex()), 1);
        QTRY_COMPARE(favoriteModel->data(favoriteModel->index(0, 0, QModelIndex()), EntityTreeModel::CollectionIdRole).value<Akonadi::Collection::Id>(),
                     favoriteCollection.id());
        // the collection got referenced
        QTRY_VERIFY(model->etmPrivate()->isMonitored(favoriteCollection.id()));
        // the collection is not yet buffered though
        QTRY_VERIFY(!model->etmPrivate()->isBuffered(favoriteCollection.id()));
    }

    // Ensure the collection is no longer reference counted after being added to the favorite model, and moved to the buffer
    {
        favoriteModel->removeCollection(favoriteCollection);
        // moved from being reference counted to being buffered
        QTRY_VERIFY(model->etmPrivate()->isBuffered(favoriteCollection.id()));
        QTRY_COMPARE(favoriteModel->rowCount(QModelIndex()), 0);
    }
}

void FavoriteProxyTest::testLoadConfig()
{
    InspectableETM *model = createETM();

    const int numberOfRootCollections = 4;
    // Wait for initial listing to complete
    QVERIFY(waitForPopulation(QModelIndex(), model, numberOfRootCollections));
    const QModelIndex res3Index = getIndex(QStringLiteral("res3"), model);
    QVERIFY(res3Index.isValid());
    const auto favoriteCollection = res3Index.data(EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    QVERIFY(favoriteCollection.isValid());

    KConfigGroup configGroup(KSharedConfig::openConfig(), QLatin1String("favoritecollectionsmodeltest"));
    configGroup.writeEntry("FavoriteCollectionIds", QList<Akonadi::Collection::Id>() << favoriteCollection.id());
    configGroup.writeEntry("FavoriteCollectionLabels", QStringList() << QStringLiteral("label1"));

    auto favoriteModel = new FavoriteCollectionsModel(model, configGroup, this);

    {
        QTRY_COMPARE(favoriteModel->rowCount(QModelIndex()), 1);
        QTRY_COMPARE(favoriteModel->data(favoriteModel->index(0, 0, QModelIndex()), EntityTreeModel::CollectionIdRole).value<Akonadi::Collection::Id>(),
                     favoriteCollection.id());
        // the collection got referenced
        QTRY_VERIFY(model->etmPrivate()->isMonitored(favoriteCollection.id()));
    }
}

class Filter : public QSortFilterProxyModel
{
public:
    bool filterAcceptsRow(int /*source_row*/, const QModelIndex & /*source_parent*/) const override
    {
        return accepts;
    }
    bool accepts;
};

void FavoriteProxyTest::testInsertAfterModelCreation()
{
    InspectableETM *model = createETM();
    Filter filter;
    filter.accepts = false;
    filter.setSourceModel(model);

    const int numberOfRootCollections = 4;
    // Wait for initial listing to complete
    QVERIFY(waitForPopulation(QModelIndex(), model, numberOfRootCollections));
    const QModelIndex res3Index = getIndex(QStringLiteral("res3"), model);
    QVERIFY(res3Index.isValid());
    const auto favoriteCollection = res3Index.data(EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    QVERIFY(favoriteCollection.isValid());

    KConfigGroup configGroup(KSharedConfig::openConfig(), QLatin1String("favoritecollectionsmodeltest2"));

    auto favoriteModel = new FavoriteCollectionsModel(&filter, configGroup, this);

    // Make sure the filter is not letting anything through
    QTest::qWait(0);
    QCOMPARE(filter.rowCount(QModelIndex()), 0);

    // The collection is not in the model yet
    favoriteModel->addCollection(favoriteCollection);
    filter.accepts = true;
    filter.invalidate();

    {
        QTRY_COMPARE(favoriteModel->rowCount(QModelIndex()), 1);
        QTRY_COMPARE(favoriteModel->data(favoriteModel->index(0, 0, QModelIndex()), EntityTreeModel::CollectionIdRole).value<Akonadi::Collection::Id>(),
                     favoriteCollection.id());
        // the collection got referenced
        QTRY_VERIFY(model->etmPrivate()->isMonitored(favoriteCollection.id()));
    }
}

#include "favoriteproxytest.moc"

QTEST_AKONADIMAIN(FavoriteProxyTest)
