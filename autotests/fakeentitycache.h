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

#ifndef FAKEENTITYCACHE_H
#define FAKEENTITYCACHE_H

#include "monitor_p.h"
#include "notificationsource_p.h"

#include "collectionfetchscope.h"
#include "itemfetchscope.h"

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

  void insert(T t)
  {
    m_data.insert(t.id(), t);
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

class FakeNotificationSource : public QObject
{
  Q_OBJECT
public:
  FakeNotificationSource(QObject *parent = 0)
    : QObject(parent)
  {
  }

  void emitNotify(const Akonadi::NotificationMessageV3::List &msgs ) { notifyV3(msgs); }

public Q_SLOTS:
  void setAllMonitored( bool allMonitored )
  {
    Q_UNUSED( allMonitored )
  }
  void setMonitoredCollection( qlonglong id, bool monitored )
  {
    Q_UNUSED( id )
    Q_UNUSED( monitored )
  }
  void setMonitoredItem( qlonglong id, bool monitored )
  {
    Q_UNUSED( id )
    Q_UNUSED( monitored )
  }
  void setMonitoredResource( const QByteArray &resource, bool monitored )
  {
    Q_UNUSED( resource )
    Q_UNUSED( monitored )
  }
  void setMonitoredMimeType( const QString &mimeType, bool monitored )
  {
    Q_UNUSED( mimeType )
    Q_UNUSED( monitored )
  }
  void setIgnoredSession( const QByteArray &session, bool ignored )
  {
    Q_UNUSED( session )
    Q_UNUSED( ignored )
  }

Q_SIGNALS:
  void notifyV3( const Akonadi::NotificationMessageV3::List &msgs );
};

class FakeMonitorDependeciesFactory : public Akonadi::ChangeNotificationDependenciesFactory
{
public:

  FakeMonitorDependeciesFactory(FakeItemCache* itemCache_, FakeCollectionCache* collectionCache_)
    : Akonadi::ChangeNotificationDependenciesFactory(), itemCache(itemCache_), collectionCache(collectionCache_)
  {

  }

  virtual Akonadi::NotificationSource* createNotificationSource(QObject *parent)
  {
    return new Akonadi::NotificationSource( new FakeNotificationSource( parent ) );
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

#endif
