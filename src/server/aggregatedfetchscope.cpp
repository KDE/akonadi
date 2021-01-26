/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "aggregatedfetchscope.h"
#include <shared/akranges.h>

#include <QMutexLocker>
#include <QRecursiveMutex>

#define LOCKED_D(name)                                                                                                                                         \
    Q_D(name);                                                                                                                                                 \
    QMutexLocker lock(&d->lock);

namespace Akonadi
{
namespace Server
{
class AggregatedFetchScopePrivate
{
public:
    AggregatedFetchScopePrivate()
        : lock() // recursive so that we can call our own getters/setters
    {
    }

    inline void addToSet(const QByteArray &value, QSet<QByteArray> &set, QHash<QByteArray, int> &count)
    {
        auto it = count.find(value);
        if (it == count.end()) {
            it = count.insert(value, 0);
            set.insert(value);
        }
        ++(*it);
    }

    inline void removeFromSet(const QByteArray &value, QSet<QByteArray> &set, QHash<QByteArray, int> &count)
    {
        auto it = count.find(value);
        if (it == count.end()) {
            return;
        }

        if (--(*it) == 0) {
            count.erase(it);
            set.remove(value);
        }
    }

    inline void updateBool(bool newValue, int &store)
    {
        store += newValue ? 1 : -1;
    }

    inline void applySet(const QSet<QByteArray> &oldSet, const QSet<QByteArray> &newSet, QSet<QByteArray> &set, QHash<QByteArray, int> &count)
    {
        const auto added = newSet - oldSet;
        for (const auto &value : added) {
            addToSet(value, set, count);
        }
        const auto removed = oldSet - newSet;
        for (const auto &value : removed) {
            removeFromSet(value, set, count);
        }
    }

public:
    mutable QRecursiveMutex lock;
};

class AggregatedCollectionFetchScopePrivate : public AggregatedFetchScopePrivate
{
public:
    QSet<QByteArray> attrs;
    QHash<QByteArray, int> attrsCount;
    int subscribers = 0;
    int fetchIdOnly = 0;
    int fetchStats = 0;
};

class AggregatedTagFetchScopePrivate : public AggregatedFetchScopePrivate
{
public:
    QSet<QByteArray> attrs;
    QHash<QByteArray, int> attrsCount;
    int subscribers = 0;
    int fetchIdOnly = 0;
    int fetchRemoteId = 0;
    int doNotFetchAllAttributes = 0;
};

class AggregatedItemFetchScopePrivate : public AggregatedFetchScopePrivate
{
public:
    mutable Protocol::ItemFetchScope mCachedScope;
    mutable bool mCachedScopeValid = false; // use std::optional for mCachedScope

    QSet<QByteArray> parts;
    QHash<QByteArray, int> partsCount;
    QSet<QByteArray> tags;
    QHash<QByteArray, int> tagsCount;
    int subscribers = 0;
    int ancestors[3] = {0, 0, 0}; // 3 = size of AncestorDepth enum
    int cacheOnly = 0;
    int fullPayload = 0;
    int allAttributes = 0;
    int fetchSize = 0;
    int fetchMTime = 0;
    int fetchRRev = 0;
    int ignoreErrors = 0;
    int fetchFlags = 0;
    int fetchRID = 0;
    int fetchGID = 0;
    int fetchTags = 0;
    int fetchRelations = 0;
    int fetchVRefs = 0;
};

} // namespace Server
} // namespace Akonadi

using namespace Akonadi;
using namespace Akonadi::Protocol;
using namespace Akonadi::Server;

AggregatedCollectionFetchScope::AggregatedCollectionFetchScope()
    : d_ptr(new AggregatedCollectionFetchScopePrivate)
{
}

AggregatedCollectionFetchScope::~AggregatedCollectionFetchScope()
{
    delete d_ptr;
}

