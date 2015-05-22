/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_PROTOCOL_P_H
#define AKONADI_PROTOCOL_P_H

#include "akonadiprivate_export.h"

#include "scope_p.h"

#include <QtCore/QFlags>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QDateTime>

class QDataStream;

/**
  @file protocol_p.h Shared constants used in the communication protocol between
  the Akonadi server and its clients.

  @todo Fill this file with command names, item/collection property names
  item part names, etc. and replace the usages accordingly.
*/

// D-Bus service names
#define AKONADI_DBUS_SERVER_SERVICE           "org.freedesktop.Akonadi"
#define AKONADI_DBUS_CONTROL_SERVICE          "org.freedesktop.Akonadi.Control"
#define AKONADI_DBUS_CONTROL_SERVICE_LOCK     "org.freedesktop.Akonadi.Control.lock"
#define AKONADI_DBUS_AGENTSERVER_SERVICE      "org.freedesktop.Akonadi.AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_SERVICE   "org.freedesktop.Akonadi.Janitor"
#define AKONADI_DBUS_SERVER_SERVICE_UPGRADING "org.freedesktop.Akonadi.upgrading"

#define AKONADI_DBUS_AGENTMANAGER_PATH   "/AgentManager"
#define AKONADI_DBUS_AGENTSERVER_PATH    "/AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_PATH "/Janitor"


// Commands
#define AKONADI_CMD_APPEND           "APPEND"
#define AKONADI_CMD_BEGIN            "BEGIN"
#define AKONADI_CMD_CAPABILITY       "CAPABILITY"
#define AKONADI_CMD_COLLECTION       "COLLECTION"
#define AKONADI_CMD_COLLECTIONCOPY   "COLCOPY"
#define AKONADI_CMD_COLLECTIONMOVE   "COLMOVE"
#define AKONADI_CMD_COMMIT           "COMMIT"
#define AKONADI_CMD_ITEMCOPY         "COPY"
#define AKONADI_CMD_COLLECTIONCREATE "CREATE"
#define AKONADI_CMD_COLLECTIONDELETE "DELETE"
#define AKONADI_CMD_EXPUNGE          "EXPUNGE"
#define AKONADI_CMD_ITEMFETCH        "FETCH"
#define AKONADI_CMD_GID              "GID"
#define AKONADI_CMD_HRID             "HRID"
#define AKONADI_CMD_ITEMLINK         "LINK"
#define AKONADI_CMD_LIST             "LIST"
#define AKONADI_CMD_LOGIN            "LOGIN"
#define AKONADI_CMD_LOGOUT           "LOGOUT"
#define AKONADI_CMD_LSUB             "LSUB"
#define AKONADI_CMD_MERGE            "MERGE"
#define AKONADI_CMD_COLLECTIONMODIFY "MODIFY"
#define AKONADI_CMD_ITEMMOVE         "MOVE"
#define AKONADI_CMD_ITEMDELETE       "REMOVE"
#define AKONADI_CMD_RESOURCESELECT   "RESSELECT"
#define AKONADI_CMD_RID              "RID"
#define AKONADI_CMD_ROLLBACK         "ROLLBACK"
#define AKONADI_CMD_RELATIONSTORE    "RELATIONSTORE"
#define AKONADI_CMD_RELATIONREMOVE   "RELATIONREMOVE"
#define AKONADI_CMD_RELATIONFETCH    "RELATIONFETCH"
#define AKONADI_CMD_SUBSCRIBE        "SUBSCRIBE"
#define AKONADI_CMD_SEARCH           "SEARCH"
#define AKONADI_CMD_SEARCH_RESULT    "SEARCH_RESULT"
#define AKONADI_CMD_SEARCH_STORE     "SEARCH_STORE"
#define AKONADI_CMD_SELECT           "SELECT"
#define AKONADI_CMD_STATUS           "STATUS"
#define AKONADI_CMD_ITEMMODIFY       "STORE"
#define AKONADI_CMD_TAGAPPEND        "TAGAPPEND"
#define AKONADI_CMD_TAGFETCH         "TAGFETCH"
#define AKONADI_CMD_TAGREMOVE        "TAGREMOVE"
#define AKONADI_CMD_TAGSTORE         "TAGSTORE"
#define AKONADI_CMD_UID              "UID"
#define AKONADI_CMD_ITEMUNLINK       "UNLINK"
#define AKONADI_CMD_UNSUBSCRIBE      "UNSUBSCRIBE"
#define AKONADI_CMD_ITEMCREATE       "X-AKAPPEND"
#define AKONADI_CMD_X_AKLIST         "X-AKLIST"
#define AKONADI_CMD_X_AKLSUB         "X-AKLSUB"

namespace Akonadi
{
namespace Protocol
{
class FetchScope;
class PartMetaData;
class CachePolicy;
class Ancestor;

class Command;
class Response;
class HelloResponse;
class LoginCommand;
class LoginResponse;
class TransactionCommand;
class TransactionResponse;
class CreateItemCommand;
class CreateItemResponse;
class CopyItemsCommand;
class CopyItemsResponse;
class DeleteItemsCommand;
class DeleteItemsResponse;
class FetchRelationsCommand;
class FetchRelationsResponse;
class FetchTagsCommand;
class FetchTagsResponse;
class FetchItemsCommand;
class FetchItemsResponse;
class LinkItemsCommand;
class LinkItemsResponse;
class ModifyItemsCommand;
class ModifyItemsResponse;
class MoveItemsCommand;
class MoveItemsResponse;
class CreateCollectionCommand;
class CreateCollectionResponse;
class CopyCollectionCommand;
class CopyCollectionResponse;
class DeleteCollectionCommand;
class DeleteCollectionResponse;
class FetchCollectionStatsCommand;
class FetchCollectionStatsResponse;
class FetchCollectionsCommand;
class FetchCollectionsResponse;
class ModifyCollectionCommand;
class ModifyCollectionResponse;
class MoveCollectionCommand;
class MoveCollectionResponse;
class SelectCollectionCommand;
class SelectCollectionResponse;
class SearchCommand;
class SearchResponse;
class SearchResultCommand;
class SearchResultResponse;
class StoreSearchCommand;
class StoreSearchResponse;
class CreateTagCommand;
class CreateTagResponse;
class DeleteTagCommand;
class DeleteTagResponse;
class ModifyTagCommand;
class ModifyTagResponse;
class ModifyRelationCommand;
class ModifyRelationResponse;
class RemoveRelationsCommand;
class RemoveRelationsResponse;
class SelectResourceCommand;
class SelectResourceResponse;
class StreamPayloadCommand;
class PayloadStreamResponse;
}
}
QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchScope &scope);
QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchScope &scope);
QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::Part &part);
QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::Part &part);
QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CachePolicy &policy);
QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CachePolicy &policy);
QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::Ancestor &ancestor);
QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::Ancestor &ancestor);

AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchScope &scope);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchScope &scope);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::Part &part);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::Part &part);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CachePolicy &policy);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CachePolicy &policy);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::Ancestor &ancestor);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::Ancestor &ancestor);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::Command &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::Command &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::Response &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::Response &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::HelloResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::HelloResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::LoginCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::LoginCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::TransactionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::TransactionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CreateItemCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CreateItemCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CopyItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CopyItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::DeleteItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::DeleteItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchRelationsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchRelationsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchRelationsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchRelationsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchTagsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchTagsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchTagsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchTagsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchItemsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchItemsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::LinkItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::LinkItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::ModifyItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::MoveItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::MoveItemsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CreateCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CreateCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CopyCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CopyCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::DeleteCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::DeleteCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionStatsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionStatsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionStatsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionStatsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionsResponse &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::ModifyCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::MoveCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::MoveCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::SelectCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::SelectCollectionCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::SearchCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::SearchCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::SearchResultCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::SearchResultCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::StoreSearchCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::StoreSearchCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::CreateTagCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::CreateTagCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::DeleteTagCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::DeleteTagCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyTagCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::ModifyTagCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyRelationCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::ModifyRelationCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::RemoveRelationsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::RemoveRelationsCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::SelectResourceCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::SelectResourceCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::PayloadStreamCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::PayloadStreamCommand &command);
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Protocol::PayloadStreamResponse&command);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Protocol::PayloadStreamResponse &command);

namespace Akonadi
{

// TODO: Move to elswhere so we don't have to include whole protocol_p.h
// everywhere
enum class Tristate : qint8
{
    False     = 0,
    True      = 1,
    Undefined = 2
};

namespace Protocol
{

class AKONADIPRIVATE_EXPORT FetchScope
{
public:
    enum FetchFlag : int {
        None            = 0,
        CacheOnly       = 1 << 0,
        CheckCachedPayloadPartsOnly = 1 << 1,
        FullPayload     = 1 << 2,
        AllAttributes   = 1 << 3,
        Size            = 1 << 4,
        MTime           = 1 << 5,
        RemoteRevision  = 1 << 6,
        IgnoreErrors    = 1 << 7,
        Flags           = 1 << 8,
        RemoteID        = 1 << 9,
        GID             = 1 << 10,
        Tags            = 1 << 11,
        Relations       = 1 << 12,
        VirtReferences  = 1 << 13
    };
    Q_DECLARE_FLAGS(FetchFlags, FetchFlag)

    FetchScope()
        : mFetchFlags(None)
    {}

