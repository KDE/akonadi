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

#include "test_utils.h"

#include <KStandardDirs>
#include <akonadi/entitytreemodel.h>
#include <akonadi/control.h>
#include <akonadi/entitytreemodel_p.h>
#include <akonadi/monitor_p.h>
#include <akonadi/changerecorder_p.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/itemcreatejob.h>

using namespace Akonadi;

class ModelSignalSpy : public QObject {
    Q_OBJECT
public:
    explicit ModelSignalSpy(QAbstractItemModel &model) {
        connect(&model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(onRowsInserted(QModelIndex,int,int)));
        connect(&model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(onRowsRemoved(QModelIndex,int,int)));
        connect(&model, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), this, SLOT(onRowsMoved(QModelIndex,int,int, QModelIndex, int)));
        connect(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onDataChanged(QModelIndex,QModelIndex)));
        connect(&model, SIGNAL(layoutChanged()), this, SLOT(onLayoutChanged()));
        connect(&model, SIGNAL(modelReset()), this, SLOT(onModelReset()));
    }

    QStringList mSignals;
    QModelIndex parent;
    int start;
    int end;

public Q_SLOTS:
    void onRowsInserted(QModelIndex p, int s, int e) {
        kDebug() << p << s << e;
        kDebug() << p.data().toString();
        mSignals << QLatin1String("rowsInserted");
        parent = p;
        start = s;
        end = e;
    }
    void onRowsRemoved(QModelIndex p, int s, int e) {
        mSignals << QLatin1String("rowsRemoved");
        parent = p;
        start = s;
        end = e;
    }
    void onRowsMoved(QModelIndex,int,int,QModelIndex,int) {
        mSignals << QLatin1String("rowsMoved");
    }
    void onDataChanged(QModelIndex s, QModelIndex e) {
        kDebug() << s << e;
        kDebug() << s.data().toString();
        kDebug() << e.data().toString();
        mSignals << QLatin1String("dataChanged");
    }
    void onLayoutChanged() {
        mSignals << QLatin1String("layoutChanged");
    }
    void onModelReset() {
        mSignals << QLatin1String("modelReset");
    }
};

class InspectableETM: public EntityTreeModel
{
public:
  explicit InspectableETM(ChangeRecorder* monitor, QObject* parent = 0)
  :EntityTreeModel(monitor, parent) {}
  EntityTreeModelPrivate *etmPrivate() { return d_ptr; };
};

