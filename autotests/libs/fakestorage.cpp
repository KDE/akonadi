/*
    Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>

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

#include "fakestorage.h"
#include "fakesession.h"
#include "fakeentitycache.h"

#include "collection.h"
#include "item.h"
#include "itemfetchscope.h"
#include "attributefactory.h"
#include "expected.h"
#include <shared/akranges.h>

#include <QTimer>

#include <algorithm>

using namespace Akonadi;
using namespace AkRanges;

class FakeStoragePrivate
{
public:
    FakeItemCache itemCache;
    FakeCollectionCache collectionCache;
    FakeMonitorDependenciesFactory dependenciesFactory{&itemCache, &collectionCache};

    QVector<QByteArray> resources;
    QHash<qint64, Collection> collections;
    QHash<qint64, Item> items;
    QHash<qint64, Tag> tags;
    QHash<qint64, Relation> relations;
    QHash<qint64, QVector<qint64>> virtualCols;

    qint64 lastItemId = 0;
    qint64 lastCollectionId = 0;
    qint64 lastTagId = 0;
    qint64 lastRelationId = 0;

    void notify() {}

    auto findItemConst(const Item &item) const -> Expected<typename decltype(items)::const_iterator>
    {
        auto it = items.find(item.id());
        if (it == items.cend()) {
            return tl::make_unexpected(Error{1, QStringLiteral("No such Item %1").arg(item.id())});
        }
        return it;
    }

    auto findItem(const Item &item) -> Expected<typename decltype(items)::iterator>
    {
        auto it = items.find(item.id());
        if (it == items.end()) {
            return tl::make_unexpected(Error{1, QStringLiteral("No such Item %1").arg(item.id())});
        }
        return it;
    }

    auto findItems(const Item::List &items, const Collection &sourceCol) const -> Expected<Item::List>
    {
        Item::List rv;
        rv.reserve(items.size());
        for (const auto &item : items) {
            if (item.id() > 0) {
                const auto it = this->items.constFind(item.id());
                if (it == this->items.cend()) {
                    return tl::make_unexpected(Error{1, QStringLiteral("No such Item %1").arg(item.id())});
                }
                rv.push_back(*it);
            } else {
                const auto it = std::find_if(this->items.cbegin(), this->items.cend(), [item, sourceCol](const auto &i) {
                        return i.remoteId() == item.remoteId() && item.parentCollection() == sourceCol;
                });
                if (it == this->items.cend()) {
                    return tl::make_unexpected(Error{1, QStringLiteral("No such Item with RID %1 in Collection %2").arg(item.remoteId()).arg(sourceCol.id())});
                }
                rv.push_back(*it);
            }
        }

        return rv;
    }

    auto findCollectionConst(const Collection &col) const -> Expected<typename decltype(collections)::const_iterator>
    {
        auto it = collections.find(col.id());
        if (it == collections.cend()) {
            return tl::make_unexpected(Error{1, QStringLiteral("No such Collection %1").arg(col.id())});
        }
        return it;
    }

    auto findCollection(const Collection &col) -> Expected<typename decltype(collections)::iterator>
    {
        auto it = collections.find(col.id());
        if (it == collections.end()) {
            return tl::make_unexpected(Error{1, QStringLiteral("No such Collection %1").arg(col.id())});
        }
        return it;
    }

    auto findCollections(const Collection::List &collections) const -> Expected<Collection::List>
    {
        Collection::List rv;
        rv.reserve(collections.size());

        for (const auto &col : collections) {
            const auto it = this->collections.find(col.id());
            if (it == this->collections.cend()) {
                return tl::make_unexpected(Error{1, QStringLiteral("No such Collection %1").arg(col.id())});
            }
            rv.push_back(*it);
        }

        return rv;
    }

    auto findTagConst(const Tag &tag) const -> Expected<typename decltype(tags)::const_iterator>
    {
        auto it = tags.constFind(tag.id());
        if (it == tags.cend()) {
            return tl::make_unexpected(Error{1, QStringLiteral("No such Tag %1").arg(tag.id())});
        }
        return it;
    }

    auto findTags(const Tag::List &tags) const -> Expected<Tag::List>
    {
        Tag::List rv;
        rv.reserve(tags.size());

        for (const auto &tag : tags) {
            const auto it = this->tags.find(tag.id());
            if (it == this->tags.cend()) {
                return tl::make_unexpected(Error{1, QStringLiteral("No such Tag %1").arg(tag.id())});
            }
            rv.push_back(*it);
        }

        return rv;
    }

    template<typename Cb>
    auto task(Cb &&cb) -> Task<typename std::invoke_result_t<Cb>::value_type>
    {
        Task<typename std::invoke_result_t<Cb>::value_type> task;
        QTimer::singleShot(0, [cb = std::forward<Cb>(cb), task]() mutable {
            cb().or_else([&task](const Error &e) mutable { task.setError(e); })
                .map([&task](auto && ... val) { task.setResult(std::forward<decltype(val)>(val) ...); });
        });
        return task;
    }

    void reset()
    {
        resources = {QByteArray("akonadi_fakeresource_0"), QByteArray("akonadi_fakeresource_1")};
        collections = {
            {0, Collection::root()},
            createCollection(QStringLiteral("res1"), 0),
            createCollection(QStringLiteral("foo"), 1),
            createCollection(QStringLiteral("bar"), 1),
            createCollection(QStringLiteral("bla"), 3),
            createCollection(QStringLiteral("bla"), 2)
        };
        items = {
            createItem(QStringLiteral("A"), 2, "testmailbody", {{"ATR:HEAD", "From: <test@user.tst>"}},
                    {QByteArray{"\\SEEN"}, QByteArray{"\\FLAGGED"}, QByteArray{"\\DRAFT"}}),
            createItem(QStringLiteral("B"), 2, "testmailbody1", {{"ATR:HEAD", "From <test1@user.tst>"}},
                    {QByteArray{"\\FLAGGED"}}),
            createItem(QStringLiteral("C"), 2, "testmailbody2", {{"ATR:HEAD", "From <test2@user.tst>"}}),
            createItem(QStringLiteral("D"), 2, "testmailbody3", {{"ATR:HEAD", "From <test3@user.tst>"}}),
            createItem(QStringLiteral("E"), 2, "testmailbody4", {{"ATR:HEAD", "From <test4@user.tst>"}}),
            createItem(QStringLiteral("F"), 2, "testmailbody5", {{"ATR:HEAD", "From <test5@user.tst>"}}),
            createItem(QStringLiteral("G"), 2, "testmailbody6", {{"ATR:HEAD", "From <test6@user.tst>"}}),
            createItem(QStringLiteral("H"), 2, "testmailbody7", {{"ATR:HEAD", "From <test7@user.tst>"}}),
            createItem(QStringLiteral("I"), 2, "testmailbody8", {{"ATR:HEAD", "From <test8@user.tst>"}}),
            createItem(QStringLiteral("J"), 2, "testmailbody9", {{"ATR:HEAD", "From <test9user.tst>"}}),
            createItem(QStringLiteral("K"), 2, "testmailbody10", {{"ATR:HEAD", "From <test10@user.tst>"}}),
            createItem(QStringLiteral("L"), 2, "testmailbody11", {{"ATR:HEAD", "From <test11@user.tst>"}}),
            createItem(QStringLiteral("M"), 2, "testmailbody12", {{"ATR:HEAD", "From <test12@user.tst>"}}),
            createItem(QStringLiteral("N"), 2, "testmailbody13", {{"ATR:HEAD", "From <test13@user.tst>"}}),
            createItem(QStringLiteral("O"), 2, "testmailbody14", {{"ATR:HEAD", "From <test14@user.tst>"}})
        };
        tags = {
            createTag(QStringLiteral("name"), "type", "tagrid", "taggid")
        };
    }

private:
    std::pair<qint64, Collection> createCollection(const QString &name, qint64 parent)
    {
        Collection col;
        col.setId(++lastCollectionId);
        col.setName(name);
        col.setParentCollection(Collection{parent});
        return {col.id(), col};
    }

    std::pair<qint64, Item> createItem(const QString &rid, qint64 parentId, const QByteArray &pld, const QVector<std::pair<QByteArray, QByteArray>> &attrs, const QVector<QByteArray> &flags = {})
    {
        Item item;
        item.setId(++lastItemId);
        item.setParentCollection(Collection{parentId});
        item.setRemoteId(rid);
        item.setMimeType(QStringLiteral("application/octet-stream"));
        item.setPayload(pld);
        for (const auto &[type, value] : attrs) {
            auto *attr = AttributeFactory::createAttribute(type);
            attr->deserialize(value);
            item.addAttribute(attr);
        }
        // TODO: attributes
        for (const auto &flag : flags) {
            item.setFlag(flag);
        }

        return {item.id(), item};
    }

    std::pair<qint64, Tag> createTag(const QString &name, const QByteArray &type, const QByteArray &rid, const QByteArray &gid)
    {
        Tag tag;
        tag.setId(++lastTagId);
        tag.setName(name);
        tag.setType(type);
        tag.setRemoteId(rid);
        tag.setGid(gid);

        return {tag.id(), tag};
    }
};

FakeStorage::FakeStorage()
    : mStorage(std::make_unique<FakeStoragePrivate>())
{}

FakeStorage::~FakeStorage() = default;

void FakeStorage::reset()
{
    mStorage->reset();
}

Session *FakeStorage::createSession(const QByteArray &name)
{
    return new FakeSession(name);
}

Session *FakeStorage::defaultSession()
{
    return new FakeSession("default");
}

Task<Item::List> FakeStorage::fetchItemsFromCollection(const Collection &collection, const ItemFetchOptions &options,
                                          Session *session)
{
    Q_UNUSED(session);
    Q_UNUSED(options);

    return mStorage->task([this, collection]() -> Expected<Item::List> {
        Item::List items;
        std::copy_if(mStorage->items.cbegin(), mStorage->items.cend(), std::back_inserter(items),
                     [collection](const auto &item) { return item.parentCollection() == collection; });
        return items;
    });
}

Task<Item::List> FakeStorage::fetchItems(const Item::List &items, const ItemFetchOptions &options, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, options]() -> Expected<Item::List> {
        Item::List rv;
        rv.reserve(items.size());
        for (const auto &item : items) {
            auto it = mStorage->items.constFind(item.id());
            if (it == mStorage->items.cend()) {
                if (options.itemFetchScope().ignoreRetrievalErrors()) {
                    continue;
                }

                return tl::make_unexpected(Error{1, QStringLiteral("No such Item %1").arg(item.id())});
            }
            rv.push_back(it.value());
        }
        return rv;
    });
}

Task<Item::List> FakeStorage::fetchItemsForTag(const Tag &tag, const ItemFetchOptions &options, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, tag]() -> Expected<Item::List> {
        Item::List items;
        std::copy_if(mStorage->items.cbegin(), mStorage->items.cend(), std::back_inserter(items),
                     [tag](const auto &item) { return item.hasTag(tag); });
        return items;
    });
}

Task<Item> FakeStorage::fetchItem(const Item &item, const ItemFetchOptions &options, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, item, options]() -> Expected<Item> {
        auto it = mStorage->items.constFind(item.id());
        if (it == mStorage->items.cend()) {
            if (options.itemFetchScope().ignoreRetrievalErrors()) {
                return Item{};
            } else {
                return tl::make_unexpected(Error{1, QStringLiteral("No such Item %1").arg(item.id())});
            }
        }

        return *it;
    });
}

namespace {
auto checkCollectionRights(Collection::Rights rights)
{
    return [=](auto colIt) {
        return (colIt->rights() & rights)
                ? makeExpected(std::forward<decltype(colIt)>(colIt))
                : Expected<decltype(colIt)>(tl::make_unexpected(Error{2, QStringLiteral("Insufficient rights for Collection %1").arg(colIt->id())}));
    };
}
} // namespace

Task<Item> FakeStorage::createItem(const Item &item, const Collection &collection, ItemCreateFlags flags, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, item, collection, flags]() -> Expected<Item> {
        return mStorage->findCollectionConst(collection)
                .and_then(checkCollectionRights(Collection::CanCreateItem))
                .and_then([this, item, flags](auto &&colIt) {
                    Item newItem = item;
                    newItem.setId(++mStorage->lastItemId);
                    newItem.setParentCollection(*colIt);
                    mStorage->items.insert(newItem.id(), newItem);

                    if (!(flags & ItemCreateFlag::Silent)) {
                        mStorage->notify();
                    }

                    return Expected<Item>{newItem};
                });
    });
}

namespace {

auto updateItemStorage(decltype(FakeStoragePrivate::items)::iterator itemIt, const Item &item, ItemModifyFlags flags)
{
    if (flags & ItemModifyFlag::IgnorePayload) {
        itemIt->setFlags(item.flags());
        itemIt->setGid(item.gid());
        itemIt->setMimeType(item.mimeType());
        itemIt->setModificationTime(item.modificationTime());
        itemIt->setRemoteId(item.remoteId());
        itemIt->setRemoteRevision(item.remoteRevision());
        itemIt->setRevision(item.revision());
        itemIt->setTags(item.tags());
    } else {
        *itemIt = item;
    }
}
} // namespace

Task<Item> FakeStorage::updateItem(const Item &item, ItemModifyFlags flags, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, item, flags]() -> Expected<Item> {
        return mStorage->findItem(item)
                .and_then([this, item, flags](auto &&itemIt) {
                    updateItemStorage(itemIt, item, flags);
                    mStorage->notify();
                    return Expected<Item>{*itemIt};
                });
    });
}

namespace {

Expected<void> checkMissingItems(FakeStoragePrivate &storage, const Item::List &items)
{
    const auto missingItem = std::find_if_not(items.cbegin(), items.cend(), [&storage](const Item &item) {
                                              return storage.items.contains(item.id()); });
    if (missingItem == items.cend()) {
        return {};
    } else {
        return tl::make_unexpected(Error{1, QStringLiteral("No such Item &1").arg(missingItem->id())});
    }
}
} // namespace

Task<Item::List> FakeStorage::updateItems(const Item::List &items, ItemModifyFlags flags, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, flags]() -> Expected<Item::List> {
        return checkMissingItems(*mStorage, items)
                .and_then([this, items, flags]() {
                    Item::List rv;
                    rv.reserve(items.size());
                    for (const auto &item : items) {
                        auto it = mStorage->items.find(item.id());
                        Q_ASSERT(it != mStorage->items.end());
                        updateItemStorage(it, item, flags);
                        rv.push_back(*it);
                    }
                    mStorage->notify();
                    return Expected<Item::List>{rv};
                });
    });
}

Task<void> FakeStorage::deleteItem(const Item &item, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, item]() -> Expected<void> {
        return mStorage->findItem(item)
                .and_then([this](auto &&itemIt) {
                    mStorage->items.erase(itemIt);
                    mStorage->notify();

                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::deleteItems(const Item::List &items, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items]() -> Expected<void> {
        return checkMissingItems(*mStorage, items)
                .and_then([this, items]() {
                    for (const auto &item : items) {
                        mStorage->items.remove(item.id());
                    }
                    mStorage->notify();
                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::deleteItemsFromCollection(const Collection &collection, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collection]() -> Expected<void> {
        return mStorage->findCollectionConst(collection)
                .and_then([this, collection](const auto colIt) {
                    for (auto it = mStorage->items.begin(); it != mStorage->items.end();) {
                        if (it->parentCollection() == *colIt) {
                            it = mStorage->items.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    mStorage->notify();

                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::deleteItemsWithTag(const Tag &tag, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, tag]() -> Expected<void> {
        return mStorage->findTagConst(tag)
                .and_then([this](const auto tagIt) {
                    for (auto it = mStorage->items.begin(); it != mStorage->items.end();) {
                        if (it->hasTag(*tagIt)) {
                            it = mStorage->items.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    mStorage->notify();

                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::moveItem(const Item &item, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, item, destination]() -> Expected<void> {
        return mStorage->findItem(item)
                .and_then([this, destination](auto itemIt) {
                    return mStorage->findCollectionConst(destination).and_then([this, itemIt](const auto colIt) {
                        itemIt->setParentCollection(*colIt);
                        mStorage->notify();
                        return Expected<void>{};
                    });
                });
    });
}

namespace {
auto moveItemsStorage(FakeStoragePrivate &storage, const Item::List &items,
                      const typename decltype(FakeStoragePrivate::collections)::const_iterator destIt)
{
    for (const auto &item : items) {
        auto it = storage.items.find(item.id());
        Q_ASSERT(it != storage.items.end());
        it->setParentCollection(*destIt);
    }
}
} // namespace

Task<void> FakeStorage::moveItems(const Item::List &items, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, destination]() -> Expected<void> {
        return checkMissingItems(*mStorage, items)
                .and_then([this, destination]() { return mStorage->findCollectionConst(destination); })
                .and_then([this, items](const auto destIt) {
                    moveItemsStorage(*mStorage, items, destIt);
                    mStorage->notify();
                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::moveItemsFromCollection(const Item::List &items, const Collection &source,
                                   const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, source, destination]() -> Expected<void> {
        return mStorage->findItems(items, source)
                .and_then([this, destination](const auto &items) {
                    return mStorage->findCollectionConst(destination)
                            .and_then([this, items](const auto destIt) {
                                moveItemsStorage(*mStorage, items, destIt);
                                mStorage->notify();
                                return Expected<void>{};
                            });
                });
    });
}

namespace {

void copyItemStorage(FakeStoragePrivate &storage, const Item &item, const Collection &destination)
{
    Item copy = item;
    copy.setId(++storage.lastItemId);
    copy.setParentCollection(destination);
    storage.items.insert(copy.id(), copy);
}

} // namespace

Task<void> FakeStorage::copyItem(const Item &item, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, item, destination]() -> Expected<void> {
        return mStorage->findItemConst(item)
                .and_then([this, destination](const auto &itemIt) {
                    return mStorage->findCollectionConst(destination)
                            .and_then([this, itemIt](const auto &destIt) {
                                copyItemStorage(*mStorage, *itemIt, *destIt);
                                mStorage->notify();
                                return Expected<void>{};
                            });
                });

    });
}

Task<void> FakeStorage::copyItems(const Item::List &items, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, destination]() -> Expected<void> {
        return mStorage->findItems(items, {})
                .and_then([this, destination](const auto &items) {
                    return mStorage->findCollectionConst(destination)
                        .and_then([this, items](const auto &destIt) {
                            for (const auto &item : items) {
                                copyItemStorage(*mStorage, item, *destIt);
                            }
                            mStorage->notify();
                            return Expected<void>{};
                        });
                });
    });
}

Task<void> FakeStorage::linkItems(const Item::List &items, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, destination]() -> Expected<void> {
        return checkMissingItems(*mStorage, items)
                .and_then([this, destination]() { return mStorage->findCollectionConst(destination); })
                .and_then([this, items](const auto &destIt) -> Expected<void> {
                    auto &virts = mStorage->virtualCols[destIt->id()];
                    for (const auto &item : items) {
                        if (!virts.contains(item.id())) {
                            virts.push_back(item.id());
                        }
                    }

                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::unlinkItems(const Item::List &items, const Collection &collection, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, items, collection]() -> Expected<void> {
        return checkMissingItems(*mStorage, items)
                .and_then([this, collection]() { return mStorage->findCollectionConst(collection); })
                .and_then([this, items](const auto &colIt) -> Expected<void> {
                    auto &virts = mStorage->virtualCols[colIt->id()];
                    for (const auto &item : items) {
                        virts.removeOne(item.id());
                    }

                    return Expected<void>{};
                });
    });

}

Task<Item::List> FakeStorage::searchItems(const SearchQuery &searchQuery, const ItemSearchOptions &options,
                             Session *session)
{
    Q_UNUSED(searchQuery);
    Q_UNUSED(options);
    Q_UNUSED(session);

    // TODO
    Q_ASSERT_X(false, "FakeStorage::searchItems", "Not Implemented");
    return Task<Item::List>{};
}

Task<Collection> FakeStorage::fetchCollection(const Collection &collection, const CollectionFetchOptions &options,
                                 Session *session)
{
    Q_UNUSED(session);
    Q_UNUSED(options);

    return mStorage->task([this, collection]() -> Expected<Collection> {
        return mStorage->findCollectionConst(collection)
                    .and_then([](const auto colIt) { return Expected<Collection>{*colIt}; });
    });
}

Task<Collection::List> FakeStorage::fetchCollectionsRecursive(const Collection &root, const CollectionFetchOptions &options,
                                                 Session *session)
{
    Q_UNUSED(session);
    Q_UNUSED(options);

    return mStorage->task([this, root]() -> Expected<Collection::List> {
        return mStorage->findCollectionConst(root)
            .and_then([this](const auto rootIt) -> Expected<Collection::List> {
                Collection::List list;

                Collection::List toProcess;
                toProcess.push_back(*rootIt);
                while (!toProcess.empty()) {
                    auto parent = toProcess.takeFirst();
                    list.push_back(parent);

                    toProcess.append(
                            mStorage->collections
                                | Views::values
                                | Views::filter([parent](const auto &col) {
                                        return col.parentCollection() == parent;
                                  })
                                | Actions::toQVector);
                }

                return list;
            });
    });
}

Task<Collection::List> FakeStorage::fetchCollectionsRecursive(const Collection::List &root, const CollectionFetchOptions &options,
                                                 Session *session)
{
    Q_UNUSED(root);
    Q_UNUSED(options);
    Q_UNUSED(session);
    // TODO

    Q_ASSERT_X(false, "FakeStorage::fetchCollectionsRecursive()", "Not Implemented");
    return Task<Collection::List>{};
}

Task<Collection::List> FakeStorage::fetchCollections(const Collection::List &collections, const CollectionFetchOptions &options,
                                        Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collections]() -> Expected<Collection::List> {
            return mStorage->findCollections(collections);
    });
}

Task<Collection::List> FakeStorage::fetchSubcollections(const Collection &base, const CollectionFetchOptions &options,
                                           Session *session)
{
    Q_UNUSED(options);
    Q_UNUSED(session);

    return mStorage->task([this, base]() -> Expected<Collection::List> {
        return mStorage->findCollectionConst(base)
            .and_then([this](const auto &baseIt) -> Expected<Collection::List> {
                return mStorage->collections
                        | Views::values
                        | Views::filter([baseIt](const auto &col) { return col.parentCollection() == *baseIt; })
                        | Actions::toQVector;
            });
    });
}

Task<Collection> FakeStorage::createCollection(const Collection &collection, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collection]() -> Expected<Collection> {
        Collection col = collection;
        col.setId(++mStorage->lastCollectionId);
        mStorage->collections.insert(col.id(), col);
        mStorage->notify();
        return col;
    });
}

Task<Collection> FakeStorage::updateCollection(const Collection &collection, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collection]() -> Expected<Collection> {
        return mStorage->findCollection(collection)
            .and_then([this, collection](auto colIt) -> Expected<Collection> {
                *colIt = collection;
                mStorage->notify();
                return *colIt;
            });
    });
}

Task<void> FakeStorage::deleteCollection(const Collection &collection, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collection]() -> Expected<void> {
        return mStorage->findCollection(collection)
            .and_then([this](const auto colIt) -> Expected<void> {
                mStorage->collections.erase(colIt);
                mStorage->notify();
                return Expected<void>{};
            });
    });
}

namespace {

auto prepareCollections(FakeStoragePrivate &storage, const Collection &col, const Collection &dest)
{
    return storage.findCollection(col)
            .and_then([&storage, dest](auto colIt) {
                return storage.findCollection(dest)
                    .map([colIt](auto destIt) {
                        return std::make_pair(colIt, destIt);
                    });
            });

}

} // namespace

Task<void> FakeStorage::moveCollection(const Collection &collection, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collection, destination]() -> Expected<void> {
        return prepareCollections(*mStorage, collection, destination)
                .and_then([this](const auto pair) {
                    auto [colIt, destIt] = pair;
                    colIt->setParentCollection(*destIt);
                    mStorage->notify();
                    return Expected<void>{};
                });
    });
}

Task<void> FakeStorage::copyCollection(const Collection &collection, const Collection &destination, Session *session)
{
    Q_UNUSED(session);

    return mStorage->task([this, collection, destination]() -> Expected<void> {
        return prepareCollections(*mStorage, collection, destination)
                .and_then([this](const auto pair) -> Expected<void> {
                        auto [colIt, destIt] = pair;
                        Collection col = *colIt;
                        col.setId(++mStorage->lastCollectionId);
                        col.setParentCollection(*destIt);
                        mStorage->collections.insert(col.id(), col);
                        mStorage->notify();
                        return Expected<void>{};
                });
    });
}

Task<CollectionStatistics> FakeStorage::fetchCollectionStatistics(const Collection &collection, Session *session)
{
    Q_UNUSED(collection);
    Q_UNUSED(session);

    // TODO
    Q_ASSERT_X(false, "FakeStorage::fetchCollectionStatistics()", "Not implemented");
    return Task<CollectionStatistics>{};
}


Task<Tag> FakeStorage::fetchTag(const Tag &tag, const TagFetchOptions &options, Session *session)
{
    Q_UNUSED(options);
    Q_UNUSED(session);

    return mStorage->task([this, tag]() -> Expected<Tag> {
        return mStorage->findTagConst(tag).map([](auto tagIt) { return *tagIt; });
    });
}

Task<Tag::List> FakeStorage::fetchTags(const Tag::List &tags, const TagFetchOptions &options,
                                 Session *session)
{
    Q_UNUSED(options);
    Q_UNUSED(session);

    return mStorage->task([this, tags]() -> Expected<Tag::List> {
        return mStorage->findTags(tags);
    });
}

Task<Tag::List> FakeStorage::fetchAllTags(const TagFetchOptions &options, Session *session)
{
    Q_UNUSED(options);
    Q_UNUSED(session);

    return mStorage->task([this]() -> Expected<Tag::List> {
        return mStorage->tags | Views::values | Actions::toQVector;
    });
}

Task<Tag> FakeStorage::createTag(const Tag &tag, Session *session)
{
    Q_UNUSED(tag);
    Q_UNUSED(session);
    // TODO
    return Task<Tag>{};
}

Task<Tag> FakeStorage::updateTag(const Tag &tag, Session *session)
{
    Q_UNUSED(tag);
    Q_UNUSED(session);
    // TODO
    return Task<Tag>{};
}

Task<void> FakeStorage::deleteTag(const Tag &tag, Session *session)
{
    Q_UNUSED(tag);
    Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

Task<void> FakeStorage::deleteTags(const Tag::List &tags, Session *session)
{
    Q_UNUSED(tags);
    Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

Task<Relation::List> FakeStorage::fetchRelation(const Relation &relation, Session *session)
{
    Q_UNUSED(relation);
    Q_UNUSED(session);
    // TODO
    return Task<Relation::List>{};
}

Task<Relation::List> FakeStorage::fetchRelationsOfTypes(const QVector<QByteArray> &types, Session *session)
{
    Q_UNUSED(types);
    Q_UNUSED(session);
    // TODO
    return Task<Relation::List>{};
}

Task<Relation> FakeStorage::createRelation(const Relation &relation, Session *session)
{
    Q_UNUSED(relation);
    Q_UNUSED(session);
    // TODO
    return Task<Relation>{};
}

Task<void> FakeStorage::deleteRelation(const Relation &relation, Session *session)
{
    Q_UNUSED(relation);
    Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

Task<Collection> FakeStorage::createSearchCollection(const QString &name, const SearchQuery &searchQuery,
                                        const ItemSearchOptions &options, Session *session)
{
    Q_UNUSED(name);
    Q_UNUSED(searchQuery);
    Q_UNUSED(options);
    Q_UNUSED(session);
    // TODO
    return Task<Collection>{};
}

Task<void> FakeStorage::beginTransaction(Session *session)
{
    Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

Task<void> FakeStorage::commitTransaction(Session *session)
{
    Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

Task<void> FakeStorage::rollbackTransaction(Session *session)
{
    Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

Task<void> FakeStorage::selectResource(const QString &identifier, Session *session)
{
    Q_UNUSED(identifier); Q_UNUSED(session);
    // TODO
    return Task<void>{};
}

ChangeNotificationDependenciesFactory &FakeStorage::changeNotificationDependenciesFactory()
{
    return mStorage->dependenciesFactory;
}

