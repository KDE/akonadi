/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonaditestfake_export.h"
#include "collectionfetchscope.h"
#include "itemfetchscope.h"
#include "monitor_p.h"
#include "notificationsource_p.h"
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
        Q_EMIT Cache::dataAvailable();
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
using FakeCollectionCache = FakeEntityCache<Akonadi::Collection, Akonadi::CollectionCache>;
using FakeItemCache = FakeEntityCache<Akonadi::Item, Akonadi::ItemCache>;

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
        Q_UNUSED(session)
    }
};

class AKONADITESTFAKE_EXPORT FakeNotificationConnection : public Akonadi::Connection
{
    Q_OBJECT

public:
    explicit FakeNotificationConnection(Akonadi::CommandBuffer *buffer)
        : Connection(Connection::NotificationConnection, "", buffer)
        , mBuffer(buffer)
    {
    }

    ~FakeNotificationConnection() override
    {
    }

    void emitNotify(const Akonadi::Protocol::ChangeNotificationPtr &ntf)
    {
        Akonadi::CommandBufferLocker locker(mBuffer);
        mBuffer->enqueue(3, ntf);
    }

private:
    Akonadi::CommandBuffer *mBuffer;
};

class FakeMonitorDependenciesFactory : public Akonadi::ChangeNotificationDependenciesFactory
{
public:
    FakeMonitorDependenciesFactory(FakeItemCache *itemCache_, FakeCollectionCache *collectionCache_)
        : Akonadi::ChangeNotificationDependenciesFactory()
        , itemCache(itemCache_)
        , collectionCache(collectionCache_)
    {
    }

    Akonadi::Connection *createNotificationConnection(Akonadi::Session *parent, Akonadi::CommandBuffer *buffer) override
    {
        auto conn = new FakeNotificationConnection(buffer);
        addConnection(parent, conn);
        return conn;
    }

    void destroyNotificationConnection(Akonadi::Session *parent, Akonadi::Connection *connection) override
    {
        Q_UNUSED(parent)
        delete connection;
    }

    Akonadi::CollectionCache *createCollectionCache(int maxCapacity, Akonadi::Session *session) override
    {
        Q_UNUSED(maxCapacity)
        Q_UNUSED(session)
        return collectionCache;
    }

    Akonadi::ItemCache *createItemCache(int maxCapacity, Akonadi::Session *session) override
    {
        Q_UNUSED(maxCapacity)
        Q_UNUSED(session)
        return itemCache;
    }

private:
    FakeItemCache *itemCache = nullptr;
    FakeCollectionCache *collectionCache = nullptr;
};
