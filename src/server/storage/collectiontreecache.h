/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"
#include "entities.h"

#include <private/tristate_p.h>

#include <QHash>
#include <QReadWriteLock>
#include <QVector>

namespace Akonadi
{
class Scope;

namespace Server
{
class CommandContext;

class CollectionTreeCache : public AkThread
{
    Q_OBJECT

protected:
    class Node
    {
    public:
        explicit Node();
        explicit Node(const Collection &query);

        ~Node();

        void appendChild(Node *child);
        void removeChild(Node *child);

        Node *parent = nullptr;
        QVector<Node *> children;
        QAtomicInt lruCounter;
        qint64 id;

        Collection collection;
    };

public:
    explicit CollectionTreeCache();
    ~CollectionTreeCache() override;

    QVector<Collection>
    retrieveCollections(const Scope &scope, int depth, int ancestorDepth, const QString &resource = QString(), CommandContext *context = nullptr) const;

public Q_SLOTS:
    void collectionAdded(const Collection &col);
    void collectionChanged(const Collection &col);
    void collectionMoved(const Collection &col);
    void collectionRemoved(const Collection &col);

protected:
    void init() override;
    void quit() override;

    Node *findNode(const QString &rid, const QString &resource) const;

    template<typename Predicate>
    Node *findNode(Node *root, Predicate pred) const;

    QVector<Collection> retrieveCollections(Node *root, int depth, int ancestorDepth) const;

protected:
    mutable QReadWriteLock mLock;

    Node *mRoot = nullptr;

    QHash<qint64 /* col ID */, Node *> mNodeLookup;
};

// Non-recursive depth-first tree traversal, looking for first Node matching the predicate
template<typename Predicate>
CollectionTreeCache::Node *CollectionTreeCache::findNode(Node *root, Predicate pred) const
{
    QList<Node *> toVisit = {root};
    // We expect a single subtree to not contain more than 1/4 of all collections,
    // which is an arbitrary guess, but should be good enough for most cases.
    toVisit.reserve(mNodeLookup.size() / 4);
    while (!toVisit.isEmpty()) {
        auto node = toVisit.takeFirst();
        if (pred(node)) {
            return node;
        }

        for (auto child : std::as_const(node->children)) {
            toVisit.prepend(child);
        }
    }

    return nullptr;
}

} // namespace Server
} // namespace Akonadi
