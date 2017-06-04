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

#ifndef COLLECTIONTREECACHE_H
#define COLLECTIONTREECACHE_H

#include "akthread.h"
#include "entities.h"

#include <private/tristate_p.h>

#include <QReadWriteLock>
#include <QVector>
#include <QHash>
#include <QVector>

class QSqlQuery;

namespace Akonadi {

class Scope;

namespace Server {

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

        Node *parent;
        QVector<Node *> children;
        QAtomicInt lruCounter;
        qint64 id;

        Collection collection;
    };

public:
    explicit CollectionTreeCache();
    ~CollectionTreeCache();

    QVector<Collection> retrieveCollections(const Scope &scope,
                                            int depth, int ancestorDepth,
                                            const QString &resource = QString(),
                                            CommandContext *context = Q_NULLPTR) const;

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

    Node *mRoot;

    QHash<qint64 /* col ID */, Node *> mNodeLookup;
};


// Non-recursive depth-first tree traversal, looking for first Node matching the predicate
template<typename Predicate>
CollectionTreeCache::Node *CollectionTreeCache::findNode(Node *root, Predicate pred) const
{
    QList<Node *> toVisit = { root };
    // We expect a single subtree to not contain more than 1/4 of all collections,
    // which is an arbitrary guess, but should be good enough for most cases.
    toVisit.reserve(mNodeLookup.size() / 4);
    while (!toVisit.isEmpty()) {
        auto node = toVisit.takeFirst();
        if (pred(node)) {
            return node;
        }

        for (auto child : qAsConst(node->children)) {
            toVisit.prepend(child);
        }
    }

    return Q_NULLPTR;
}

} // namespace Server
} // namespace Akonadi
#endif // COLLECTIONTREECACHE