void AggregatedCollectionFetchScope::apply(const Protocol::CollectionFetchScope &oldScope, const Protocol::CollectionFetchScope &newScope)
{
    LOCKED_D(AggregatedCollectionFetchScope)

    if (newScope.includeStatistics() != oldScope.includeStatistics()) {
        d->updateBool(newScope.includeStatistics(), d->fetchStats);
    }
    if (newScope.fetchIdOnly() != oldScope.fetchIdOnly()) {
        d->updateBool(newScope.fetchIdOnly(), d->fetchIdOnly);
    }
    if (newScope.attributes() != oldScope.attributes()) {
        d->applySet(oldScope.attributes(), newScope.attributes(), d->attrs, d->attrsCount);
    }
}

QSet<QByteArray> AggregatedCollectionFetchScope::attributes() const
{
    LOCKED_D(const AggregatedCollectionFetchScope)
    return d->attrs;
}

bool AggregatedCollectionFetchScope::fetchIdOnly() const
{
    LOCKED_D(const AggregatedCollectionFetchScope)
    // Aggregation: we can return true only if everyone wants fetchIdOnly,
    // otherwise there's at least one subscriber who wants everything
    return d->fetchIdOnly == d->subscribers;
}

bool AggregatedCollectionFetchScope::fetchStatistics() const
{
    LOCKED_D(const AggregatedCollectionFetchScope);
    // Aggregation: return true if at least one subscriber wants stats
    return d->fetchStats > 0;
}

void AggregatedCollectionFetchScope::addSubscriber()
{
    LOCKED_D(AggregatedCollectionFetchScope)
    ++d->subscribers;
}

void AggregatedCollectionFetchScope::removeSubscriber()
{
    LOCKED_D(AggregatedCollectionFetchScope)
    --d->subscribers;
}

AggregatedItemFetchScope::AggregatedItemFetchScope()
    : d_ptr(new AggregatedItemFetchScopePrivate)
{
}

AggregatedItemFetchScope::~AggregatedItemFetchScope()
{
    delete d_ptr;
}

void AggregatedItemFetchScope::apply(const Protocol::ItemFetchScope &oldScope, const Protocol::ItemFetchScope &newScope)
{
    LOCKED_D(AggregatedItemFetchScope);

    const auto newParts = newScope.requestedParts() | AkRanges::Actions::toQSet;
    const auto oldParts = oldScope.requestedParts() | AkRanges::Actions::toQSet;
    if (newParts != oldParts) {
        d->applySet(oldParts, newParts, d->parts, d->partsCount);
    }
    if (newScope.ancestorDepth() != oldScope.ancestorDepth()) {
        updateAncestorDepth(oldScope.ancestorDepth(), newScope.ancestorDepth());
    }
    if (newScope.cacheOnly() != oldScope.cacheOnly()) {
        d->updateBool(newScope.cacheOnly(), d->cacheOnly);
    }
    if (newScope.fullPayload() != oldScope.fullPayload()) {
        d->updateBool(newScope.fullPayload(), d->fullPayload);
    }
    if (newScope.allAttributes() != oldScope.allAttributes()) {
        d->updateBool(newScope.allAttributes(), d->allAttributes);
    }
    if (newScope.fetchSize() != oldScope.fetchSize()) {
        d->updateBool(newScope.fetchSize(), d->fetchSize);
    }
    if (newScope.fetchMTime() != oldScope.fetchMTime()) {
        d->updateBool(newScope.fetchMTime(), d->fetchMTime);
    }
    if (newScope.fetchRemoteRevision() != oldScope.fetchRemoteRevision()) {
        d->updateBool(newScope.fetchRemoteRevision(), d->fetchRRev);
    }
    if (newScope.ignoreErrors() != oldScope.ignoreErrors()) {
        d->updateBool(newScope.ignoreErrors(), d->ignoreErrors);
    }
    if (newScope.fetchFlags() != oldScope.fetchFlags()) {
        d->updateBool(newScope.fetchFlags(), d->fetchFlags);
    }
    if (newScope.fetchRemoteId() != oldScope.fetchRemoteId()) {
        d->updateBool(newScope.fetchRemoteId(), d->fetchRID);
    }
    if (newScope.fetchGID() != oldScope.fetchGID()) {
        d->updateBool(newScope.fetchGID(), d->fetchGID);
    }
    if (newScope.fetchTags() != oldScope.fetchTags()) {
        d->updateBool(newScope.fetchTags(), d->fetchTags);
    }
    if (newScope.fetchRelations() != oldScope.fetchRelations()) {
        d->updateBool(newScope.fetchRelations(), d->fetchRelations);
    }
    if (newScope.fetchVirtualReferences() != oldScope.fetchVirtualReferences()) {
        d->updateBool(newScope.fetchVirtualReferences(), d->fetchVRefs);
    }

    d->mCachedScopeValid = false;
}

