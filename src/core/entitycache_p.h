/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ENTITYCACHE_P_H
#define AKONADI_ENTITYCACHE_P_H

#include "item.h"
#include "itemfetchscope.h"
#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "tag.h"
#include "tagfetchjob.h"
#include "tagfetchscope.h"
#include "session.h"
#include "interface.h"

#include "akonaditests_export.h"

#include <QObject>
#include <QQueue>
#include <QVariant>
#include <QHash>

class KJob;

Q_DECLARE_METATYPE(QList<qint64>)

template<typename T> class MyStruct;

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
};

template <typename T>
struct EntityCacheNode {
    explicit EntityCacheNode() = default;
    explicit EntityCacheNode(typename T::Id id)
        : entity(T(id))
        , pending(true)
    {}

    T entity;
    bool pending = false;
    bool invalid = false;
};

/**
 * @internal
 * A in-memory FIFO cache for a small amount of Item or Collection objects.
 */
template<typename T, typename FetchScope_>
class EntityCache : public EntityCacheBase
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
        auto *node = cacheNodeForId(id);
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
        auto *node = cacheNodeForId(id);
        if (node && !node->pending && !node->invalid) {
            return node->entity;
        }
        return T();
    }

    /** Marks the cache entry as invalid, use in case the object has been deleted on the server. */
    void invalidate(typename T::Id id)
    {
        if (auto *node = cacheNodeForId(id)) {
            node->invalid = true;
        }
    }

    /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
    void update(typename T::Id id, const FetchScope &scope)
    {
        if (auto *node = cacheNodeForId(id)) {
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
        if (auto *node = cacheNodeForId(id); node) {
            return !node->pending;
        } else {
            request(id, scope);
            return false;
        }
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
        fetch(id, scope).then(
            [this, id](const auto &elem) {
                processResult(id, elem);
            },
            [this, id](const Akonadi::Error &) {
                //This can happen if we have stale notifications for items that have already been removed
                processResult(id, {});
            });
        mCache.enqueue(new EntityCacheNode<T>(id));
    }

private:
    EntityCacheNode<T> *cacheNodeForId(typename T::Id id) const
    {
        const auto it = std::find_if(mCache.cbegin(), mCache.cend(), [id](const auto *node) {
                return node->entity.id() == id;
        });
        return it != mCache.cend() ? *it : nullptr;
    }

    void processResult(typename T::Id id, const T &result)
    {
        auto *node = cacheNodeForId(id);
        if (!node) {
            return; // got replaced in the meantime
        }

        node->pending = false;
        node->entity = result;
        // make sure we find this node again if something went wrong here,
        // most likely the object got deleted from the server in the meantime
        if (node->entity.id() != id) {
            // TODO: Recursion guard? If this is called with non-existing ids, the if will never be true!
            node->entity.setId(id);
            node->invalid = true;
        }
        Q_EMIT this->dataAvailable();
    }

    auto fetch(typename T::Id id, const FetchScope &scope)
    {
        if constexpr (std::is_same_v<T, Akonadi::Item>) {
            return Akonadi::fetchItem(Item{id}, scope);
        } else if constexpr (std::is_same_v<T, Akonadi::Collection>) {
            return Akonadi::fetchCollection(Collection{id}, scope);
        } else if constexpr (std::is_same_v<T, Akonadi::Tag>) {
            return Akonadi::fetchTag(Tag{id}, scope);
        } else {
            assert(false); //EntityCache used with unsupported entity type
        }
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

using CollectionCache = EntityCache<Collection, CollectionFetchScope>;
using ItemCache = EntityCache<Item, ItemFetchScope>;
using TagCache = EntityCache<Tag, TagFetchScope>;

template <typename T>
struct EntityListCacheNode {
    explicit EntityListCacheNode() = default;
    explicit EntityListCacheNode(typename T::Id id)
        : entity(id)
        , pending(true)
    {}

    T entity;
    bool pending = false;
    bool invalid = false;
};

template<typename T, typename FetchScope_>
class EntityListCache : public EntityCacheBase
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
    template<template<typename> class Container = QVector>
    typename T::List retrieve(const Container<typename T::Id> &ids) const
    {
        typename T::List list;

        for (auto id : ids) {
            auto *node = mCache.value(id);
            if (!node || node->pending || node->invalid) {
                return typename T::List();
            }

            list.push_back(node->entity);
        }

        return list;
    }

    /** Requests the object to be cached if it is not yet in the cache. @returns @c true if it was in the cache already. */
    template<template<typename> class Container = QVector>
    bool ensureCached(const Container<typename T::Id> &ids, const FetchScope &scope)
    {
        QVector<typename T::Id> toRequest;
        bool result = true;

        for (auto id : ids) {
            auto *node = mCache.value(id);
            if (!node) {
                toRequest.push_back(id);
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
    template<template<typename> class Container = QVector>
    void invalidate(const Container<typename T::Id> &ids)
    {
        for (auto id : ids) {
            if (auto *node = mCache.value(id)) {
                node->invalid = true;
            }
        }
    }

    /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
    template<template<typename> class Container = QVector>
    void update(const Container<typename T::Id> &ids, const FetchScope &scope)
    {
        QVector<typename T::Id> toRequest;

        for (auto id : ids) {
            if (auto *node = mCache.value(id)) {
                mCache.remove(id);
                if (node->pending) {
                    toRequest.push_back(id);
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
    template<template<typename> class Cont1 = QVector, template<typename> class Cont2 = QVector>
    void request(const Cont1<typename T::Id> &ids, const FetchScope &scope,
                 const Cont2<typename T::Id> &preserveIds = {})
    {
        Q_ASSERT(isNotRequested(ids));
        shrinkCache(preserveIds);
        for (auto id : ids) {
            this->mCache.insert(id, new EntityListCacheNode<T>(id));
        }

        fetch(ids, scope).then(
            [this, ids](const auto &results) {
                this->processResult(ids, results);
            },
            [this, ids](const Akonadi::Error &/*error*/) {
                this->processResult(ids, {});
            });
    }

    template<template<typename> class Container = QVector>
    bool isNotRequested(const Container<typename T::Id> &ids) const
    {
        return std::none_of(ids.cbegin(), ids.cend(), [this](auto id) { return mCache.contains(id); });
    }

    /** Object is available in the cache and can be retrieved. */
    template<template<typename> class Container = QVector>
    bool isCached(const Container<typename T::Id> &ids) const
    {
        return std::all_of(ids.cbegin(), ids.cend(), [this](auto id) {
            const auto *node = mCache.value(id);
            return node && !node->pending;
        });
    }

private:
    /** Tries to reduce the cache size until at least one more object fits in. */
    template<template<typename> class Container>
    void shrinkCache(const Container<typename T::Id> &preserveIds)
    {
        auto iter = mCache.begin();
        while (iter != mCache.end() && mCache.size() >= mCapacity) {
            if (iter.value()->pending || preserveIds.contains(iter.key())) {
                ++iter;
                continue;
            }

            delete iter.value();
            iter = mCache.erase(iter);
        }
    }

    template<template<typename> class Container>
    typename T::List idsToEntities(const Container<typename T::Id> &ids)
    {
        typename T::List entities;
        entities.reserve(ids.size());
        std::transform(ids.cbegin(), ids.cend(), std::back_inserter(entities), [](auto id) { return T{id}; });
        return entities;
    }

    template<template<typename> class Container>
    auto fetch(const Container<typename T::Id> &ids, const FetchScope &scope)
    {
        if constexpr (std::is_same_v<T, Akonadi::Item>) {
            return Akonadi::fetchItems(idsToEntities(ids), scope);
        } else if constexpr (std::is_same_v<T, Akonadi::Collection>) {
            return Akonadi::fetchCollections(idsToEntities(ids), scope);
        } else if constexpr (std::is_same_v<T, Akonadi::Tag>) {
            return Akonadi::fetchTags(idsToEntities(ids), scope);
        } else {
            assert(false); // EntityListCache used with unsupported entity type
        }
    }

    template<template<typename> class Container>
    void processResult(const Container<typename T::Id> &ids, const typename T::List &entities) {
        for (auto id : ids) {
            auto *node = mCache.value(id);
            if (!node) {
                continue; // got replaced in the meantime
            }

            node->pending = false;

            T result;
            auto it = std::find_if(entities.begin(), entities.end(), [id](const auto &entity) {
                return entity.id() == id;
            });
            if (it != entities.end()) {
                result = *it;
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

private:
    QHash<typename T::Id, EntityListCacheNode<T> *> mCache;
    int mCapacity;
};

using CollectionListCache = EntityListCache<Collection, CollectionFetchScope>;
using ItemListCache = EntityListCache<Item, ItemFetchScope>;
using TagListCache = EntityListCache<Tag, TagFetchScope>;

} // namespace Akonadi

#endif
