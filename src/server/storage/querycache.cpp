/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#include "querycache.h"
#include "dbtype.h"
#include "datastore.h"

#include <QSqlQuery>
#include <QThreadStorage>
#include <QHash>
#include <QTimer>

#include <chrono>
#include <list>

using namespace std::chrono_literals;
using namespace Akonadi;
using namespace Akonadi::Server;

namespace {

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
        QObject::connect(&m_cleanupTimer, &QTimer::timeout, std::bind(&Cache::cleanup, this));
        m_cleanupTimer.setSingleShot(true);
    }

    std::optional<QSqlQuery> query(const QString &queryStatement)
    {
        m_cleanupTimer.start(CleanupTimeout);
        auto it = m_keys.find(queryStatement);
        if (it == m_keys.end()) {
            return std::nullopt;
        }

        auto node = **it;
        m_queries.erase(*it);
        m_queries.push_front(node);
        *it = m_queries.begin();
        return node.query;
    }

    void insert(const QString &queryStatement, const QSqlQuery &query)
    {
        if (m_queries.size() >= MaxCacheSize) {
            m_keys.remove(m_queries.back().queryStatement);
            m_queries.pop_back();
        }

        m_queries.emplace_front(Node{queryStatement, query});
        m_keys.insert(queryStatement, m_queries.begin());
    }

    void cleanup()
    {
        m_keys.clear();
        m_queries.clear();
    }

public: // public, this is just a helper class
    struct Node {
        QString queryStatement;
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

void QueryCache::insert(const QString &queryStatement, const QSqlQuery &query)
{
    if (DbType::type(DataStore::self()->database()) != DbType::Sqlite) {
        perThreadCache()->insert(queryStatement, query);
    }
}

void QueryCache::clear()
{
    if (!g_queryCache.hasLocalData()) {
        return;
    }

    g_queryCache.localData()->cleanup();
}