    void setRequestedParts(const QVector<QByteArray> &requestedParts)
    {
        mRequestedParts = requestedParts;
    }
    QVector<QByteArray> requestedParts() const
    {
        return mRequestedParts;
    }
    void setRequestedPayloads(const QStringList &requestedPayloads)
    {
        mRequestedPayloads = requestedPayloads;
    }
    QStringList requestedPayloads() const
    {
        return mRequestedPayloads;
    }
    void setChangedSince(const QDateTime &changedSince)
    {
        mChangedSince = changedSince;
    }
    QDateTime changedSince() const
    {
        return mChangedSince;
    }
    void setTagFetchScope(const QVector<QByteArray> &tagFetchScope)
    {
        mTagFetchScope = tagFetchScope;
    }
    QVector<QByteArray> tagFetchScope() const
    {
        return mTagFetchScope;
    }
    void setAncestorDepth(int depth)
    {
        mAncestorDepth = depth;
    }
    int ancestorDepth() const
    {
        return mAncestorDepth;
    }

    bool cacheOnly() const
    {
        return mFetchFlags & CacheOnly;
    }
    bool checkCachedPayloadPartsOnly() const
    {
        return mFetchFlags & CheckCachedPayloadPartsOnly;
    }
    bool fullPayload() const
    {
        return mFetchFlags & FullPayload;
    }
    bool allAttributes() const
    {
        return mFetchFlags & AllAttributes;
    }
    bool fetchSize() const
    {
        return mFetchFlags & Size;
    }
    bool fetchMTime() const
    {
        return mFetchFlags & MTime;
    }
    bool fetchRemoteRevision() const
    {
        return mFetchFlags & RemoteRevision;
    }
    bool ignoreErrors() const
    {
        return mFetchFlags & IgnoreErrors;
    }
    bool fetchFlags() const
    {
        return mFetchFlags & Flags;
    }
    bool fetchRemoteId() const
    {
        return mFetchFlags & RemoteID;
    }
    bool fetchGID() const
    {
        return mFetchFlags & GID;
    }
    bool fetchTags() const
    {
        return mFetchFlags & Tags;
    }
    bool fetchRelations() const
    {
        return mFetchFlags & Relations;
    }
    bool fetchVirtualReferences() const
    {
        return mFetchFlags & VirtReferences;
    }

    void setFetch(FetchFlag attribute, bool fetch = true)
    {
        if (fetch) {
            mFetchFlags |= attribute;
        } else {
            mFetchFlags &= ~attribute;
        }
    }

    bool fetch(FetchFlag flag) const
    {
        return mFetchFlags & flag;
    }

private:
    QVector<QByteArray> mRequestedParts;
    QStringList mRequestedPayloads;
    QDateTime mChangedSince;
    QVector<QByteArray> mTagFetchScope;
    int mAncestorDepth;
    FetchFlags mFetchFlags;

    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchScope &scope);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchScope &scope);
};

class AKONADIPRIVATE_EXPORT PartMetaData
{
public:
    PartMetaData()
        : mSize(0)
        , mVersion(0)
    {}

    void setName(const QByteArray &name)
    {
        mName = name;
    }
    QByteArray name() const
    {
        return mName;
    }

    void setSize(qint64 size)
    {
        mSize = size;
    }
    qint64 size() const
    {
        return mSize;
    }

    void setVersion(int version)
    {
        mVersion = version;
    }
    int version() const
    {
        return mVersion;
    }

private:
    QByteArray mName;
    qint64 mSize;
    int mVersion;

    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::PartMetaData &part);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::PartMetaData &part);
};

class AKONADIPRIVATE_EXPORT CachePolicy
{
public:
    CachePolicy()
        : mInterval(-1)
        , mCacheTimeout(-1)
        , mSyncOnDemand(false)
        , mInherit(true)
    {}

    void setInherit(bool inherit)
    {
        mInherit = inherit;
    }
    bool inherit() const
    {
        return mInherit;
    }

    void setCheckInterval(int interval)
    {
        mInterval = interval;
    }
    int checkInterval() const
    {
        return mInterval;
    }

    void setCacheTimeout(int timeout)
    {
        mCacheTimeout = timeout;
    }
    int cacheTimeout() const
    {
        return mCacheTimeout;
    }

    void setSyncOnDemand(bool onDemand)
    {
        mSyncOnDemand = onDemand;
    }
    bool syncOnDemand() const
    {
        return mSyncOnDemand;
    }

    void setLocalParts(const QStringList &parts)
    {
        mLocalParts = parts;
    }
    QStringList localParts() const
    {
        return mLocalParts;
    }

private:
    QStringList mLocalParts;
    int mInterval;
    int mCacheTimeout;
    bool mSyncOnDemand;
    bool mInherit;

    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::CachePolicy &policy);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::CachePolicy &policy);
};

class AKONADIPRIVATE_EXPORT Ancestor
{
public:
    Ancestor()
        : mId(-1)
    {}

    void setId(qint64 id)
    {
        mId = id;
    }
    qint64 id() const
    {
        return mId;
    }

    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }

    void setAttributes(const QMap<QByteArray, QByteArray> &attrs)
    {
        mAttributes = attrs;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }

private:
    qint64 mId;
    QString mRemoteId;
    QMap<QByteArray, QByteArray> mAttributes;

    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::Ancestor &ancestor);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::Ancestor &ancestor);
};

class AKONADIPRIVATE_EXPORT Command
{
public:
    enum Type : qint8 {
        Invalid = 0,

        // Session management
        Hello = 1,
        Login,
        Logout,

        // Transactions
        Transaction = 10,

        // Items
        CreateItem = 20,
        CopyItems,
        DeleteItems,
        FetchItems,
        LinkItems,
        ModifyItems,
        MoveItems,

        // Collections
        CreateCollection = 40,
        CopyCollection,
        DeleteCollection,
        FetchCollections,
        FetchCollectionStats,
        ModifyCollection,
        MoveCollection,
        SelectCollection,

        // Search
        Search = 60,
        SearchResult,
        StoreSearch,

        // Tag
        CreateTag = 70,
        DeleteTag,
        FetchTags,
        ModifyTag,

        // Relation
        FetchRelations = 80,
        ModifyRelation,
        RemoveRelations,

        // Resources
        SelectResource = 90,

        // Other...?
        StreamPayload = 100
    };

    virtual ~Command()
    {}

    Type type() const
    {
        return mCommandType;
    }

    bool isValid() const
    {
        return mCommandType != Invalid;
    }

protected:
    Command(Type type)
        : mCommandType(type)
    {}

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::Command &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::Command &command);

    Type mCommandType;
};

class AKONADIPRIVATE_EXPORT Response : public Command
{
public:
    void setError(int code, const QString &message)
    {
        mErrorCode = code;
        mErrorMsg = message;
    }

    bool isError() const
    {
        return mErrorCode;
    }
    int errorCode() const
    {
        return mErrorCode;
    }
    QString errorMessage() const
    {
        return mErrorMsg;
    }

protected:
    Response(Command::Type type)
    : Command(type)
        , mErrorCode(0)
    {
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::Response &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::Response &command);

    int mErrorCode;
    QString mErrorMsg;
};

class AKONADIPRIVATE_EXPORT HelloResponse : public Response
{
public:
    HelloResponse(const QString &server, const QString &message, int protocol)
        : Response(Hello)
        , mServer(server)
        , mMessage(message)
        , mVersion(protocol)
    {}

    HelloResponse()
        : Response(Hello)
        , mVersion(0)
    {}

    QString serverName() const
    {
        return mServer;
    }
    QString message() const
    {
        return mMessage;
    }
    int protocolVersion() const
    {
        return mVersion;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::HelloResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::HelloResponse &command);

    QString mServer;
    QString mMessage;
    int mVersion;
};

class AKONADIPRIVATE_EXPORT LoginCommand : public Command
{
public:
    LoginCommand(const QString &sessionId)
        : Command(Login)
        , mSession(sessionId)
    {}
    LoginCommand()
        : Command(Login)
    {}

    QString sessionId() const
    {
        return mSession;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::LoginCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::LoginCommand &command);

    QString mSession;
};

class AKONADIPRIVATE_EXPORT LoginResponse : public Response
{
public:
    LoginResponse()
        : Response(Login)
    {}
};

class AKONADIPRIVATE_EXPORT LogoutCommand : public Command
{
public:
    LogoutCommand()
        : Command(Logout)
    {}
};

class AKONADIPRIVATE_EXPORT LogoutResponse : public Response
{
public:
    LogoutResponse()
        : Response(Logout)
    {}
};

class AKONADIPRIVATE_EXPORT TransactionCommand : public Command
{
public:
    enum Mode : qint8 {
        Invalid = 0,
        Begin,
        Commit,
        Rollback
    };

    TransactionCommand()
        : Command(Transaction)
        , mMode(Invalid)
    {}

    TransactionCommand(Mode mode)
        : Command(Transaction)
        , mMode(mode)
    {}

    Mode mode() const
    {
        return mMode;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::TransactionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::TransactionCommand &command);

    Mode mMode;
};

class AKONADIPRIVATE_EXPORT TransactionResponse : public Response
{
public:
    TransactionResponse()
        : Response(Transaction)
    {}
};

class AKONADIPRIVATE_EXPORT CreateItemCommand : public Command
{
public:
    enum MergeMode : qint8 {
        None = 0,
        GID = 1,
        RemoteID = 2,
        Silent = 4
    };
    Q_DECLARE_FLAGS(MergeModes, MergeMode);

