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
#include "akonaditestfake_export.h"
#include "private/protocol_p.h"


template<typename T, typename Cache>
class FakeEntityCache : public Cache
{
public:
    FakeEntityCache(Akonadi::Session *session = nullptr, QObject *parent = nullptr)
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

    void emitDataAvailable()
    {
        emit Cache::dataAvailable();
    }

    T retrieve(typename T::Id id) const override
    {
        return m_data.value(id);
    }

    void request(typename T::Id id, const typename Cache::FetchScope &scope) override
    {
        Q_UNUSED(id)
        Q_UNUSED(scope)
    }

    bool ensureCached(typename T::Id id, const typename Cache::FetchScope &scope) override
    {
        Q_UNUSED(scope)
        return m_data.contains(id);
    }

private:
    QHash<typename T::Id, T> m_data;
};
typedef FakeEntityCache<Akonadi::Collection, Akonadi::CollectionCache> FakeCollectionCache;
typedef FakeEntityCache<Akonadi::Item, Akonadi::ItemCache> FakeItemCache;

class AKONADITESTFAKE_EXPORT FakeNotificationSource : public QObject
{
    Q_OBJECT
public:
    explicit FakeNotificationSource(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

public Q_SLOTS:
    void setAllMonitored(bool allMonitored)
    {
        Q_UNUSED(allMonitored)
    }
    void setMonitoredCollection(qlonglong id, bool monitored)
    {
        Q_UNUSED(id)
        Q_UNUSED(monitored)
    }
    void setMonitoredItem(qlonglong id, bool monitored)
    {
        Q_UNUSED(id)
        Q_UNUSED(monitored)
    }
    void setMonitoredResource(const QByteArray &resource, bool monitored)
    {
        Q_UNUSED(resource)
        Q_UNUSED(monitored)
    }
    void setMonitoredMimeType(const QString &mimeType, bool monitored)
    {
        Q_UNUSED(mimeType)
        Q_UNUSED(monitored)
    }
    void setIgnoredSession(const QByteArray &session, bool ignored)
    {
        Q_UNUSED(session)
        Q_UNUSED(ignored)
    }

    void setSession(const QByteArray &session)
    {
        Q_UNUSED(session);
    }
};

class AKONADITESTFAKE_EXPORT FakeNotificationConnection : public Akonadi::Connection
{
    Q_OBJECT

public:
    explicit FakeNotificationConnection(Akonadi::Session *session, Akonadi::CommandBuffer *buffer)
        : Connection(Connection::NotificationConnection, "", buffer, session)
    {}

    virtual ~FakeNotificationConnection()
    {}

    void emitNotify(const Akonadi::Protocol::ChangeNotificationPtr &ntf)
    {
        Q_EMIT commandReceived(3, ntf);
    }

    /*
Q_SIGNALS:
    void notify(const Akonadi::Protocol::ChangeNotification &ntf);
    */
};

class FakeMonitorDependeciesFactory : public Akonadi::ChangeNotificationDependenciesFactory
{
public:

    FakeMonitorDependeciesFactory(FakeItemCache *itemCache_, FakeCollectionCache *collectionCache_)
        : Akonadi::ChangeNotificationDependenciesFactory()
        , itemCache(itemCache_)
        , collectionCache(collectionCache_)
    {
    }

    Akonadi::Connection *createNotificationConnection(Akonadi::Session *parent,
                                                      Akonadi::CommandBuffer *buffer) override {
        return new FakeNotificationConnection(parent, buffer);
    }

    Akonadi::CollectionCache *createCollectionCache(int maxCapacity, Akonadi::Session *session) override {
        Q_UNUSED(maxCapacity)
        Q_UNUSED(session)
        return collectionCache;
    }

    Akonadi::ItemCache *createItemCache(int maxCapacity, Akonadi::Session *session) override {
        Q_UNUSED(maxCapacity)
        Q_UNUSED(session)
        return itemCache;
    }

private:
    FakeItemCache *itemCache = nullptr;
    FakeCollectionCache *collectionCache = nullptr;

};

#endif
