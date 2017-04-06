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

//krazy:excludeall=dpointer,explicit,inline

#ifndef AKONADI_PROTOCOL_P_H
#define AKONADI_PROTOCOL_P_H

#include "akonadiprivate_export.h"

#include <QSharedDataPointer>
#include <QDebug>

#include "tristate_p.h"

class DataStream;
class QString;
class QByteArray;
template<typename T1, typename T2> class QMap;
template<typename T1> class QSet;
class QDateTime;

namespace Akonadi
{
class Scope;
}

/**
  @file protocol_p.h Shared constants used in the communication protocol between
  the Akonadi server and its clients.
*/

namespace Akonadi
{

namespace Protocol
{

// NOTE: Q_DECLARE_PRIVATE does not work with QSharedDataPointer<T> when T is incomplete
#ifndef AKONADI_DECLARE_PRIVATE
#define AKONADI_DECLARE_PRIVATE(Class) \
    Class##Private* d_func(); \
    const Class##Private* d_func() const; \
    friend class Class##Private;
#endif

typedef QMap<QByteArray, QByteArray> Attributes;
class Factory;

AKONADIPRIVATE_EXPORT int version();

class DataStream;
class DebugBlock;
class CommandPrivate;
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

        _ResponseBit = 0x80 // reserved
    };

    explicit Command();
    Command(Command &&other);
    Command(const Command &other);
    ~Command();

    Command &operator=(Command &&other);
    Command &operator=(const Command &other);

    bool operator==(const Command &other) const;
    bool operator!=(const Command &other) const;

    Type type() const;
    bool isValid() const;
    bool isResponse() const;

    QString debugString() const;
    QString debugString(DebugBlock &blck) const;

protected:
    explicit Command(CommandPrivate *dd);

    QSharedDataPointer<CommandPrivate> d_ptr;
    AKONADI_DECLARE_PRIVATE(Command)

private:
    friend class Factory;

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::Command &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::Command &command);
};

class ResponsePrivate;
class AKONADIPRIVATE_EXPORT Response : public Command
{
public:
    explicit Response();
    explicit Response(const Command &other);

    void setError(int code, const QString &message);
    bool isError() const;

    int errorCode() const;
    QString errorMessage() const;

protected:
    explicit Response(ResponsePrivate *dd);
    AKONADI_DECLARE_PRIVATE(Response)

private:
    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::Response &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::Response &command);
};

class AKONADIPRIVATE_EXPORT Factory
{
public:
    static Command command(Command::Type type);
    static Response response(Command::Type type);
};

AKONADIPRIVATE_EXPORT void serialize(QIODevice *device, const Command &command);
AKONADIPRIVATE_EXPORT Command deserialize(QIODevice *device);

class AncestorPrivate;
class AKONADIPRIVATE_EXPORT Ancestor
{
public:
    enum Depth : uchar {
        NoAncestor,
        ParentAncestor,
        AllAncestors
    };

    explicit Ancestor();
    explicit Ancestor(qint64 id);
    Ancestor(qint64 id, const QString &remoteId);
    Ancestor(Ancestor &&other);
    Ancestor(const Ancestor &other);
    ~Ancestor();

    Ancestor &operator=(Ancestor &&other);
    Ancestor &operator=(const Ancestor &other);

    bool operator==(const Ancestor &other) const;
    bool operator!=(const Ancestor &other) const;

    void setId(qint64 id);
    qint64 id() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setName(const QString &name);
    QString name() const;

    void setAttributes(const Attributes &attrs);
    Attributes attributes() const;

    void debugString(DebugBlock &blck) const;
private:
    QSharedDataPointer<AncestorPrivate> d;

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::Ancestor &ancestor);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::Ancestor &ancestor);
};

class FetchScopePrivate;
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

    explicit FetchScope();
    FetchScope(FetchScope &&other);
    FetchScope(const FetchScope &other);
    ~FetchScope();

    FetchScope &operator=(FetchScope &&other);
    FetchScope &operator=(const FetchScope &other);

    bool operator==(const FetchScope &other) const;
    bool operator!=(const FetchScope &other) const;

    void setRequestedParts(const QVector<QByteArray> &requestedParts);
    QVector<QByteArray> requestedParts() const;
    QVector<QByteArray> requestedPayloads() const;

    void setChangedSince(const QDateTime &changedSince);
    QDateTime changedSince() const;

    void setTagFetchScope(const QSet<QByteArray> &tagFetchScope);
    QSet<QByteArray> tagFetchScope() const;

    void setAncestorDepth(Ancestor::Depth depth);
    Ancestor::Depth ancestorDepth() const;

    bool cacheOnly() const;
    bool checkCachedPayloadPartsOnly() const;
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

    void setFetch(FetchFlags attributes, bool fetch = true);
    bool fetch(FetchFlags flags) const;

    void debugString(DebugBlock &blck) const;
private:
    QSharedDataPointer<FetchScopePrivate> d;

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchScope &scope);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchScope &scope);
};

class ScopeContextPrivate;
class AKONADIPRIVATE_EXPORT ScopeContext
{
public:
    enum Type : uchar {
        Any = 0,
        Collection,
        Tag
    };

    explicit ScopeContext();
    ScopeContext(Type type, qint64 id);
    ScopeContext(Type type, const QString &id);
    ScopeContext(const ScopeContext &other);
    ScopeContext(ScopeContext &&other);
    ~ScopeContext();

    ScopeContext &operator=(const ScopeContext &other);
    ScopeContext &operator=(ScopeContext &&other);

    bool operator==(const ScopeContext &other) const;
    bool operator!=(const ScopeContext &other) const;

    bool isEmpty() const;

    void setContext(Type type, qint64 id);
    void setContext(Type type, const QString &id);
    void clearContext(Type type);

    bool hasContextId(Type type) const;
    qint64 contextId(Type type) const;

    bool hasContextRID(Type type) const;
    QString contextRID(Type type) const;

    void debugString(DebugBlock &blck) const;
private:
    QSharedDataPointer<ScopeContextPrivate> d;

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ScopeContext &context);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ScopeContext &context);
};

class PartMetaDataPrivate;
class AKONADIPRIVATE_EXPORT PartMetaData
{
public:
    enum StorageType {
        Internal,
        External,
        Foreign
    };

    explicit PartMetaData();
    PartMetaData(const QByteArray &name, qint64 size, int version = 0,
                 StorageType storageType = Internal);
    PartMetaData(PartMetaData &&other);
    PartMetaData(const PartMetaData &other);
    ~PartMetaData();

    PartMetaData &operator=(PartMetaData &&other);
    PartMetaData &operator=(const PartMetaData &other);

    bool operator==(const PartMetaData &other) const;
    bool operator!=(const PartMetaData &other) const;

    bool operator<(const PartMetaData &other) const;

    void setName(const QByteArray &name);
    QByteArray name() const;

    void setSize(qint64 size);
    qint64 size() const;

    void setVersion(int version);
    int version() const;

    void setStorageType(StorageType storage);
    StorageType storageType() const;

private:
    QSharedDataPointer<PartMetaDataPrivate> d;

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::PartMetaData &part);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::PartMetaData &part);
};