    CreateItemCommand()
        : Command(CreateItem)
        , mItemSize(-1)
        , mMergeMode(None)
    {}

    void setMergeMode(const MergeModes &mode)
    {
        mMergeMode = mode;
    }
    MergeModes mergeMode() const
    {
        return mMergeMode;
    }

    void setCollection(const Scope &collection)
    {
        mCollection = collection;
    }
    Scope collection() const
    {
        return mCollection;
    }

    void setItemSize(qint64 size)
    {
        mItemSize = size;
    }
    qint64 itemSize() const
    {
        return mItemSize;
    }

    void setMimeType(const QString &mimeType)
    {
        mMimeType = mimeType;
    }
    QString mimeType() const
    {
        return mMimeType;
    }

    void setGID(const QString &gid)
    {
        mGid = gid;
    }
    QString gid() const
    {
        return mGid;
    }

    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }

    void setRemoteRevision(const QString &remoteRevision)
    {
        mRemoteRev = remoteRevision;
    }

    QString remoteRevision() const
    {
        return mRemoteRev;
    }

    void setDateTime(const QDateTime &dateTime)
    {
        mDateTime = dateTime;
    }
    QDateTime dateTime() const
    {
        return mDateTime;
    }

    void setFlags(const QVector<QByteArray> &flags)
    {
        mFlags = flags;
    }
    QVector<QByteArray> flags() const
    {
        return mFlags;
    }
    void setAddedFlags(const QVector<QByteArray> &flags)
    {
        mAddedFlags = flags;
    }
    QVector<QByteArray> addedFlags() const
    {
        return mAddedFlags;
    }
    void setRemovedFlags(const QVector<QByteArray> &flags)
    {
        mRemovedFlags = flags;
    }
    QVector<QByteArray> removedFlags() const
    {
        return mRemovedFlags;
    }

    void setTags(const QVector<QByteArray> &tags)
    {
        mTags = tags;
    }
    Scope tags() const
    {
        return mTags;
    }
    void setAddedTags(const Scope &tags)
    {
        mAddedTags = tags;
    }
    QVector<QByteArray> addedTags() const
    {
        return mAddedTags;
    }
    void setRemovedTags(const QVector<QByteArray> &tags)
    {
        mRemovedTags = tags;
    }
    QVector<QByteArray> removedTags() const
    {
        return mRemovedTags;
    }
    void setRemovedParts(const QVector<QByteArray> &removedParts)
    {
        mRemovedParts = removedParts;
    }
    QVector<QByteArray> removedParts() const
    {
        return mRemovedParts;
    }
    void setParts(const QVector<PartMetaData> &parts)
    {
        mParts = parts;
    }
    QVector<PartMetaData> parts() const
    {
        return mParts;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::CreateItemCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::CreateItemCommand &command);

    Scope mCollection;
    QString mMimeType;
    QString mGid;
    QString mRemoteId;
    QString mRemoteRev;
    QDateTime mDateTime;
    Scope mTags;
    QVector<QByteArray> mFlags;
    QVector<QByteArray> mAddedFlags;
    QVector<QByteArray> mRemovedFlags;
    QVector<QByteArray> mAddedTags;
    QVector<QByteArray> mRemovedTags;
    QVector<QByteArray> mRemovedParts;
    QVector<PartMetaData> mParts;
    MergeModes mMergeMode;
    qint64 mItemSize;
};

class AKONADIPRIVATE_EXPORT CreateItemResponse : public Response
{
public:
    CreateItemResponse()
        : Response(CreateItem)
    {}
};

class AKONADIPRIVATE_EXPORT CopyItemsCommand : public Command
{
public:
    CopyItemsCommand()
        : Command(CopyItems)
    {}

    CopyItemsCommand(const Scope &items, const Scope &destination)
        : Command(CopyItems)
        , mScope(items)
        , mDest(destination)
    {}

    Scope items() const
    {
        return mScope;
    }
    Scope destination() const
    {
        return mDest;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::CopyItemsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::CopyItemsCommand &command);

    Scope mScope;
    Scope mDest;
};

class AKONADIPRIVATE_EXPORT CopyItemsResponse : public Response
{
public:
    CopyItemsResponse()
        : Response(CopyItems)
    {}
};

class AKONADIPRIVATE_EXPORT DeleteItemsCommand : public Command
{
public:
    DeleteItemsCommand()
        : Command(DeleteItems)
    {}

    DeleteItemsCommand(const Scope &scope)
        : Command(DeleteItems)
        , mScope(scope)
    {}

    Scope scope() const
    {
        return mScope;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::DeleteItemsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::DeleteItemsCommand &command);

    Scope mScope;
};

class AKONADIPRIVATE_EXPORT DeleteItemsResponse : public Response
{
public:
    DeleteItemsResponse()
        : Response(DeleteItems)
    {}
};


class AKONADIPRIVATE_EXPORT FetchRelationsCommand : public Command
{
public:
    FetchRelationsCommand()
        : Command(FetchRelations)
        , mLeft(-1)
        , mRight(-1)
        , mSide(-1)
    {}

    void setLeft(qint64 left)
    {
        mLeft = left;
    }
    qint64 left() const
    {
        return mLeft;
    }
    void setRight(qint64 right)
    {
        mRight = right;
    }
    qint64 right() const
    {
        return mRight;
    }
    void setSide(qint64 side)
    {
        mSide = side;
    }
    qint64 side() const
    {
        return mSide;
    }
    void setType(const QString &type)
    {
        mType = type;
    }
    QString type() const
    {
        return mType;
    }
    void setResource(const QString &resource)
    {
        mResource = resource;
    }
    QString resource() const
    {
        return mResource;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchRelationsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchRelationsCommand &command);

    qint64 mLeft;
    qint64 mRight;
    qint64 mSide;
    QString mType;
    QString mResource;
};

class AKONADIPRIVATE_EXPORT FetchRelationsResponse : public Response
{
public:
    FetchRelationsResponse()
        : Response(FetchRelations)
        , mLeft(-1)
        , mRight(-1)
    {}

    FetchRelationsResponse(qint64 left, qint64 right, const QString &type)
        : Response(FetchRelations)
        , mLeft(left)
        , mRight(right)
        , mType(type)
    {}

    qint64 left() const
    {
        return mLeft;
    }
    qint64 right() const
    {
        return mRight;
    }
    QString type() const
    {
        return mType;
    }
    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId()
    {
        return mRemoteId;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchRelationsResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchRelationsResponse &command);

    qint64 mLeft;
    qint64 mRight;
    QString mType;
    QString mRemoteId;
};


class AKONADIPRIVATE_EXPORT FetchTagsCommand : public Command
{
public:
    FetchTagsCommand()
        : Command(FetchTags)
    {}

    FetchTagsCommand(const Scope &scope)
        : Command(FetchTags)
        , mScope(scope)
    {}

    Scope scope() const
    {
        return mScope;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchTagsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchTagsCommand &command);

    Scope mScope;
};

class AKONADIPRIVATE_EXPORT FetchTagsResponse : public Response
{
public:
    FetchTagsResponse()
        : Response(FetchTags)
        , mId(-1)
        , mParentId(-1)
    {}

    FetchTagsResponse(qint64 id)
        : Response(FetchTags)
        , mId(id)
    {}

    qint64 id() const
    {
        return mId;
    }
    void setParentId(qint64 parentId)
    {
        mParentId = parentId;
    }
    qint64 parentId() const
    {
        return mParentId;
    }
    void setGid(const QString &gid)
    {
        mGid = gid;
    }
    QString gid() const
    {
        return mGid;
    }
    void setType(const QString &type)
    {
        mType = type;
    }
    QString type() const
    {
        return mType;
    }
    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setAttributes(const QMap<QByteArray, QByteArray> &attributes)
    {
        mAttributes = attributes;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchTagsResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchTagsResponse &command);

    qint64 mId;
    qint64 mParentId;
    QString mGid;
    QString mType;
    QString mRemoteId;
    QMap<QByteArray, QByteArray> mAttributes;
};


class AKONADIPRIVATE_EXPORT FetchItemsCommand : public Command
{
public:
    FetchItemsCommand()
        : Command(FetchItems)
    {}

    FetchItemsCommand(const Scope &scope, const FetchScope &fetchScope)
        : Command(FetchItems)
        , mScope(scope)
        , mFetchScope(fetchScope)
    {}

    Scope scope() const
    {
        return mScope;
    }

    FetchScope fetchScope() const
    {
        return mFetchScope;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchItemsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchItemsCommand &command);

    Scope mScope;
    FetchScope mFetchScope;
};


class AKONADIPRIVATE_EXPORT FetchItemsResponse : public Response
{
public:
    FetchItemsResponse()
        : Response(FetchItems)
        , mId(-1)
        , mCollectionId(-1)
        , mSize(0)
        , mRevision(0)
    {}

    FetchItemsResponse(qint64 id)
        : Response(FetchItems)
        , mId(id)
        , mCollectionId(-1)
        , mSize(0)
        , mRevision(0)
    {}

    qint64 id() const
    {
        return mId;
    }

    void setRevision(int revision)
    {
        mRevision = revision;
    }
    int revision() const
    {
        return mRevision;
    }

