/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiontreecache.h"
#include "akonadiserver_debug.h"
#include "commandcontext.h"
#include "selectquerybuilder.h"

#include "private/scope_p.h"

#include <QStack>

#include <algorithm>
#include <iterator>
#include <map>
#include <tuple>

using namespace Akonadi::Server;

namespace
{
enum Column {
    IdColumn,
    ParentIdColumn,
    RIDColumn,
    DisplayPrefColumn,
    SyncPrefColumn,
    IndexPrefColumn,
    EnabledColumn,
    ReferencedColumn,
    ResourceNameColumn,
};
}

CollectionTreeCache::Node::Node()
    : parent(nullptr)
    , lruCounter(0)
    , id(-1)
{
}

CollectionTreeCache::Node::Node(const Collection &col)
    : parent(nullptr)
    , id(col.id())
    , collection(col)

{
}

CollectionTreeCache::Node::~Node()
{
    qDeleteAll(children);
}

void CollectionTreeCache::Node::appendChild(Node *child)
{
    child->parent = this;
    children.push_back(child);
}

void CollectionTreeCache::Node::removeChild(Node *child)
{
    child->parent = nullptr;
    children.removeOne(child);
}

CollectionTreeCache::CollectionTreeCache()
    : AkThread(QStringLiteral("CollectionTreeCache"))
{
}

CollectionTreeCache::~CollectionTreeCache()
{
    quitThread();
}

void CollectionTreeCache::init()
{
    AkThread::init();

    QWriteLocker locker(&mLock);

    mRoot = new Node;
    mRoot->id = 0;
    mRoot->parent = nullptr;
    mNodeLookup.insert(0, mRoot);

    SelectQueryBuilder<Collection> qb;
    qb.addSortColumn(Collection::idFullColumnName(), Query::Ascending);

    if (!qb.exec()) {
        qCCritical(AKONADISERVER_LOG) << "Failed to initialize Collection tree cache!";
        return;
    }

    // std's reverse iterators makes processing pendingNodes much easier.
    std::multimap<qint64 /* parentID */, Node *> pendingNodes;
    const auto collections = qb.result();
    for (const auto &col : collections) {
        auto parent = mNodeLookup.value(col.parentId(), nullptr);

        auto node = new Node(col);

        if (parent) {
            parent->appendChild(node);
            mNodeLookup.insert(node->id, node);
        } else {
            pendingNodes.insert({col.parentId(), node});
        }
    }

    if (!pendingNodes.empty()) {
        int inserts = 0;

        // Thanks to the SQL results being ordered by ID we already inserted most
        // of the nodes in the loop above and here we only handle the rare cases
        // when child has ID lower than its parent, i.e. moved collections.
        //
        // In theory we may need multiple passes to insert all nodes, but we can
        // optimize by iterating the ordered map in reverse order. This way we find
        // the parents with higher IDs first and their children later, thus needing
        // fewer passes to process all the nodes.
        auto it = pendingNodes.rbegin();
        while (!pendingNodes.empty()) {
            auto parent = mNodeLookup.value(it->first, nullptr);
            if (!parent) {
                // Parent of this node is still somewhere in pendingNodes, let's skip
                // this one for now and try again in the next iteration
                ++it;
            } else {
                auto node = it->second;
                parent->appendChild(node);
                mNodeLookup.insert(node->id, node);
                pendingNodes.erase((++it).base());
                ++inserts;
            }

            if (it == pendingNodes.rend()) {
                if (Q_UNLIKELY(inserts == 0)) {
                    // This means we iterated through the entire pendingNodes but did
                    // not manage to insert any collection to the node tree. That
                    // means that there is an unreferenced collection in the database
                    // that points to an invalid parent (or has a parent which points
                    // to an invalid parent etc.). This should not happen
                    // anymore with DB constraints, but who knows...
                    qCWarning(AKONADISERVER_LOG) << "Found unreferenced Collections!";
                    auto unref = pendingNodes.begin();
                    while (unref != pendingNodes.end()) {
                        qCWarning(AKONADISERVER_LOG) << "\tCollection" << unref->second->id << "references an invalid parent" << it->first;
                        // Remove the unreferenced collection from the map
                        delete unref->second;
                        unref = pendingNodes.erase(unref);
                    }
                    qCWarning(AKONADISERVER_LOG) << "Please run \"akonadictl fsck\" to correct the inconsistencies!";
                    // pendingNodes should be empty now so break the loop here
                    break;
                }

                it = pendingNodes.rbegin();
                inserts = 0;
            }
        }
    }

    Q_ASSERT(pendingNodes.empty());
    Q_ASSERT(mNodeLookup.size() == collections.count() + 1 /* root */);
    // Now we should have a complete tree built, yay!
}

void CollectionTreeCache::quit()
{
    delete mRoot;

    AkThread::quit();
}

void CollectionTreeCache::collectionAdded(const Collection &col)
{
    QWriteLocker locker(&mLock);

    auto parent = mNodeLookup.value(col.parentId(), nullptr);
    if (!parent) {
        qCWarning(AKONADISERVER_LOG) << "Received a new collection (" << col.id() << ") with unknown parent (" << col.parentId() << ")";
        return;
    }

    auto node = new Node(col);
    parent->appendChild(node);
    mNodeLookup.insert(node->id, node);
}

