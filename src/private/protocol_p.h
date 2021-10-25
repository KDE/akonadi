/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiprivate_export.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QJsonObject>
#include <QSharedPointer>
#include <QVector>

#include "scope_p.h"
#include "tristate_p.h"

// clazy:excludeall=function-args-by-value

/**
  @file protocol_p.h Shared constants used in the communication protocol between
  the Akonadi server and its clients.
*/

namespace Akonadi
{
namespace Protocol
{
class Factory;
class DataStream;

class Command;
class Response;
class ItemFetchScope;
class ScopeContext;
class ChangeNotification;

using Attributes = QMap<QByteArray, QByteArray>;

} // namespace Protocol
} // namespace Akonadi

namespace Akonadi
{
namespace Protocol
{
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::Command &cmd);
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::Command &cmd);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::Command &cmd);

AKONADIPRIVATE_EXPORT void toJson(const Command *cmd, QJsonObject &json);

using CommandPtr = QSharedPointer<Command>;

class AKONADIPRIVATE_EXPORT Command
{
public:
    enum Type : quint8 {
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

        // Other
        StreamPayload = 100,

        // Notifications
        ItemChangeNotification = 110,
        CollectionChangeNotification,
        TagChangeNotification,
        RelationChangeNotification,
        SubscriptionChangeNotification,
        DebugChangeNotification,
        CreateSubscription,
        ModifySubscription,

        // _MaxValue = 127
        _ResponseBit = 0x80U // reserved
    };

    explicit Command() = default;
    explicit Command(const Command &) = default;
    Command(Command &&) = default;
    ~Command() = default;

    Command &operator=(const Command &) = default;
    Command &operator=(Command &&) = default;

    bool operator==(const Command &other) const;
    inline bool operator!=(const Command &other) const
    {
        return !operator==(other);
    }

    inline Type type() const
    {
        return static_cast<Type>(mType & ~_ResponseBit);
    }
    inline bool isValid() const
    {
        return type() != Invalid;
    }
    inline bool isResponse() const
    {
        return mType & _ResponseBit;
    }

    void toJson(QJsonObject &stream) const;

protected:
    explicit Command(quint8 type);

    quint8 mType = Invalid;
    // unused 7 bytes

private:
    friend class Factory;
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::Command &cmd);
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::Command &cmd);
    friend AKONADIPRIVATE_EXPORT QDebug operator<<(::QDebug dbg, const Akonadi::Protocol::Command &cmd);
    friend AKONADIPRIVATE_EXPORT void toJson(const Akonadi::Protocol::Command *cmd, QJsonObject &json);
};

AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, Command::Type type);

} // namespace Protocol
} // namespace Akonadi

Q_DECLARE_METATYPE(Akonadi::Protocol::Command::Type)
Q_DECLARE_METATYPE(Akonadi::Protocol::CommandPtr)

namespace Akonadi
{
namespace Protocol
{
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::Response &cmd);
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::Response &cmd);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::Response &response);

using ResponsePtr = QSharedPointer<Response>;

class AKONADIPRIVATE_EXPORT Response : public Command
{
public:
    explicit Response();
    explicit Response(const Response &) = default;
    Response(Response &&) = default;
    Response &operator=(const Response &) = default;
    Response &operator=(Response &&) = default;

    inline void setError(int code, const QString &message)
    {
        mErrorCode = code;
        mErrorMsg = message;
    }

    bool operator==(const Response &other) const;
    inline bool operator!=(const Response &other) const
    {
        return !operator==(other);
    }

    inline bool isError() const
    {
        return mErrorCode > 0;
    }

    inline int errorCode() const
    {
        return mErrorCode;
    }
    inline QString errorMessage() const
    {
        return mErrorMsg;
    }

    void toJson(QJsonObject &json) const;

protected:
    explicit Response(Command::Type type);

    int mErrorCode;
    QString mErrorMsg;

private:
    friend class Factory;
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::Response &cmd);
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::Response &cmd);
    friend AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::Response &cmd);
};

} // namespace Protocol
} // namespace Akonadi

namespace Akonadi
{
namespace Protocol
{
template<typename X, typename T> inline const X &cmdCast(const QSharedPointer<T> &p)
{
    return static_cast<const X &>(*p);
}

template<typename X, typename T> inline X &cmdCast(QSharedPointer<T> &p)
{
    return static_cast<X &>(*p);
}

class AKONADIPRIVATE_EXPORT Factory
{
public:
    static CommandPtr command(Command::Type type);
    static ResponsePtr response(Command::Type type);

private:
    template<typename T> friend AKONADIPRIVATE_EXPORT CommandPtr deserialize(QIODevice *device);
};

AKONADIPRIVATE_EXPORT void serialize(DataStream &stream, const CommandPtr &command);
AKONADIPRIVATE_EXPORT CommandPtr deserialize(QIODevice *device);
AKONADIPRIVATE_EXPORT QString debugString(const Command &command);
AKONADIPRIVATE_EXPORT inline QString debugString(const CommandPtr &command)
{
    return debugString(*command);
}

} // namespace Protocol
} // namespace Akonadi

