/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "session.h"
#include "tag.h"
#include "tagfetchjob.h"
#include "tagfetchscope.h"

#include "akonaditests_export.h"

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QVariant>

class KJob;

Q_DECLARE_METATYPE(QList<qint64>)

namespace Akonadi
{
/**
  @internal
  QObject part of EntityCache.
*/
class AKONADI_TESTS_EXPORT EntityCacheBase : public QObject
{
    Q_OBJECT
public:
    explicit EntityCacheBase(Session *session, QObject *parent = nullptr);

    void setSession(Session *session);

protected:
    Session *session = nullptr;

Q_SIGNALS:
    void dataAvailable();

private Q_SLOTS:
    virtual void processResult(KJob *job) = 0;
};

template<typename T> struct EntityCacheNode {
    EntityCacheNode()
        : pending(false)
        , invalid(false)
    {
    }
    EntityCacheNode(typename T::Id id)
        : entity(T(id))
        , pending(true)
        , invalid(false)
    {
    }
    T entity;
    bool pending;
    bool invalid;
};

/**
 * @internal
 * A in-memory FIFO cache for a small amount of Item or Collection objects.
 */
template<typename T, typename FetchJob, typename FetchScope_> class EntityCache : public EntityCacheBase
{
public:
    using FetchScope = FetchScope_;
    explicit EntityCache(int maxCapacity, Session *session = nullptr, QObject *parent = nullptr)
        : EntityCacheBase(session, parent)
        , mCapacity(maxCapacity)
    {
    }

    ~EntityCache() override
    {
        qDeleteAll(mCache);
    }

    /** Object is available in the cache and can be retrieved. */
    bool isCached(typename T::Id id) const
    {
        EntityCacheNode<T> *node = cacheNodeForId(id);
        return node && !node->pending;
    }

    /** Object has been requested but is not yet loaded into the cache or is already available. */
    bool isRequested(typename T::Id id) const
    {
        return cacheNodeForId(id);
    }

    /** Returns the cached object if available, an empty instance otherwise. */
    virtual T retrieve(typename T::Id id) const
    {
        EntityCacheNode<T> *node = cacheNodeForId(id);
        if (node && !node->pending && !node->invalid) {
            return node->entity;
        }
        return T();
    }

    /** Marks the cache entry as invalid, use in case the object has been deleted on the server. */
    void invalidate(typename T::Id id)
    {
        EntityCacheNode<T> *node = cacheNodeForId(id);
        if (node) {
            node->invalid = true;
        }
    }

    /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
    void update(typename T::Id id, const FetchScope &scope)
    {
        EntityCacheNode<T> *node = cacheNodeForId(id);
        if (node) {
            mCache.removeAll(node);
            if (node->pending) {
                request(id, scope);
            }
            delete node;
        }
    }

    /** Requests the object to be cached if it is not yet in the cache. @returns @c true if it was in the cache already. */
    virtual bool ensureCached(typename T::Id id, const FetchScope &scope)
    {
        EntityCacheNode<T> *node = cacheNodeForId(id);
        if (!node) {
            request(id, scope);
            return false;
        }
        return !node->pending;
    }

    /**
      Asks the cache to retrieve @p id. @p request is used as
      a token to indicate which request has been finished in the
      dataAvailable() signal.
    */
    virtual void request(typename T::Id id, const FetchScope &scope)
    {
        Q_ASSERT(!isRequested(id));
        shrinkCache();
        auto node = new EntityCacheNode<T>(id);
        FetchJob *job = createFetchJob(id, scope);
        job->setProperty("EntityCacheNode", QVariant::fromValue<typename T::Id>(id));
        connect(job, SIGNAL(result(KJob *)), SLOT(processResult(KJob *)));
        mCache.enqueue(node);
    }

private:
    EntityCacheNode<T> *cacheNodeForId(typename T::Id id) const
    {
        for (typename QQueue<EntityCacheNode<T> *>::const_iterator it = mCache.constBegin(), endIt = mCache.constEnd(); it != endIt; ++it) {
            if ((*it)->entity.id() == id) {
                return *it;
            }
        }
        return nullptr;
    }

    void processResult(KJob *job) override
    {
        if (job->error()) {
            // This can happen if we have stale notifications for items that have already been removed
        }
        auto id = job->property("EntityCacheNode").template value<typename T::Id>();
        EntityCacheNode<T> *node = cacheNodeForId(id);
        if (!node) {
            return; // got replaced in the meantime
        }

        node->pending = false;
        extractResult(node, job);
        // make sure we find this node again if something went wrong here,
        // most likely the object got deleted from the server in the meantime
        if (node->entity.id() != id) {
            // TODO: Recursion guard? If this is called with non-existing ids, the if will never be true!
            node->entity.setId(id);
            node->invalid = true;
        }
        Q_EMIT dataAvailable();
    }

    void extractResult(EntityCacheNode<T> *node, KJob *job) const;

    inline FetchJob *createFetchJob(typename T::Id id, const FetchScope &scope)
    {
        auto fetch = new FetchJob(T(id), session);
        fetch->setFetchScope(scope);
        return fetch;
    }

    /** Tries to reduce the cache size until at least one more object fits in. */
    void shrinkCache()
    {
        while (mCache.size() >= mCapacity && !mCache.first()->pending) {
            delete mCache.dequeue();
        }
    }

private:
    QQueue<EntityCacheNode<T> *> mCache;
    int mCapacity;
};

template<> inline void EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>::extractResult(EntityCacheNode<Collection> *node, KJob *job) const
{
    auto fetch = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->collections().isEmpty()) {
        node->entity = Collection();
    } else {
        node->entity = fetch->collections().at(0);
    }
}

