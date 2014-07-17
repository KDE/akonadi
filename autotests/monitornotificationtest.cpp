/*
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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


#include <akonadi/monitor.h>

#include "fakeserverdata.h"
#include "fakesession.h"
#include "inspectablemonitor.h"
#include "inspectablechangerecorder.h"

using namespace Akonadi;

class MonitorNotificationTest : public QObject
{
  Q_OBJECT
public:
  MonitorNotificationTest(QObject *parent = 0)
    : QObject(parent)
  {
    m_sessionName = "MonitorNotificationTest fake session";
    m_fakeSession = new FakeSession( m_sessionName, FakeSession::EndJobsImmediately);
    m_fakeSession->setAsDefaultSession();
  }

private Q_SLOTS:
  void testSingleMessage();
  void testFillPipeline();
  void testMonitor();

  void testSingleMessage_data();
  void testFillPipeline_data();
  void testMonitor_data();

private:
  template<typename MonitorImpl>
  void testSingleMessage_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);
  template<typename MonitorImpl>
  void testFillPipeline_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);
  template<typename MonitorImpl>
  void testMonitor_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);

private:
  FakeSession *m_fakeSession;
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

  FakeCollectionCache *collectionCache = new FakeCollectionCache(m_fakeSession);
  FakeItemCache *itemCache = new FakeItemCache(m_fakeSession);
  FakeMonitorDependeciesFactory *depsFactory = new FakeMonitorDependeciesFactory(itemCache, collectionCache);

  if (!useChangeRecorder)
  {
    testSingleMessage_impl(new InspectableMonitor(depsFactory, this), collectionCache, itemCache);
  } else {
    InspectableChangeRecorder *changeRecorder = new InspectableChangeRecorder(depsFactory, this);
    changeRecorder->setChangeRecordingEnabled(false);
    testSingleMessage_impl(changeRecorder, collectionCache, itemCache);
  }
}

template <typename MonitorImpl>
void MonitorNotificationTest::testSingleMessage_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
  Q_UNUSED(itemCache)

  monitor->setSession(m_fakeSession);
  monitor->fetchCollection(true);

  NotificationMessageV3::List list;

  Collection parent(1);
  Collection added(2);

  NotificationMessageV3 msg;
  msg.setParentCollection(parent.id());
  msg.setOperation(Akonadi::NotificationMessageV2::Add);
  msg.setType(Akonadi::NotificationMessageV2::Collections);
  msg.addEntity( added.id() );

  QHash<Collection::Id, Collection> data;
  data.insert(parent.id(), parent);
  data.insert(added.id(), added);

  list << msg;

  // Pending notifications remains empty because we don't fill the pipeline with one message.

  QVERIFY(monitor->pipeline().isEmpty());
  QVERIFY(monitor->pendingNotifications().isEmpty());

  monitor->notifier()->emitNotify(list);

  QCOMPARE(monitor->pipeline().size(), 1);
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

  FakeCollectionCache *collectionCache = new FakeCollectionCache(m_fakeSession);
  FakeItemCache *itemCache = new FakeItemCache(m_fakeSession);
  FakeMonitorDependeciesFactory *depsFactory = new FakeMonitorDependeciesFactory(itemCache, collectionCache);

  if (!useChangeRecorder)
  {
    testFillPipeline_impl(new InspectableMonitor(depsFactory, this), collectionCache, itemCache);
  } else {
    InspectableChangeRecorder *changeRecorder = new InspectableChangeRecorder(depsFactory, this);
    changeRecorder->setChangeRecordingEnabled(false);
    testFillPipeline_impl(changeRecorder, collectionCache, itemCache);
  }
}

template <typename MonitorImpl>
void MonitorNotificationTest::testFillPipeline_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
  Q_UNUSED(itemCache)

  monitor->setSession(m_fakeSession);
  monitor->fetchCollection(true);

  NotificationMessageV3::List list;
  QHash<Collection::Id, Collection> data;

  int i = 1;
  while (i < 40) {
    Collection parent(i++);
    Collection added(i++);

    NotificationMessageV3 msg;
    msg.setParentCollection(parent.id());
    msg.setOperation(Akonadi::NotificationMessageV2::Add);
    msg.setType(Akonadi::NotificationMessageV2::Collections);
    msg.addEntity( added.id() );

    data.insert(parent.id(), parent);
    data.insert(added.id(), added);

    list << msg;
  }

  QVERIFY(monitor->pipeline().isEmpty());
  QVERIFY(monitor->pendingNotifications().isEmpty());

  monitor->notifier()->emitNotify(list);

  QCOMPARE(monitor->pipeline().size(), 5);
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

  FakeCollectionCache *collectionCache = new FakeCollectionCache(m_fakeSession);
  FakeItemCache *itemCache = new FakeItemCache(m_fakeSession);
  FakeMonitorDependeciesFactory *depsFactory = new FakeMonitorDependeciesFactory(itemCache, collectionCache);

  if (!useChangeRecorder) {
    testMonitor_impl(new InspectableMonitor(depsFactory, this), collectionCache, itemCache);
  } else {
    InspectableChangeRecorder *changeRecorder = new InspectableChangeRecorder(depsFactory, this);
    changeRecorder->setChangeRecordingEnabled(false);
    testMonitor_impl(changeRecorder, collectionCache, itemCache);
  }
}

template <typename MonitorImpl>
void MonitorNotificationTest::testMonitor_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
  Q_UNUSED(itemCache)

  monitor->setSession(m_fakeSession);
  monitor->fetchCollection(true);

  NotificationMessageV3::List list;

  Collection col2(2);
  col2.setParentCollection(Collection::root());

  collectionCache->insert(col2);

  int i = 4;

  while (i < 8) {
    Collection added(i++);

    NotificationMessageV3 msg;
    msg.setParentCollection( i % 2 ? 2 : added.id() - 1);
    msg.setOperation(Akonadi::NotificationMessageV2::Add);
    msg.setType(Akonadi::NotificationMessageV2::Collections);
    msg.addEntity( added.id() );

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
  QSignalSpy collectionAddedSpy(monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)));

  collectionCache->emitDataAvailable();

  QVERIFY(monitor->pipeline().isEmpty());
  QVERIFY(monitor->pendingNotifications().isEmpty());

  monitor->notifier()->emitNotify(list);

  // Collection 6 is not notified, because Collection 5 has held up the pipeline
  QCOMPARE(collectionAddedSpy.size(), 1);
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

QTEST_MAIN( MonitorNotificationTest )
#include "monitornotificationtest.moc"