    void setParentId(qint64 parent)
    {
        mCollectionId = parent;
    }
    qint64 parentId() const
    {
        return mCollectionId;
    }

    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }

    void setRemoteRevision(const QString &remoteRev)
    {
        mRemoteRev = remoteRev;
    }
    QString remoteRevision() const
    {
        return mRemoteRev;
    }

    void setGid(const QString &gid)
    {
        mGid = gid;
    }
    QString gid() const
    {
        return mGid;
    }

    void setSize(qint64 size)
    {
        mSize = size;
    }
    qint64 size() const
    {
        return mSize;
    }

    void setMimeType(const QString &mimeType)
    {
        mMimeType = mimeType;
    }
    QString mimeType() const
    {
        return mMimeType;
    }

    void setMTime(const QDateTime &mtime)
    {
        mTime = mtime;
    }
    QDateTime MTime() const
    {
        return mTime;
    }

    void setFlags(const QVector<QByteArray> &flags)
    {
        mFlags = flags;
    }
    QVector<QByteArray> flags() const
    {
        return mFlags;
    }

    void setTags(const QVector<FetchTagsResponse> &tags)
    {
        mTags = tags;
    }
    QVector<FetchTagsResponse> tags() const
    {
        return mTags;
    }

    void setVirtualReferences(const QVector<qint64> &virtRefs)
    {
        mVirtRefs = virtRefs;
    }
    QVector<qint64> virtualReferences() const
    {
        return mVirtRefs;
    }

    void setRelations(const QVector<FetchRelationsResponse> &relations)
    {
        mRelations = relations;
    }
    QVector<FetchRelationsResponse> relations() const
    {
        return mRelations;
    }

    void setAncestors(const QVector<Ancestor> &ancestors)
    {
        mAncestors = ancestors;
    }
    QVector<Ancestor> ancestors() const
    {
        return mAncestors;
    }

    void setParts(const QMap<PartMetaData, StreamPayloadResponse> &parts)
    {
        mParts = parts;
    }
    QMap<PartMetaData, StreamPayloadResponse> parts() const
    {
        return mParts;
    }
    void setCachedParts(const QVector<QByteArray> &cachedParts)
    {
        mCachedParts = cachedParts;
    }
    QVector<QByteArray> cachedParts() const
    {
        return mCachedParts;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchItemsResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchItemsResponse &command);

    QString mRemoteId;
    QString mRemoteRev;
    QString mGid;
    QString mMimeType;
    QDateTime mTime;
    QVector<QByteArray> mFlags;
    QVector<FetchTagsResponse> mTags;
    QVector<qint64> mVirtRefs;
    QVector<FetchRelationsResponse> mRelations;
    QVector<Ancestor> mAncestors;
    QMap<PartMetaData, StreamPayloadResponse> mParts;
    QVector<QByteArray> mCachedParts;
    qint64 mId;
    qint64 mCollectionId;
    qint64 mSize;
    int mRevision;
};

class AKONADIPRIVATE_EXPORT LinkItemsCommand : public Command
{
public:
    enum Action : bool {
        Link,
        Unlink
    };

    LinkItemsCommand()
        : Command(LinkItems)
        , mAction(Link)
    {}

    LinkItemsCommand(Action action, Scope &scope, Scope &dest)
        : Command(LinkItems)
        , mAction(action)
        , mScope(scope)
        , mDest(dest)
    {}

    Action action() const
    {
        return mAction;
    }
    Scope scope() const
    {
        return mScope;
    }
    Scope destination() const
    {
        return mDest;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::LinkItemsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::LinkItemsCommand &command);

    Action mAction;
    Scope mScope;
    Scope mDest;
};

class AKONADIPRIVATE_EXPORT LinkItemsResponse : public Response
{
public:
    LinkItemsResponse()
        : Response(LinkItems)
    {}
};


class AKONADIPRIVATE_EXPORT ModifyItemsCommand : public Command
{
public:
    enum ModifiedPart {
        None            = 0,
        Flags           = 1 << 0,
        AddedFlags      = 1 << 2,
        RemovedFlags    = 1 << 3,
        Tags            = 1 << 4,
        AddedTags       = 1 << 5,
        RemovedTags     = 1 << 6,
        RemoteID        = 1 << 7,
        RemoteRevision  = 1 << 8,
        GID             = 1 << 9,
        Size            = 1 << 10,
        Parts           = 1 << 11,
        RemovedParts    = 1 << 12
    };
    Q_DECLARE_FLAGS(ModifiedParts, ModifiedPart);

    ModifyItemsCommand()
        : Command(ModifyItems)
        , mSize(0)
        , mOldRevision(-1)
        , mDirty(true)
        , mInvalidate(false)
        , mNoResponse(false)
        , mNotify(true)
        , mModifiedParts(None)
    {}

    ModifyItemsCommand(const Scope &scope)
        : Command(ModifyItems)
        , mScope(scope)
        , mOldRevision(-1)
        , mSize(0)
        , mDirty(true)
        , mInvalidate(false)
        , mNoResponse(false)
        , mNotify(true)
        , mModifiedParts(None)
    {}

    ModifiedParts modifiedParts() const
    {
        return mModifiedParts;
    }

    void setScope(const Scope &scope)
    {
        mScope = scope;
    }
    Scope scope() const
    {
        return mScope;
    }

    void setOldRevision(int oldRevision)
    {
        mOldRevision = oldRevision;
    }
    int oldRevision() const
    {
        return mOldRevision;
    }

    void setFlags(const QVector<QByteArray> &flags)
    {
        mModifiedParts |= Flags;
        mFlags = flags;
    }
    QVector<QByteArray> flags() const
    {
        return mFlags;
    }
    void setAddedFlags(const QVector<QByteArray> &flags)
    {
        mModifiedParts |= AddedFlags;
        mAddedFlags = flags;
    }
    QVector<QByteArray> addedFlags() const
    {
        return mAddedFlags;
    }
    void setRemovedFlags(const QVector<QByteArray> &flags)
    {
        mModifiedParts |= RemovedFlags;
        mRemovedFlags = flags;
    }
    QVector<QByteArray> removedFlags() const
    {
        return mRemovedFlags;
    }

    void setTags(const Scope &tags)
    {
        mModifiedParts |= Tags;
        mTags = tags;
    }
    Scope tags() const
    {
        return mTags;
    }
    void setAddedTags(const Scope &tags)
    {
        mModifiedParts |= AddedTags;
        mAddedTags = tags;
    }
    Scope addedTags() const
    {
        return mAddedTags;
    }
    void setRemovedTags(const Scope &tags)
    {
        mModifiedParts |= RemovedTags;
        mRemovedTags = tags;
    }
    Scope removedTags() const
    {
        return mRemovedTags;
    }

    void setRemoteId(const QString &remoteId)
    {
        mModifiedParts |= RemoteID;
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setGid(const QString &gid)
    {
        mModifiedParts |= GID;
        mGid = gid;
    }
    QString gid() const
    {
        return mGid;
    }
    void setRemoteRevision(const QString &remoteRevision)
    {
        mModifiedParts |= RemoteRevision;
        mRemoteRev = remoteRevision;
    }
    QString remoteRevision() const
    {
        return mRemoteRev;
    }
    void setDirty(bool dirty)
    {
        mDirty = dirty;
    }
    bool dirty() const
    {
        return mDirty;
    }
    void setInvalidateCache(bool invalidate)
    {
        mInvalidate = invalidate;
    }
    bool invalidateCache() const
    {
        return mInvalidate;
    }
    void setNoResponse(bool noResponse)
    {
        mNoResponse = noResponse;
    }
    bool noResponse() const
    {
        return mNoResponse;
    }
    void setNotify(bool notify)
    {
        mNotify = notify;
    }
    bool notify() const
    {
        return mNotify;
    }
    void setItemSize(qint64 size)
    {
        mModifiedParts |= Size;
        mSize = size;
    }
    qint64 itemSize() const
    {
        return mSize;
    }
    void setRemovedParts(const QVector<QByteArray> &removedParts)
    {
        mModifiedParts |= RemovedParts;
        mRemovedParts = removedParts;
    }
    QVector<QByteArray> removedParts() const
    {
        return mRemovedParts;
    }
    void setParts(const QVector<PartMetaData> &parts)
    {
        mModifiedParts |= Parts;
        mParts = parts;
    }
    QVector<PartMetaData> parts() const
    {
        return mParts;
    }
private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyItemsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::ModifyItemsCommand &command);

    Scope mScope;
    QVector<QByteArray> mFlags;
    QVector<QByteArray> mAddedFlags;
    QVector<QByteArray> mRemovedFlags;
    Scope mTags;
    Scope mAddedTags;
    Scope mRemovedTags;

    QString mRemoteId;
    QString mGid;
    QString mRemoteRev;
    QVector<QByteArray> mRemovedParts;
    QVector<PartMetaData> mParts;
    qint64 mSize;
    int mOldRevision;
    bool mDirty;
    bool mInvalidate;
    bool mNoResponse;
    bool mNotify;

    ModifiedParts mModifiedParts;
};

class AKONADIPRIVATE_EXPORT ModifyItemsResponse : public Response
{
public:
    ModifyItemsResponse()
        : Response(ModifyItems)
    {}
};


class AKONADIPRIVATE_EXPORT MoveItemsCommand : public Command
{
public:
    MoveItemsCommand()
        : Command(MoveItems)
    {}

    MoveItemsCommand(const Scope &scope, const Scope &dest)
        : Command(MoveItems)
        , mScope(scope)
        , mDest(dest)
    {}