class CachePolicyPrivate;
class AKONADIPRIVATE_EXPORT CachePolicy
{
public:
    explicit CachePolicy();
    CachePolicy(CachePolicy &&other);
    CachePolicy(const CachePolicy &other);
    ~CachePolicy();

    CachePolicy &operator=(CachePolicy &&other);
    CachePolicy &operator=(const CachePolicy &other);

    bool operator==(const CachePolicy &other) const;
    bool operator!=(const CachePolicy &other) const;

    void setInherit(bool inherit);
    bool inherit() const;

    void setCheckInterval(int interval);
    int checkInterval() const;

    void setCacheTimeout(int timeout);
    int cacheTimeout() const;

    void setSyncOnDemand(bool onDemand);
    bool syncOnDemand() const;

    void setLocalParts(const QStringList &parts);
    QStringList localParts() const;

    void debugString(DebugBlock &blck) const;
private:
    QSharedDataPointer<CachePolicyPrivate> d;

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CachePolicy &policy);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CachePolicy &policy);
};

class HelloResponsePrivate;
class AKONADIPRIVATE_EXPORT HelloResponse : public Response
{
public:
    explicit HelloResponse();
    explicit HelloResponse(const Command &command);

    void setServerName(const QString &server);
    QString serverName() const;

    void setMessage(const QString &message);
    QString message() const;

    void setProtocolVersion(int protocolVersion);
    int protocolVersion() const;

    void setGeneration(uint generation);
    uint generation() const;

private:
    AKONADI_DECLARE_PRIVATE(HelloResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::HelloResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::HelloResponse &command);
};

class LoginCommandPrivate;
class AKONADIPRIVATE_EXPORT LoginCommand : public Command
{
public:
    explicit LoginCommand();
    explicit LoginCommand(const QByteArray &sessionId);
    LoginCommand(const Command &command);

    void setSessionId(const QByteArray &sessionId);
    QByteArray sessionId() const;

private:
    AKONADI_DECLARE_PRIVATE(LoginCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::LoginCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::LoginCommand &command);
};

class AKONADIPRIVATE_EXPORT LoginResponse : public Response
{
public:
    explicit LoginResponse();
    LoginResponse(const Command &command);
};

class AKONADIPRIVATE_EXPORT LogoutCommand : public Command
{
public:
    explicit LogoutCommand();
    LogoutCommand(const Command &command);
};

class AKONADIPRIVATE_EXPORT LogoutResponse : public Response
{
public:
    explicit LogoutResponse();
    LogoutResponse(const Command &command);
};

class TransactionCommandPrivate;
class AKONADIPRIVATE_EXPORT TransactionCommand : public Command
{
public:
    enum Mode : uchar {
        Invalid = 0,
        Begin,
        Commit,
        Rollback
    };

    explicit TransactionCommand();
    explicit TransactionCommand(Mode mode);
    TransactionCommand(const Command &command);

    void setMode(Mode mode);
    Mode mode() const;

private:
    AKONADI_DECLARE_PRIVATE(TransactionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::TransactionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::TransactionCommand &command);
};

class AKONADIPRIVATE_EXPORT TransactionResponse : public Response
{
public:
    explicit TransactionResponse();
    TransactionResponse(const Command &command);
};

class CreateItemCommandPrivate;
class AKONADIPRIVATE_EXPORT CreateItemCommand : public Command
{
public:
    enum MergeMode : uchar {
        None = 0,
        GID = 1,
        RemoteID = 2,
        Silent = 4
    };
    Q_DECLARE_FLAGS(MergeModes, MergeMode)

    explicit CreateItemCommand();
    explicit CreateItemCommand(const Command &command);

    void setMergeModes(const MergeModes &mode);
    MergeModes mergeModes() const;

    void setCollection(const Scope &collection);
    Scope collection() const;

    void setItemSize(qint64 size);
    qint64 itemSize() const;

    void setMimeType(const QString &mimeType);
    QString mimeType() const;

    void setGID(const QString &gid);
    QString gid() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRevision);
    QString remoteRevision() const;

    void setDateTime(const QDateTime &dateTime);
    QDateTime dateTime() const;

    void setFlags(const QSet<QByteArray> &flags);
    QSet<QByteArray> flags() const;
    void setAddedFlags(const QSet<QByteArray> &flags);
    QSet<QByteArray> addedFlags() const;
    void setRemovedFlags(const QSet<QByteArray> &flags);
    QSet<QByteArray> removedFlags() const;

    void setTags(const Scope &tags);
    Scope tags() const;
    void setAddedTags(const Scope &tags);
    Scope addedTags() const;
    void setRemovedTags(const Scope &tags);
    Scope removedTags() const;

    void setAttributes(const Attributes &attributes);
    Attributes attributes() const;

    void setParts(const QSet<QByteArray> &parts);
    QSet<QByteArray> parts() const;

private:
    AKONADI_DECLARE_PRIVATE(CreateItemCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CreateItemCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CreateItemCommand &command);
};

class AKONADIPRIVATE_EXPORT CreateItemResponse : public Response
{
public:
    explicit CreateItemResponse();
    CreateItemResponse(const Command &command);
};

class CopyItemsCommandPrivate;
class AKONADIPRIVATE_EXPORT CopyItemsCommand : public Command
{
public:
    explicit CopyItemsCommand();
    explicit CopyItemsCommand(const Scope &items, const Scope &destination);
    CopyItemsCommand(const Command &command);

    void setItems(const Scope &items);
    Scope items() const;
    void setDestination(const Scope &destination);
    Scope destination() const;

private:
    AKONADI_DECLARE_PRIVATE(CopyItemsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CopyItemsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CopyItemsCommand &command);
};

class AKONADIPRIVATE_EXPORT CopyItemsResponse : public Response
{
public:
    explicit CopyItemsResponse();
    CopyItemsResponse(const Command &command);
};

class DeleteItemsCommandPrivate;
class AKONADIPRIVATE_EXPORT DeleteItemsCommand : public Command
{
public:
    explicit DeleteItemsCommand();
    explicit DeleteItemsCommand(const Scope &scope, const ScopeContext &context = ScopeContext());
    DeleteItemsCommand(const Command &command);

    ScopeContext scopeContext() const;
    Scope items() const;

private:
    AKONADI_DECLARE_PRIVATE(DeleteItemsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::DeleteItemsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::DeleteItemsCommand &command);
};

class AKONADIPRIVATE_EXPORT DeleteItemsResponse : public Response
{
public:
    explicit DeleteItemsResponse();
    DeleteItemsResponse(const Command &command);
};

class FetchRelationsCommandPrivate;
class AKONADIPRIVATE_EXPORT FetchRelationsCommand : public Command
{
public:
    explicit FetchRelationsCommand();
    FetchRelationsCommand(qint64 side, const QVector<QByteArray> &types = QVector<QByteArray>(),
                          const QString &resource = QString());
    FetchRelationsCommand(qint64 left, qint64 right, const QVector<QByteArray> &types = QVector<QByteArray>(),
                          const QString &resource = QString());
    FetchRelationsCommand(const Command &command);

    void setLeft(qint64 left);
    qint64 left() const;

    void setRight(qint64 right);
    qint64 right() const;

    void setSide(qint64 side);
    qint64 side() const;

    void setTypes(const QVector<QByteArray> &types);
    QVector<QByteArray> types() const;

