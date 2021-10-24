/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "monitor.h"

#include "akonaditestfake_export.h"
#include "fakeserverdata.h"
#include "fakesession.h"
#include "inspectablechangerecorder.h"
#include "inspectablemonitor.h"
#include <QSignalSpy>
#include <QTest>

using namespace Akonadi;

class MonitorNotificationTest : public QObject
{
    Q_OBJECT
public:
    explicit MonitorNotificationTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_sessionName = "MonitorNotificationTest fake session";
        m_fakeSession = new FakeSession(m_sessionName, FakeSession::EndJobsImmediately);
        m_fakeSession->setAsDefaultSession();
    }

    ~MonitorNotificationTest() override
    {
        delete m_fakeSession;
    }

private Q_SLOTS:
    void testSingleMessage();
    void testFillPipeline();
    void testMonitor();

    void testSingleMessage_data();
    void testFillPipeline_data();
    void testMonitor_data();

private:
    template<typename MonitorImpl> void testSingleMessage_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);
    template<typename MonitorImpl> void testFillPipeline_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);
    template<typename MonitorImpl> void testMonitor_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);

private:
    FakeSession *m_fakeSession = nullptr;
    QByteArray m_sessionName;
};

void MonitorNotificationTest::testSingleMessage_data()
{
    QTest::addColumn<bool>("useChangeRecorder");

    QTest::newRow("useChangeRecorder") << true;
    QTest::newRow("useMonitor") << false;
}

void MonitorNotificationTest::testSingleMessage()
{
    QFETCH(bool, useChangeRecorder);

    auto collectionCache = new FakeCollectionCache(m_fakeSession);
    FakeItemCache itemCache(m_fakeSession);
    auto depsFactory = new FakeMonitorDependenciesFactory(&itemCache, collectionCache);

    if (!useChangeRecorder) {
        testSingleMessage_impl(new InspectableMonitor(depsFactory, this), collectionCache, &itemCache);
    } else {
        auto changeRecorder = new InspectableChangeRecorder(depsFactory, this);
        changeRecorder->setChangeRecordingEnabled(false);
        testSingleMessage_impl(changeRecorder, collectionCache, &itemCache);
    }
}

template<typename MonitorImpl>
void MonitorNotificationTest::testSingleMessage_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
    Q_UNUSED(itemCache)

    // Workaround for the QTimer::singleShot() in fake monitors to happen
    QTest::qWait(10);

    monitor->setSession(m_fakeSession);
    monitor->fetchCollection(true);

    Protocol::ChangeNotificationList list;

    Collection parent(1);
    Collection added(2);

    auto msg = Protocol::CollectionChangeNotificationPtr::create();
    msg->setParentCollection(parent.id());
    msg->setOperation(Protocol::CollectionChangeNotification::Add);
    msg->setCollection(Protocol::FetchCollectionsResponse(added.id()));
    // With notification payloads most requests by-pass the pipeline as the
    // notification already contains everything. To force pipelineing we set
    // the internal metadata (normally set by ChangeRecorder)
    msg->addMetadata("FETCH_COLLECTION");

    QHash<Collection::Id, Collection> data;
    data.insert(parent.id(), parent);
    data.insert(added.id(), added);

    // Pending notifications remains empty because we don't fill the pipeline with one message.

    QVERIFY(monitor->pipeline().isEmpty());
    QVERIFY(monitor->pendingNotifications().isEmpty());

    monitor->notificationConnection()->emitNotify(msg);

    QTRY_COMPARE(monitor->pipeline().size(), 1);
    QVERIFY(monitor->pendingNotifications().isEmpty());

    collectionCache->setData(data);
    collectionCache->emitDataAvailable();

    QVERIFY(monitor->pipeline().isEmpty());
    QVERIFY(monitor->pendingNotifications().isEmpty());
}

void MonitorNotificationTest::testFillPipeline_data()
{
    QTest::addColumn<bool>("useChangeRecorder");

    QTest::newRow("useChangeRecorder") << true;
    QTest::newRow("useMonitor") << false;
}

void MonitorNotificationTest::testFillPipeline()
{
    QFETCH(bool, useChangeRecorder);

    auto collectionCache = new FakeCollectionCache(m_fakeSession);
    FakeItemCache itemCache(m_fakeSession);
    auto depsFactory = new FakeMonitorDependenciesFactory(&itemCache, collectionCache);

    if (!useChangeRecorder) {
        testFillPipeline_impl(new InspectableMonitor(depsFactory, this), collectionCache, &itemCache);
    } else {
        auto changeRecorder = new InspectableChangeRecorder(depsFactory, this);
        changeRecorder->setChangeRecordingEnabled(false);
        testFillPipeline_impl(changeRecorder, collectionCache, &itemCache);
    }
}