namespace Akonadi
{
namespace Protocol
{
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::ItemFetchScope &scope);
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::ItemFetchScope &scope);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::ItemFetchScope &scope);

class AKONADIPRIVATE_EXPORT ItemFetchScope
{
public:
    enum FetchFlag : int {
        None = 0,
        CacheOnly = 1 << 0,
        CheckCachedPayloadPartsOnly = 1 << 1,
        FullPayload = 1 << 2,
        AllAttributes = 1 << 3,
        Size = 1 << 4,
        MTime = 1 << 5,
        RemoteRevision = 1 << 6,
        IgnoreErrors = 1 << 7,
        Flags = 1 << 8,
        RemoteID = 1 << 9,
        GID = 1 << 10,
        Tags = 1 << 11,
        Relations = 1 << 12,
        VirtReferences = 1 << 13
    };
    Q_DECLARE_FLAGS(FetchFlags, FetchFlag)

    enum AncestorDepth : ushort {
        NoAncestor,
        ParentAncestor,
        AllAncestors,
    };

    explicit ItemFetchScope() = default;
    ItemFetchScope(const ItemFetchScope &) = default;
    ItemFetchScope(ItemFetchScope &&other) = default;
    ~ItemFetchScope() = default;

    ItemFetchScope &operator=(const ItemFetchScope &) = default;
    ItemFetchScope &operator=(ItemFetchScope &&) = default;

    bool operator==(const ItemFetchScope &other) const;
    inline bool operator!=(const ItemFetchScope &other) const
    {
        return !operator==(other);
    }

    inline void setRequestedParts(const QVector<QByteArray> &requestedParts)
    {
        mRequestedParts = requestedParts;
    }
    inline QVector<QByteArray> requestedParts() const
    {
        return mRequestedParts;
    }
    QVector<QByteArray> requestedPayloads() const;

    inline void setChangedSince(const QDateTime &changedSince)
    {
        mChangedSince = changedSince;
    }
    inline QDateTime changedSince() const
    {
        return mChangedSince;
    }

    inline void setAncestorDepth(AncestorDepth depth)
    {
        mAncestorDepth = depth;
    }
    inline AncestorDepth ancestorDepth() const
    {
        return mAncestorDepth;
    }

    inline bool cacheOnly() const
    {
        return mFlags & CacheOnly;
    }
    inline bool checkCachedPayloadPartsOnly() const
    {
        return mFlags & CheckCachedPayloadPartsOnly;
    }
    inline bool fullPayload() const
    {
        return mFlags & FullPayload;
    }
    inline bool allAttributes() const
    {
        return mFlags & AllAttributes;
    }
    inline bool fetchSize() const
    {
        return mFlags & Size;
    }
    inline bool fetchMTime() const
    {
        return mFlags & MTime;
    }
    inline bool fetchRemoteRevision() const
    {
        return mFlags & RemoteRevision;
    }
    inline bool ignoreErrors() const
    {
        return mFlags & IgnoreErrors;
    }
    inline bool fetchFlags() const
    {
        return mFlags & Flags;
    }
    inline bool fetchRemoteId() const
    {
        return mFlags & RemoteID;
    }
    inline bool fetchGID() const
    {
        return mFlags & GID;
    }
    inline bool fetchTags() const
    {
        return mFlags & Tags;
    }
    inline bool fetchRelations() const
    {
        return mFlags & Relations;
    }
    inline bool fetchVirtualReferences() const
    {
        return mFlags & VirtReferences;
    }

    void setFetch(FetchFlags attributes, bool fetch = true);
    bool fetch(FetchFlags flags) const;

    void toJson(QJsonObject &json) const;

private:
    AncestorDepth mAncestorDepth = NoAncestor;
    // 2 bytes free
    FetchFlags mFlags = None;
    QVector<QByteArray> mRequestedParts;
    QDateTime mChangedSince;

    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream,
                                                                           const Akonadi::Protocol::ItemFetchScope &scope);
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::ItemFetchScope &scope);
    friend AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::ItemFetchScope &scope);
};

} // namespace Protocol
} // namespace Akonadi

Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::Protocol::ItemFetchScope::FetchFlags)