    void setResource(const QString &resource);
    QString resource() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchRelationsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchRelationsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchRelationsCommand &command);
};

class FetchRelationsResponsePrivate;
class AKONADIPRIVATE_EXPORT FetchRelationsResponse : public Response
{
public:
    explicit FetchRelationsResponse();
    explicit FetchRelationsResponse(qint64 left, const QByteArray &leftMimeType, qint64 right, const QByteArray &rightMimeType, const QByteArray &type,
                                    const QByteArray &remoteId = QByteArray());
    FetchRelationsResponse(const Command &command);

    qint64 left() const;
    QByteArray leftMimeType() const;
    qint64 right() const;
    QByteArray rightMimeType() const;
    QByteArray type() const;

    void setRemoteId(const QByteArray &remoteId);
    QByteArray remoteId() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchRelationsResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchRelationsResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchRelationsResponse &command);
};

class FetchTagsCommandPrivate;
class AKONADIPRIVATE_EXPORT FetchTagsCommand : public Command
{
public:
    explicit FetchTagsCommand();
    explicit FetchTagsCommand(const Scope &scope);
    FetchTagsCommand(const Command &command);

    Scope scope() const;

    void setAttributes(const QSet<QByteArray> &attributes);
    QSet<QByteArray> attributes() const;

    void setIdOnly(bool idOnly);
    bool idOnly() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchTagsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchTagsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchTagsCommand &command);
};

class FetchTagsResponsePrivate;
class AKONADIPRIVATE_EXPORT FetchTagsResponse : public Response
{
public:
    explicit FetchTagsResponse();
    explicit FetchTagsResponse(qint64 id);
    FetchTagsResponse(qint64 id, const QByteArray &gid, const QByteArray &type,
                      const QByteArray &remoteId = QByteArray(),
                      qint64 parentId = 0,
                      const Attributes &attrs = Attributes());
    FetchTagsResponse(const Command &command);

    qint64 id() const;

    void setParentId(qint64 parentId);
    qint64 parentId() const;

    void setGid(const QByteArray &gid);
    QByteArray gid() const;

    void setType(const QByteArray &type);
    QByteArray type() const;

    void setRemoteId(const QByteArray &remoteId);
    QByteArray remoteId() const;

    void setAttributes(const Attributes &attributes);
    Attributes attributes() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchTagsResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchTagsResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchTagsResponse &command);
};

class FetchItemsCommandPrivate;
class AKONADIPRIVATE_EXPORT FetchItemsCommand : public Command
{
public:
    explicit FetchItemsCommand();
    explicit FetchItemsCommand(const Scope &scope, const FetchScope &fetchScope = FetchScope());
    explicit FetchItemsCommand(const Scope &scope, const ScopeContext &ctx,
                               const FetchScope &fetchScope = FetchScope());
    FetchItemsCommand(const Command &command);

    Scope scope() const;
    ScopeContext scopeContext() const;
    FetchScope fetchScope() const;
    FetchScope &fetchScope();

private:
    AKONADI_DECLARE_PRIVATE(FetchItemsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchItemsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchItemsCommand &command);
};

class StreamPayloadResponse;
class FetchItemsResponsePrivate;
class AKONADIPRIVATE_EXPORT FetchItemsResponse : public Response
{
public:
    explicit FetchItemsResponse();
    explicit FetchItemsResponse(qint64 id);
    FetchItemsResponse(const Command &command);

    qint64 id() const;

    void setRevision(int revision);
    int revision() const;

    void setParentId(qint64 parent);
    qint64 parentId() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRev);
    QString remoteRevision() const;

    void setGid(const QString &gid);
    QString gid() const;

    void setSize(qint64 size);
    qint64 size() const;

    void setMimeType(const QString &mimeType);
    QString mimeType() const;

    void setMTime(const QDateTime &mtime);
    QDateTime MTime() const;

    void setFlags(const QVector<QByteArray> &flags);
    QVector<QByteArray> flags() const;

    void setTags(const QVector<FetchTagsResponse> &tags);
    QVector<FetchTagsResponse> tags() const;

    void setVirtualReferences(const QVector<qint64> &virtRefs);
    QVector<qint64> virtualReferences() const;

    void setRelations(const QVector<FetchRelationsResponse> &relations);
    QVector<FetchRelationsResponse> relations() const;

    void setAncestors(const QVector<Ancestor> &ancestors);
    QVector<Ancestor> ancestors() const;

    void setParts(const QVector<StreamPayloadResponse> &parts);
    QVector<StreamPayloadResponse> parts() const;

    void setCachedParts(const QVector<QByteArray> &cachedParts);
    QVector<QByteArray> cachedParts() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchItemsResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchItemsResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchItemsResponse &command);
};

class LinkItemsCommandPrivate;
class AKONADIPRIVATE_EXPORT LinkItemsCommand : public Command
{
public:
    enum Action : bool {
        Link,
        Unlink
    };

    explicit LinkItemsCommand();
    explicit LinkItemsCommand(Action action, const Scope &items, const Scope &dest);
    LinkItemsCommand(const Command &command);

    Action action() const;
    Scope items() const;
    Scope destination() const;

private:
    AKONADI_DECLARE_PRIVATE(LinkItemsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::LinkItemsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::LinkItemsCommand &command);
};

class AKONADIPRIVATE_EXPORT LinkItemsResponse : public Response
{
public:
    explicit LinkItemsResponse();
    LinkItemsResponse(const Command &command);
};

class ModifyItemsCommandPrivate;
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
        RemovedParts    = 1 << 12,
        Attributes      = 1 << 13
    };
    Q_DECLARE_FLAGS(ModifiedParts, ModifiedPart)

    explicit ModifyItemsCommand();
    explicit ModifyItemsCommand(const Scope &scope);
    ModifyItemsCommand(const Command &command);

    ModifiedParts modifiedParts() const;

    void setItems(const Scope &scope);
    Scope items() const;

    void setOldRevision(int oldRevision);
    int oldRevision() const;

    void setFlags(const QSet<QByteArray> &flags);
    QSet<QByteArray> flags() const;
    void setAddedFlags(const QSet<QByteArray> &flags);
    QSet<QByteArray> addedFlags() const;
    void setRemovedFlags(const QSet<QByteArray> &flags);
    QSet<QByteArray> removedFlags() const;

    void setTags(const Scope &tags);
    Scope tags() const;
    void setAddedTags(const Scope &tags);
    Scope addedTags() const;
    void setRemovedTags(const Scope &tags);
    Scope removedTags() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setGid(const QString &gid);
    QString gid() const;

    void setRemoteRevision(const QString &remoteRevision);
    QString remoteRevision() const;

    void setDirty(bool dirty);
    bool dirty() const;

    void setInvalidateCache(bool invalidate);
    bool invalidateCache() const;

    void setNoResponse(bool noResponse);
    bool noResponse() const;

    void setNotify(bool notify);
    bool notify() const;

    void setItemSize(qint64 size);
    qint64 itemSize() const;

    void setRemovedParts(const QSet<QByteArray> &removedParts);
    QSet<QByteArray> removedParts() const;

    void setParts(const QSet<QByteArray> &parts);
    QSet<QByteArray> parts() const;

    void setAttributes(const Protocol::Attributes &attributes);
    Protocol::Attributes attributes() const;