void CollectionTreeCache::collectionChanged(const Collection &col)
{
    QWriteLocker locker(&mLock);

    auto node = mNodeLookup.value(col.id(), nullptr);
    if (!node) {
        qCWarning(AKONADISERVER_LOG) << "Received an unknown changed collection (" << col.id() << ")";
        return;
    }

    // Only update non-expired nodes
    if (node->collection.isValid()) {
        node->collection = col;
    }
}

void CollectionTreeCache::collectionMoved(const Collection &col)
{
    QWriteLocker locker(&mLock);

    auto node = mNodeLookup.value(col.id(), nullptr);
    if (!node) {
        qCWarning(AKONADISERVER_LOG) << "Received an unknown moved collection (" << col.id() << ")";
        return;
    }
    auto oldParent = node->parent;

    auto newParent = mNodeLookup.value(col.parentId(), nullptr);
    if (!newParent) {
        qCWarning(AKONADISERVER_LOG) << "Received a moved collection (" << col.id() << ") with an unknown move destination (" << col.parentId() << ")";
        return;
    }

    oldParent->removeChild(node);
    newParent->appendChild(node);
    if (node->collection.isValid()) {
        node->collection = col;
    }
}

void CollectionTreeCache::collectionRemoved(const Collection &col)
{
    QWriteLocker locker(&mLock);

    auto node = mNodeLookup.value(col.id(), nullptr);
    if (!node) {
        qCWarning(AKONADISERVER_LOG) << "Received unknown removed collection (" << col.id() << ")";
        return;
    }

    auto parent = node->parent;
    parent->removeChild(node);
    mNodeLookup.remove(node->id);
    delete node;
}

CollectionTreeCache::Node *CollectionTreeCache::findNode(const QString &rid, const QString &resource) const
{
    QReadLocker locker(&mLock);

    // Find a subtree that belongs to the respective resource
    auto root = std::find_if(mRoot->children.cbegin(), mRoot->children.cend(), [resource](Node *node) {
        // resource().name() may seem expensive, but really
        // there are only few resources and they are all cached
        // in memory.
        return node->collection.resource().name() == resource;
    });
    if (root == mRoot->children.cend()) {
        return nullptr;
    }

    return findNode((*root), [rid](Node *node) {
        return node->collection.remoteId() == rid;
    });
}

QList<Collection> CollectionTreeCache::retrieveCollections(CollectionTreeCache::Node *root, int depth, int ancestorDepth) const
{
    QReadLocker locker(&mLock);

    QList<Node *> nodes;
    // Get all ancestors for root
    Node *parent = root->parent;
    for (int i = 0; i < ancestorDepth && parent != nullptr; ++i) {
        nodes.push_back(parent);
        parent = parent->parent;
    }

    struct StackTuple {
        Node *node;
        int depth;
    };
    QStack<StackTuple> stack;
    stack.push({root, 0});
    while (!stack.isEmpty()) {
        auto c = stack.pop();
        if (c.depth > depth) {
            break;
        }

        if (c.node->id > 0) { // skip root
            nodes.push_back(c.node);
        }

        for (auto child : std::as_const(c.node->children)) {
            stack.push({child, c.depth + 1});
        }
    }

    QList<Collection> cols;
    QList<Node *> missing;
    for (auto node : nodes) {
        if (node->collection.isValid()) {
            cols.push_back(node->collection);
        } else {
            missing.push_back(node);
        }
    }

    if (!missing.isEmpty()) {
        // TODO: Check if no-one else is currently retrieving the same collections
        SelectQueryBuilder<Collection> qb;
        Query::Condition cond(Query::Or);
        for (auto node : std::as_const(missing)) {
            cond.addValueCondition(Collection::idFullColumnName(), Query::Equals, node->id);
        }
        qb.addCondition(cond);
        if (!qb.exec()) {
            qCWarning(AKONADISERVER_LOG) << "Failed to retrieve collections from the database";
            return {};
        }

        const auto results = qb.result();
        if (results.size() != missing.size()) {
            qCWarning(AKONADISERVER_LOG) << "Could not obtain all missing collections! Node tree refers to a non-existent collection";
        }

        cols += results;

        // Relock for write
        // TODO: Needs a better lock-upgrade mechanism
        locker.unlock();
        QWriteLocker wLocker(&mLock);
        for (auto node : std::as_const(missing)) {
            auto it = std::find_if(results.cbegin(), results.cend(), [node](const Collection &col) {
                return node->id == col.id();
            });
            if (Q_UNLIKELY(it == results.cend())) {
                continue;
            }

            node->collection = *it;
        }
    }

    return cols;
}

QList<Collection>
CollectionTreeCache::retrieveCollections(const Scope &scope, int depth, int ancestorDepth, const QString &resource, CommandContext *context) const
{
    if (scope.isEmpty()) {
        return retrieveCollections(mRoot, depth, ancestorDepth);
    } else if (scope.scope() == Scope::Rid) {
        // Caller must ensure!
        Q_ASSERT(!resource.isEmpty() || (context && context->resource().isValid()));

        Node *node = nullptr;
        if (!resource.isEmpty()) {
            node = findNode(scope.rid(), resource);
        } else if (context && context->resource().isValid()) {
            node = findNode(scope.rid(), context->resource().name());
        } else {
            return {};
        }

        if (Q_LIKELY(node)) {
            return retrieveCollections(node, depth, ancestorDepth);
        }
    } else if (scope.scope() == Scope::Uid) {
        Node *node = mNodeLookup.value(scope.uid());
        if (Q_LIKELY(node)) {
            return retrieveCollections(node, depth, ancestorDepth);
        }
    }

    return {};
}

#include "moc_collectiontreecache.cpp"