    Scope scope() const
    {
        return mScope;
    }
    Scope destination() const
    {
        return mDest;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::MoveItemsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::MoveItemsCommand &command);

    Scope mScope;
    Scope mDest;
};

class AKONADIPRIVATE_EXPORT MoveItemsResponse : public Response
{
public:
    MoveItemsResponse()
        : Response(MoveItems)
    {}
};



class AKONADIPRIVATE_EXPORT CreateCollectionCommand : public Command
{
public:
    CreateCollectionCommand()
        : Command(CreateCollection)
        , mEnabled(Tristate::Undefined)
        , mSync(Tristate::Undefined)
        , mDisplay(Tristate::Undefined)
        , mIndex(Tristate::Undefined)
        , mVirtual(false)
    {}

    void setParent(const Scope &scope)
    {
        mParent = scope;
    }
    Scope parent() const
    {
        return mParent;
    }
    void setName(const QString &name)
    {
        mName = name;
    }
    QString name() const
    {
        return mName;
    }
    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setRemoteRevision(const QString &remoteRevision)
    {
        mRemoteRev = remoteRevision;
    }
    QString remoteRevision() const
    {
        return mRemoteRev;
    }
    void setMimeTypes(const QStringList &mimeTypes)
    {
        mMimeTypes = mimeTypes;
    }
    QStringList mimeTypes() const
    {
        return mMimeTypes;
    }
    void setCachePolicy(const CachePolicy &cachePolicy)
    {
        mCachePolicy = cachePolicy;
    }
    CachePolicy cachePolicy() const
    {
        return mCachePolicy;
    }
    void setAttributes(const QMap<QByteArray, QByteArray> &attributes)
    {
        mAttributes = attributes;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }
    void setIsVirtual(bool isVirtual)
    {
        mVirtual = isVirtual;
    }
    bool isVirtual() const
    {
        return mVirtual;
    }
    void setEnabled(Tristate enabled)
    {
        mEnabled = enabled;
    }
    Tristate enabled() const
    {
        return mEnabled;
    }
    void setSyncPref(Tristate sync)
    {
        mSync = sync;
    }
    Tristate syncPref() const
    {
        return mSync;
    }
    void setDisplayPref(Tristate display)
    {
        mDisplay = display;
    }
    Tristate displayPref() const
    {
        return mDisplay;
    }
    void setIndexPref(Tristate index)
    {
        mIndex = index;
    }
    Tristate indexPref() const
    {
        return mIndex;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::CreateCollectionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::CreateCollectionCommand &command);

    Scope mParent;
    QString mName;
    QString mRemoteId;
    QString mRemoteRev;
    QStringList mMimeTypes;
    CachePolicy mCachePolicy;
    QMap<QByteArray, QByteArray> mAttributes;
    Tristate mEnabled;
    Tristate mSync;
    Tristate mDisplay;
    Tristate mIndex;
    bool mVirtual;
};

class AKONADIPRIVATE_EXPORT CreateCollectionResponse : public Response
{
public:
    CreateCollectionResponse()
        : Response(CreateCollection)
    {}
};

class AKONADIPRIVATE_EXPORT CopyCollectionCommand : public Command
{
public:
    CopyCollectionCommand()
        : Command(CopyCollection)
    {}

    CopyCollectionCommand(const Scope &collection, const Scope &dest)
        : Command(CopyCollection)
        , mScope(collection)
        , mDest(dest)
    {}

    Scope collection() const
    {
        return mScope;
    }
    Scope destination() const
    {
        return mDest;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::CopyCollectionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::CopyCollectionCommand &command);

    Scope mScope;
    Scope mDest;
};

class AKONADIPRIVATE_EXPORT CopyCollectionResponse : public Response
{
public:
    CopyCollectionResponse()
        : Response(CopyCollection)
    {}
};

class AKONADIPRIVATE_EXPORT DeleteCollectionCommand : public Command
{
public:
    DeleteCollectionCommand()
        : Command(DeleteCollection)
    {}
    DeleteCollectionCommand(const Scope &scope)
        : Command(DeleteCollection)
        , mScope(scope)
    {}

    Scope scope() const
    {
        return mScope;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::DeleteCollectionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::DeleteCollectionCommand &command);

    Scope mScope;
};

class AKONADIPRIVATE_EXPORT DeleteCollectionResponse : public Response
{
public:
    DeleteCollectionResponse()
        : Response(DeleteCollection)
    {}
};



class AKONADIPRIVATE_EXPORT FetchCollectionStatsCommand : public Command
{
public:
    FetchCollectionStatsCommand()
        : Command(FetchCollectionStats)
    {}

    FetchCollectionStatsCommand(const Scope &col)
        : Command(FetchCollectionStats)
        , mScope(col)
    {}

    Scope collection() const
    {
        return mScope;
    }
private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionStatsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionStatsCommand &command);

    Scope mScope;
};

class AKONADIPRIVATE_EXPORT FetchCollectionStatsResponse : public Response
{
public:
    FetchCollectionStatsResponse()
        : Response(FetchCollectionStats)
    {}

    FetchCollectionStatsResponse(qint64 count, qint64 unseen, qint64 size)
        : Response(FetchCollectionStats)
        , mCount(count)
        , mUnseen(unseen)
        , mSize(size)
    {}

    qint64 count() const
    {
        return mCount;
    }
    qint64 unseen() const
    {
        return mUnseen;
    }
    qint64 size() const
    {
        return mSize;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionStatsResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionStatsResponse &command);

    qint64 mCount;
    qint64 mUnseen;
    qint64 mSize;
};


class AKONADIPRIVATE_EXPORT FetchCollectionsCommand : public Command
{
public:
    FetchCollectionsCommand()
        : Command(FetchCollections)
        , mDepth(0)
        , mAncestorsDepth(-1)
        , mEnabled(false)
        , mSync(false)
        , mDisplay(false)
        , mIndex(false)
        , mStats(false)
    {}

    FetchCollectionsCommand(const Scope &scope)
        : Command(FetchCollections)
        , mScope(scope)
        , mDepth(0)
        , mAncestorsDepth(-1)
        , mEnabled(false)
        , mSync(false)
        , mDisplay(false)
        , mIndex(false)
        , mStats(false)
    {}

    void setDepth(int depth)
    {
        mDepth = depth;
    }
    int depth() const
    {
        return mDepth;
    }

    Scope scope() const
    {
        return mScope;
    }
    void setResource(const QString &resourceId)
    {
        mResource = resourceId;
    }
    QString resource() const
    {
        return mResource;
    }
    void setMimeTypes(const QStringList &mimeTypes)
    {
        mMimeTypes = mimeTypes;
    }
    QStringList mimeTypes() const
    {
        return mMimeTypes;
    }
    void setAncestorsDepth(int depth)
    {
        mAncestorsDepth = depth;
    }
    int ancestorsDepth() const
    {
        return mAncestorsDepth;
    }
    void setAncestorsAttributes(const QStringList &attributes)
    {
        mAncestorsAttributes = attributes;
    }
    QStringList ancestorsAttributes() const
    {
        return mAncestorsAttributes;
    }
    void setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }
    bool enabled() const
    {
        return mEnabled;
    }
    void setSyncPref(bool sync)
    {
        mSync = sync;
    }
    bool syncPref() const
    {
        return mSync;
    }
    void setDisplayPref(bool display)
    {
        mDisplay = display;
    }
    bool displayPref() const
    {
        return mDisplay;
    }
    void setIndexPref(bool index)
    {
        mIndex = index;
    }
    bool indexPref() const
    {
        return mIndex;
    }
    void setFetchStats(bool stats)
    {
        mStats = stats;
    }
    bool fetchStats() const
    {
        return mStats;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionsCommand &command);

    Scope mScope;
    QString mResource;
    QStringList mMimeTypes;
    QStringList mAncestorsAttributes;
    int mDepth;
    int mAncestorsDepth;
    bool mEnabled;
    bool mSync;
    bool mDisplay;
    bool mIndex;
    bool mStats;
};


class FetchCollectionStatistics;
class AKONADIPRIVATE_EXPORT FetchCollectionsResponse : public Response
{
public:
    FetchCollectionsResponse()
        : Response(FetchCollections)
    {}

    FetchCollectionsResponse(qint64 id)
        : Response(FetchCollections)
        , mId(id)
    {}