private:
    AKONADI_DECLARE_PRIVATE(ModifyItemsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifyItemsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifyItemsCommand &command);
};

class ModifyItemsResponsePrivate;
class AKONADIPRIVATE_EXPORT ModifyItemsResponse : public Response
{
public:
    explicit ModifyItemsResponse();
    ModifyItemsResponse(qint64 id, int newRevision);
    ModifyItemsResponse(const QDateTime &modificationDT);
    ModifyItemsResponse(const Command &command);

    qint64 id() const;
    int newRevision() const;
    QDateTime modificationDateTime() const;

private:
    AKONADI_DECLARE_PRIVATE(ModifyItemsResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifyItemsResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifyItemsResponse &command);
};

class MoveItemsCommandPrivate;
class AKONADIPRIVATE_EXPORT MoveItemsCommand : public Command
{
public:
    explicit MoveItemsCommand();
    explicit MoveItemsCommand(const Scope &items, const Scope &dest);
    explicit MoveItemsCommand(const Scope &items, const ScopeContext &context, const Scope &dest);
    MoveItemsCommand(const Command &command);

    Scope items() const;
    ScopeContext itemsContext() const;
    Scope destination() const;

private:
    AKONADI_DECLARE_PRIVATE(MoveItemsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::MoveItemsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::MoveItemsCommand &command);
};

class AKONADIPRIVATE_EXPORT MoveItemsResponse : public Response
{
public:
    explicit MoveItemsResponse();
    MoveItemsResponse(const Command &command);
};

class CreateCollectionCommandPrivate;
class AKONADIPRIVATE_EXPORT CreateCollectionCommand : public Command
{
public:
    explicit CreateCollectionCommand();
    CreateCollectionCommand(const Command &command);

    void setParent(const Scope &scope);
    Scope parent() const;

    void setName(const QString &name);
    QString name() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRevision);
    QString remoteRevision() const;

    void setMimeTypes(const QStringList &mimeTypes);
    QStringList mimeTypes() const;

    void setCachePolicy(const CachePolicy &cachePolicy);
    CachePolicy cachePolicy() const;

    void setAttributes(const Attributes &attributes);
    Attributes attributes() const;

    void setIsVirtual(bool isVirtual);
    bool isVirtual() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void setSyncPref(Tristate sync);
    Tristate syncPref() const;

    void setDisplayPref(Tristate display);
    Tristate displayPref() const;

    void setIndexPref(Tristate index);
    Tristate indexPref() const;

private:
    AKONADI_DECLARE_PRIVATE(CreateCollectionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CreateCollectionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CreateCollectionCommand &command);
};

class AKONADIPRIVATE_EXPORT CreateCollectionResponse : public Response
{
public:
    explicit CreateCollectionResponse();
    CreateCollectionResponse(const Command &command);
};

class CopyCollectionCommandPrivate;
class AKONADIPRIVATE_EXPORT CopyCollectionCommand : public Command
{
public:
    explicit CopyCollectionCommand();
    explicit CopyCollectionCommand(const Scope &collection, const Scope &dest);
    CopyCollectionCommand(const Command &command);

    Scope collection() const;
    Scope destination() const;

private:
    AKONADI_DECLARE_PRIVATE(CopyCollectionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CopyCollectionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CopyCollectionCommand &command);
};

class AKONADIPRIVATE_EXPORT CopyCollectionResponse : public Response
{
public:
    explicit CopyCollectionResponse();
    CopyCollectionResponse(const Command &command);
};

class DeleteCollectionCommandPrivate;
class AKONADIPRIVATE_EXPORT DeleteCollectionCommand : public Command
{
public:
    explicit DeleteCollectionCommand();
    explicit DeleteCollectionCommand(const Scope &scope);
    DeleteCollectionCommand(const Command &command);

    Scope collection() const;

private:
    AKONADI_DECLARE_PRIVATE(DeleteCollectionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::DeleteCollectionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::DeleteCollectionCommand &command);
};

class AKONADIPRIVATE_EXPORT DeleteCollectionResponse : public Response
{
public:
    explicit DeleteCollectionResponse();
    DeleteCollectionResponse(const Command &command);
};

class FetchCollectionStatsCommandPrivate;
class AKONADIPRIVATE_EXPORT FetchCollectionStatsCommand : public Command
{
public:
    explicit FetchCollectionStatsCommand();
    explicit FetchCollectionStatsCommand(const Scope &col);
    FetchCollectionStatsCommand(const Command &command);

    Scope collection() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchCollectionStatsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchCollectionStatsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchCollectionStatsCommand &command);
};

class FetchCollectionStatsResponsePrivate;
class AKONADIPRIVATE_EXPORT FetchCollectionStatsResponse : public Response
{
public:
    explicit FetchCollectionStatsResponse();
    explicit FetchCollectionStatsResponse(qint64 count, qint64 unseen, qint64 size);
    FetchCollectionStatsResponse(const Command &command);

    qint64 count() const;
    qint64 unseen() const;
    qint64 size() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchCollectionStatsResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchCollectionStatsResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchCollectionStatsResponse &command);
};

class FetchCollectionsCommandPrivate;
class AKONADIPRIVATE_EXPORT FetchCollectionsCommand : public Command
{
public:
    enum Depth : uchar {
        BaseCollection,
        ParentCollection,
        AllCollections
    };

    explicit FetchCollectionsCommand();
    explicit FetchCollectionsCommand(const Scope &scope);
    FetchCollectionsCommand(const Command &command);

    Scope collections() const;

    void setDepth(Depth depth);
    Depth depth() const;

    void setResource(const QString &resourceId);
    QString resource() const;

    void setMimeTypes(const QStringList &mimeTypes);
    QStringList mimeTypes() const;

    void setAncestorsDepth(Ancestor::Depth depth);
    Ancestor::Depth ancestorsDepth() const;

    void setAncestorsAttributes(const QSet<QByteArray> &attributes);
    QSet<QByteArray> ancestorsAttributes() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void setSyncPref(bool sync);
    bool syncPref() const;

    void setDisplayPref(bool display);
    bool displayPref() const;

    void setIndexPref(bool index);
    bool indexPref() const;

    void setFetchStats(bool stats);
    bool fetchStats() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchCollectionsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchCollectionsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchCollectionsCommand &command);
};

class FetchCollectionsResponsePrivate;
class AKONADIPRIVATE_EXPORT FetchCollectionsResponse : public Response
{
public:
    explicit FetchCollectionsResponse();
    explicit FetchCollectionsResponse(qint64 id);
    FetchCollectionsResponse(const Command &command);

    qint64 id() const;

    void setParentId(qint64 id);
    qint64 parentId() const;

    void setName(const QString &name);
    QString name() const;

