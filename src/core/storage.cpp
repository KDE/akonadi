/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#include "storage.h"

#include "session.h"
#include "session_p.h"
#include "collectionstatistics.h"
#include "jobs/itemfetchjob.h"
#include "jobs/itemmodifyjob.h"
#include "jobs/itemdeletejob.h"
#include "jobs/itemmovejob.h"
#include "jobs/itemcreatejob.h"
#include "jobs/itemcopyjob.h"
#include "jobs/linkjob.h"
#include "jobs/unlinkjob.h"
#include "jobs/itemsearchjob.h"
#include "jobs/collectionfetchjob.h"
#include "jobs/collectionmodifyjob.h"
#include "jobs/collectionmovejob.h"
#include "jobs/collectioncopyjob.h"
#include "jobs/collectioncreatejob.h"
#include "jobs/collectiondeletejob.h"
#include "jobs/collectionstatisticsjob.h"
#include "jobs/tagfetchjob.h"
#include "jobs/tagcreatejob.h"
#include "jobs/tagdeletejob.h"
#include "jobs/tagmodifyjob.h"
#include "jobs/relationfetchjob.h"
#include "jobs/relationcreatejob.h"
#include "jobs/relationdeletejob.h"
#include "jobs/searchcreatejob.h"
#include "jobs/transactionjobs.h"
#include "jobs/resourceselectjob_p.h"

#include <functional>

using namespace Akonadi;

Storage::Storage() = default;
Storage::~Storage() = default;

namespace {

template<typename CreateFunc, typename ExtractFunc, typename ... Args>
auto asyncJob(CreateFunc createFunc, ExtractFunc extractFunc, Session *session, Args && ... args)
{
    using Job = std::decay_t<std::remove_pointer_t<decltype(createFunc(session, args ...))>>;
    using Result = std::decay_t<std::invoke_result_t<ExtractFunc, Job *>>;

    Task<Result> task;
    QObject::connect(createFunc(session, std::forward<Args>(args) ...), &Job::result,
                     [task, extractor = std::move(extractFunc)](KJob *job) mutable {
                        if (job->error()) {
                            task.setError(job->error(), job->errorText());
                        } else {
                            task.setResult(std::invoke(extractor, static_cast<Job *>(job)));
                        }
                     });
    return task;
}

template<typename CreateFunc, typename ... Args>
auto asyncJob(CreateFunc createFunc, Session *session, Args && ... args)
{
    using Job = std::decay_t<std::remove_pointer_t<decltype(createFunc(session, args ...))>>;

    Task<void> task;
    QObject::connect(createFunc(session, std::forward<Args>(args) ...), &Job::result,
                     [task](KJob *job) mutable {
                        if (job->error()) {
                            task.setError(job->error(), job->errorText());
                        } else {
                            task.setResult();
                        }
                    });
    return task;
}

template<typename Job>
struct createJob
{
    template<typename ... Args>
    auto operator()(Session *session, Args && ... args)
    {
        return new Job(std::forward<Args>(args) ..., session);
    }
};

template<>
struct createJob<ItemFetchJob> {
    template<typename ... Args>
    auto operator()(Session *session, const ItemFetchOptions &options, Args && ... args)
    {
        auto *job = new ItemFetchJob(std::forward<Args>(args) ..., session);
        job->setFetchScope(options.itemFetchScope());
        if (options.collection().isValid()) {
            job->setCollection(options.collection());
        }
        return job;
    }
};

template<>
struct createJob<ItemModifyJob> {
    template<typename ... Args>
    auto operator()(Session *session, ItemModifyFlags flags, Args && ... args)
    {
        auto *job = new ItemModifyJob(std::forward<Args>(args) ..., session);
        job->setIgnorePayload(flags & ItemModifyFlag::IgnorePayload);
        job->setUpdateGid(flags & ItemModifyFlag::UpdateGid);
        if (!(flags & ItemModifyFlag::RevisionCheck)) {
            job->disableRevisionCheck();
        }
        if (!(flags & ItemModifyFlag::AutomaticConflictHandling)) {
            job->disableAutomaticConflictHandling();
        }
        return job;
    }
};

template<>
struct createJob<CollectionFetchJob> {
    template<typename ... Args>
    auto operator()(Session *session, const CollectionFetchOptions &options, Args && ... args)
    {
        auto *job = new CollectionFetchJob(std::forward<Args>(args) ..., session);
        job->setFetchScope(options.fetchScope());
        return job;
    }
};

template<>
struct createJob<TagFetchJob> {
    template<typename ... Args>
    auto operator()(Session *session, const TagFetchOptions &options, Args && ... args)
    {
        auto *job = new TagFetchJob(std::forward<Args>(args) ..., session);
        job->setFetchScope(options.fetchScope());
        return job;
    }
};

} // namespace