ItemFetchScope AggregatedItemFetchScope::toFetchScope() const
{
    LOCKED_D(const AggregatedItemFetchScope);
    if (d->mCachedScopeValid) {
        return d->mCachedScope;
    }

    d->mCachedScope = ItemFetchScope();
    d->mCachedScope.setRequestedParts(d->parts | AkRanges::Actions::toQVector);
    d->mCachedScope.setAncestorDepth(ancestorDepth());

    d->mCachedScope.setFetch(ItemFetchScope::CacheOnly, cacheOnly());
    d->mCachedScope.setFetch(ItemFetchScope::FullPayload, fullPayload());
    d->mCachedScope.setFetch(ItemFetchScope::AllAttributes, allAttributes());
    d->mCachedScope.setFetch(ItemFetchScope::Size, fetchSize());
    d->mCachedScope.setFetch(ItemFetchScope::MTime, fetchMTime());
    d->mCachedScope.setFetch(ItemFetchScope::RemoteRevision, fetchRemoteRevision());
    d->mCachedScope.setFetch(ItemFetchScope::IgnoreErrors, ignoreErrors());
    d->mCachedScope.setFetch(ItemFetchScope::Flags, fetchFlags());
    d->mCachedScope.setFetch(ItemFetchScope::RemoteID, fetchRemoteId());
    d->mCachedScope.setFetch(ItemFetchScope::GID, fetchGID());
    d->mCachedScope.setFetch(ItemFetchScope::Tags, fetchTags());
    d->mCachedScope.setFetch(ItemFetchScope::Relations, fetchRelations());
    d->mCachedScope.setFetch(ItemFetchScope::VirtReferences, fetchVirtualReferences());
    d->mCachedScopeValid = true;
    return d->mCachedScope;
}

QSet<QByteArray> AggregatedItemFetchScope::requestedParts() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    return d->parts;
}

ItemFetchScope::AncestorDepth AggregatedItemFetchScope::ancestorDepth() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return the largest depth with at least one subscriber
    if (d->ancestors[ItemFetchScope::AllAncestors] > 0) {
        return ItemFetchScope::AllAncestors;
    } else if (d->ancestors[ItemFetchScope::ParentAncestor] > 0) {
        return ItemFetchScope::ParentAncestor;
    } else {
        return ItemFetchScope::NoAncestor;
    }
}

void AggregatedItemFetchScope::updateAncestorDepth(ItemFetchScope::AncestorDepth oldDepth, ItemFetchScope::AncestorDepth newDepth)
{
    LOCKED_D(AggregatedItemFetchScope)
    if (d->ancestors[oldDepth] > 0) {
        --d->ancestors[oldDepth];
    }
    ++d->ancestors[newDepth];
}

bool AggregatedItemFetchScope::cacheOnly() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: we can return true only if everyone wants cached data only,
    // otherwise there's at least one subscriber who wants uncached data
    return d->cacheOnly == d->subscribers;
}

bool AggregatedItemFetchScope::fullPayload() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants the
    // full payload
    return d->fullPayload > 0;
}

bool AggregatedItemFetchScope::allAttributes() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants
    // all attributes
    return d->allAttributes > 0;
}

bool AggregatedItemFetchScope::fetchSize() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants size
    return d->fetchSize > 0;
}

bool AggregatedItemFetchScope::fetchMTime() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants mtime
    return d->fetchMTime > 0;
}