    void setMimeTypes(const QStringList &mimeType);
    QStringList mimeTypes() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRev);
    QString remoteRevision() const;

    void setResource(const QString &resourceId);
    QString resource() const;

    void setStatistics(const FetchCollectionStatsResponse &stats);
    FetchCollectionStatsResponse statistics() const;

    void setSearchQuery(const QString &searchQuery);
    QString searchQuery() const;

    void setSearchCollections(const QVector<qint64> &searchCols);
    QVector<qint64> searchCollections() const;

    void setAncestors(const QVector<Ancestor> &ancestors);
    QVector<Ancestor> ancestors() const;

    void setCachePolicy(const CachePolicy &cachePolicy);
    CachePolicy cachePolicy() const;
    CachePolicy &cachePolicy();

    void setAttributes(const Attributes &attrs);
    Attributes attributes() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void setDisplayPref(Tristate displayPref);
    Tristate displayPref() const;

    void setSyncPref(Tristate syncPref);
    Tristate syncPref() const;

    void setIndexPref(Tristate indexPref);
    Tristate indexPref() const;

    void setReferenced(bool ref);
    bool referenced() const;

    void setIsVirtual(bool isVirtual);
    bool isVirtual() const;

private:
    AKONADI_DECLARE_PRIVATE(FetchCollectionsResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::FetchCollectionsResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::FetchCollectionsResponse &command);
};

class ModifyCollectionCommandPrivate;
class AKONADIPRIVATE_EXPORT ModifyCollectionCommand : public Command
{
public:
    enum ModifiedPart {
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
    Q_DECLARE_FLAGS(ModifiedParts, ModifiedPart)

    explicit ModifyCollectionCommand();
    explicit ModifyCollectionCommand(const Scope &scope);
    ModifyCollectionCommand(const Command &command);

    ModifiedParts modifiedParts() const;

    Scope collection() const;

    void setParentId(qint64 parentId);
    qint64 parentId() const;

    void setMimeTypes(const QStringList &mimeTypes);
    QStringList mimeTypes() const;

    void setCachePolicy(const Protocol::CachePolicy &cachePolicy);
    Protocol::CachePolicy cachePolicy() const;

    void setName(const QString &name);
    QString name() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRevision);
    QString remoteRevision() const;

    void setPersistentSearchQuery(const QString &query);
    QString persistentSearchQuery() const;

    void setPersistentSearchCollections(const QVector<qint64> &cols);
    QVector<qint64> persistentSearchCollections() const;

    void setPersistentSearchRemote(bool remote);
    bool persistentSearchRemote() const;

    void setPersistentSearchRecursive(bool recursive);
    bool persistentSearchRecursive() const;

    void setRemovedAttributes(const QSet<QByteArray> &removedAttributes);
    QSet<QByteArray> removedAttributes() const;

    void setAttributes(const Protocol::Attributes &attributes);
    Protocol::Attributes attributes() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void setSyncPref(Tristate sync);
    Tristate syncPref() const;

    void setDisplayPref(Tristate display);
    Tristate displayPref() const;

    void setIndexPref(Tristate index);
    Tristate indexPref() const;

    void setReferenced(bool referenced);
    bool referenced() const;

private:
    AKONADI_DECLARE_PRIVATE(ModifyCollectionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifyCollectionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifyCollectionCommand &command);
};

class AKONADIPRIVATE_EXPORT ModifyCollectionResponse : public Response
{
public:
    explicit ModifyCollectionResponse();
    ModifyCollectionResponse(const Command &command);
};

class MoveCollectionCommandPrivate;
class AKONADIPRIVATE_EXPORT MoveCollectionCommand : public Command
{
public:
    explicit MoveCollectionCommand();
    explicit MoveCollectionCommand(const Scope &col, const Scope &dest);
    MoveCollectionCommand(const Command &command);

    Scope collection() const;
    Scope destination() const;

private:
    AKONADI_DECLARE_PRIVATE(MoveCollectionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::MoveCollectionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::MoveCollectionCommand &command);
};

class AKONADIPRIVATE_EXPORT MoveCollectionResponse : public Response
{
public:
    explicit MoveCollectionResponse();
    MoveCollectionResponse(const Command &command);
};

class SearchCommandPrivate;
class AKONADIPRIVATE_EXPORT SearchCommand : public Command
{
public:
    explicit SearchCommand();
    SearchCommand(const Command &command);

    void setMimeTypes(const QStringList &mimeTypes);
    QStringList mimeTypes() const;

    void setCollections(const QVector<qint64> &collections);
    QVector<qint64> collections() const;

    void setQuery(const QString &query);
    QString query() const;

    void setFetchScope(const FetchScope &fetchScope);
    FetchScope fetchScope() const;

    void setRecursive(bool recursive);
    bool recursive() const;

    void setRemote(bool remote);
    bool remote() const;

private:
    AKONADI_DECLARE_PRIVATE(SearchCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::SearchCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::SearchCommand &command);
};

class AKONADIPRIVATE_EXPORT SearchResponse : public Response
{
public:
    explicit SearchResponse();
    SearchResponse(const Command &command);
};

class SearchResultCommandPrivate;
class AKONADIPRIVATE_EXPORT SearchResultCommand : public Command
{
public:
    explicit SearchResultCommand();
    explicit SearchResultCommand(const QByteArray &searchId, qint64 collectionId, const Scope &result);
    SearchResultCommand(const Command &command);

    QByteArray searchId() const;
    qint64 collectionId() const;
    Scope result() const;

private:
    AKONADI_DECLARE_PRIVATE(SearchResultCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::SearchResultCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::SearchResultCommand &command);
};

class AKONADIPRIVATE_EXPORT SearchResultResponse : public Response
{
public:
    explicit SearchResultResponse();
    SearchResultResponse(const Command &command);
};

class StoreSearchCommandPrivate;
class AKONADIPRIVATE_EXPORT StoreSearchCommand : public Command
{
public:
    explicit StoreSearchCommand();
    StoreSearchCommand(const Command &command);

    void setName(const QString &name);
    QString name() const;

    void setQuery(const QString &query);
    QString query() const;

    void setMimeTypes(const QStringList &mimeTypes);
    QStringList mimeTypes() const;

    void setQueryCollections(const QVector<qint64> &queryCols);
    QVector<qint64> queryCollections() const;

    void setRemote(bool remote);
    bool remote() const;

    void setRecursive(bool recursive);
    bool recursive() const;

private:
    AKONADI_DECLARE_PRIVATE(StoreSearchCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::StoreSearchCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::StoreSearchCommand &command);
};

class AKONADIPRIVATE_EXPORT StoreSearchResponse : public Response
{
public:
    explicit StoreSearchResponse();
    StoreSearchResponse(const Command &command);
};

class CreateTagCommandPrivate;
class AKONADIPRIVATE_EXPORT CreateTagCommand : public Command
{
public:
    explicit CreateTagCommand();
    CreateTagCommand(const Command &command);

    void setGid(const QByteArray &gid);
    QByteArray gid() const;

    void setRemoteId(const QByteArray &remoteId);
    QByteArray remoteId() const;

    void setType(const QByteArray &type);
    QByteArray type() const;

    void setAttributes(const Attributes &attributes);
    Attributes attributes() const;

    void setParentId(qint64 parentId);
    qint64 parentId() const;

    void setMerge(bool merge);
    bool merge() const;

private:
    AKONADI_DECLARE_PRIVATE(CreateTagCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CreateTagCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CreateTagCommand &command);
};

class AKONADIPRIVATE_EXPORT CreateTagResponse : public Response
{
public:
    explicit CreateTagResponse();
    CreateTagResponse(const Command &command);
};

class DeleteTagCommandPrivate;
class AKONADIPRIVATE_EXPORT DeleteTagCommand : public Command
{
public:
    explicit DeleteTagCommand();
    explicit DeleteTagCommand(const Scope &scope);
    DeleteTagCommand(const Command &command);