namespace Akonadi
{
namespace Protocol
{
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::ScopeContext &ctx);
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::ScopeContext &ctx);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::ScopeContext &ctx);

class AKONADIPRIVATE_EXPORT ScopeContext
{
public:
    enum Type : uchar {
        Any = 0,
        Collection,
        Tag,
    };

    explicit ScopeContext() = default;
    ScopeContext(Type type, qint64 id);
    ScopeContext(Type type, const QString &id);
    ScopeContext(const ScopeContext &) = default;
    ScopeContext(ScopeContext &&) = default;
    ~ScopeContext() = default;

    ScopeContext &operator=(const ScopeContext &) = default;
    ScopeContext &operator=(ScopeContext &&) = default;

    bool operator==(const ScopeContext &other) const;
    inline bool operator!=(const ScopeContext &other) const
    {
        return !operator==(other);
    }

    inline bool isEmpty() const
    {
        return mColCtx.isNull() && mTagCtx.isNull();
    }

    inline void setContext(Type type, qint64 id)
    {
        setCtx(type, id);
    }
    inline void setContext(Type type, const QString &id)
    {
        setCtx(type, id);
    }
    inline void clearContext(Type type)
    {
        setCtx(type, QVariant());
    }

    inline bool hasContextId(Type type) const
    {
        return ctx(type).type() == QVariant::LongLong;
    }
    inline qint64 contextId(Type type) const
    {
        return hasContextId(type) ? ctx(type).toLongLong() : 0;
    }

    inline bool hasContextRID(Type type) const
    {
        return ctx(type).type() == QVariant::String;
    }
    inline QString contextRID(Type type) const
    {
        return hasContextRID(type) ? ctx(type).toString() : QString();
    }

    void toJson(QJsonObject &json) const;

private:
    QVariant mColCtx;
    QVariant mTagCtx;

    inline QVariant ctx(Type type) const
    {
        return type == Collection ? mColCtx : type == Tag ? mTagCtx : QVariant();
    }

    inline void setCtx(Type type, const QVariant &v)
    {
        if (type == Collection) {
            mColCtx = v;
        } else if (type == Tag) {
            mTagCtx = v;
        }
    }

    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream,
                                                                           const Akonadi::Protocol::ScopeContext &context);
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::ScopeContext &context);
    friend AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::ScopeContext &ctx);
};

} // namespace Protocol
} // namespace akonadi

namespace Akonadi
{
namespace Protocol
{
class FetchItemsResponse;
using FetchItemsResponsePtr = QSharedPointer<FetchItemsResponse>;

AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::ChangeNotification &ntf);
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::ChangeNotification &ntf);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::ChangeNotification &ntf);

using ChangeNotificationPtr = QSharedPointer<ChangeNotification>;
using ChangeNotificationList = QVector<ChangeNotificationPtr>;

class AKONADIPRIVATE_EXPORT ChangeNotification : public Command
{
public:
    static QList<qint64> itemsToUids(const QVector<Akonadi::Protocol::FetchItemsResponse> &items);

    class Relation
    {
    public:
        Relation() = default;
        Relation(const Relation &) = default;
        Relation(Relation &&) = default;
        inline Relation(qint64 leftId, qint64 rightId, const QString &type)
            : leftId(leftId)
            , rightId(rightId)
            , type(type)
        {
        }

        Relation &operator=(const Relation &) = default;
        Relation &operator=(Relation &&) = default;

        inline bool operator==(const Relation &other) const
        {
            return leftId == other.leftId && rightId == other.rightId && type == other.type;
        }

        void toJson(QJsonObject &json) const
        {
            json[QStringLiteral("leftId")] = leftId;
            json[QStringLiteral("rightId")] = rightId;
            json[QStringLiteral("type")] = type;
        }

        qint64 leftId = -1;
        qint64 rightId = -1;
        QString type;
    };

    ChangeNotification &operator=(const ChangeNotification &) = default;
    ChangeNotification &operator=(ChangeNotification &&) = default;

    bool operator==(const ChangeNotification &other) const;
    inline bool operator!=(const ChangeNotification &other) const
    {
        return !operator==(other);
    }

    bool isRemove() const;
    bool isMove() const;

    inline QByteArray sessionId() const
    {
        return mSessionId;
    }
    inline void setSessionId(const QByteArray &sessionId)
    {
        mSessionId = sessionId;
    }

    inline void addMetadata(const QByteArray &metadata)
    {
        mMetaData << metadata;
    }
    inline void removeMetadata(const QByteArray &metadata)
    {
        mMetaData.removeAll(metadata);
    }
    QVector<QByteArray> metadata() const
    {
        return mMetaData;
    }

