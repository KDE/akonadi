/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "entitytreemodel.h"
#include "control.h"
#include "entitytreemodel_p.h"
#include "monitor_p.h"
#include "changerecorder_p.h"
#include "qtest_akonadi.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "itemcreatejob.h"

using namespace Akonadi;

class ModelSignalSpy : public QObject
{
    Q_OBJECT
public:
    explicit ModelSignalSpy(QAbstractItemModel &model)
    {
        connect(&model, &QAbstractItemModel::rowsInserted, this, &ModelSignalSpy::onRowsInserted);
        connect(&model, &QAbstractItemModel::rowsRemoved, this, &ModelSignalSpy::onRowsRemoved);
        connect(&model, &QAbstractItemModel::rowsMoved, this, &ModelSignalSpy::onRowsMoved);
        connect(&model, &QAbstractItemModel::dataChanged, this, &ModelSignalSpy::onDataChanged);
        connect(&model, &QAbstractItemModel::layoutChanged, this, &ModelSignalSpy::onLayoutChanged);
        connect(&model, &QAbstractItemModel::modelReset, this, &ModelSignalSpy::onModelReset);
    }

    QStringList mSignals;
    QModelIndex parent;
    int start;
    int end;

public Q_SLOTS:
    void onRowsInserted(const QModelIndex &p, int s, int e)
    {
        qDebug() << "rowsInserted( parent =" << p << ", start = " << s << ", end = " << e << ", data = " << p.data().toString() << ")";
        mSignals << QStringLiteral("rowsInserted");
        parent = p;
        start = s;
        end = e;
    }
    void onRowsRemoved(const QModelIndex &p, int s, int e)
    {
        qDebug() << "rowsRemoved( parent = " << p << ", start = " << s << ", end = " << e << ")";
        mSignals << QStringLiteral("rowsRemoved");
        parent = p;
        start = s;
        end = e;
    }
    void onRowsMoved(const QModelIndex & /*unused*/, int /*unused*/, int /*unused*/, const QModelIndex & /*unused*/, int /*unused*/)
    {
        mSignals << QStringLiteral("rowsMoved");
    }
    void onDataChanged(const QModelIndex &tl, const QModelIndex &br)
    {
        qDebug() << "dataChanged( topLeft =" << tl << "(" << tl.data().toString() << "), bottomRight =" << br << "(" << br.data().toString() << ") )";
        mSignals << QStringLiteral("dataChanged");
    }
    void onLayoutChanged()
    {
        mSignals << QStringLiteral("layoutChanged");
    }
    void onModelReset()
    {
        mSignals << QStringLiteral("modelReset");
    }
};

class InspectableETM: public EntityTreeModel
{
public:
    explicit InspectableETM(ChangeRecorder *monitor, QObject *parent = nullptr)
        : EntityTreeModel(monitor, parent) {}
    EntityTreeModelPrivate *etmPrivate()
    {
        return d_ptr;
    }
};

QModelIndex getIndex(const QString &string, EntityTreeModel *model)
{
    QModelIndexList list = model->match(model->index(0, 0), Qt::DisplayRole, string, 1, Qt::MatchRecursive);
    if (list.isEmpty()) {
        return QModelIndex();
    }
    return list.first();
}

Akonadi::Collection createCollection(const QString &name, const Akonadi::Collection &parent, bool enabled = true, const QStringList &mimeTypes = QStringList())
{
    Akonadi::Collection col;
    col.setParentCollection(parent);
    col.setName(name);
    col.setEnabled(enabled);
    col.setContentMimeTypes(mimeTypes);

    CollectionCreateJob *create = new CollectionCreateJob(col);
    create->exec();
    if (create->error()) {
        qWarning() << create->errorString();
    }
    return create->collection();
}

/**
 * This is a test for the initial population of the ETM.
 */
class EtmPopulationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMonitoringCollectionsPreset();
    void testMonitoringCollections();
    void testFullPopulation();
    void testAddMonitoringCollections();
    void testRemoveMonitoringCollections();
    void testDisplayFilter();
    void testLoadingOfHiddenCollection();

private:
    Collection res;
    QString mainCollectionName;
    Collection monitorCol;
    Collection col1;
    Collection col2;
    Collection col3;
    Collection col4;
};

