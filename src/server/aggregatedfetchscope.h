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

#ifndef AKONADI_AGGREGATED_FETCHSCOPE_H_
#define AKONADI_AGGREGATED_FETCHSCOPE_H_

#include <QSet>

#include <private/protocol_p.h>

class QByteArray;

namespace Akonadi {
namespace Server {

class AggregatedCollectionFetchScopePrivate;
class AggregatedCollectionFetchScope
{
public:
    explicit AggregatedCollectionFetchScope();
    ~AggregatedCollectionFetchScope();

    void apply(const Protocol::CollectionFetchScope &oldScope,
               const Protocol::CollectionFetchScope &newScope);

    QSet<QByteArray> attributes() const;

    bool fetchIdOnly() const;
    bool fetchStatistics() const;

    void addSubscriber();
    void removeSubscriber();

private:
    AggregatedCollectionFetchScopePrivate * const d_ptr;
    Q_DECLARE_PRIVATE(AggregatedCollectionFetchScope)
};

class AggregatedItemFetchScopePrivate;
class AggregatedItemFetchScope
{
public:
    explicit AggregatedItemFetchScope();
    ~AggregatedItemFetchScope();

    void apply(const Protocol::ItemFetchScope &oldScope,
               const Protocol::ItemFetchScope &newScope);
    Protocol::ItemFetchScope toFetchScope() const;

    QSet<QByteArray> requestedParts() const;

    Protocol::ItemFetchScope::AncestorDepth ancestorDepth() const;
    void updateAncestorDepth(Protocol::ItemFetchScope::AncestorDepth oldDepth,
                             Protocol::ItemFetchScope::AncestorDepth newDepth);

    bool cacheOnly() const;
    bool fullPayload() const;
    bool allAttributes() const;
    bool fetchSize() const;
    bool fetchMTime() const;
    bool fetchRemoteRevision() const;
    bool ignoreErrors() const;
    bool fetchFlags() const;
    bool fetchRemoteId() const;
    bool fetchGID() const;
    bool fetchTags() const;
    bool fetchRelations() const;
    bool fetchVirtualReferences() const;

    void addSubscriber();
    void removeSubscriber();

private:
    AggregatedItemFetchScopePrivate * const d_ptr;
    Q_DECLARE_PRIVATE(AggregatedItemFetchScope)
};


class AggregatedTagFetchScopePrivate;
class AggregatedTagFetchScope
{
public:
    explicit AggregatedTagFetchScope();
    ~AggregatedTagFetchScope();

    void apply(const Protocol::TagFetchScope &oldScope,
               const Protocol::TagFetchScope &newScope);
    Protocol::TagFetchScope toFetchScope() const;

    QSet<QByteArray> attributes() const;

    void addSubscriber();
    void removeSubscriber();

    bool fetchIdOnly() const;
    bool fetchRemoteId() const;
    bool fetchAllAttributes() const;

private:
    AggregatedTagFetchScopePrivate * const d_ptr;
    Q_DECLARE_PRIVATE(AggregatedTagFetchScope)
};

} // namespace Server
} // namespace Akonadi

#endif
