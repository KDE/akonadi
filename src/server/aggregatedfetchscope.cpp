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
#include <private/protocol_p.h>
#include <shared/vectorhelper.h>

#include <QMutex>
#include <QMutexLocker>

#define LOCKED_D(name) \
    Q_D(name); \
    QMutexLocker lock(&d->lock);


namespace Akonadi {
namespace Server {

class AggregatedFetchScopePrivate
{
public:
    AggregatedFetchScopePrivate()
        : lock(QMutex::Recursive) // recursive so that we can call our own getters/setters
    {}

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

    inline void  applySet(const QSet<QByteArray> &oldSet, const QSet<QByteArray> &newSet,
                          QSet<QByteArray> &set, QHash<QByteArray, int> &count)
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
    mutable QMutex lock;
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
    int fetchAllAttributes = 0;
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
    int ancestors[3] = { 0, 0, 0 }; // 3 = size of AncestorDepth enum
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

void AggregatedCollectionFetchScope::apply(const Protocol::CollectionFetchScope &oldScope,
                                           const Protocol::CollectionFetchScope &newScope)
{
    LOCKED_D(AggregatedCollectionFetchScope)

    if (newScope.includeStatistics() != oldScope.includeStatistics()) {
        setFetchStatistics(newScope.includeStatistics());
    }
    if (newScope.fetchIdOnly() != oldScope.fetchIdOnly()) {
        setFetchIdOnly(newScope.fetchIdOnly());
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

void AggregatedCollectionFetchScope::addAttribute(const QByteArray &attribute)
{
    LOCKED_D(AggregatedCollectionFetchScope)
    d->addToSet(attribute, d->attrs, d->attrsCount);
}

void AggregatedCollectionFetchScope::removeAttribute(const QByteArray &attribute)
{
    LOCKED_D(AggregatedCollectionFetchScope)
    d->removeFromSet(attribute, d->attrs, d->attrsCount);
}

bool AggregatedCollectionFetchScope::fetchIdOnly() const
{
    LOCKED_D(const AggregatedCollectionFetchScope)
    // Aggregation: we can return true only if everyone wants fetchIdOnly,
    // otherwise there's at least one subscriber who wants everything
    return d->fetchIdOnly == d->subscribers;
}

void AggregatedCollectionFetchScope::setFetchIdOnly(bool fetchIdOnly)
{
    LOCKED_D(AggregatedCollectionFetchScope)
    d->updateBool(fetchIdOnly, d->fetchIdOnly);
}

bool AggregatedCollectionFetchScope::fetchStatistics() const
{
    LOCKED_D(const AggregatedCollectionFetchScope);
    // Aggregation: return true if at least one subscriber wants stats
    return d->fetchStats > 0;
}

void AggregatedCollectionFetchScope::setFetchStatistics(bool fetchStats)
{
    LOCKED_D(AggregatedCollectionFetchScope);
    d->updateBool(fetchStats, d->fetchStats);
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

void AggregatedItemFetchScope::apply(const Protocol::ItemFetchScope &oldScope,
                                     const Protocol::ItemFetchScope &newScope)
{
    LOCKED_D(AggregatedItemFetchScope);

    const auto newParts = vectorToSet(newScope.requestedParts());
    const auto oldParts = vectorToSet(oldScope.requestedParts());
    if (newParts != oldParts) {
        d->applySet(oldParts, newParts, d->parts, d->partsCount);
    }
    if (newScope.ancestorDepth() != oldScope.ancestorDepth()) {
        updateAncestorDepth(oldScope.ancestorDepth(), newScope.ancestorDepth());
    }
    if (newScope.cacheOnly() != oldScope.cacheOnly()) {
        setCacheOnly(newScope.cacheOnly());
    }
    if (newScope.fullPayload() != oldScope.fullPayload()) {
        setFullPayload(newScope.fullPayload());
    }
    if (newScope.allAttributes() != oldScope.allAttributes()) {
        setAllAttributes(newScope.allAttributes());
    }
    if (newScope.fetchSize() != oldScope.fetchSize()) {
        setFetchSize(newScope.fetchSize());
    }
    if (newScope.fetchMTime() != oldScope.fetchMTime()) {
        setFetchMTime(newScope.fetchMTime());
    }
    if (newScope.fetchRemoteRevision() != oldScope.fetchRemoteRevision()) {
        setFetchRemoteRevision(newScope.fetchRemoteRevision());
    }
    if (newScope.ignoreErrors() != oldScope.ignoreErrors()) {
        setIgnoreErrors(newScope.ignoreErrors());
    }
    if (newScope.fetchFlags() != oldScope.fetchFlags()) {
        setFetchFlags(newScope.fetchFlags());
    }
    if (newScope.fetchRemoteId() != oldScope.fetchRemoteId()) {
        setFetchRemoteId(newScope.fetchRemoteId());
    }
    if (newScope.fetchGID() != oldScope.fetchGID()) {
        setFetchGID(newScope.fetchGID());
    }
    if (newScope.fetchTags() != oldScope.fetchTags()) {
        setFetchTags(newScope.fetchTags());
    }
    if (newScope.fetchRelations() != oldScope.fetchRelations()) {
        setFetchRelations(newScope.fetchRelations());
    }
    if (newScope.fetchVirtualReferences() != oldScope.fetchVirtualReferences()) {
        setFetchVirtualReferences(newScope.fetchVirtualReferences());
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
    d->mCachedScope.setRequestedParts(setToVector(d->parts));
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

void AggregatedItemFetchScope::addRequestedPart(const QByteArray &part)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->addToSet(part, d->parts, d->partsCount);
}

void AggregatedItemFetchScope::removeRequestedPart(const QByteArray &part)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->removeFromSet(part, d->parts, d->partsCount);
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

void AggregatedItemFetchScope::updateAncestorDepth(ItemFetchScope::AncestorDepth oldDepth,
                                                   ItemFetchScope::AncestorDepth newDepth)
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

void AggregatedItemFetchScope::setCacheOnly(bool cacheOnly)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(cacheOnly, d->cacheOnly);
}

bool AggregatedItemFetchScope::fullPayload() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants the
    // full payload
    return d->fullPayload > 0;
}

void AggregatedItemFetchScope::setFullPayload(bool fullPayload)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fullPayload, d->fullPayload);
}

bool AggregatedItemFetchScope::allAttributes() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants
    // all attributes
    return d->allAttributes > 0;
}

void AggregatedItemFetchScope::setAllAttributes(bool allAttributes)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(allAttributes, d->allAttributes);
}

bool AggregatedItemFetchScope::fetchSize() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants size
    return d->fetchSize > 0;
}