void EtmPopulationTest::initTestCase()
{
    qRegisterMetaType<Akonadi::Collection::Id>("Akonadi::Collection::Id");
    AkonadiTest::checkTestIsIsolated();
    AkonadiTest::setAllResourcesOffline();

    res = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));

    mainCollectionName = QStringLiteral("main");
    monitorCol = createCollection(mainCollectionName, res);
    QVERIFY(monitorCol.isValid());
    col1 = createCollection(QStringLiteral("col1"), monitorCol);
    QVERIFY(col1.isValid());
    col2 = createCollection(QStringLiteral("col2"), monitorCol);
    QVERIFY(col2.isValid());
    col3 = createCollection(QStringLiteral("col3"), monitorCol);
    QVERIFY(col3.isValid());
    col4 = createCollection(QStringLiteral("col4"), col2);
    QVERIFY(col4.isValid());
}

void EtmPopulationTest::testMonitoringCollectionsPreset()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(col1, true);
    changeRecorder->setCollectionMonitored(col2, true);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QTRY_VERIFY(getIndex(QStringLiteral("col1"), model).isValid());
    QTRY_VERIFY(getIndex(QStringLiteral("col2"), model).isValid());
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(!getIndex(QStringLiteral("col3"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col4"), model).isValid());

    QTRY_VERIFY(getIndex(QStringLiteral("col1"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col2"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col4"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testMonitoringCollections()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);
    Akonadi::Collection::List monitored;
    monitored << col1 << col2;
    model->setCollectionsMonitored(monitored);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex(QStringLiteral("col1"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col2"), model).isValid());
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(!getIndex(QStringLiteral("col3"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col4"), model).isValid());

    QTRY_VERIFY(getIndex(QStringLiteral("col1"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col2"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col4"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testFullPopulation()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    // changeRecorder->setCollectionMonitored(Akonadi::Collection::root());
    changeRecorder->setAllMonitored(true);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex(QStringLiteral("col1"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col2"), model).isValid());
    QVERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(getIndex(QStringLiteral("col3"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col4"), model).isValid());

    QTRY_VERIFY(getIndex(QStringLiteral("col1"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col2"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col4"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testAddMonitoringCollections()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(col1, true);
    changeRecorder->setCollectionMonitored(col2, true);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    //The main collection may be loaded a little later since it is in the fetchAncestors path
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());

    model->setCollectionMonitored(col3, true);

    QVERIFY(getIndex(QStringLiteral("col1"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col2"), model).isValid());
    QTRY_VERIFY(getIndex(QStringLiteral("col3"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col4"), model).isValid());
    QVERIFY(getIndex(mainCollectionName, model).isValid());

    QTRY_VERIFY(getIndex(QStringLiteral("col1"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col2"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col3"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(QStringLiteral("col4"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testRemoveMonitoringCollections()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(col1, true);
    changeRecorder->setCollectionMonitored(col2, true);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    //The main collection may be loaded a little later since it is in the fetchAncestors path
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());

    model->setCollectionMonitored(col2, false);

    QVERIFY(getIndex(QStringLiteral("col1"), model).isValid());
    QVERIFY(!getIndex(QStringLiteral("col2"), model).isValid());
    QVERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(!getIndex(QStringLiteral("col3"), model).isValid());
    QVERIFY(!getIndex(QStringLiteral("col4"), model).isValid());

    QTRY_VERIFY(getIndex(QStringLiteral("col1"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(QStringLiteral("col2"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(QStringLiteral("col4"), model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testDisplayFilter()
{
    Collection col5 = createCollection(QStringLiteral("col5"), monitorCol, false);
    QVERIFY(col5.isValid());

    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);
    model->setListFilter(Akonadi::CollectionFetchScope::Display);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(getIndex(QStringLiteral("col1"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col2"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col3"), model).isValid());
    QVERIFY(getIndex(QStringLiteral("col4"), model).isValid());
    QVERIFY(!getIndex(QStringLiteral("col5"), model).isValid());

    Akonadi::CollectionDeleteJob *deleteJob = new Akonadi::CollectionDeleteJob(col5);
    AKVERIFYEXEC(deleteJob);
}

/*
 * Col5 and it's ancestors should be included although the ancestors don't match the mimetype filter.
 */
void EtmPopulationTest::testLoadingOfHiddenCollection()
{
    Collection col5 = createCollection(QStringLiteral("col5"), monitorCol, false, QStringList() << QStringLiteral("application/test"));
    QVERIFY(col5.isValid());

    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setMimeTypeMonitored(QStringLiteral("application/test"), true);
    AkonadiTest::akWaitForSignal(changeRecorder, &Monitor::monitorReady);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex(QStringLiteral("col5"), model).isValid());

    Akonadi::CollectionDeleteJob *deleteJob = new Akonadi::CollectionDeleteJob(col5);
    AKVERIFYEXEC(deleteJob);
}

#include "etmpopulationtest.moc"

QTEST_AKONADIMAIN(EtmPopulationTest)