    Scope tag() const;

private:
    AKONADI_DECLARE_PRIVATE(DeleteTagCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::DeleteTagCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::DeleteTagCommand &command);
};

class AKONADIPRIVATE_EXPORT DeleteTagResponse : public Response
{
public:
    explicit DeleteTagResponse();
    DeleteTagResponse(const Command &command);
};

class ModifyTagCommandPrivate;
class AKONADIPRIVATE_EXPORT ModifyTagCommand : public Command
{
public:
    enum ModifiedPart {
        None            = 0,
        ParentId        = 1 << 0,
        Type            = 1 << 1,
        RemoteId        = 1 << 2,
        RemovedAttributes = 1 << 3,
        Attributes      = 1 << 4
    };
    Q_DECLARE_FLAGS(ModifiedParts, ModifiedPart)

    explicit ModifyTagCommand();
    explicit ModifyTagCommand(qint64 tagId);
    ModifyTagCommand(const Command &command);

    qint64 tagId() const;

    ModifiedParts modifiedParts() const;

    void setParentId(qint64 parentId);
    qint64 parentId() const;

    void setType(const QByteArray &type);
    QByteArray type() const;

    void setRemoteId(const QByteArray &remoteId);
    QByteArray remoteId() const;

    void setRemovedAttributes(const QSet<QByteArray> &removed);
    QSet<QByteArray> removedAttributes() const;

    void setAttributes(const Protocol::Attributes &attrs);
    Protocol::Attributes attributes() const;

private:
    AKONADI_DECLARE_PRIVATE(ModifyTagCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifyTagCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifyTagCommand &command);
};

class AKONADIPRIVATE_EXPORT ModifyTagResponse : public Response
{
public:
    explicit ModifyTagResponse();
    ModifyTagResponse(const Command &command);
};

class ModifyRelationCommandPrivate;
class AKONADIPRIVATE_EXPORT ModifyRelationCommand : public Command
{
public:
    explicit ModifyRelationCommand();
    ModifyRelationCommand(qint64 left, qint64 right, const QByteArray &type,
                          const QByteArray &remoteId = QByteArray());
    ModifyRelationCommand(const Command &command);

    void setLeft(qint64 left);
    qint64 left() const;

    void setRight(qint64 right);
    qint64 right() const;

    void setType(const QByteArray &type);
    QByteArray type() const;

    void setRemoteId(const QByteArray &remoteId);
    QByteArray remoteId() const;

private:
    AKONADI_DECLARE_PRIVATE(ModifyRelationCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifyRelationCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifyRelationCommand &command);
};

class AKONADIPRIVATE_EXPORT ModifyRelationResponse : public Response
{
public:
    explicit ModifyRelationResponse();
    ModifyRelationResponse(const Command &command);
};

class RemoveRelationsCommandPrivate;
class AKONADIPRIVATE_EXPORT RemoveRelationsCommand : public Command
{
public:
    explicit RemoveRelationsCommand();
    RemoveRelationsCommand(qint64 left, qint64 right, const QByteArray &type = QByteArray());
    RemoveRelationsCommand(const Command &command);

    void setLeft(qint64 left);
    qint64 left() const;

    void setRight(qint64 right);
    qint64 right() const;

    void setType(const QByteArray &type);
    QByteArray type() const;

private:
    AKONADI_DECLARE_PRIVATE(RemoveRelationsCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::RemoveRelationsCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::RemoveRelationsCommand &command);
};

class AKONADIPRIVATE_EXPORT RemoveRelationsResponse : public Response
{
public:
    explicit RemoveRelationsResponse();
    RemoveRelationsResponse(const Command &command);
};

class SelectResourceCommandPrivate;
class AKONADIPRIVATE_EXPORT SelectResourceCommand : public Command
{
public:
    explicit SelectResourceCommand();
    explicit SelectResourceCommand(const QString &resourceId);
    SelectResourceCommand(const Command &command);

    QString resourceId() const;

private:
    AKONADI_DECLARE_PRIVATE(SelectResourceCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::SelectResourceCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::SelectResourceCommand &command);
};

class AKONADIPRIVATE_EXPORT SelectResourceResponse : public Response
{
public:
    explicit SelectResourceResponse();
    SelectResourceResponse(const Command &command);
};

class StreamPayloadCommandPrivate;
class AKONADIPRIVATE_EXPORT StreamPayloadCommand : public Command
{
public:
    enum Request : uchar {
        MetaData,
        Data
    };

    explicit StreamPayloadCommand();
    StreamPayloadCommand(const QByteArray &payloadName, Request request,
                         const QString &dest = QString());
    StreamPayloadCommand(const Command &command);

    void setPayloadName(const QByteArray &name);
    QByteArray payloadName() const;

    void setDestination(const QString &dest);
    QString destination() const;

    void setRequest(Request request);
    Request request() const;

private:
    AKONADI_DECLARE_PRIVATE(StreamPayloadCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::StreamPayloadCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::StreamPayloadCommand &command);
};

class StreamPayloadResponsePrivate;
class AKONADIPRIVATE_EXPORT StreamPayloadResponse : public Response
{
public:
    explicit StreamPayloadResponse();
    StreamPayloadResponse(const QByteArray &payloadName,
                          const PartMetaData &metadata = PartMetaData());
    StreamPayloadResponse(const QByteArray &payloadName,
                          const QByteArray &data);
    StreamPayloadResponse(const QByteArray &payloadName,
                          const PartMetaData &metaData,
                          const QByteArray &data);
    StreamPayloadResponse(const Command &command);

    void setPayloadName(const QByteArray &payloadName);
    QByteArray payloadName() const;

    void setMetaData(const PartMetaData &metaData);
    PartMetaData metaData() const;

    void setData(const QByteArray &data);
    QByteArray data() const;

private:
    AKONADI_DECLARE_PRIVATE(StreamPayloadResponse)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::StreamPayloadResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::StreamPayloadResponse &command);
};

class ChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT ChangeNotification : public Command
{
public:
    typedef qint64 Id;
    typedef QVector<ChangeNotification> List;

    ChangeNotification();
    ChangeNotification(const Command &other);

    bool isRemove() const;
    bool isMove() const;

    QByteArray sessionId() const;
    void setSessionId(const QByteArray &sessionId);

    void addMetadata(const QByteArray &metadata);
    void removeMetadata(const QByteArray &metadata);
    QVector<QByteArray> metadata() const;

protected:
    ChangeNotification(ChangeNotificationPrivate *dptr);

private:
    AKONADI_DECLARE_PRIVATE(ChangeNotification)
};

class ItemChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT ItemChangeNotification : public ChangeNotification
{
public:
    typedef QVector<ItemChangeNotification> List;

    enum Operation : uchar {
        InvalidOp,
        Add,
        Modify,
        Move,
        Remove,
        Link,
        Unlink,
        ModifyFlags,
        ModifyTags,
        ModifyRelations
    };

    class Item
    {
    public:
        Item()
            : id(-1)
        {}

        Item(Id id, const QString &remoteId, const QString &remoteRevision, const QString &mimeType)
            : id(id)
            , remoteId(remoteId)
            , remoteRevision(remoteRevision)
            , mimeType(mimeType)
        {}

