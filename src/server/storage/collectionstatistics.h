/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

class QMutex;

#include <QHash>
#include <QMutex>

namespace Akonadi
{
namespace Server
{
class QueryBuilder;
class Collection;

/**
 * Provides cache for collection statistics
 *
 * Collection statistics are requested very often, so to take some load from the
 * database we cache the results until the statistics are invalidated (see
 * NotificationCollector, which takes care for invalidating the statistics).
 *
 * The cache (together with optimization of the actual SQL query) seems to
 * massively improve initial folder listing on system start (when IO and CPU loads
 * are very high).
 */
class CollectionStatistics
{
public:
    struct Statistics {
        qint64 count;
        qint64 size;
        qint64 read;
    };

    explicit CollectionStatistics(bool prefetch = true);
    virtual ~CollectionStatistics() = default;

    Statistics statistics(const Collection &col);

    void itemAdded(const Collection &col, qint64 size, bool seen);
    void itemsSeenChanged(const Collection &col, qint64 seenCount);

    void invalidateCollection(const Collection &col);

    void expireCache();

protected:
    QueryBuilder prepareGenericQuery();

    virtual Statistics calculateCollectionStatistics(const Collection &col);

    QMutex mCacheLock;
    QHash<qint64, Statistics> mCache;
};

} // namespace Server
} // namespace Akonadi