    qint64 id() const
    {
        return mId;
    }
    void setName(const QString &name)
    {
        mName = name;
    }
    QString name() const
    {
        return mName;
    }
    void setMimeType(const QString &mimeType)
    {
        mMimeType = mimeType;
    }
    QString mimeType() const
    {
        return mMimeType;
    }
    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setRemoteRevision(const QString &remoteRev)
    {
        mRemoteRev = remoteRev;
    }
    QString remoteRevision() const
    {
        return mRemoteRev;
    }
    void setResource(const QByteArray &resourceId)
    {
        mResource = resourceId;
    }
    QByteArray resource() const
    {
        return mResource;
    }
    void setStatistics(const FetchCollectionStatsResponse &stats)
    {
        mStats = stats;
    }
    FetchCollectionStatsResponse statistics() const
    {
        return mStats;
    }
    void setSearchQuery(const QString &searchQuery)
    {
        mSearchQuery = searchQuery;
    }
    QString searchQuery() const
    {
        return mSearchQuery;
    }
    void setSearchCollections(const QVector<qint64> &searchCols)
    {
        mSearchCols = searchCols;
    }
    QVector<qint64> searchCollections() const
    {
        return mSearchCols;
    }
    void setAncestors(const QVector<Ancestor> &ancestors)
    {
        mAncestors = ancestors;
    }
    QVector<Ancestor> ancestors() const
    {
        return mAncestors;
    }
    void setCachePolicy(const CachePolicy &cachePolicy)
    {
        mCachePolicy = cachePolicy;
    }
    CachePolicy cachePolicy() const
    {
        return mCachePolicy;
    }
    void setAttributes(const QMap<QByteArray, QByteArray> &attrs)
    {
        mAttributes = attrs;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }
    void setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }
    bool enabled() const
    {
        return mEnabled;
    }
    void setDisplayPref(Tristate displayPref)
    {
        mDisplay = displayPref;
    }
    Tristate displayPref() const
    {
        return mDisplay;
    }
    void setSyncPref(Tristate syncPref)
    {
        mSync = syncPref;
    }
    Tristate syncPref() const
    {
        return mSync;
    }
    void setIndexPref(Tristate indexPref)
    {
        mIndex = indexPref;
    }
    Tristate indexPref() const
    {
        return mIndex;
    }
    void setReferenced(bool ref)
    {
        mReferenced = ref;
    }
    bool referenced() const
    {
        return mReferenced;
    }
    void setIsVirtual(bool isVirtual)
    {
        mIsVirtual = isVirtual;
    }
    bool isVirtual() const
    {
        return mIsVirtual;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::FetchCollectionsResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::FetchCollectionsResponse &command);

    QString mName;
    QString mMimeType;
    QString mRemoteId;
    QString mRemoteRev;
    QByteArray mResource;
    FetchCollectionStatsResponse mStats;
    QString mSearchQuery;
    QVector<qint64> mSearchCols;
    QVector<Ancestor> mAncestors;
    CachePolicy mCachePolicy;
    QMap<QByteArray, QByteArray> mAttributes;
    qint64 mId;
    Tristate mDisplay;
    Tristate mSync;
    Tristate mIndex;
    bool mIsVirtual;
    bool mReferenced;
    bool mEnabled;
};

class AKONADIPRIVATE_EXPORT ModifyCollectionCommand : public Command
{
public:
    enum ModifiedCollectionPart {
        None            = 0,
        Name            = 1 << 0,
        RemoteID        = 1 << 1,
        RemoteRevision  = 1 << 2,
        ParentID        = 1 << 3,
        MimeTypes       = 1 << 4,
        CachePolicy     = 1 << 5,
        PersistentSearch = 1 << 6,
        RemovedAttributes = 1 << 7,
        Attributes      = 1 << 8,
        ListPreferences = 1 << 9,
        Referenced      = 1 << 10
    };
    Q_DECLARE_FLAGS(ModifiedCollectionParts, ModifiedCollectionPart)

    ModifyCollectionCommand()
        : Command(ModifyCollection)
        , mEnabled(Tristate::Undefined)
        , mSync(Tristate::Undefined)
        , mDisplay(Tristate::Undefined)
        , mIndex(Tristate::Undefined)
        , mParentId(-1)
        , mReferenced(false)
        , mPersistentSearchRemote(false)
        , mPersistentSearchRecursive(false)
        , mModifiedParts(None)
    {}

    ModifyCollectionCommand(const Scope &scope)
        : Command(ModifyCollection)
        , mScope(scope)
        , mEnabled(Tristate::Undefined)
        , mSync(Tristate::Undefined)
        , mDisplay(Tristate::Undefined)
        , mIndex(Tristate::Undefined)
        , mParentId(-1)
        , mReferenced(false)
        , mPersistentSearchRemote(false)
        , mPersistentSearchRecursive(false)
        , mModifiedParts(None)
    {}

    ModifiedCollectionParts modifiedParts() const
    {
        return mModifiedParts;
    }

    Scope scope() const
    {
        return mScope;
    }
    void setParentId(qint64 parentId)
    {
        mParentId = parentId;
    }
    qint64 parentId() const
    {
        return mParentId;
    }
    void setMimeTypes(const QStringList &mimeTypes)
    {
        mModifiedParts |= MimeTypes;
        mModifiedParts |= PersistentSearch;
        mMimeTypes = mimeTypes;
    }
    QStringList mimeTypes() const
    {
        return mMimeTypes;
    }
    void setCachePolicy(const Protocol::CachePolicy &cachePolicy)
    {
        mModifiedParts |= CachePolicy;
        mCachePolicy = cachePolicy;
    }
    Protocol::CachePolicy cachePolicy() const
    {
        return mCachePolicy;
    }
    void setName(const QString &name)
    {
        mModifiedParts |= Name;
        mName = name;
    }
    QString name() const
    {
        return mName;
    }
    void setRemoteId(const QString &remoteId)
    {
        mModifiedParts |= RemoteID;
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setRemoteRevision(const QString &remoteRevision)
    {
        mModifiedParts |= RemoteRevision;
        mRemoteRev = remoteRevision;
    }
    QString remoteRevision() const
    {
        return mRemoteRev;
    }
    void setPersistentSearchQuery(const QString &query)
    {
        mModifiedParts |= PersistentSearch;
        mPersistentSearchQuery = query;
    }
    QString persistentSearchQuery() const
    {
        return mPersistentSearchQuery;
    }
    void setPersistentSearchCollectins(const QVector<qint64> &cols)
    {
        mModifiedParts |= PersistentSearch;
        mPersistentSearchCols = cols;
    }
    QVector<qint64> persistentSearchCollections() const
    {
        return mPersistentSearchCols;
    }
    void setPersistentSearchRemote(bool remote)
    {
        mModifiedParts |= PersistentSearch;
        mPersistentSearchRemote = remote;
    }
    bool persistentSearchRemote() const
    {
        return mPersistentSearchRemote;
    }
    void setPersistentSearchRecursive(bool recursive)
    {
        mModifiedParts |= PersistentSearch;
        mPersistentSearchRecursive = recursive;
    }
    bool persistentSearchRecursive() const
    {
        return mPersistentSearchRecursive;
    }
    void setRemovedAttributes(const QSet<QByteArray> &removedAttributes)
    {
        mModifiedParts |= RemovedAttributes;
        mRemovedAttributes = removedAttributes;
    }
    QSet<QByteArray> removedAttributes() const
    {
        return mRemovedAttributes;
    }
    void setAttributes(const QMap<QByteArray, QByteArray> &attributes)
    {
        mModifiedParts |= Attributes;
        mAttributes = attributes;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }
    void setEnabled(Tristate enabled)
    {
        mModifiedParts |= ListPreferences;
        mEnabled = enabled;
    }
    Tristate enabled() const
    {
        return mEnabled;
    }
    void setSyncPref(Tristate sync)
    {
        mModifiedParts |= ListPreferences;
        mSync = sync;
    }
    Tristate syncPref() const
    {
        return mSync;
    }
    void setDisplayPref(Tristate display)
    {
        mModifiedParts |= ListPreferences;
        mDisplay = display;
    }
    Tristate displayPref() const
    {
        return mDisplay;
    }
    void setIndexPref(Tristate index)
    {
        mModifiedParts |= ListPreferences;
        mIndex = index;
    }
    Tristate indexPref() const
    {
        return mIndex;
    }
    void setReferenced(bool referenced)
    {
        mModifiedParts |= Referenced;
        mReferenced = referenced;
    }
    bool referenced() const
    {
        return mReferenced;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyCollectionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::ModifyCollectionCommand &command);


    Scope mScope;
    QStringList mMimeTypes;
    Protocol::CachePolicy mCachePolicy;
    QString mName;
    QString mRemoteId;
    QString mRemoteRev;
    QString mPersistentSearchQuery;
    QVector<qint64> mPersistentSearchCols;
    QSet<QByteArray> mRemovedAttributes;
    QMap<QByteArray, QByteArray> mAttributes;
    Tristate mEnabled;
    Tristate mSync;
    Tristate mDisplay;
    Tristate mIndex;
    qint64 mParentId;
    bool mReferenced;
    bool mPersistentSearchRemote;
    bool mPersistentSearchRecursive;

    ModifiedCollectionParts mModifiedParts;
};

class AKONADIPRIVATE_EXPORT ModifyCollectionResponse : public Response
{
public:
    ModifyCollectionResponse()
        : Response(ModifyCollection)
    {}
};


class AKONADIPRIVATE_EXPORT MoveCollectionCommand : public Command
{
public:
    MoveCollectionCommand()
        : Command(MoveCollection)
    {}

    MoveCollectionCommand(const Scope &col, const Scope &dest)
        : Command(MoveCollection)
        , mCol(col)
        , mDest(dest)
    {}

    Scope collection() const
    {
        return mCol;
    }
    Scope destination() const
    {
        return mDest;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::MoveCollectionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::MoveCollectionCommand &command);

    Scope mCol;
    Scope mDest;
};

class AKONADIPRIVATE_EXPORT MoveCollectionResponse : public Response
{
public:
    MoveCollectionResponse()
        : Response(MoveCollection)
    {}
};

class AKONADIPRIVATE_EXPORT SelectCollectionCommand : public Command
{
public:
    SelectCollectionCommand()
        : Command(SelectCollection)
    {}

    SelectCollectionCommand(const Scope &col)
        : Command(SelectCollection)
        , mScope(col)
    {}

    Scope collection() const
    {
        return mScope;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::SelectCollectionCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::SelectCollectionCommand &command);