template<> inline void EntityCache<Item, ItemFetchJob, ItemFetchScope>::extractResult(EntityCacheNode<Item> *node, KJob *job) const
{
    auto fetch = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->items().isEmpty()) {
        node->entity = Item();
    } else {
        node->entity = fetch->items().at(0);
    }
}

template<> inline void EntityCache<Tag, TagFetchJob, TagFetchScope>::extractResult(EntityCacheNode<Tag> *node, KJob *job) const
{
    auto fetch = qobject_cast<TagFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->tags().isEmpty()) {
        node->entity = Tag();
    } else {
        node->entity = fetch->tags().at(0);
    }
}

template<>
inline CollectionFetchJob *EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>::createFetchJob(Collection::Id id,
                                                                                                             const CollectionFetchScope &scope)
{
    auto fetch = new CollectionFetchJob(Collection(id), CollectionFetchJob::Base, session);
    fetch->setFetchScope(scope);
    return fetch;
}

using CollectionCache = EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>;
using ItemCache = EntityCache<Item, ItemFetchJob, ItemFetchScope>;
using TagCache = EntityCache<Tag, TagFetchJob, TagFetchScope>;

template<typename T> struct EntityListCacheNode {
    EntityListCacheNode()
        : pending(false)
        , invalid(false)
    {
    }
    EntityListCacheNode(typename T::Id id)
        : entity(id)
        , pending(true)
        , invalid(false)
    {
    }

    T entity;
    bool pending;
    bool invalid;
};