    static bool appendAndCompress(ChangeNotificationList &list, const ChangeNotificationPtr &msg);

    void toJson(QJsonObject &json) const;

protected:
    explicit ChangeNotification() = default;
    explicit ChangeNotification(Command::Type type);
    ChangeNotification(const ChangeNotification &) = default;
    ChangeNotification(ChangeNotification &&) = default;

    QByteArray mSessionId;

    // For internal use only: Akonadi server can add some additional information
    // that might be useful when evaluating the notification for example, but
    // it is never transferred to clients
    QVector<QByteArray> mMetaData;

    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream,
                                                                           const Akonadi::Protocol::ChangeNotification &ntf);
    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::ChangeNotification &ntf);
    friend AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::ChangeNotification &ntf);
};

inline uint qHash(const ChangeNotification::Relation &rel)
{
    return ::qHash(rel.leftId + rel.rightId);
}

// TODO: Internalize?
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream,
                                                                const Akonadi::Protocol::ChangeNotification::Relation &relation);
AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream,
                                                                Akonadi::Protocol::ChangeNotification::Relation &relation);

} // namespace Protocol
} // namespace Akonadi

Q_DECLARE_METATYPE(Akonadi::Protocol::ChangeNotificationPtr)
Q_DECLARE_METATYPE(Akonadi::Protocol::ChangeNotificationList)
Q_DECLARE_TYPEINFO(Akonadi::Protocol::ChangeNotification::Relation, Q_MOVABLE_TYPE);

/******************************************************************************/

// Here comes the actual generated Protocol. See protocol.xml for definitions,
// and genprotocol folder for the generator.
#include "protocol_gen.h"

/******************************************************************************/

// Command parameters
#define AKONADI_PARAM_ATR "ATR:"
#define AKONADI_PARAM_CACHEPOLICY "CACHEPOLICY"
#define AKONADI_PARAM_DISPLAY "DISPLAY"
#define AKONADI_PARAM_ENABLED "ENABLED"
#define AKONADI_PARAM_FLAGS "FLAGS"
#define AKONADI_PARAM_TAGS "TAGS"
#define AKONADI_PARAM_GID "GID"
#define AKONADI_PARAM_INDEX "INDEX"
#define AKONADI_PARAM_MIMETYPE "MIMETYPE"
#define AKONADI_PARAM_NAME "NAME"
#define AKONADI_PARAM_PARENT "PARENT"
#define AKONADI_PARAM_PERSISTENTSEARCH "PERSISTENTSEARCH"
#define AKONADI_PARAM_PLD "PLD:"
#define AKONADI_PARAM_PLD_RFC822 "PLD:RFC822"
#define AKONADI_PARAM_RECURSIVE "RECURSIVE"
#define AKONADI_PARAM_REMOTE "REMOTE"
#define AKONADI_PARAM_REMOTEID "REMOTEID"
#define AKONADI_PARAM_REMOTEREVISION "REMOTEREVISION"
#define AKONADI_PARAM_REVISION "REV"
#define AKONADI_PARAM_SIZE "SIZE"
#define AKONADI_PARAM_SYNC "SYNC"
#define AKONADI_PARAM_TAG "TAG"
#define AKONADI_PARAM_TYPE "TYPE"
#define AKONADI_PARAM_VIRTUAL "VIRTUAL"

// Flags
#define AKONADI_FLAG_GID "\\Gid"
#define AKONADI_FLAG_IGNORED "$IGNORED"
#define AKONADI_FLAG_MIMETYPE "\\MimeType"
#define AKONADI_FLAG_REMOTEID "\\RemoteId"
#define AKONADI_FLAG_REMOTEREVISION "\\RemoteRevision"
#define AKONADI_FLAG_TAG "\\Tag"
#define AKONADI_FLAG_RTAG "\\RTag"
#define AKONADI_FLAG_SEEN "\\SEEN"

// Attributes
#define AKONADI_ATTRIBUTE_HIDDEN "ATR:HIDDEN"
#define AKONADI_ATTRIBUTE_MESSAGES "MESSAGES"
#define AKONADI_ATTRIBUTE_UNSEEN "UNSEEN"

// special resource names
#define AKONADI_SEARCH_RESOURCE "akonadi_search_resource"

namespace Akonadi
{
static const QString CollectionMimeType = QStringLiteral("inode/directory");
static const QString VirtualCollectionMimeType = QStringLiteral("application/x-vnd.akonadi.collection.virtual");

}