        bool operator==(const Item &other) const
        {
            return id == other.id
                   && remoteId == other.remoteId
                   && remoteRevision == other.remoteRevision
                   && mimeType == other.mimeType;
        }

        Id id;
        QString remoteId;
        QString remoteRevision;
        QString mimeType;
    };

    class Relation
    {
    public:
        Relation()
            : leftId(-1)
            , rightId(-1)
        {
        }

        Relation(qint64 leftId, qint64 rightId, const QString &type)
            : leftId(leftId)
            , rightId(rightId)
            , type(type)
        {
        }

        bool operator==(const Relation &other) const
        {
            return leftId == other.leftId
                   && rightId == other.rightId
                   && type == other.type;
        }

        qint64 leftId;
        qint64 rightId;
        QString type;
    };

    explicit ItemChangeNotification();
    ItemChangeNotification(const Command &other);

    bool isValid() const;

    Operation operation() const;
    void setOperation(Operation operation);

    void addItem(Id id, const QString &remoteId = QString(), const QString &remoteRevision = QString(), const QString &mimeType = QString());
    void setItem(const QVector<Item> &items);
    QMap<Id, Item> items() const;
    Item item(Id id) const;
    QList<Id> uids() const;
    void clearItems();

    QByteArray resource() const;
    void setResource(const QByteArray &resource);

    Id parentCollection() const;
    void setParentCollection(Id parent);

    Id parentDestCollection() const;
    void setParentDestCollection(Id parent);

    QByteArray destinationResource() const;
    void setDestinationResource(const QByteArray &destResource);

    QSet<QByteArray> itemParts() const;
    void setItemParts(const QSet<QByteArray> &parts);

    QSet<QByteArray> addedFlags() const;
    void setAddedFlags(const QSet<QByteArray> &parts);

    QSet<QByteArray> removedFlags() const;
    void setRemovedFlags(const QSet<QByteArray> &parts);

    QSet<qint64> addedTags() const;
    void setAddedTags(const QSet<qint64> &tags);

    QSet<qint64> removedTags() const;
    void setRemovedTags(const QSet<qint64> &tags);

    QSet<Relation> addedRelations() const;
    void setAddedRelations(const QSet<Relation> &relations);

    QSet<Relation> removedRelations() const;
    void setRemovedRelations(const QSet<Relation> &relations);
private:
    AKONADI_DECLARE_PRIVATE(ItemChangeNotification)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ItemChangeNotification &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ItemChangeNotification &command);
};

inline uint qHash(const ItemChangeNotification::Relation &rel)
{
    return ::qHash(rel.leftId + rel.rightId);
}

class CollectionChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT CollectionChangeNotification : public ChangeNotification
{
public:
    typedef QVector<CollectionChangeNotification> List;

    enum Operation : uchar {
        InvalidOp,
        Add,
        Modify,
        Move,
        Remove,
        Subscribe,
        Unsubscribe
    };

    explicit CollectionChangeNotification();
    CollectionChangeNotification(const Command &other);

    bool isValid() const;

    Operation operation() const;
    void setOperation(Operation operation);

    Id id() const;
    void setId(Id id);

    QString remoteId() const;
    void setRemoteId(const QString &remoteId);

    QString remoteRevision() const;
    void setRemoteRevision(const QString &remoteRevision);

    QByteArray resource() const;
    void setResource(const QByteArray &resource);

    Id parentCollection() const;
    void setParentCollection(Id parent);

    Id parentDestCollection() const;
    void setParentDestCollection(Id parent);

    QByteArray destinationResource() const;
    void setDestinationResource(const QByteArray &destResource);

    QSet<QByteArray> changedParts() const;
    void setChangedParts(const QSet<QByteArray> &parts);

    /**
      Adds a new notification message to the given list and compresses notifications
      where possible.
    */
    static bool appendAndCompress(ChangeNotification::List &list, const CollectionChangeNotification &msg);

private:
    AKONADI_DECLARE_PRIVATE(CollectionChangeNotification)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CollectionChangeNotification &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CollectionChangeNotification &command);
};

class TagChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT TagChangeNotification : public ChangeNotification
{
public:
    typedef QVector<TagChangeNotification> List;

    enum Operation : uchar {
        InvalidOp,
        Add,
        Modify,
        Remove
    };

    explicit TagChangeNotification();
    TagChangeNotification(const Command &other);

    bool isValid() const;

    Operation operation() const;
    void setOperation(Operation operation);

    void setId(Id id);
    Id id() const;

    void setRemoteId(const QString &remoteId);
    QString remoteId() const;

    QByteArray resource() const;
    void setResource(const QByteArray &resource);

private:
    AKONADI_DECLARE_PRIVATE(TagChangeNotification)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::TagChangeNotification &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::TagChangeNotification &command);
};

class RelationChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT RelationChangeNotification : public ChangeNotification
{
public:
    typedef QVector<RelationChangeNotification> List;

    enum Operation : uchar {
        InvalidOp,
        Add,
        Remove
    };

    explicit RelationChangeNotification();
    RelationChangeNotification(const Command &other);

    Operation operation() const;
    void setOperation(Operation operation);

    Id leftItem() const;
    void setLeftItem(Id left);

    Id rightItem() const;
    void setRightItem(Id right);

    QString remoteId() const;
    void setRemoteId(const QString &remoteId);

    QString type() const;
    void setType(const QString &type);

private:
    AKONADI_DECLARE_PRIVATE(RelationChangeNotification)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::RelationChangeNotification &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::RelationChangeNotification &command);
};

class CreateSubscriptionCommandPrivate;
class AKONADIPRIVATE_EXPORT CreateSubscriptionCommand : public Command
{
public:
    explicit CreateSubscriptionCommand();
    explicit CreateSubscriptionCommand(const QByteArray &subscriberName,
                                       const QByteArray &session);
    CreateSubscriptionCommand(const Command &other);

    void setSubscriberName(const QByteArray &name);
    QByteArray subscriberName() const;

    void setSession(const QByteArray &session);
    QByteArray session() const;

private:
    AKONADI_DECLARE_PRIVATE(CreateSubscriptionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::CreateSubscriptionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::CreateSubscriptionCommand &command);
};

class AKONADIPRIVATE_EXPORT CreateSubscriptionResponse : public Response
{
public:
    explicit CreateSubscriptionResponse();
    CreateSubscriptionResponse(const Command &other);
};

class ModifySubscriptionCommandPrivate;
class AKONADIPRIVATE_EXPORT ModifySubscriptionCommand : public Command
{
public:
    enum ModifiedPart {
        None            = 0,
        Collections     = 1 << 0,
        Items           = 1 << 1,
        Tags            = 1 << 2,
        Types           = 1 << 3,
        Resources       = 1 << 4,
        MimeTypes       = 1 << 5,
        AllFlag         = 1 << 6,
        ExclusiveFlag   = 1 << 7,
        Sessions        = 1 << 8,

        Add             = 1 << 14,
        Remove          = 1 << 15
    };
    Q_DECLARE_FLAGS(ModifiedParts, ModifiedPart)

    enum ChangeType {
        NoType          = 0,
        ItemChanges,
        CollectionChanges,
        TagChanges,
        RelationChanges,
        SubscriptionChanges,
        ChangeNotifications,
    };

