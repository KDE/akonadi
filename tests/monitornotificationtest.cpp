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

#include <QtTest>

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
  void testMonitorNonRoot();

  void testSingleMessage_data();
  void testFillPipeline_data();
  void testMonitorNonRoot_data();

private:
  template<typename MonitorImpl>
  void testSingleMessage_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);
  template<typename MonitorImpl>
  void testFillPipeline_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);
  template<typename MonitorImpl>
  void testMonitorNonRoot_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache);

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
  monitor->setAllMonitored(true);

  monitor->setCollectionMonitored(Collection::root());

  NotificationMessageV2::List list;

  Collection parent(1);
  Collection added(2);

  NotificationMessageV2 msg;
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
  monitor->setAllMonitored(true);

  monitor->setCollectionMonitored(Collection::root());

  NotificationMessageV2::List list;
  QHash<Collection::Id, Collection> data;

  int i = 1;
  while (i < 40) {
    Collection parent(i++);
    Collection added(i++);

    NotificationMessageV2 msg;
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

void MonitorNotificationTest::testMonitorNonRoot_data()
{
  QTest::addColumn<bool>("useChangeRecorder");

  QTest::newRow("useChangeRecorder") << true;
  QTest::newRow("useMonitor") << false;
}

void MonitorNotificationTest::testMonitorNonRoot()
{
  QFETCH(bool, useChangeRecorder);

  FakeCollectionCache *collectionCache = new FakeCollectionCache(m_fakeSession);
  FakeItemCache *itemCache = new FakeItemCache(m_fakeSession);
  FakeMonitorDependeciesFactory *depsFactory = new FakeMonitorDependeciesFactory(itemCache, collectionCache);

  if (!useChangeRecorder)
  {
    testMonitorNonRoot_impl(new InspectableMonitor(depsFactory, this), collectionCache, itemCache);
  } else {
    InspectableChangeRecorder *changeRecorder = new InspectableChangeRecorder(depsFactory, this);
    changeRecorder->setChangeRecordingEnabled(false);
    testMonitorNonRoot_impl(changeRecorder, collectionCache, itemCache);
  }
}

template <typename MonitorImpl>
void MonitorNotificationTest::testMonitorNonRoot_impl(MonitorImpl *monitor, FakeCollectionCache *collectionCache, FakeItemCache *itemCache)
{
  Q_UNUSED(itemCache)

  monitor->setSession(m_fakeSession);
  monitor->fetchCollection(true);

  monitor->setCollectionMonitored(Collection(2));

  NotificationMessageV2::List list;

  Collection col2(2);
  col2.setParentCollection(Collection::root());

  collectionCache->insert(col2);

  int i = 4;

  while (i < 10) {
    Collection added(i++);

    NotificationMessageV2 msg;
    msg.setParentCollection(added.id() % 2 == 0 ? 2 : added.id() - 1);
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

  QCOMPARE(collectionAddedSpy.size(), 2);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 4);
  // 5 is filtered out because we don't monitor descendants.
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 6);
  QCOMPARE(monitor->pipeline().size(), 1);
  QCOMPARE(monitor->pendingNotifications().size(), 0);

  Collection col7(7);
  col7.setParentCollection(col6);

  collectionCache->insert(col7);
  collectionCache->emitDataAvailable();

  QCOMPARE(collectionAddedSpy.size(), 0);
  QCOMPARE(monitor->pipeline().size(), 1);
  QCOMPARE(monitor->pendingNotifications().size(), 0);

  Collection col5(5);
  col5.setParentCollection(col4);

  collectionCache->insert(col5);
  collectionCache->emitDataAvailable();

  QCOMPARE(collectionAddedSpy.size(), 0);
  QCOMPARE(monitor->pipeline().size(), 1);
  QCOMPARE(monitor->pendingNotifications().size(), 0);

  list.clear();

  QHash<Collection::Id, Collection> extraCollections;

  while (i < 20) {
    Collection added(i++);

    NotificationMessageV2 msg;
    msg.setParentCollection(added.id() % 2 == 0 ? 2 : added.id() - 1);
    msg.setOperation(Akonadi::NotificationMessageV2::Add);
    msg.setType(Akonadi::NotificationMessageV2::Collections);
    msg.addEntity( added.id() );

    added.setParentCollection(added.id() % 2 == 0 ? col2 : extraCollections.value(added.id() - 1));

    extraCollections.insert(added.id(), added);

    list << msg;
  }
  monitor->notifier()->emitNotify(list);

  QCOMPARE(collectionAddedSpy.size(), 0);
  QCOMPARE(monitor->pipeline().size(), 5);
  QCOMPARE(monitor->pendingNotifications().size(), 1);

  Collection col8(8);
  col8.setParentCollection(col2);

  collectionCache->insert(col8);
  collectionCache->emitDataAvailable();

  QCOMPARE(collectionAddedSpy.size(), 1);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 8);
  QCOMPARE(monitor->pipeline().size(), 5);
  QCOMPARE(monitor->pendingNotifications().size(), 0);

  Collection col9(9);
  col9.setParentCollection(col8);
  collectionCache->insert(col9);
  collectionCache->emitDataAvailable();

  QCOMPARE(collectionAddedSpy.size(), 0);
  QCOMPARE(monitor->pipeline().size(), 5);
  QCOMPARE(monitor->pendingNotifications().size(), 0);

  {
    QHash<Collection::Id, Collection>::const_iterator it = extraCollections.constBegin();
    const QHash<Collection::Id, Collection>::const_iterator end = extraCollections.constEnd();

    for (; it != end; ++it)
      collectionCache->insert(*it);
    collectionCache->emitDataAvailable();
  }
  QCOMPARE(collectionAddedSpy.size(), 5);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 10);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 12);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 14);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 16);
  QCOMPARE((int)collectionAddedSpy.takeFirst().first().value<Akonadi::Collection>().id(), 18);

  QCOMPARE(collectionAddedSpy.size(), 0);
  QVERIFY(monitor->pipeline().isEmpty());
  QVERIFY(monitor->pendingNotifications().isEmpty());

}

QTEST_MAIN( MonitorNotificationTest )
#include "monitornotificationtest.moc"