    Scope mScope;
};

class AKONADIPRIVATE_EXPORT SelectCollectionResponse : public Response
{
public:
    SelectCollectionResponse()
        : Response(SelectCollection)
    {}
};


class AKONADIPRIVATE_EXPORT SearchCommand : public Command
{
public:
    SearchCommand()
        : Command(Search)
        , mRecursive(false)
        , mRemote(false)
    {}

    void setMimeTypes(const QStringList &mimeTypes)
    {
        mMimeTypes = mimeTypes;
    }
    QStringList mimeTypes() const
    {
        return mMimeTypes;
    }
    void setCollections(const QVector<qint64> &collections)
    {
        mCollections = collections;
    }
    QVector<qint64> collections() const
    {
        return mCollections;
    }
    void setQuery(const QString &query)
    {
        mQuery = query;
    }
    QString query() const
    {
        return mQuery;
    }
    void setFetchScope(const FetchScope &fetchScope)
    {
        mFetchScope = fetchScope;
    }
    FetchScope fetchScope() const
    {
        return mFetchScope;
    }
    void setRecursive(bool recursive)
    {
        mRecursive = recursive;
    }
    bool recursive() const
    {
        return mRecursive;
    }
    void setRemote(bool remote)
    {
        mRemote = remote;
    }
    bool remote() const
    {
        return mRemote;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::SearchCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::SearchCommand &command);

    QStringList mMimeTypes;
    QVector<qint64> mCollections;
    QString mQuery;
    FetchScope mFetchScope;
    bool mRecursive;
    bool mRemote;
};

class AKONADIPRIVATE_EXPORT SearchResponse : public Response
{
public:
    SearchResponse()
        : Response(Search)
    {}
};


class AKONADIPRIVATE_EXPORT SearchResultCommand : public Command
{
public:
    SearchResultCommand()
        : Command(SearchResult)
        , mCollectionId(-1)
    {}

    SearchResultCommand(const QByteArray &searchId, qint64 collectionId, const Scope &result)
        : Command(SearchResult)
        , mResult(result)
        , mCollectionId(collectionId)
    {}

    Scope result() const
    {
        return mResult;
    }
    qint64 collectionId() const
    {
        return mCollectionId;
    }
    QByteArray  searchId() const
    {
        return mSearchId;
    }
private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::SearchResultCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::SearchResultCommand &command);

    QByteArray mSearchId;
    Scope mResult;
    qint64 mCollectionId;
};

class AKONADIPRIVATE_EXPORT SearchResultResponse : public Response
{
public:
    SearchResultResponse()
        : Response(SearchResult)
    {}
};

class AKONADIPRIVATE_EXPORT StoreSearchCommand : public Command
{
public:
    StoreSearchCommand()
        : Command(StoreSearch)
    {}

    void setName(const QString &name)
    {
        mName = name;
    }
    QString name() const
    {
        return mName;
    }
    void setQuery(const QString &query)
    {
        mQuery = query;
    }
    QString query() const
    {
        return mQuery;
    }
    void setMimeTypes(const QStringList &mimeTypes)
    {
        mMimeTypes = mimeTypes;
    }
    QStringList mimeTypes() const
    {
        return mMimeTypes;
    }
    void setQueryCollections(const QVector<qint64> &queryCols)
    {
        mQueryCols = queryCols;
    }
    QVector<qint64> queryCollections() const
    {
        return mQueryCols;
    }
    void setRemote(bool remote)
    {
        mRemote = remote;
    }
    bool remote() const
    {
        return mRemote;
    }
    void setRecursive(bool recursive)
    {
        mRecursive = recursive;
    }
    bool recursive() const
    {
        return mRecursive;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::StoreSearchCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::StoreSearchCommand &command);

    QString mName;
    QString mQuery;
    QStringList mMimeTypes;
    QVector<qint64> mQueryCols;
    bool mRemote;
    bool mRecursive;
};

class AKONADIPRIVATE_EXPORT StoreSearchResponse : public Response
{
public:
    StoreSearchResponse()
        : Response(StoreSearch)
    {}
};

class AKONADIPRIVATE_EXPORT CreateTagCommand : public Command
{
public:
    CreateTagCommand()
        : Command(CreateTag)
    {}

    void setGid(const QString &gid)
    {
        mGid = gid;
    }
    QString gid() const
    {
        return mGid;
    }
    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setType(const QString &type)
    {
        mType = type;
    }
    QString type() const
    {
        return mType;
    }
    void setAttributes(const QMap<QByteArray, QByteArray> &attributes)
    {
        mAttributes = attributes;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }
    void setParentId(qint64 parentId)
    {
        mParentId = parentId;
    }
    qint64 parentId() const
    {
        return mParentId;
    }
    void setMerge(bool merge)
    {
        mMerge = merge;
    }
    bool merge() const
    {
        return mMerge;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::CreateTagCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::CreateTagCommand &command);

    QString mGid;
    QString mRemoteId;
    QString mType;
    QMap<QByteArray, QByteArray> mAttributes;
    qint64 mParentId;
    bool mMerge;
};

class AKONADIPRIVATE_EXPORT CreateTagResponse : public Response
{
public:
    CreateTagResponse()
        : Response(CreateTag)
    {}
};


class AKONADIPRIVATE_EXPORT DeleteTagCommand : public Command
{
public:
    DeleteTagCommand()
        : Command(DeleteTag)
    {}

    DeleteTagCommand(const Scope &scope)
        : Command(DeleteTag)
        , mScope(scope)
    {}

    Scope scope() const
    {
        return mScope;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::DeleteTagCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::DeleteTagCommand &command);

    Scope mScope;
};

class AKONADIPRIVATE_EXPORT DeleteTagResponse : public Response
{
public:
    DeleteTagResponse()
        : Response(DeleteTag)
    {}
};


class AKONADIPRIVATE_EXPORT ModifyTagCommand : public Command
{
public:
    enum ModifiedTagPart {
        None            = 0,
        ParentId        = 1 << 0,
        Type            = 1 << 1,
        RemoteId        = 1 << 2,
        RemovedAttributes = 1 << 3,
        Attributes      = 1 << 4
    };
    Q_DECLARE_FLAGS(ModifiedTagParts, ModifiedTagPart)

    ModifyTagCommand()
        : Command(ModifyTag)
        , mTagId(-1)
        , mParentId(-1)
        , mModifiedParts(None)
    {}


    ModifyTagCommand(qint64 tagId)
        : Command(ModifyTag)
        , mTagId(tagId)
        , mParentId(-1)
        , mModifiedParts(None)
    {}

    qint64 tagId() const
    {
        return mTagId;
    }
    ModifiedTagParts modifiedParts() const
    {
        return mModifiedParts;
    }
    void setParentId(qint64 parentId)
    {
        mModifiedParts |= ParentId;
        mParentId = parentId;
    }
    qint64 parentId() const
    {
        return mParentId;
    }
    void setType(const QString &type)
    {
        mModifiedParts |= Type;
        mType = type;
    }
    QString type() const
    {
        return mType;
    }
    void setRemoteId(const QString &remoteId)
    {
        mModifiedParts |= RemoteId;
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }
    void setRemovedAttributes(const QSet<QByteArray> &removed)
    {
        mModifiedParts |= RemovedAttributes;
        mRemovedAttributes = removed;
    }
    QSet<QByteArray> removedAttributes() const
    {
        return mRemovedAttributes;
    }
    void setAttributes(const QMap<QByteArray, QByteArray> &attrs)
    {
        mModifiedParts |= Attributes;
        mAttributes = attrs;
    }
    QMap<QByteArray, QByteArray> attributes() const
    {
        return mAttributes;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyTagCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::ModifyTagCommand &command);

    QString mType;
    QString mRemoteId;
    QSet<QByteArray> mRemovedAttributes;
    QMap<QByteArray, QByteArray> mAttributes;
    qint64 mTagId;
    qint64 mParentId;
    ModifiedTagParts mModifiedParts;
};

class AKONADIPRIVATE_EXPORT ModifyTagResponse : public Response
{
public:
    ModifyTagResponse()
        : Response(ModifyTag)
    {}
};


class AKONADIPRIVATE_EXPORT ModifyRelationCommand : public Command
{
public:
    ModifyRelationCommand()
        : Command(ModifyRelation)
        , mLeft(-1)
        , mRight(-1)
    {}

    void setLeft(qint64 left)
    {
        mLeft = left;
    }
    qint64 left() const
    {
        return mLeft;
    }
    void setRight(qint64 right)
    {
        mRight = right;
    }
    qint64 right() const
    {
        return mRight;
    }
    void setType(const QString &type)
    {
        mType = type;
    }
    QString type() const
    {
        return mType;
    }
    void setRemoteId(const QString &remoteId)
    {
        mRemoteId = remoteId;
    }
    QString remoteId() const
    {
        return mRemoteId;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::ModifyRelationCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::ModifyRelationCommand &command);

    qint64 mLeft;
    qint64 mRight;
    QString mType;
    QString mRemoteId;
};

class AKONADIPRIVATE_EXPORT ModifyRelationResponse : public Response
{
public:
    ModifyRelationResponse()
        : Response(ModifyRelation)
    {}
};

class AKONADIPRIVATE_EXPORT RemoveRelationsCommand : public Command
{
public:
    RemoveRelationsCommand()
        : Command(RemoveRelations)
        , mLeft(-1)
        , mRight(-1)
    {}