Session *Storage::createSession(const QByteArray &sessionId)
{
    return new Session(sessionId);
}

Session *Storage::defaultSession()
{
    return Session::defaultSession();
}

Task<Item::List> Storage::fetchItemsFromCollection(const Collection &collection, const ItemFetchOptions &options,
                                                   Session *session)
{
    return asyncJob(createJob<ItemFetchJob>(), &ItemFetchJob::items, session, options, collection);
}

Task<Item::List> Storage::fetchItems(const Item::List &items, const ItemFetchOptions &options,
                                            Session *session)
{
    return asyncJob(createJob<ItemFetchJob>(), &ItemFetchJob::items, session, options, items);
}

Task<Item::List> Storage::fetchItemsForTag(const Tag &tag, const ItemFetchOptions &options, Session *session)
{
    return asyncJob(createJob<ItemFetchJob>(), &ItemFetchJob::items, session, options, tag);
}

Task<Item> Storage::fetchItem(const Item &item, const ItemFetchOptions &options, Session *session)
{
    return asyncJob(createJob<ItemFetchJob>(),
            [](ItemFetchJob *job) {
                const auto items = job->items();
                return items.empty() ? Item() : items[0];
            },
            session, options, item);
}

Task<Item> Storage::createItem(const Item &item, const Collection &collection, ItemCreateFlags flags,
                               Session *session)
{
    return asyncJob(
            [](Session *session, ItemCreateFlags flags, const Item &item, const Collection &collection) {
                auto *job = new ItemCreateJob(item, collection, session);
                ItemCreateJob::MergeOptions options;
                if (flags & ItemCreateFlag::RID) { options |= ItemCreateJob::RID; }
                if (flags & ItemCreateFlag::GID) { options |= ItemCreateJob::GID; }
                if (flags & ItemCreateFlag::Silent) { options |= ItemCreateJob::Silent; }
                job->setMerge(options);
                return job;
            },
            &ItemCreateJob::item,
            session, flags, item, collection);
}

Task<Item> Storage::updateItem(const Item &item, ItemModifyFlags flags, Session *session)
{
    return asyncJob(createJob<ItemModifyJob>(), &ItemModifyJob::item, session, flags, item);
}

Task<Item::List> Storage::updateItems(const Item::List &items, ItemModifyFlags flags,
                                             Session *session)
{
    return asyncJob(createJob<ItemModifyJob>(), &ItemModifyJob::items, session, flags, items);
}

Task<void> Storage::deleteItem(const Item &item, Session *session)
{
    return asyncJob(createJob<ItemDeleteJob>(), session, item);
}

Task<void> Storage::deleteItems(const Item::List &items, Session *session)
{
    return asyncJob(createJob<ItemDeleteJob>(), session, items);
}

Task<void> Storage::deleteItemsFromCollection(const Collection &collection, Session *session)
{
    return asyncJob(createJob<ItemDeleteJob>(), session, collection);
}

Task<void> Storage::deleteItemsWithTag(const Tag &tag, Session *session)
{
    return asyncJob(createJob<ItemDeleteJob>(), session, tag);
}

Task<void> Storage::moveItem(const Item &item, const Collection &destination, Session *session)
{
    return asyncJob(createJob<ItemMoveJob>(), session, item, destination);
}

