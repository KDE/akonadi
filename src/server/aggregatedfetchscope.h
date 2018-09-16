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
    void addAttribute(const QByteArray &attribute);
    void removeAttribute(const QByteArray &attribute);

    bool fetchIdOnly() const;
    void setFetchIdOnly(bool fetchIdOnly);

    bool fetchStatistics() const;
    void setFetchStatistics(bool fetchStats);

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
    void addRequestedPart(const QByteArray &part);
    void removeRequestedPart(const QByteArray &part);

    Protocol::ItemFetchScope::AncestorDepth ancestorDepth() const;
    void updateAncestorDepth(Protocol::ItemFetchScope::AncestorDepth oldDepth,
                             Protocol::ItemFetchScope::AncestorDepth newDepth);

    bool cacheOnly() const;
    void setCacheOnly(bool cacheOnly);
    bool fullPayload() const;
    void setFullPayload(bool fullPayload);
    bool allAttributes() const;
    void setAllAttributes(bool allAttributes);
    bool fetchSize() const;
    void setFetchSize(bool fetchSize);
    bool fetchMTime() const;
    void setFetchMTime(bool fetchMTime);
    bool fetchRemoteRevision() const;
    void setFetchRemoteRevision(bool remoteRevision);
    bool ignoreErrors() const;
    void setIgnoreErrors(bool ignoreErrors);
    bool fetchFlags() const;
    void setFetchFlags(bool fetchFlags);
    bool fetchRemoteId() const;
    void setFetchRemoteId(bool fetchRemoteId);
    bool fetchGID() const;
    void setFetchGID(bool fetchGid);
    bool fetchRelations() const;
    void setFetchRelations(bool fetchRelations);
    bool fetchVirtualReferences() const;
    void setFetchVirtualReferences(bool fetchVRefs);

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
    void addAttribute(const QByteArray &attribute);
    void removeAttribute(const QByteArray &attribute);

    bool fetchIdOnly() const;
    void setFetchIdOnly(bool fetchIdOnly);

    bool fetchRemoteId() const;
    void setFetchRemoteId(bool fetchRemoteId);

    bool fetchAllAttributes() const;
    void setFetchAllAttributes(bool fetchAllAttributes);
private:
    AggregatedTagFetchScopePrivate * const d_ptr;
    Q_DECLARE_PRIVATE(AggregatedTagFetchScope)
};

} // namespace Server
} // namespace Akonadi

#endif
