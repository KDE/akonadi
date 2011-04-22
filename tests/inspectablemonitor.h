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

#ifndef INSPECTABLEMONITOR_H
#define INSPECTABLEMONITOR_H

#include "entitycache_p.h"
#include "monitor.h"
#include "monitor_p.h"

#include "fakeakonadiservercommand.h"

template<typename T, typename Cache>
class FakeEntityCache : public Cache
{
public:
  FakeEntityCache(Akonadi::Session *session = 0, QObject *parent = 0 )
    : Cache(0, session, parent)
  {

  }

  void setData(const QHash<typename T::Id, T> &data)
  {
    m_data = data;
  }

  void emitDataAvailable() { emit Cache::dataAvailable(); }

  T retrieve( typename T::Id id ) const
  {
    return m_data.value(id);
  }

  void request( typename T::Id id, const typename Cache::FetchScope &scope )
  {
    Q_UNUSED(id)
    Q_UNUSED(scope)
  }

  bool ensureCached(typename T::Id id, const typename Cache::FetchScope &scope )
  {
    Q_UNUSED(scope)
    return m_data.contains(id);
  }

private:
  QHash<typename T::Id, T> m_data;
};
typedef FakeEntityCache<Akonadi::Collection, Akonadi::CollectionCache> FakeCollectionCache;
typedef FakeEntityCache<Akonadi::Item, Akonadi::ItemCache> FakeItemCache;

class InspectableMonitor;

class FakeNotificationSource : public QObject
{
  Q_OBJECT
public:
  FakeNotificationSource(QObject *parent = 0);

  void emitNotify(const Akonadi::NotificationMessage::List &msgs ) { notify(msgs); }

signals:
  void notify( const Akonadi::NotificationMessage::List &msgs );
};

class FakeMonitorDependeciesFactory : public Akonadi::MonitorDependeciesFactory
{
public:

  FakeMonitorDependeciesFactory(FakeItemCache* itemCache_, FakeCollectionCache* collectionCache_)
    : Akonadi::MonitorDependeciesFactory(), itemCache(itemCache_), collectionCache(collectionCache_)
  {

  }

  virtual QObject* createNotificationSource(QObject *parent)
  {
    return new FakeNotificationSource(parent);
  }

  virtual Akonadi::CollectionCache* createCollectionCache(int maxCapacity, Akonadi::Session *session)
  {
    Q_UNUSED(maxCapacity)
    Q_UNUSED(session)
    return collectionCache;
  }

  virtual Akonadi::ItemCache* createItemCache(int maxCapacity, Akonadi::Session *session)
  {
    Q_UNUSED(maxCapacity)
    Q_UNUSED(session)
    return itemCache;
  }

private:
  FakeItemCache* itemCache;
  FakeCollectionCache* collectionCache;

};

class InspectableMonitorPrivate : public Akonadi::MonitorPrivate
{
public:
  InspectableMonitorPrivate(FakeMonitorDependeciesFactory *dependenciesFactory, InspectableMonitor* parent);
  ~InspectableMonitorPrivate() {}

  virtual bool emitNotification( const Akonadi::NotificationMessage& msg )
  {
    // TODO: Check/Log
    return Akonadi::MonitorPrivate::emitNotification(msg);
  }
};

class InspectableMonitor : public Akonadi::Monitor
{
  Q_OBJECT
public:
  InspectableMonitor(FakeMonitorDependeciesFactory *dependenciesFactory, QObject *parent = 0);

  FakeNotificationSource* notifier() const {
    return qobject_cast<FakeNotificationSource*>(d_ptr->notificationSource);
  }

  QQueue<Akonadi::NotificationMessage> pendingNotifications() const { return d_ptr->pendingNotifications; }
  QQueue<Akonadi::NotificationMessage> pipeline() const { return d_ptr->pipeline; }

signals:
  void dummySignal();

private slots:
  void dispatchNotifications()
  {
    d_ptr->dispatchNotifications();
  }

  void doConnectToNotificationManager();

private:
  struct MessageStruct {
    enum Position {
      Queued,
      FilterPipelined,
      Pipelined,
      Emitted
    };
    Position position;
  };
  QQueue<MessageStruct> m_messages;
};

#endif