Task<void> Storage::moveItems(const Item::List &items, const Collection &destination, Session *session)
{
    return asyncJob(createJob<ItemMoveJob>(), session, items, destination);
}

Task<void> Storage::moveItemsFromCollection(const Item::List &items, const Collection &source, const Collection &destination,
                                            Session *session)
{
    return asyncJob(createJob<ItemMoveJob>(), session, items, source, destination);
}

Task<void> Storage::copyItem(const Item &item, const Collection &destination, Session *session)
{
    return asyncJob(createJob<ItemCopyJob>(), session, item, destination);
}

Task<void> Storage::copyItems(const Item::List &items, const Collection &destination,
                                              Session *session)
{
    return asyncJob(createJob<ItemCopyJob>(), session, items, destination);
}

Task<void> Storage::linkItems(const Item::List &items, const Collection &destination, Session *session)
{
    return asyncJob(createJob<LinkJob>(), session, destination, items);
}

Task<void> Storage::unlinkItems(const Item::List &items, const Collection &collection, Session *session)
{
    return asyncJob(createJob<UnlinkJob>(), session, collection, items);
}

Task<Item::List> Storage::searchItems(const SearchQuery &searchQuery, const ItemSearchOptions &options,
                                      Session *session)
{
    return asyncJob(
            [](Session *session, const ItemSearchOptions &options, const SearchQuery &searchQuery) {
                auto *job = new ItemSearchJob(searchQuery, session);
                job->setFetchScope(options.itemFetchScope());
                job->setTagFetchScope(options.tagFetchScope());
                job->setMimeTypes(options.mimeTypes());
                job->setSearchCollections(options.searchCollections());
                job->setRecursive(options.isRecursive());
                job->setRemoteSearchEnabled(options.isRemoteSearchEnabled());
                return job;
            },
            &ItemSearchJob::items, session, options, searchQuery);
}

Task<Collection> Storage::fetchCollection(const Collection &collection, const CollectionFetchOptions &options,
                                          Session *session)
{
    return asyncJob(
            createJob<CollectionFetchJob>(),
            [](CollectionFetchJob *job) {
                const auto cols = job->collections();
                return cols.empty() ? Collection() : cols[0];
            },
            session, options, collection, CollectionFetchJob::Base);
}

Task<Collection::List> Storage::fetchCollectionsRecursive(const Collection &root, const CollectionFetchOptions &options,
                                                          Session *session)
{
    return asyncJob(createJob<CollectionFetchJob>(), &CollectionFetchJob::collections, session, options, root,
                        CollectionFetchJob::Recursive);
}

Task<Collection::List> Storage::fetchCollectionsRecursive(const Collection::List &collections, const CollectionFetchOptions &options,
                                                          Session *session)
{
    return asyncJob(createJob<CollectionFetchJob>(), &CollectionFetchJob::collections, session, options, collections,
                        CollectionFetchJob::Recursive);
}

Task<Collection::List> Storage::fetchCollections(const Collection::List &collections, const CollectionFetchOptions &options,
                                                 Session *session)
{
    return asyncJob(createJob<CollectionFetchJob>(), &CollectionFetchJob::collections, session, options, collections);
}

Task<Collection::List> Storage::fetchSubcollections(const Collection &base, const CollectionFetchOptions &options,
                                                    Session *session)
{
    return asyncJob(createJob<CollectionFetchJob>(), &CollectionFetchJob::collections, session, options, base,
                        CollectionFetchJob::FirstLevel);
}

Task<Collection> Storage::createCollection(const Collection &collection, Session *session)
{
    return asyncJob(createJob<CollectionCreateJob>(), &CollectionCreateJob::collection, session, collection);
}

Task<Collection> Storage::updateCollection(const Collection &collection, Session *session)
{
    return asyncJob(createJob<CollectionModifyJob>(), &CollectionModifyJob::collection, session, collection);
}

Task<void> Storage::deleteCollection(const Collection &collection, Session *session)
{
    return asyncJob(createJob<CollectionDeleteJob>(), session, collection);
}

