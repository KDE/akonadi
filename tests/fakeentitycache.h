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

#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchscope.h>

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

  void emitNotify(const Akonadi::NotificationMessageV2::List &msgs ) { notifyV2(msgs); }

signals:
  void notifyV2( const Akonadi::NotificationMessageV2::List &msgs );
};

class FakeMonitorDependeciesFactory : public Akonadi::ChangeNotificationDependenciesFactory
{
public:

  FakeMonitorDependeciesFactory(FakeItemCache* itemCache_, FakeCollectionCache* collectionCache_)
    : Akonadi::ChangeNotificationDependenciesFactory(), itemCache(itemCache_), collectionCache(collectionCache_)
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

#endif