    void setLeft(qint64 left)
    {
        mLeft = left;
    }
    qint64 left() const
    {
        return mLeft;
    }
    void setRight(qint64 right)
    {
        mRight = right;
    }
    qint64 right() const
    {
        return mRight;
    }
    void setType(const QString &type)
    {
        mType = type;
    }
    QString type() const
    {
        return mType;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::RemoveRelationsCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::RemoveRelationsCommand &command);

    qint64 mLeft;
    qint64 mRight;
    QString mType;
};

class AKONADIPRIVATE_EXPORT RemoveRelationsResponse : public Response
{
public:
    RemoveRelationsResponse()
        : Response(RemoveRelations)
    {}
};


class AKONADIPRIVATE_EXPORT SelectResourceCommand : public Command
{
public:
    SelectResourceCommand()
        : Command(SelectResource)
    {}

    SelectResourceCommand(const QByteArray &resourceId)
        : Command(SelectResource)
        , mResourceId(resourceId)
    {}

    QByteArray resourceId() const
    {
        return mResourceId;
    }

private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::SelectResourceCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::SelectResourceCommand &command);

    QByteArray mResourceId;
};

class AKONADIPRIVATE_EXPORT SelectResourceResponse : public Response
{
public:
    SelectResourceResponse()
        : Response(SelectResource)
    {}
};

class AKONADIPRIVATE_EXPORT StreamPayloadCommand : public Command
{
public:
    StreamPayloadCommand()
        : Command(StreamPayload)
        , mExpectedSize(0)
    {}

    void setPayloadName(const QString &name)
    {
        mPayloadName = name;
    }
    QString payloadName() const
    {
        return mPayloadName;
    }

    void setExpectedSize(qint64 size)
    {
        mExpectedSize = size;
    }
    qint64 expectedSize() const
    {
        return mExpectedSize;
    }

    void setExternalFile(const QString &externalFile)
    {
        mExternalFile = externalFile;
    }
    QString externalFile() const
    {
        return mExternalFile;
    }
private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::StreamPayloadCommand &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::StreamPayloadCommand &command);

    QString mPayloadName;
    QString mExternalFile;
    qint64 mExpectedSize;
};

class AKONADIPRIVATE_EXPORT StreamPayloadResponse : public Response
{
public:
    StreamPayloadResponse()
        : Response(StreamPayload)
    {}

    void setIsExternal(bool external)
    {
        mIsExternal = external;
    }
    bool isExternal() const
    {
        return mIsExternal;
    }
    void setData(const QByteArray &data)
    {
        mData = data;
    }
    QByteArray data() const
    {
        return mData;
    }
private:
    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Protocol::StreamPayloadResponse &command);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Protocol::StreamPayloadResponse &command);

    QByteArray mData;
    bool mIsExternal;
};

} // namespace Protocol
} // namespace Akonadi


// Command parameters
#define AKONADI_PARAM_CAPABILITY_AKAPPENDSTREAMING "AKAPPENDSTREAMING"
#define AKONADI_PARAM_ALLATTRIBUTES                "ALLATTR"
#define AKONADI_PARAM_ANCESTORS                    "ANCESTORS"
#define AKONADI_PARAM_ANCESTORATTRIBUTE            "ANCESTORATTR"
#define AKONADI_PARAM_ATR                          "ATR:"
#define AKONADI_PARAM_CACHEONLY                    "CACHEONLY"
#define AKONADI_PARAM_CACHEDPARTS                  "CACHEDPARTS"
#define AKONADI_PARAM_CACHETIMEOUT                 "CACHETIMEOUT"
#define AKONADI_PARAM_CACHEPOLICY                  "CACHEPOLICY"
#define AKONADI_PARAM_CHANGEDSINCE                 "CHANGEDSINCE"
#define AKONADI_PARAM_CHARSET                      "CHARSET"
#define AKONADI_PARAM_CHECKCACHEDPARTSONLY         "CHECKCACHEDPARTSONLY"
#define AKONADI_PARAM_COLLECTION                   "COLLECTION"
#define AKONADI_PARAM_COLLECTIONID                 "COLLECTIONID"
#define AKONADI_PARAM_COLLECTIONS                  "COLLECTIONS"
#define AKONADI_PARAM_MTIME                        "DATETIME"
#define AKONADI_PARAM_CAPABILITY_DIRECTSTREAMING   "DIRECTSTREAMING"
#define AKONADI_PARAM_UNDIRTY                      "DIRTY"
#define AKONADI_PARAM_DISPLAY                      "DISPLAY"
#define AKONADI_PARAM_EXTERNALPAYLOAD              "EXTERNALPAYLOAD"
#define AKONADI_PARAM_ENABLED                      "ENABLED"
#define AKONADI_PARAM_FLAGS                        "FLAGS"
#define AKONADI_PARAM_TAGS                         "TAGS"
#define AKONADI_PARAM_FULLPAYLOAD                  "FULLPAYLOAD"
#define AKONADI_PARAM_GID                          "GID"
#define AKONADI_PARAM_GTAGS                        "GTAGS"
#define AKONADI_PARAM_IGNOREERRORS                 "IGNOREERRORS"
#define AKONADI_PARAM_INDEX                        "INDEX"
#define AKONADI_PARAM_INHERIT                      "INHERIT"
#define AKONADI_PARAM_INTERVAL                     "INTERVAL"
#define AKONADI_PARAM_INVALIDATECACHE              "INVALIDATECACHE"
#define AKONADI_PARAM_MIMETYPE                     "MIMETYPE"
#define AKONADI_PARAM_MERGE                        "MERGE"
#define AKONADI_PARAM_LEFT                         "LEFT"
#define AKONADI_PARAM_LOCALPARTS                   "LOCALPARTS"
#define AKONADI_PARAM_NAME                         "NAME"
#define AKONADI_PARAM_CAPABILITY_NOTIFY            "NOTIFY"
#define AKONADI_PARAM_CAPABILITY_NOPAYLOADPATH     "NOPAYLOADPATH"
#define AKONADI_PARAM_PARENT                       "PARENT"
#define AKONADI_PARAM_PERSISTENTSEARCH             "PERSISTENTSEARCH"
#define AKONADI_PARAM_PARTS                        "PARTS"
#define AKONADI_PARAM_PLD                          "PLD:"
#define AKONADI_PARAM_PLD_RFC822                   "PLD:RFC822"
#define AKONADI_PARAM_PERSISTENTSEARCH_QUERYCOLLECTIONS "QUERYCOLLECTIONS"
#define AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG   "QUERYLANGUAGE"
#define AKONADI_PARAM_PERSISTENTSEARCH_QUERYSTRING "QUERYSTRING"
#define AKONADI_PARAM_QUERY                        "QUERY"
#define AKONADI_PARAM_RIGHT                        "RIGHT"
#define AKONADI_PARAM_RECURSIVE                    "RECURSIVE"
#define AKONADI_PARAM_REFERENCED                   "REFERENCED"
#define AKONADI_PARAM_RELATIONS                    "RELATIONS"
#define AKONADI_PARAM_REMOTE                       "REMOTE"
#define AKONADI_PARAM_REMOTEID                     "REMOTEID"
#define AKONADI_PARAM_REMOTEREVISION               "REMOTEREVISION"
#define AKONADI_PARAM_RESOURCE                     "RESOURCE"
#define AKONADI_PARAM_REVISION                     "REV"
#define AKONADI_PARAM_RTAGS                        "RTAGS"
#define AKONADI_PARAM_SILENT                       "SILENT"
#define AKONADI_PARAM_DOT_SILENT                   ".SILENT"
#define AKONADI_PARAM_CAPABILITY_SERVERSEARCH      "SERVERSEARCH"
#define AKONADI_PARAM_SIDE                         "SIDE"
#define AKONADI_PARAM_SIZE                         "SIZE"
#define AKONADI_PARAM_STATISTICS                   "STATISTICS"
#define AKONADI_PARAM_SYNC                         "SYNC"
#define AKONADI_PARAM_SYNCONDEMAND                 "SYNCONDEMAND"
#define AKONADI_PARAM_TAG                          "TAG"
#define AKONADI_PARAM_TAGID                        "TAGID"
#define AKONADI_PARAM_TYPE                         "TYPE"
#define AKONADI_PARAM_UID                          "UID"
#define AKONADI_PARAM_VIRTREF                      "VIRTREF"
#define AKONADI_PARAM_VIRTUAL                      "VIRTUAL"

// Flags
#define AKONADI_FLAG_GID                           "\\Gid"
#define AKONADI_FLAG_IGNORED                       "$IGNORED"
#define AKONADI_FLAG_MIMETYPE                      "\\MimeType"
#define AKONADI_FLAG_REMOTEID                      "\\RemoteId"
#define AKONADI_FLAG_REMOTEREVISION                "\\RemoteRevision"
#define AKONADI_FLAG_TAG                           "\\Tag"
#define AKONADI_FLAG_RTAG                          "\\RTag"
#define AKONADI_FLAG_SEEN                          "\\SEEN"

// Attributes
#define AKONADI_ATTRIBUTE_HIDDEN                   "ATR:HIDDEN"
#define AKONADI_ATTRIBUTE_MESSAGES                 "MESSAGES"
#define AKONADI_ATTRIBUTE_UNSEEN                   "UNSEEN"

// special resource names
#define AKONADI_SEARCH_RESOURCE                    "akonadi_search_resource"
#endif