Task<void> Storage::moveCollection(const Collection &collection, const Collection &destination,
                                   Session *session)
{
    return asyncJob(createJob<CollectionMoveJob>(), session, collection, destination);
}

Task<void> Storage::copyCollection(const Collection &collection, const Collection &destination,
                                   Session *session)
{
    return asyncJob(createJob<CollectionCopyJob>(), session, collection, destination);
}

Task<CollectionStatistics> Storage::fetchCollectionStatistics(const Collection &collection,
                                                              Session *session)
{
    return asyncJob(createJob<CollectionStatisticsJob>(), &CollectionStatisticsJob::statistics, session, collection);
}

Task<Tag> Storage::fetchTag(const Tag &tag, const TagFetchOptions &options, Session *session)
{
    return asyncJob(
            createJob<TagFetchJob>(),
                [](TagFetchJob *job) {
                    const auto tags = job->tags();
                    return tags.empty() ? Tag() : tags[0];
                },
            session, options, tag);
}

Task<Tag::List> Storage::fetchTags(const Tag::List &tags, const TagFetchOptions &options, Session *session)
{
    return asyncJob(createJob<TagFetchJob>(), &TagFetchJob::tags, session, options, tags);
}

Task<Tag::List> Storage::fetchAllTags(const TagFetchOptions &options, Session *session)
{
    return asyncJob(createJob<TagFetchJob>(), &TagFetchJob::tags, session, options);
}

Task<Tag> Storage::createTag(const Tag &tag, Session *session)
{
    return asyncJob(createJob<TagCreateJob>(), &TagCreateJob::tag, session, tag);
}

Task<Tag> Storage::updateTag(const Tag &tag, Session *session)
{
    return asyncJob(createJob<TagModifyJob>(), &TagModifyJob::tag, session, tag);
}

Task<void> Storage::deleteTag(const Tag &tag, Session *session)
{
    return asyncJob(createJob<TagDeleteJob>(), session, tag);
}

Task<void> Storage::deleteTags(const Tag::List &tags, Session *session)
{
    return asyncJob(createJob<TagDeleteJob>(), session, tags);
}

Task<Relation::List> Storage::fetchRelation(const Relation &relation, Session *session)
{
    return asyncJob(createJob<RelationFetchJob>(), &RelationFetchJob::relations, session, relation);
}

Task<Relation::List> Storage::fetchRelationsOfTypes(const QVector<QByteArray> &types, Session *session)
{
    return asyncJob(createJob<RelationFetchJob>(), &RelationFetchJob::relations, session, types);
}

Task<Relation> Storage::createRelation(const Relation &relation, Session *session)
{
    return asyncJob(createJob<RelationCreateJob>(), &RelationCreateJob::relation, session, relation);
}

Task<void> Storage::deleteRelation(const Relation &relation, Session *session)
{
    return asyncJob(createJob<RelationDeleteJob>(), session, relation);
}

Task<Collection> Storage::createSearchCollection(const QString &name, const SearchQuery &searchQuery,
                                                 const ItemSearchOptions &options, Session *session)
{
    return asyncJob(
            [](Session *session, const ItemSearchOptions &options, const QString &name, const SearchQuery &searchQuery) {
                auto *job = new SearchCreateJob(name, searchQuery, session);
                job->setSearchMimeTypes(options.mimeTypes());
                job->setSearchCollections(options.searchCollections());
                job->setRecursive(options.isRecursive());
                job->setRemoteSearchEnabled(options.isRemoteSearchEnabled());
                return job;
            },
            &SearchCreateJob::createdCollection, session, options, name, searchQuery);
}

Task<void> Storage::beginTransaction(Session *session)
{
    return asyncJob(createJob<TransactionBeginJob>(), session);
}

Task<void> Storage::commitTransaction(Session *session)
{
    return asyncJob(createJob<TransactionCommitJob>(), session);
}

Task<void> Storage::rollbackTransaction(Session *session)
{
    return asyncJob(createJob<TransactionRollbackJob>(), session);
}

Task<void> Storage::selectResource(const QString &identifier, Session *session)
{
    return asyncJob(createJob<ResourceSelectJob>(), session, identifier);
}
