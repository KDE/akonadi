/*
    Copyright (c) 2017  Daniel Vrátil <dvratil@kde.org>

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

#include "aggregatedfetchscope.h"

#include <QMutex>
#include <QMutexLocker>

namespace Akonadi {
namespace Server {

class AggregatedCollectionFetchScopePrivate
{
public:
    explicit AggregatedCollectionFetchScopePrivate()
        : fetchIdOnly(0)
        , fetchStats(0)
    {
    }

    mutable QMutex lock;
    QSet<QByteArray> attrs;
    QHash<QByteArray, int> attrsCount;
    int fetchIdOnly;
    int fetchStats;
};


class AggregatedTagFetchScopePrivate
{
public:
    explicit AggregatedTagFetchScopePrivate()
        : fetchIdOnly(0)
    {
    }

    mutable QMutex lock;
    QSet<QByteArray> attrs;
    QHash<QByteArray, int> attrsCount;
    int fetchIdOnly;
};

} // namespace Server
} // namespace Akonadi

using namespace Akonadi::Server;

AggregatedCollectionFetchScope::AggregatedCollectionFetchScope()
    : d_ptr(new AggregatedCollectionFetchScopePrivate)
{
}

AggregatedCollectionFetchScope::~AggregatedCollectionFetchScope()
{
    delete d_ptr;
}

QSet<QByteArray> AggregatedCollectionFetchScope::attributes() const
{
    Q_D(const AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);
    return d->attrs;
}

void AggregatedCollectionFetchScope::addAttribute(const QByteArray &attribute)
{
    Q_D(AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);
    auto it = d->attrsCount.find(attribute);
    if (it == d->attrsCount.end()) {
        it = d->attrsCount.insert(attribute, 0);
        d->attrs.insert(attribute);
    }
    ++(*it);
}

void AggregatedCollectionFetchScope::removeAttribute(const QByteArray &attribute)
{
    Q_D(AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);
    auto it = d->attrsCount.find(attribute);
    if (it == d->attrsCount.end()) {
        return;
    }

    if (--(*it) == 0) {
        d->attrsCount.erase(it);
        d->attrs.remove(attribute);
    }
}

bool AggregatedCollectionFetchScope::fetchIdOnly() const
{
    Q_D(const AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);
    // Aggregation: we can return true only if everyone wants fetchIdOnly,
    // otherwise there's at least one subscriber who wants everything
    return d->fetchIdOnly == 0;
}

void AggregatedCollectionFetchScope::setFetchIdOnly(bool fetchIdOnly)
{
    Q_D(AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);

    d->fetchIdOnly += fetchIdOnly ? 1 : -1;
}

bool AggregatedCollectionFetchScope::fetchStatistics() const
{
    Q_D(const AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);
    // Aggregation: return true if at least one subscriber wants stats
    return d->fetchStats > 0;
}

void AggregatedCollectionFetchScope::setFetchStatistics(bool fetchStats)
{
    Q_D(AggregatedCollectionFetchScope);
    QMutexLocker locker(&d->lock);

    d->fetchStats += fetchStats ? 1 : -1;
}





AggregatedTagFetchScope::AggregatedTagFetchScope()
    : d_ptr(new AggregatedCollectionFetchScopePrivate)
{
}

AggregatedTagFetchScope::~AggregatedTagFetchScope()
{
    delete d_ptr;
}

bool AggregatedTagFetchScope::fetchIdOnly() const
{
    Q_D(const AggregatedTagFetchScope);
    QMutexLocker locker(&d->lock);

    // Aggregation: we can return true only if everyone wants fetchIdOnly,
    // otherwise there's at least one subscriber who wants everything
    return d->fetchIdOnly == 0;
}

void AggregatedTagFetchScope::setFetchIdOnly(bool fetchIdOnly)
{
    Q_D(AggregatedTagFetchScope);
    QMutexLocker locker(&d->lock);

    d->fetchIdOnly += fetchIdOnly ? 1 : -1;
}

QSet<QByteArray> AggregatedTagFetchScope::attributes() const
{
    Q_D(const AggregatedTagFetchScope);
    QMutexLocker locker(&d->lock);
    return d->attrs;
}

void AggregatedTagFetchScope::addAttribute(const QByteArray &attribute)
{
    Q_D(AggregatedTagFetchScope);
    QMutexLocker locker(&d->lock);
    auto it = d->attrsCount.find(attribute);
    if (it == d->attrsCount.end()) {
        it = d->attrsCount.insert(attribute, 0);
        d->attrs.insert(attribute);
    }
    ++(*it);
}

void AggregatedTagFetchScope::removeAttribute(const QByteArray &attribute)
{
    Q_D(AggregatedTagFetchScope);
    QMutexLocker locker(&d->lock);
    auto it = d->attrsCount.find(attribute);
    if (it == d->attrsCount.end()) {
        return;
    }

    if (--(*it) == 0) {
        d->attrsCount.erase(it);
        d->attrs.remove(attribute);
    }
}