template<typename T, typename FetchJob, typename FetchScope_> class EntityListCache : public EntityCacheBase
{
public:
    using FetchScope = FetchScope_;

    explicit EntityListCache(int maxCapacity, Session *session = nullptr, QObject *parent = nullptr)
        : EntityCacheBase(session, parent)
        , mCapacity(maxCapacity)
    {
    }

    ~EntityListCache() override
    {
        qDeleteAll(mCache);
    }

    /** Returns the cached object if available, an empty instance otherwise. */
    typename T::List retrieve(const QList<typename T::Id> &ids) const
    {
        typename T::List list;

        for (typename T::Id id : ids) {
            EntityListCacheNode<T> *node = mCache.value(id);
            if (!node || node->pending || node->invalid) {
                return typename T::List();
            }

            list << node->entity;
        }

        return list;
    }

    /** Requests the object to be cached if it is not yet in the cache. @returns @c true if it was in the cache already. */
    bool ensureCached(const QList<typename T::Id> &ids, const FetchScope &scope)
    {
        QList<typename T::Id> toRequest;
        bool result = true;

        for (typename T::Id id : ids) {
            EntityListCacheNode<T> *node = mCache.value(id);
            if (!node) {
                toRequest << id;
                continue;
            }

            if (node->pending) {
                result = false;
            }
        }

        if (!toRequest.isEmpty()) {
            request(toRequest, scope, ids);
            return false;
        }

        return result;
    }

    /** Marks the cache entry as invalid, use in case the object has been deleted on the server. */
    void invalidate(const QList<typename T::Id> &ids)
    {
        for (typename T::Id id : ids) {
            EntityListCacheNode<T> *node = mCache.value(id);
            if (node) {
                node->invalid = true;
            }
        }
    }

    /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
    void update(const QList<typename T::Id> &ids, const FetchScope &scope)
    {
        QList<typename T::Id> toRequest;

        for (typename T::Id id : ids) {
            EntityListCacheNode<T> *node = mCache.value(id);
            if (node) {
                mCache.remove(id);
                if (node->pending) {
                    toRequest << id;
                }
                delete node;
            }
        }

        if (!toRequest.isEmpty()) {
            request(toRequest, scope);
        }
    }

    /**
      Asks the cache to retrieve @p id. @p request is used as
      a token to indicate which request has been finished in the
      dataAvailable() signal.
    */
    void request(const QList<typename T::Id> &ids, const FetchScope &scope, const QList<typename T::Id> &preserveIds = QList<typename T::Id>())
    {
        Q_ASSERT(isNotRequested(ids));
        shrinkCache(preserveIds);
        for (typename T::Id id : ids) {
            auto node = new EntityListCacheNode<T>(id);
            mCache.insert(id, node);
        }
        FetchJob *job = createFetchJob(ids, scope);
        job->setProperty("EntityListCacheIds", QVariant::fromValue<QList<typename T::Id>>(ids));
        connect(job, SIGNAL(result(KJob *)), SLOT(processResult(KJob *)));
    }

    bool isNotRequested(const QList<typename T::Id> &ids) const
    {
        for (typename T::Id id : ids) {
            if (mCache.contains(id)) {
                return false;
            }
        }

        return true;
    }

    /** Object is available in the cache and can be retrieved. */
    bool isCached(const QList<typename T::Id> &ids) const
    {
        for (typename T::Id id : ids) {
            EntityListCacheNode<T> *node = mCache.value(id);
            if (!node || node->pending) {
                return false;
            }
        }
        return true;
    }

private:
    /** Tries to reduce the cache size until at least one more object fits in. */
    void shrinkCache(const QList<typename T::Id> &preserveIds)
    {
        typename QHash<typename T::Id, EntityListCacheNode<T> *>::Iterator iter = mCache.begin();
        while (iter != mCache.end() && mCache.size() >= mCapacity) {
            if (iter.value()->pending || preserveIds.contains(iter.key())) {
                ++iter;
                continue;
            }

            delete iter.value();
            iter = mCache.erase(iter);
        }
    }

    inline FetchJob *createFetchJob(const QList<typename T::Id> &ids, const FetchScope &scope)
    {
        auto job = new FetchJob(ids, session);
        job->setFetchScope(scope);
        return job;
    }

    void processResult(KJob *job) override
    {
        if (job->error()) {
            qWarning() << job->errorString();
        }
        const auto ids = job->property("EntityListCacheIds").value<QList<typename T::Id>>();

        typename T::List entities;
        extractResults(job, entities);

        for (typename T::Id id : ids) {
            EntityListCacheNode<T> *node = mCache.value(id);
            if (!node) {
                continue; // got replaced in the meantime
            }

            node->pending = false;

            T result;
            typename T::List::Iterator iter = entities.begin();
            for (; iter != entities.end(); ++iter) {
                if ((*iter).id() == id) {
                    result = *iter;
                    entities.erase(iter);
                    break;
                }
            }

            // make sure we find this node again if something went wrong here,
            // most likely the object got deleted from the server in the meantime
            if (!result.isValid()) {
                node->entity = T(id);
                node->invalid = true;
            } else {
                node->entity = result;
            }
        }

        Q_EMIT dataAvailable();
    }

    void extractResults(KJob *job, typename T::List &entities) const;

private:
    QHash<typename T::Id, EntityListCacheNode<T> *> mCache;
    int mCapacity;
};

template<> inline void EntityListCache<Collection, CollectionFetchJob, CollectionFetchScope>::extractResults(KJob *job, Collection::List &collections) const
{
    auto fetch = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    collections = fetch->collections();
}

template<> inline void EntityListCache<Item, ItemFetchJob, ItemFetchScope>::extractResults(KJob *job, Item::List &items) const
{
    auto fetch = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetch);
    items = fetch->items();
}

template<> inline void EntityListCache<Tag, TagFetchJob, TagFetchScope>::extractResults(KJob *job, Tag::List &tags) const
{
    auto fetch = qobject_cast<TagFetchJob *>(job);
    Q_ASSERT(fetch);
    tags = fetch->tags();
}

template<>
inline CollectionFetchJob *EntityListCache<Collection, CollectionFetchJob, CollectionFetchScope>::createFetchJob(const QList<Collection::Id> &ids,
                                                                                                                 const CollectionFetchScope &scope)
{
    auto fetch = new CollectionFetchJob(ids, CollectionFetchJob::Base, session);
    fetch->setFetchScope(scope);
    return fetch;
}

using CollectionListCache = EntityListCache<Collection, CollectionFetchJob, CollectionFetchScope>;
using ItemListCache = EntityListCache<Item, ItemFetchJob, ItemFetchScope>;
using TagListCache = EntityListCache<Tag, TagFetchJob, TagFetchScope>;
}