bool AggregatedItemFetchScope::fetchRemoteRevision() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants rrev
    return d->fetchRRev > 0;
}

bool AggregatedItemFetchScope::ignoreErrors() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true only if everyone wants to ignore errors, otherwise
    // there's at least one subscriber who does not want to ignore them
    return d->ignoreErrors == d->subscribers;
}

bool AggregatedItemFetchScope::fetchFlags() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants flags
    return d->fetchFlags > 0;
}

bool AggregatedItemFetchScope::fetchRemoteId() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants RID
    return d->fetchRID > 0;
}

bool AggregatedItemFetchScope::fetchGID() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants GID
    return d->fetchGID > 0;
}

bool AggregatedItemFetchScope::fetchTags() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants tags
    return d->fetchTags > 0;
}

bool AggregatedItemFetchScope::fetchRelations() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants relations
    return d->fetchRelations > 0;
}

bool AggregatedItemFetchScope::fetchVirtualReferences() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants vrefs
    return d->fetchVRefs > 0;
}

void AggregatedItemFetchScope::addSubscriber()
{
    LOCKED_D(AggregatedItemFetchScope)
    ++d->subscribers;
}

void AggregatedItemFetchScope::removeSubscriber()
{
    LOCKED_D(AggregatedItemFetchScope)
    --d->subscribers;
}

AggregatedTagFetchScope::AggregatedTagFetchScope()
    : d_ptr(new AggregatedTagFetchScopePrivate)
{
}

AggregatedTagFetchScope::~AggregatedTagFetchScope()
{
    delete d_ptr;
}

void AggregatedTagFetchScope::apply(const Protocol::TagFetchScope &oldScope, const Protocol::TagFetchScope &newScope)
{
    LOCKED_D(AggregatedTagFetchScope)

    if (newScope.fetchIdOnly() != oldScope.fetchIdOnly()) {
        d->updateBool(newScope.fetchIdOnly(), d->fetchIdOnly);
    }
    if (newScope.fetchRemoteID() != oldScope.fetchRemoteID()) {
        d->updateBool(newScope.fetchRemoteID(), d->fetchRemoteId);
    }
    if (newScope.fetchAllAttributes() != oldScope.fetchAllAttributes()) {
        // Count the number of subscribers who call with false
        d->updateBool(!newScope.fetchAllAttributes(), d->doNotFetchAllAttributes);
    }
    if (newScope.attributes() != oldScope.attributes()) {
        d->applySet(oldScope.attributes(), newScope.attributes(), d->attrs, d->attrsCount);
    }
}

Protocol::TagFetchScope AggregatedTagFetchScope::toFetchScope() const
{
    Protocol::TagFetchScope tfs;
    tfs.setFetchIdOnly(fetchIdOnly());
    tfs.setFetchRemoteID(fetchRemoteId());
    tfs.setFetchAllAttributes(fetchAllAttributes());
    tfs.setAttributes(attributes());
    return tfs;
}

bool AggregatedTagFetchScope::fetchIdOnly() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    // Aggregation: we can return true only if everyone wants fetchIdOnly,
    // otherwise there's at least one subscriber who wants everything
    return d->fetchIdOnly == d->subscribers;
}

bool AggregatedTagFetchScope::fetchRemoteId() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    return d->fetchRemoteId > 0;
}

bool AggregatedTagFetchScope::fetchAllAttributes() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    // The default value for fetchAllAttributes is true, so we return false only if every subscriber said "do not fetch all attributes"
    return d->doNotFetchAllAttributes != d->subscribers;
}

QSet<QByteArray> AggregatedTagFetchScope::attributes() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    return d->attrs;
}

void AggregatedTagFetchScope::addSubscriber()
{
    LOCKED_D(AggregatedTagFetchScope)
    ++d->subscribers;
}

void AggregatedTagFetchScope::removeSubscriber()
{
    LOCKED_D(AggregatedTagFetchScope)
    --d->subscribers;
}

#undef LOCKED_D
