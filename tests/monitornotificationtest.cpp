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

using namespace Akonadi;

class MonitorNotificationTest : public QObject
{
  Q_OBJECT
public:
  MonitorNotificationTest(QObject *parent = 0)
    : QObject(parent)
  {
    m_sessionName = "MonitorNotificationTest fake session";
    m_fakeSession = new FakeSession( m_sessionName, this);
  }

private Q_SLOTS:
  void testSingleMessage();
  void testFillPipeline();

private:
  FakeSession *m_fakeSession;
  QByteArray m_sessionName;
};

void MonitorNotificationTest::testSingleMessage()
{
  FakeCollectionCache *collectionCache = new FakeCollectionCache(m_fakeSession);
  FakeItemCache *itemCache = new FakeItemCache(m_fakeSession);
  FakeMonitorDependeciesFactory *depsFactory = new FakeMonitorDependeciesFactory(itemCache, collectionCache);
  InspectableMonitor *monitor = new InspectableMonitor(depsFactory, this);
  monitor->setSession(m_fakeSession);
  monitor->fetchCollection(true);
  monitor->setAllMonitored(true);

  monitor->setCollectionMonitored(Collection::root());

  NotificationMessage::List list;

  Collection parent(1);
  Collection added(2);

  NotificationMessage msg;
  msg.setParentCollection(parent.id());
  msg.setOperation(Akonadi::NotificationMessage::Add);
  msg.setType(Akonadi::NotificationMessage::Collection);
  msg.setUid(added.id());

  QHash<Collection::Id, Collection> data;
  data.insert(parent.id(), parent);
  data.insert(added.id(), added);

  list << msg;

  // Pending notifications remains empty because we don't fill the pipeline with one message.

  Q_ASSERT(monitor->pipeline().isEmpty());
  Q_ASSERT(monitor->pendingNotifications().isEmpty());

  monitor->notifier()->emitNotify(list);

  Q_ASSERT(monitor->pipeline().size() == 1);
  Q_ASSERT(monitor->pendingNotifications().isEmpty());

  collectionCache->setData(data);
  collectionCache->emitDataAvailable();

  Q_ASSERT(monitor->pipeline().isEmpty());
  Q_ASSERT(monitor->pendingNotifications().isEmpty());
}

void MonitorNotificationTest::testFillPipeline()
{
  FakeCollectionCache *collectionCache = new FakeCollectionCache(m_fakeSession);
  FakeItemCache *itemCache = new FakeItemCache(m_fakeSession);
  FakeMonitorDependeciesFactory *depsFactory = new FakeMonitorDependeciesFactory(itemCache, collectionCache);
  InspectableMonitor *monitor = new InspectableMonitor(depsFactory, this);
  monitor->setSession(m_fakeSession);
  monitor->fetchCollection(true);
  monitor->setAllMonitored(true);

  monitor->setCollectionMonitored(Collection::root());

  NotificationMessage::List list;
  QHash<Collection::Id, Collection> data;

  int i = 1;
  while (i < 40) {
    Collection parent(i++);
    Collection added(i++);

    NotificationMessage msg;
    msg.setParentCollection(parent.id());
    msg.setOperation(Akonadi::NotificationMessage::Add);
    msg.setType(Akonadi::NotificationMessage::Collection);
    msg.setUid(added.id());

    data.insert(parent.id(), parent);
    data.insert(added.id(), added);

    list << msg;
  }

  Q_ASSERT(monitor->pipeline().isEmpty());
  Q_ASSERT(monitor->pendingNotifications().isEmpty());

  monitor->notifier()->emitNotify(list);

  Q_ASSERT(monitor->pipeline().size() == 5);
  Q_ASSERT(monitor->pendingNotifications().size() == 15);

  collectionCache->setData(data);
  collectionCache->emitDataAvailable();

  Q_ASSERT(monitor->pipeline().isEmpty());
  Q_ASSERT(monitor->pendingNotifications().isEmpty());
}

QTEST_MAIN( MonitorNotificationTest )
#include "monitornotificationtest.moc"
