/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_SERVER_COLLECTIONSTATISTICS_H
#define AKONADI_SERVER_COLLECTIONSTATISTICS_H

class QMutex;

#include <QHash>
#include <QMutex>

namespace Akonadi
{
namespace Server
{

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

    static CollectionStatistics *self();
    static void destroy();

    virtual ~CollectionStatistics() {}

    const Statistics statistics(const Collection &col);

    void itemAdded(const Collection &col, qint64 size, bool seen);
    void itemsSeenChanged(const Collection &col, qint64 seenCount);

    void invalidateCollection(const Collection &col);

    void expireCache();

protected:
    virtual Statistics calculateCollectionStatistics(const Collection &col);

    QMutex mCacheLock;
    QHash<qint64, Statistics> mCache;

    static CollectionStatistics *sInstance;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_COLLECTIONSTATISTICS_H