template<typename MonitorImpl>
void MonitorNotificationTest::testFillPipeline_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
    Q_UNUSED(itemCache)

    monitor->setSession(m_fakeSession);
    monitor->fetchCollection(true);

    Protocol::ChangeNotificationList list;
    QHash<Collection::Id, Collection> data;

    int i = 1;
    while (i < 40) {
        Collection parent(i++);
        Collection added(i++);

        auto msg = Protocol::CollectionChangeNotificationPtr::create();
        msg->setParentCollection(parent.id());
        msg->setOperation(Protocol::CollectionChangeNotification::Add);
        msg->setCollection(Protocol::FetchCollectionsResponse(added.id()));
        msg->addMetadata("FETCH_COLLECTION");

        data.insert(parent.id(), parent);
        data.insert(added.id(), added);

        list << msg;
    }

    QVERIFY(monitor->pipeline().isEmpty());
    QVERIFY(monitor->pendingNotifications().isEmpty());

    for (const Protocol::ChangeNotificationPtr &ntf : std::as_const(list)) {
        monitor->notificationConnection()->emitNotify(ntf);
    }

    QTRY_COMPARE(monitor->pipeline().size(), 5);
    QCOMPARE(monitor->pendingNotifications().size(), 15);

    collectionCache->setData(data);
    collectionCache->emitDataAvailable();

    QVERIFY(monitor->pipeline().isEmpty());
    QVERIFY(monitor->pendingNotifications().isEmpty());
}

void MonitorNotificationTest::testMonitor_data()
{
    QTest::addColumn<bool>("useChangeRecorder");

    QTest::newRow("useChangeRecorder") << true;
    QTest::newRow("useMonitor") << false;
}

void MonitorNotificationTest::testMonitor()
{
    QFETCH(bool, useChangeRecorder);

    auto collectionCache = new FakeCollectionCache(m_fakeSession);
    FakeItemCache itemCache(m_fakeSession);
    auto depsFactory = new FakeMonitorDependenciesFactory(&itemCache, collectionCache);

    if (!useChangeRecorder) {
        testMonitor_impl(new InspectableMonitor(depsFactory, this), collectionCache, &itemCache);
    } else {
        auto changeRecorder = new InspectableChangeRecorder(depsFactory, this);
        changeRecorder->setChangeRecordingEnabled(false);
        testMonitor_impl(changeRecorder, collectionCache, &itemCache);
    }
}

template<typename MonitorImpl>
void MonitorNotificationTest::testMonitor_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
    Q_UNUSED(itemCache)

    monitor->setSession(m_fakeSession);
    monitor->fetchCollection(true);

    Protocol::ChangeNotificationList list;

    Collection col2(2);
    col2.setParentCollection(Collection::root());

    collectionCache->insert(col2);

    int i = 4;

    while (i < 8) {
        Collection added(i++);

        auto msg = Protocol::CollectionChangeNotificationPtr::create();
        msg->setParentCollection(i % 2 ? 2 : added.id() - 1);
        msg->setOperation(Protocol::CollectionChangeNotification::Add);
        msg->setCollection(Protocol::FetchCollectionsResponse(added.id()));
        msg->addMetadata("FETCH_COLLECTION");

        list << msg;
    }

    QVERIFY(monitor->pipeline().isEmpty());
    QVERIFY(monitor->pendingNotifications().isEmpty());

    Collection col4(4);
    col4.setParentCollection(col2);
    Collection col6(6);
    col6.setParentCollection(col2);

    collectionCache->insert(col4);
    collectionCache->insert(col6);

    qRegisterMetaType<Akonadi::Collection>();
    QSignalSpy collectionAddedSpy(monitor, SIGNAL(collectionAdded(Akonadi::Collection, Akonadi::Collection)));

    collectionCache->emitDataAvailable();

    QTRY_VERIFY(monitor->pipeline().isEmpty());
    QVERIFY(monitor->pendingNotifications().isEmpty());

    for (const Protocol::ChangeNotificationPtr &ntf : std::as_const(list)) {
        monitor->notificationConnection()->emitNotify(ntf);
    }

    // Collection 6 is not notified, because Collection 5 has held up the pipeline
    QTRY_COMPARE(collectionAddedSpy.size(), 1);
    QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 4);
    QCOMPARE(monitor->pipeline().size(), 3);
    QCOMPARE(monitor->pendingNotifications().size(), 0);

    Collection col7(7);
    col7.setParentCollection(col6);

    collectionCache->insert(col7);
    collectionCache->emitDataAvailable();

    // Collection 5 is still holding the pipeline
    QCOMPARE(collectionAddedSpy.size(), 0);
    QCOMPARE(monitor->pipeline().size(), 3);
    QCOMPARE(monitor->pendingNotifications().size(), 0);

    Collection col5(5);
    col5.setParentCollection(col4);

    collectionCache->insert(col5);
    collectionCache->emitDataAvailable();

    // Collection 5 is in cache, pipeline is flushed
    QCOMPARE(collectionAddedSpy.size(), 3);
    QCOMPARE(monitor->pipeline().size(), 0);
    QCOMPARE(monitor->pendingNotifications().size(), 0);
}

QTEST_MAIN(MonitorNotificationTest)
#include "monitornotificationtest.moc"
