/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "querycache.h"
#include "datastore.h"
#include "dbtype.h"

#include <QHash>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QThreadStorage>
#include <QTimer>

#include <chrono>
#include <list>

using namespace std::chrono_literals;
using namespace Akonadi;
using namespace Akonadi::Server;

namespace
{
// After these seconds without activity the cache is cleaned
constexpr auto CleanupTimeout = 60s;
constexpr int MaxCacheSize = 50;

/// LRU cache with limited size and auto-cleanup after given
/// period of time
class Cache
{
public:
    Cache()
    {
        QObject::connect(&m_cleanupTimer, &QTimer::timeout, &m_cleanupTimer, std::bind(&Cache::cleanup, this));
        m_cleanupTimer.setSingleShot(true);
    }

    std::optional<QSqlQuery> query(const QString &queryStatement)
    {
        m_cleanupTimer.start(CleanupTimeout);
        auto it = m_keys.find(queryStatement);
        if (it == m_keys.end()) {
            return {};
        }

        auto query = std::move((*it)->query);
        m_queries.erase(*it);
        m_keys.erase(it);

        return query;
    }

    void insert(const QString &queryStatement, QSqlQuery query)
    {
        if (m_queries.size() >= MaxCacheSize) {
            m_keys.remove(queryStatement);
            m_queries.pop_back();
        }

        m_queries.emplace_front(std::move(query));
        m_keys.insert(queryStatement, m_queries.begin());
    }

    void cleanup()
    {
        m_keys.clear();
        m_queries.clear();
    }

public: // public, this is just a helper class
    struct Node {
        QSqlQuery query;
    };
    std::list<Node> m_queries;
    QHash<QString, std::list<Node>::iterator> m_keys;
    QTimer m_cleanupTimer;
};

QThreadStorage<Cache *> g_queryCache;

Cache *perThreadCache()
{
    if (!g_queryCache.hasLocalData()) {
        g_queryCache.setLocalData(new Cache());
    }

    return g_queryCache.localData();
}

} // namespace

std::optional<QSqlQuery> QueryCache::query(const QString &queryStatement)
{
    return perThreadCache()->query(queryStatement);
}

void QueryCache::insert(const QSqlDatabase &db, const QString &queryStatement, QSqlQuery query)
{
    if (DbType::type(db) != DbType::Sqlite) {
        perThreadCache()->insert(queryStatement, std::move(query));
    }
}

void QueryCache::clear()
{
    if (!g_queryCache.hasLocalData()) {
        return;
    }

    g_queryCache.localData()->cleanup();
}
