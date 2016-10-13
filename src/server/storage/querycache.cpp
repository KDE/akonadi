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

using namespace Akonadi::Server;

// After these seconds without activity the cache is cleaned
#define CLEANUP_TIMEOUT 30 // seconds

class Cache : public QObject
{
    Q_OBJECT
public:

    Cache()
    {
        connect(&m_cleanupTimer, &QTimer::timeout, this, &Cache::cleanup);
        m_cleanupTimer.setSingleShot(true);
    }

    QSqlQuery query(const QString &queryStatement)
    {
        m_cleanupTimer.start(CLEANUP_TIMEOUT * 1000);
        return m_cache.value(queryStatement);
    }

public Q_SLOTS:
    void cleanup()
    {
        m_cache.clear();
    }

public: // public, this is just a helper class
    QHash<QString, QSqlQuery> m_cache;
    QTimer m_cleanupTimer;
};

static QThreadStorage<Cache *> g_queryCache;

static Cache *perThreadCache()
{
    if (!g_queryCache.hasLocalData()) {
        g_queryCache.setLocalData(new Cache());
    }

    return g_queryCache.localData();
}

bool QueryCache::contains(const QString &queryStatement)
{
    if (DbType::type(DataStore::self()->database()) == DbType::Sqlite) {
        return false;
    } else {
        return perThreadCache()->m_cache.contains(queryStatement);
    }
}

QSqlQuery QueryCache::query(const QString &queryStatement)
{
    return perThreadCache()->query(queryStatement);
}

void QueryCache::insert(const QString &queryStatement, const QSqlQuery &query)
{
    if (DbType::type(DataStore::self()->database()) != DbType::Sqlite) {
        perThreadCache()->m_cache.insert(queryStatement, query);
    }
}

void QueryCache::clear()
{
    if (!g_queryCache.hasLocalData()) {
        return;
    }

    g_queryCache.localData()->cleanup();
}

#include <querycache.moc>