void AggregatedItemFetchScope::setFetchSize(bool fetchSize)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchSize, d->fetchSize);
}

bool AggregatedItemFetchScope::fetchMTime() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants mtime
    return d->fetchMTime > 0;
}

void AggregatedItemFetchScope::setFetchMTime(bool fetchMTime)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchMTime, d->fetchMTime);
}

bool AggregatedItemFetchScope::fetchRemoteRevision() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants rrev
    return d->fetchRRev > 0;
}

void AggregatedItemFetchScope::setFetchRemoteRevision(bool remoteRevision)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(remoteRevision, d->fetchRRev);
}

bool AggregatedItemFetchScope::ignoreErrors() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true only if everyone wants to ignore errors, otherwise
    // there's at least one subscriber who does not want to ignore them
    return d->ignoreErrors == d->subscribers;
}

void AggregatedItemFetchScope::setIgnoreErrors(bool ignoreErrors)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(ignoreErrors, d->ignoreErrors);
}

bool AggregatedItemFetchScope::fetchFlags() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants flags
    return d->fetchFlags > 0;
}

void AggregatedItemFetchScope::setFetchFlags(bool fetchFlags)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchFlags, d->fetchFlags);
}

bool AggregatedItemFetchScope::fetchRemoteId() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants RID
    return d->fetchRID > 0;
}

void AggregatedItemFetchScope::setFetchRemoteId(bool fetchRemoteId)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchRemoteId, d->fetchRID);
}

bool AggregatedItemFetchScope::fetchGID() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants GID
    return d->fetchGID > 0;
}

void AggregatedItemFetchScope::setFetchGID(bool fetchGid)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchGid, d->fetchGID);
}

bool AggregatedItemFetchScope::fetchTags() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants tags
    return d->fetchTags > 0;
}

void AggregatedItemFetchScope::setFetchTags(bool fetchTags)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchTags, d->fetchTags);
}

bool AggregatedItemFetchScope::fetchRelations() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants relations
    return d->fetchRelations > 0;
}

void AggregatedItemFetchScope::setFetchRelations(bool fetchRelations)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchRelations, d->fetchRelations);
}

bool AggregatedItemFetchScope::fetchVirtualReferences() const
{
    LOCKED_D(const AggregatedItemFetchScope)
    // Aggregation: return true if there's at least one subscriber who wants vrefs
    return d->fetchVRefs > 0;
}

void AggregatedItemFetchScope::setFetchVirtualReferences(bool fetchVRefs)
{
    LOCKED_D(AggregatedItemFetchScope)
    d->updateBool(fetchVRefs, d->fetchVRefs);
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

void AggregatedTagFetchScope::apply(const Protocol::TagFetchScope &oldScope,
                                    const Protocol::TagFetchScope &newScope)
{
    LOCKED_D(AggregatedTagFetchScope)

    if (newScope.fetchIdOnly() != oldScope.fetchIdOnly()) {
        setFetchIdOnly(newScope.fetchIdOnly());
    }
    if (newScope.fetchRemoteID() != oldScope.fetchRemoteID()) {
        setFetchRemoteId(newScope.fetchRemoteID());
    }
    if (newScope.fetchAllAttributes() != oldScope.fetchAllAttributes()) {
        setFetchAllAttributes(newScope.fetchAllAttributes());
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

void AggregatedTagFetchScope::setFetchIdOnly(bool fetchIdOnly)
{
    LOCKED_D(AggregatedTagFetchScope)
    d->updateBool(fetchIdOnly, d->fetchIdOnly);
}

bool AggregatedTagFetchScope::fetchRemoteId() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    return d->fetchRemoteId > 0;
}

void AggregatedTagFetchScope::setFetchRemoteId(bool fetchRemoteId)
{
    LOCKED_D(AggregatedTagFetchScope)
    d->updateBool(fetchRemoteId, d->fetchRemoteId);
}

bool AggregatedTagFetchScope::fetchAllAttributes() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    return d->fetchAllAttributes > 0;
}

void AggregatedTagFetchScope::setFetchAllAttributes(bool fetchAllAttributes)
{
    LOCKED_D(AggregatedTagFetchScope)
    d->updateBool(fetchAllAttributes, d->fetchAllAttributes);
}

QSet<QByteArray> AggregatedTagFetchScope::attributes() const
{
    LOCKED_D(const AggregatedTagFetchScope)
    return d->attrs;
}

void AggregatedTagFetchScope::addAttribute(const QByteArray &attribute)
{
    LOCKED_D(AggregatedTagFetchScope)
    d->addToSet(attribute, d->attrs, d->attrsCount);
}

void AggregatedTagFetchScope::removeAttribute(const QByteArray &attribute)
{
    LOCKED_D(AggregatedTagFetchScope)
    d->removeFromSet(attribute, d->attrs, d->attrsCount);
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