QModelIndex getIndex(const QString &string, EntityTreeModel *model)
{
    QModelIndexList list = model->match( model->index( 0, 0 ), Qt::DisplayRole, string, 1, Qt::MatchRecursive );
    if ( list.isEmpty() ) {
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
    Q_ASSERT(!create->error());
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
    void testReferenceCollection();
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

    res = Collection( collectionIdFromPath( "res3" ) );

    mainCollectionName = QLatin1String("main");
    monitorCol = createCollection(mainCollectionName, res);
    col1 = createCollection(QLatin1String("col1"), monitorCol);
    col2 = createCollection(QLatin1String("col2"), monitorCol);
    col3 = createCollection(QLatin1String("col3"), monitorCol);
    col4 = createCollection(QLatin1String("col4"), col2);
}

void EtmPopulationTest::testMonitoringCollectionsPreset()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(col1, true);
    changeRecorder->setCollectionMonitored(col2, true);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QTRY_VERIFY(getIndex("col1", model).isValid());
    QTRY_VERIFY(getIndex("col2", model).isValid());
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(!getIndex("col3", model).isValid());
    QVERIFY(getIndex("col4", model).isValid());

    QTRY_VERIFY(getIndex("col1", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col2", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col4", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}


void EtmPopulationTest::testMonitoringCollections()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);
    Akonadi::Collection::List monitored;
    monitored << col1 << col2;
    model->setCollectionsMonitored(monitored);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex("col1", model).isValid());
    QVERIFY(getIndex("col2", model).isValid());
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(!getIndex("col3", model).isValid());
    QVERIFY(getIndex("col4", model).isValid());

    QTRY_VERIFY(getIndex("col1", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col2", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col4", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testFullPopulation()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    // changeRecorder->setCollectionMonitored(Akonadi::Collection::root());
    changeRecorder->setAllMonitored(true);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex("col1", model).isValid());
    QVERIFY(getIndex("col2", model).isValid());
    QVERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(getIndex("col3", model).isValid());
    QVERIFY(getIndex("col4", model).isValid());

    QTRY_VERIFY(getIndex("col1", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col2", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col4", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testAddMonitoringCollections()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(col1, true);
    changeRecorder->setCollectionMonitored(col2, true);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    //The main collection may be loaded a little later since it is in the fetchAncestors path
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());

    model->setCollectionMonitored(col3, true);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex("col1", model).isValid());
    QVERIFY(getIndex("col2", model).isValid());
    QVERIFY(getIndex("col3", model).isValid());
    QVERIFY(getIndex("col4", model).isValid());
    QVERIFY(getIndex(mainCollectionName, model).isValid());

    QTRY_VERIFY(getIndex("col1", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col2", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col3", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(getIndex("col4", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testRemoveMonitoringCollections()
{
    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setCollectionMonitored(col1, true);
    changeRecorder->setCollectionMonitored(col2, true);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    //The main collection may be loaded a little later since it is in the fetchAncestors path
    QTRY_VERIFY(getIndex(mainCollectionName, model).isValid());

    model->setCollectionMonitored(col2, false);

    QVERIFY(getIndex("col1", model).isValid());
    QVERIFY(!getIndex("col2", model).isValid());
    QVERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(!getIndex("col3", model).isValid());
    QVERIFY(!getIndex("col4", model).isValid());

    QTRY_VERIFY(getIndex("col1", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex("col2", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex(mainCollectionName, model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    QTRY_VERIFY(!getIndex("col4", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
}

void EtmPopulationTest::testDisplayFilter()
{
    Collection col5 = createCollection(QLatin1String("col5"), monitorCol, false);

    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);
    model->setListFilter(Akonadi::CollectionFetchScope::Display);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex(mainCollectionName, model).isValid());
    QVERIFY(getIndex("col1", model).isValid());
    QVERIFY(getIndex("col2", model).isValid());
    QVERIFY(getIndex("col3", model).isValid());
    QVERIFY(getIndex("col4", model).isValid());
    QVERIFY(!getIndex("col5", model).isValid());

    Akonadi::CollectionDeleteJob *deleteJob = new Akonadi::CollectionDeleteJob(col5);
    AKVERIFYEXEC(deleteJob);
}

void EtmPopulationTest::testReferenceCollection()
{
    Collection col5 = createCollection(QLatin1String("col5"), monitorCol, false);

    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);
    model->setListFilter(Akonadi::CollectionFetchScope::Display);

    QTRY_VERIFY(model->isFullyPopulated());
    QVERIFY(!getIndex("col5", model).isValid());
    //Check that this random other collection is actually available
    QVERIFY(getIndex("col1", model).isValid());

    ModelSignalSpy spy(*model);

    //Reference the collection and it should appear in the model
    model->setCollectionReferenced(col5, true);

    QTRY_VERIFY(getIndex("col5", model).isValid());
    QTRY_VERIFY(getIndex("col5", model).data(Akonadi::EntityTreeModel::IsPopulatedRole).toBool());
    //Check that this random other collection is still available
    QVERIFY(getIndex("col1", model).isValid());
    //Verify the etms collection has been updated accordingly
    QTRY_VERIFY(getIndex("col5", model).data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>().referenced());

    //Ensure all signals have been delivered to the spy
    QTest::qWait(0);
    QCOMPARE(spy.mSignals.count("rowsInserted"), 1);
    //Signals for item fetch state and a data-changed signal from the referencing
    QCOMPARE(spy.mSignals.count("dataChanged"), 3);

    //Dereference the collection and it should dissapear again
    model->setCollectionReferenced(col5, false);
    QTRY_VERIFY(!getIndex("col5", model).isValid());
    //Check that this random other collection is still available
    QVERIFY(getIndex("col1", model).isValid());

    Akonadi::CollectionDeleteJob *deleteJob = new Akonadi::CollectionDeleteJob(col5);
    AKVERIFYEXEC(deleteJob);
}

/*
 * Col5 and it's ancestors should be included although the ancestors don't match the mimetype filter.
 */
void EtmPopulationTest::testLoadingOfHiddenCollection()
{
    Collection col5 = createCollection(QLatin1String("col5"), monitorCol, false, QStringList() << QLatin1String("application/test"));

    ChangeRecorder *changeRecorder = new ChangeRecorder(this);
    changeRecorder->setMimeTypeMonitored(QLatin1String("application/test"), true);
    InspectableETM *model = new InspectableETM(changeRecorder, this);
    model->setItemPopulationStrategy(EntityTreeModel::ImmediatePopulation);
    model->setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

    QTRY_VERIFY(model->isCollectionTreeFetched());
    QVERIFY(getIndex("col5", model).isValid());

    Akonadi::CollectionDeleteJob *deleteJob = new Akonadi::CollectionDeleteJob(col5);
    AKVERIFYEXEC(deleteJob);
}

#include "etmpopulationtest.moc"

QTEST_AKONADIMAIN(EtmPopulationTest, NoGUI)