    explicit ModifySubscriptionCommand();
    ModifySubscriptionCommand(const Command &other);

    void setSubscriberName(const QByteArray &subscriberName);
    QByteArray subscriberName() const;

    ModifiedParts modifiedParts() const;

    void startMonitoringCollection(qint64 id);
    QVector<qint64> startMonitoringCollections() const;

    void stopMonitoringCollection(qint64 id);
    QVector<qint64> stopMonitoringCollections() const;

    void startMonitoringItem(qint64 id);
    QVector<qint64> startMonitoringItems() const;

    void stopMonitoringItem(qint64 id);
    QVector<qint64> stopMonitoringItems() const;

    void startMonitoringTag(qint64 id);
    QVector<qint64> startMonitoringTags() const;

    void stopMonitoringTag(qint64 id);
    QVector<qint64> stopMonitoringTags() const;

    void startMonitoringType(ChangeType type);
    QVector<ChangeType> startMonitoringTypes() const;

    void stopMonitoringType(ChangeType type);
    QVector<ChangeType> stopMonitoringTypes() const;

    void startMonitoringResource(const QByteArray &resource);
    QVector<QByteArray> startMonitoringResources() const;

    void stopMonitoringResource(const QByteArray &resource);
    QVector<QByteArray> stopMonitoringResources() const;

    void startMonitoringMimeType(const QString &mimeType);
    QStringList startMonitoringMimeTypes() const;

    void stopMonitoringMimeType(const QString &mimeType);
    QStringList stopMonitoringMimeTypes() const;

    void setAllMonitored(bool allMonitored);
    bool allMonitored() const;

    void setExclusive(bool exclusive);
    bool isExclusive() const;

    void startIgnoringSession(const QByteArray &sessionId);
    QVector<QByteArray> startIgnoringSessions() const;

    void stopIgnoringSession(const QByteArray &sessionId);
    QVector<QByteArray> stopIgnoringSessions() const;

private:
    AKONADI_DECLARE_PRIVATE(ModifySubscriptionCommand)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifySubscriptionCommand &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifySubscriptionCommand &command);
};

class ModifySubscriptionResponse;
class AKONADIPRIVATE_EXPORT ModifySubscriptionResponse : public Response
{
public:
    explicit ModifySubscriptionResponse();
    ModifySubscriptionResponse(const Command &other);

private:
    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::ModifySubscriptionResponse &command);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::ModifySubscriptionResponse &command);
};

class SubscriptionChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT SubscriptionChangeNotification : public ChangeNotification
{
public:
    enum Operation {
        InvalidOp,
        Add,
        Modify,
        Remove
    };

    explicit SubscriptionChangeNotification();
    SubscriptionChangeNotification(const Command &other);

    Operation operation() const;
    void setOperation(Operation operation);

    QByteArray subscriber() const;
    void setSubscriber(const QByteArray &subscriber);

    QSet<Id> collections() const;
    void setCollections(const QSet<Id> &cols);

    QSet<Id> items() const;
    void setItems(const QSet<Id> &items);

    QSet<Id> tags() const;
    void setTags(const QSet<Id> &tags);

    QSet<ModifySubscriptionCommand::ChangeType> types() const;
    void setTypes(const QSet<ModifySubscriptionCommand::ChangeType> &types);

    QSet<QString> mimeTypes() const;
    void setMimeTypes(const QSet<QString> &mimeTypes);

    QSet<QByteArray> resources() const;
    void setResources(const QSet<QByteArray> &resources);

    QSet<QByteArray> ignoredSessions() const;
    void setIgnoredSessions(const QSet<QByteArray> &ignoredSessions);

    bool isAllMonitored() const;
    void setAllMonitored(bool allMonitored);

    bool isExclusive() const;
    void setExclusive(bool exclusive);

private:
    AKONADI_DECLARE_PRIVATE(SubscriptionChangeNotification)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::SubscriptionChangeNotification &ntf);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::SubscriptionChangeNotification &ntf);
};

class DebugChangeNotificationPrivate;
class AKONADIPRIVATE_EXPORT DebugChangeNotification : public ChangeNotification
{
public:
    explicit DebugChangeNotification();
    DebugChangeNotification(const Command &other);

    ChangeNotification notification() const;
    void setNotification(const ChangeNotification &notification);

    QVector<QByteArray> listeners() const;
    void setListeners(const QVector<QByteArray> &listeners);

    qint64 timestamp() const;
    void setTimestamp(qint64 timestamp);

private:
    AKONADI_DECLARE_PRIVATE(DebugChangeNotification)

    friend DataStream &operator<<(DataStream &stream, const Akonadi::Protocol::DebugChangeNotification &ntf);
    friend DataStream &operator>>(DataStream &stream, Akonadi::Protocol::DebugChangeNotification &ntf);
};

} // namespace Protocol
} // namespace Akonadi

Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::Protocol::FetchScope::FetchFlags)
Q_DECLARE_METATYPE(Akonadi::Protocol::Command::Type)
Q_DECLARE_METATYPE(Akonadi::Protocol::Command)
Q_DECLARE_METATYPE(Akonadi::Protocol::ChangeNotification)
Q_DECLARE_METATYPE(Akonadi::Protocol::ChangeNotification::List)

AKONADIPRIVATE_EXPORT DataStream &operator>>(DataStream &stream, Akonadi::Protocol::Command::Type &type);
AKONADIPRIVATE_EXPORT DataStream &operator<<(DataStream &stream, Akonadi::Protocol::Command::Type type);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, Akonadi::Protocol::Command::Type type);

// Command parameters
#define AKONADI_PARAM_ATR                          "ATR:"
#define AKONADI_PARAM_CACHEPOLICY                  "CACHEPOLICY"
#define AKONADI_PARAM_DISPLAY                      "DISPLAY"
#define AKONADI_PARAM_ENABLED                      "ENABLED"
#define AKONADI_PARAM_FLAGS                        "FLAGS"
#define AKONADI_PARAM_TAGS                         "TAGS"
#define AKONADI_PARAM_GID                          "GID"
#define AKONADI_PARAM_INDEX                        "INDEX"
#define AKONADI_PARAM_MIMETYPE                     "MIMETYPE"
#define AKONADI_PARAM_NAME                         "NAME"
#define AKONADI_PARAM_PARENT                       "PARENT"
#define AKONADI_PARAM_PERSISTENTSEARCH             "PERSISTENTSEARCH"
#define AKONADI_PARAM_PLD                          "PLD:"
#define AKONADI_PARAM_PLD_RFC822                   "PLD:RFC822"
#define AKONADI_PARAM_RECURSIVE                    "RECURSIVE"
#define AKONADI_PARAM_REFERENCED                   "REFERENCED"
#define AKONADI_PARAM_REMOTE                       "REMOTE"
#define AKONADI_PARAM_REMOTEID                     "REMOTEID"
#define AKONADI_PARAM_REMOTEREVISION               "REMOTEREVISION"
#define AKONADI_PARAM_REVISION                     "REV"
#define AKONADI_PARAM_SIZE                         "SIZE"
#define AKONADI_PARAM_SYNC                         "SYNC"
#define AKONADI_PARAM_TAG                          "TAG"
#define AKONADI_PARAM_TYPE                         "TYPE"
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
