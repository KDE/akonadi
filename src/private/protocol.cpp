/*
 *  Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "protocol_p.h"

#include <type_traits>

#include <QDataStream>
#include <QGlobalStatic>
#include <QHash>


// Generic code for effective streaming of enums
template<typename T>
typename std::enable_if<std::is_enum<T>::value, QDataStream>::type
&operator<<(QDataStream &stream, T val)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(val);
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, QDataStream>::type
&operator>>(QDataStream &stream, T &val)
{
    typename std::underlying_type<T>::type tmp;
    stream >> tmp;
    val = static_cast<T>(tmp);
    return stream;
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, QDataStream>::type
&operator>>(QDataStream &stream, QFlags<T> &flags)
{
    typename std::underlying_type<T>::type t;
    stream >> t;
    flags = QFlags<T>(t);
    return stream;
}


namespace Akonadi
{
namespace Protocol
{


class FactoryPrivate
{
public:
    typedef Command (*CommandFactoryFunc)();
    typedef Response (*ResponseFactoryFunc)();

    FactoryPrivate()
    {
        // Session management
        registerType<Command::Hello, HelloResponse, HelloResponse>();
        registerType<Command::Login, LoginCommand, LoginResponse>();
        registerType<Command::Logout, LogoutCommand, LogoutResponse>();

        // Transactions
        registerType<Command::Transaction, TransactionCommand, TransactionResponse>();

        // Items
        registerType<Command::CreateItem, CreateItemCommand, CreateItemResponse>();
        registerType<Command::CopyItems, CopyItemsCommand, CopyItemsResponse>();
        registerType<Command::DeleteItems, DeleteItemsCommand, DeleteItemsResponse>();
        registerType<Command::FetchItems, FetchItemsCommand, FetchItemsResponse>();
        registerType<Command::LinkItems, LinkItemsCommand, LinkItemsResponse>();
        registerType<Command::ModifyItems, ModifyItemsCommand, ModifyItemsResponse>();
        registerType<Command::MoveItems, MoveItemsCommand, MoveItemsResponse>();

        // Collections
        registerType<Command::CreateCollection, CreateCollectionCommand, CreateCollectionResponse>();
        registerType<Command::CopyCollection, CopyCollectionCommand, CopyCollectionResponse>();
        registerType<Command::DeleteCollection, DeleteCollectionCommand, DeleteCollectionResponse>();
        registerType<Command::FetchCollections, FetchCollectionsCommand, FetchCollectionsResponse>();
        registerType<Command::FetchCollectionStats, FetchCollectionStatsCommand, FetchCollectionStatsResponse>();
        registerType<Command::ModifyCollection, ModifyCollectionCommand, ModifyCollectionResponse>();
        registerType<Command::MoveCollection, MoveCollectionCommand, MoveCollectionResponse>();
        registerType<Command::SelectCollection, SelectCollectionCommand, SelectCollectionResponse>();

        // Search
        registerType<Command::Search, SearchCommand, SearchResponse>();
        registerType<Command::SearchResult, SearchResultCommand, SearchResultResponse>();
        registerType<Command::StoreSearch, StoreSearchCommand, StoreSearchResponse>();

        // Tag
        registerType<Command::CreateTag, CreateTagCommand, CreateTagResponse>();
        registerType<Command::DeleteTag, DeleteTagCommand, DeleteTagResponse>();
        registerType<Command::FetchTags, FetchTagsCommand, FetchTagsResponse>();
        registerType<Command::ModifyTag, ModifyTagCommand, ModifyTagResponse>();

        // Relation
        registerType<Command::FetchRelations, FetchRelationsCommand, FetchRelationsResponse>();
        registerType<Command::ModifyRelation, ModifyRelationCommand, ModifyRelationResponse>();
        registerType<Command::RemoveRelations, RemoveRelationsCommand, RemoveRelationsResponse>();

        // Resources
        registerType<Command::SelectResource, SelectResourceCommand, SelectResourceResponse>();

        // Other...?
        registerType<Command::StreamPayload, StreamPayloadCommand, StreamPayloadResponse>();
    }

    QHash<Command::Type, QPair<CommandFactoryFunc, ResponseFactoryFunc>> registrar;

private:
    template<typename T>
    static Command commandFactoryFunc()
    {
        return T();
    }
    template<typename T>
    static Response responseFactoryFunc()
    {
        return T();
    }

    template<Command::Type T,typename CmdType, typename RespType>
    void registerType() {
        CommandFactoryFunc cmdFunc = &commandFactoryFunc<CmdType>;
        ResponseFactoryFunc respFunc = &responseFactoryFunc<RespType>;
        registrar.insert(T, qMakePair<CommandFactoryFunc, ResponseFactoryFunc>(cmdFunc, respFunc));
    }
};

Q_GLOBAL_STATIC(FactoryPrivate, sFactoryPrivate)

Command Factory::command(Command::Type type)
{
    auto iter = sFactoryPrivate->registrar.constFind(type);
    if (iter == sFactoryPrivate->registrar.constEnd()) {
        Q_ASSERT_X(iter != sFactoryPrivate->registrar.constEnd(),
                    "Aknadi::Protocol::Factory::command()", "Invalid command");
    }
    return iter.value().first();
}

Response Factory::response(Command::Type type)
{
    auto iter = sFactoryPrivate->registrar.constFind(type);
    if (iter == sFactoryPrivate->registrar.constEnd()) {
        Q_ASSERT_X(iter != sFactoryPrivate->registrar.constEnd(),
                    "Akonadi::Protocol::Factory::response()", "Invalid response");
    }
    return iter.value().second();
}




/******************************************************************************/



class CommandPrivate : public QSharedData
{
public:
    CommandPrivate(Command::Type type)
        : commandType(type)
    {}


    Command::Type commandType;
};

Command::Command(CommandPrivate *dd)
    : d_ptr(dd)
{
}

Command& Command::Command(Command &&other)
{
    d_ptr.swap(other.d_ptr);
    return *this;
}

Command& Command::Command(const Command &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

Command::~Command()
{
}

Command& Command::operator=(Command &&other)
{
    d_ptr.swap(other.d_ptr);
    return *this;
}

Command& Command::operator=(const Command &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

Command::Type Command::type() const
{
    Q_D(Command);
    return d->commandType;
}

bool Command::isValid() const
{
    Q_D(Command);
    return d->commandType != Invalid;
}

QDataStream &operator<<(QDataStream &stream, Command::Type type)
{
    return stream << static_cast<typename std::underlying_type<Command::Type>::type>(type);
}

QDataStream &operator>>(QDataStream &stream, Command::Type &type)
{
    typename std::underlying_type<Command::Type>::type tmp;
    stream >> tmp;
    type = static_cast<Command::Type>(tmp);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Command &command)
{
    return stream << command.d_func()->commandType;
}

QDataStream &operator>>(QDataStream &stream, Command &command)
{
    return stream >> command.d_func()->commandType;
}



/******************************************************************************/



class FetchScopePrivate : public QSharedData
{
public:
    FetchScopePrivate()
        : fetchFlags(FetchScope::None)
    {}

    QVector<QByteArray> requestedParts;
    QStringList requestedPayloads;
    QDateTime changedSince;
    QVector<QByteArray> tagFetchScope;
    int ancestorDepth;
    FetchScope::FetchFlags fetchFlags;
};

FetchScope::FetchScope()
    : d(new FetchScopePrivate)
{
}

FetchScope::FetchScope(FetchScope &&other)
{
    d.swap(other.d);
}

FetchScope::FetchScope(const FetchScope &other)
{
    d = other.d;
}

FetchScope::~FetchScope()
{
}

FetchScope &FetchScope::operator=(FetchScope &&other)
{
    d.swap(other.d);
    return *this;
}

FetchScope &FetchScope::operator=(const FetchScope &other)
{
    d = other.d;
    return *this;
}

void FetchScope::setRequestedParts(const QVector<QByteArray> &requestedParts)
{
    d->requestedParts = requestedParts;
}

QVector<QByteArray> FetchScope::requestedParts() const
{
    return d->requestedParts;
}

void FetchScope::setRequestedPayloads(const QStringList &requestedPayloads)
{
    d->requestedPayloads = requestedPayloads;
}

QStringList FetchScope::requestedPayloads() const
{
    return d->requestedPayloads;
}

void FetchScope::setChangedSince(const QDateTime &changedSince)
{
    d->changedSince = changedSince;
}

QDateTime FetchScope::changedSince() const
{
    return d->changedSince;
}

void FetchScope::setTagFetchScope(const QVector<QByteArray> &tagFetchScope)
{
    d->tagFetchScope = tagFetchScope;
}

QVector<QByteArray> FetchScope::tagFetchScope() const
{
    return d->tagFetchScope;
}

void FetchScope::setAncestorDepth(int depth)
{
    d->ancestorDepth = depth;
}

int FetchScope::ancestorDepth() const
{
    return d->ancestorDepth;
}

bool FetchScope::cacheOnly() const
{
    return d->fetchFlags & CacheOnly;
}

bool FetchScope::checkCachedPayloadPartsOnly() const
{
    return d->fetchFlags & CheckCachedPayloadPartsOnly;
}
bool FetchScope::fullPayload() const
{
    return d->fetchFlags & FullPayload;
}
bool FetchScope::allAttributes() const
{
    return d->fetchFlags & AllAttributes;
}
bool FetchScope::fetchSize() const
{
    return d->fetchFlags & Size;
}
bool FetchScope::fetchMTime() const
{
    return d->fetchFlags & MTime;
}
bool FetchScope::fetchRemoteRevision() const
{
    return d->fetchFlags & RemoteRevision;
}
bool FetchScope::ignoreErrors() const
{
    return d->fetchFlags & IgnoreErrors;
}
bool FetchScope::fetchFlags() const
{
    return d->fetchFlags & Flags;
}
bool FetchScope::fetchRemoteId() const
{
    return d->fetchFlags & RemoteID;
}
bool FetchScope::fetchGID() const
{
    return d->fetchFlags & GID;
}
bool FetchScope::fetchTags() const
{
    return d->fetchFlags & Tags;
}
bool FetchScope::fetchRelations() const
{
    return d->fetchFlags & Relations;
}
bool FetchScope::fetchVirtualReferences() const
{
    return d->fetchFlags & VirtReferences;
}

void FetchScope::setFetch(FetchFlags attributes, bool fetch)
{
    if (fetch) {
        d->fetchFlags |= attributes;
    } else {
        d->fetchFlags &= ~attributes;
    }
}

bool FetchScope::fetch(FetchFlags flags)
{
    return d->fetchFlags & flags;
}

QDataStream &operator<<(QDataStream &stream, const FetchScope &scope)
{
    return stream << scope.d->requestedParts
                  << scope.d->requestedPayloads
                  << scope.d->changedSince
                  << scope.d->tagFetchScope
                  << scope.d->ancestorDepth
                  << scope.d->fetchFlags;
}

QDataStream &operator>>(QDataStream &stream, FetchScope &scope)
{
    return stream >> scope.d->requestedParts
                  >> scope.d->requestedPayloads
                  >> scope.d->changedSince
                  >> scope.d->tagFetchScope
                  >> scope.d->ancestorDepth
                  >> scope.d->fetchFlags;
}



/******************************************************************************/




class PartMetaDataPrivate : public QSharedData
{
public:
    PartMetaDataPrivate()
        : size(0)
        , version(0)
    {}

    QByteArray name;
    qint64 size;
    int version;
};

PartMetaData::PartMetaData()
    : d(new PartMetaDataPrivate)
{
}

PartMetaData::PartMetaData(PartMetaData &&other)
{
    d.swap(other.d);
}

PartMetaData::PartMetaData(const PartMetaData &other)
{
    d = other.d;
}

PartMetaData::~PartMetaData()
{
}

PartMetaData &PartMetaData::operator=(PartMetaData &&other)
{
    d.swap(other.d);
    return *this;
}

PartMetaData &PartMetaData::operator=(const PartMetaData &other)
{
    d = other.d;
    return *this;
}

bool PartMetaData::operator<(const PartMetaData &other)
{
    return d->name < other.d->name;
}

void PartMetaData::setName(const QByteArray &name)
{
    d->name = name;
}
QByteArray PartMetaData::name() const
{
    return d->name;
}

void PartMetaData::setSize(qint64 size)
{
    d->size = size;
}
qint64 PartMetaData::size() const
{
    return d->size;
}

void PartMetaData::setVersion(int version)
{
    d->version = version;
}
int PartMetaData::version() const
{
    return d->version;
}

QDataStream &operator<<(QDataStream &stream, const PartMetaData &part)
{
    return stream << part.d->name
                  << part.d->size
                  << part.d->version;
}

QDataStream &operator>>(QDataStream &stream, PartMetaData &part)
{
    return stream >> part.d->name
                  >> part.d->size
                  >> part.d->version;
}



/******************************************************************************/




class CachePolicyPrivate : public QSharedData
{
public:
    CachePolicyPrivate()
        : interval(-1)
        , cacheTimeout(-1)
        , syncOnDemand(false)
        , inherit(true)
    {}

    QStringList localParts;
    int interval;
    int cacheTimeout;
    bool syncOnDemand;
    bool inherit;
};

CachePolicy::CachePolicy()
    : d(new CachePolicyPrivate)
{
}

CachePolicy::CachePolicy(CachePolicy &&other)
{
    d.swap(other.d);
}

CachePolicy::CachePolicy(const CachePolicy &other)
{
    d = other.d;
}

CachePolicy &CachePolicy::operator=(CachePolicy &&other)
{
    d.swap(other.d);
    return *this;
}

CachePolicy &CachePolicy::operator=(const CachePolicy &other)
{
    d = other.d;
    return *this;
}

void CachePolicy::setInherit(bool inherit)
{
    d->inherit = inherit;
}
bool CachePolicy::inherit() const
{
    return d->inherit;
}

void CachePolicy::setCheckInterval(int interval)
{
    d->interval = interval;
}
int CachePolicy::checkInterval() const
{
    return d->interval;
}

void CachePolicy::setCacheTimeout(int timeout)
{
    d->cacheTimeout = timeout;
}
int CachePolicy::cacheTimeout() const
{
    return d->cacheTimeout;
}

void CachePolicy::setSyncOnDemand(bool onDemand)
{
    d->syncOnDemand = onDemand;
}
bool CachePolicy::syncOnDemand() const
{
    return d->syncOnDemand;
}

void CachePolicy::setLocalParts(const QStringList &localParts)
{
    d->localParts = localParts;
}
QStringList CachePolicy::localParts() const
{
    return d->localParts;
}


QDataStream &operator<<(QDataStream &stream, const CachePolicy &policy)
{
    return stream << policy.d->inherit
                  << policy.d->interval
                  << policy.d->cacheTimeout
                  << policy.d->syncOnDemand
                  << policy.d->localParts;
}

QDataStream &operator>>(QDataStream &stream, CachePolicy &policy)
{
    return stream >> policy.d->inherit
                  >> policy.d->interval
                  >> policy.d->cacheTimeout
                  >> policy.d->syncOnDemand
                  >> policy.d->localParts;
}



/******************************************************************************/




class AncestorPrivate : public QSharedData
{
public:
    AncestorPrivate()
        : id(-1)
    {}
    AncestorPrivate(qint64 id)
        : id(id)
    {}

    qint64 id;
    QString remoteId;
    QMap<QByteArrayDataPtr, QByteArray> attrs;
};

Ancestor::Ancestor()
    : d(new AncestorPrivate)
{
}

Ancestor::Ancestor(qint64 id)
    : d(new AncestorPrivate(id)
{
}

Ancestor::Ancestor(Ancestor &&other)
{
    d.swap(other.d);
}

Ancestor::Ancestor(const Ancestor &other)
{
    d = other.d;
}

Ancestor::~Ancestor()
{
}

Ancestor &Ancestor::operator=(Ancestor &&other)
{
    d.swap(other.d);
    return *this;
}

Ancestor &Ancestor::operator=(const Ancestor &other)
{
    d = other.d;
    return *this;
}

void Ancestor::setId(qint64 id)
{
    d->id = id;
}
qint64 Ancestor::id() const
{
    return d->id;
}

void Ancestor::setRemoteId(const QString &remoteId)
{
    d->remoteId = remoteId;
}
QString Ancestor::remoteId() const
{
    return d->remoteId;
}

void Ancestor::setAttributes(const QMap<QByteArray, QByteArray> &attributes)
{
    d->attrs = attributes;
}
QMap<QByteArray, QByteArray> Ancestor::attributes() const
{
    return d->attrs;
}


QDataStream &operator<<(QDataStream &stream, const Ancestor &ancestor)
{
    return stream << ancestor.d->id
                  << ancestor.d->remoteId
                  << ancestor.d->attrs;
}

QDataStream &operator>>(QDataStream &stream, Ancestor &ancestor)
{
    return stream >> ancestor.d->id
                  >> ancestor.d->remoteId
                  >> ancestor.d->attrs;
}




/******************************************************************************/





class ResponsePrivate : public CommandPrivate
{
public:
    ResponsePrivate(Command::Type type)
        : CommandPrivate(type)
        , errorCode(0)
    {
    }

    QString errorMsg;
    int errorCode;
};

Response::Response(ResponsePrivate *dd)
    : Command(dd)
{
}

void Response::setError(int code, const QString &message)
{
    Q_D(Response);
    d->errorCode = code;
    d->errorMsg = message;
}

bool Response::isError() const
{
    return d_func()->errorCode;
}

int Response::errorCode() const
{
    return d_func()->errorCode;
}

QString Response::errorMessage() const
{
    return d_func()->errorMsg;
}

QDataStream &operator<<(QDataStream &stream, const Response &command)
{
    return stream << command.d_func()->errorCode
                  << command.d_func()->errorMsg;
}

QDataStream &operator>>(QDataStream &stream, Response &command)
{
    return stream >> command.d_func()->errorCode
                  >> command.d_func()->errorMsg;
}




/******************************************************************************/





class HelloResponsePrivate : public ResponsePrivate
{
public:
    HelloResponsePrivate()
        : ResponsePrivate(Command::Hello)
        , protocol(0)
    {}
    HelloResponsePrivate(const QString &server, const QString &message, int protocol)
        : ResponsePrivate(Command::Hello)
        , server(server)
        , message(message)
        , protocol(protocol)
    {}

    QString server;
    QString message;
    int protocol;
};

HelloResponse::HelloResponse(const QString &server, const QString &message, int protocol)
    : Response(new HelloResponsePrivate(server, message, protocol))
{
}

HelloResponse::HelloResponse()
    : Response(new HelloResponsePrivate)
{
}

HelloResponse::~HelloResponse()
{
}

QString HelloResponse::serverName() const
{
    return d_func()->server;
}

QString HelloResponse::message() const
{
    return d_func()->message;
}

int HelloResponse::protocolVersion() const
{
    return d_func()->protocol;
}

QDataStream &operator<<(QDataStream &stream, const HelloResponse &command)
{
    return stream << command.d_func()->server
                  << command.d_func()->message
                  << command.d_func()->protocol;
}

QDataStream &operator>>(QDataStream &stream, HelloResponse &command)
{
    return stream >> command.d_func()->server
                  >> command.d_func()->message
                  >> command.d_func()->protocol;
}




/******************************************************************************/




class LoginCommandPrivate : public CommandPrivate
{
public:
    LoginCommandPrivate()
        : CommandPrivate(Command::Login)
    {}
    LoginCommandPrivate(const QByteArray &sessionId)
        : CommandPrivate(Command::Login)
        , sessionId(sessionId)
    {}

    QByteArray sessionId;
};

LoginCommand::LoginCommand()
    : Command(new LoginCommandPrivate)
{
}

LoginCommand::LoginCommand(const QByteArray &sessionId)
    : Command(new LoginCommandPrivate(sessionId))
{
}

QByteArray LoginCommand::sessionId() const
{
    return d_func()->sessionId;
}

QDataStream &operator<<(QDataStream &stream, const LoginCommand &command)
{
    return stream << command.d_func()->sessionId;
}

QDataStream &operator>>(QDataStream &stream, LoginCommand &command)
{
    return stream >> command.d_func()->sessionId;
}




/******************************************************************************/




LoginResponse::LoginResponse()
    : Response(new ResponsePrivate(Command::Login))
{
}




/******************************************************************************/




LogoutCommand::LogoutCommand()
    : Command(new CommandPrivate(Command::Logout))
{
}



/******************************************************************************/



LogoutResponse::LogoutResponse()
    : Response(new ResponsePrivate(Command::Logout))
{
}




/******************************************************************************/

class TransactionCommandPrivate : public CommandPrivate
{
    TransactionCommandPrivate(TransactionCommand::Mode mode = TransactionCommand::Invalid)
        : CommandPrivate(Command::Transaction)
        , mode(mode)
    {}

    TransactionCommand::Mode mode;
};

TransactionCommand::TransactionCommand()
    : Command(new TransactionCommandPrivate)
{
}

TransactionCommand::TransactionCommand(TransactionCommand::Mode mode)
    : Command(new TransactionCommandPrivate(mode)
{
}

TransactionCommand::Mode TransactionCommand::mode() const
{
    return d_func()->mode;
}

QDataStream &operator<<(QDataStream &stream, const TransactionCommand &command)
{
    return stream << command.d_func()->mode;
}

QDataStream &operator>>(QDataStream &stream, TransactionCommand &command)
{
    return stream >> command.d_func()->mode;
}



/******************************************************************************/




TransactionResponse::TransactionResponse()
    : Response(new ResponsePrivate(Command::Transaction))
{
}



/******************************************************************************/





class CreateItemCommandPrivate : public CommandPrivate
{
public:
    CreateItemCommandPrivate()
        : CommandPrivate(Command::CreateItem)
        , mergeMode(CreateItemCommand::None)
        , itemSize(0)
    {}

    Scope collection;
    QString mimeType;
    QString gid;
    QString remoteId;
    QString remoteRev;
    QDateTime dateTime;
    Scope tags;
    Scope addedTags;
    Scope removedTags;
    QVector<QByteArray> flags;
    QVector<QByteArray> addedFlags;
    QVector<QByteArray> removedFlags;
    QVector<QByteArray> removedParts;
    QVector<PartMetaData> parts;
    CreateItemCommand::MergeModes mergeMode;
    qint64 itemSize;
};

CreateItemCommand::CreateItemCommand()
    : Command(new CreateItemCommandPrivate)
{
}

void CreateItemCommand::setMergeMode(MergeModes mode)
{
    d_func()->mergeMode = mode;
}
CreateItemCommand::MergeModes CreateItemCommand::mergeModes() const
{
    return d_func()->mergeMode;
}

void CreateItemCommand::setCollection(const Scope &collection)
{
    d_func()->collection = collection;
}
Scope CreateItemCommand::collection() const
{
    return d_func()->collection;
}

void CreateItemCommand::setItemSize(qint64 size)
{
    d_func()->itemSize = size;
}
qint64 CreateItemCommand::itemSize() const
{
    return d_func()->itemSize;
}

void CreateItemCommand::setMimeType(const QString &mimeType)
{
    d_func()->mimeType = mimeType;
}
QString CreateItemCommand::mimeType() const
{
    return d_func()->mimeType;
}

void CreateItemCommand::setGID(const QString &gid)
{
    d_func()->gid = gid;
}
QString CreateItemCommand::gid() const
{
    return d_func()->gid;
}

void CreateItemCommand::setRemoteId(const QString &remoteId)
{
    d_func()->remoteId = remoteId;
}
QString CreateItemCommand::remoteId() const
{
    return d_func()->remoteId;
}

void CreateItemCommand::setRemoteRevision(const QString &remoteRevision)
{
    d_func()->remoteRev = remoteRevision;
}

QString CreateItemCommand::remoteRevision() const
{
    return d_func()->remoteRev;
}

void CreateItemCommand::setDateTime(const QDateTime &dateTime)
{
    d_func()->dateTime = dateTime;
}
QDateTime CreateItemCommand::dateTime() const
{
    return d_func()->dateTime;
}

void CreateItemCommand::setFlags(const QVector<QByteArray> &flags)
{
    d_func()->flags = flags;
}
QVector<QByteArray> CreateItemCommand::flags() const
{
    return d_func()->flags;
}
void CreateItemCommand::setAddedFlags(const QVector<QByteArray> &flags)
{
    d_func()->addedFlags = flags;
}
QVector<QByteArray> CreateItemCommand::addedFlags() const
{
    return d_func()->addedFlags;
}
void CreateItemCommand::setRemovedFlags(const QVector<QByteArray> &flags)
{
    d_func()->removedFlags = flags;
}
QVector<QByteArray> CreateItemCommand::removedFlags() const
{
    return d_func()->removedFlags;
}

void CreateItemCommand::setTags(const Scope &tags)
{
    d_func()->tags = tags;
}
Scope CreateItemCommand::tags() const
{
    return d_func()->tags;
}
void CreateItemCommand::setAddedTags(const Scope &tags)
{
    d_func()->addedTags = tags;
}
Scope CreateItemCommand::addedTags() const
{
    return d_func()->addedTags;
}
void CreateItemCommand::setRemovedTags(const Scope &tags)
{
    d_func()->removedTags = tags;
}
Scope CreateItemCommand::removedTags() const
{
    return d_func()->removedTags;
}

void CreateItemCommand::setRemovedParts(const QVector<QByteArray> &removedParts)
{
    d_func()->removedParts = removedParts;
}
QVector<QByteArray> CreateItemCommand::removedParts() const
{
    return d_func()->removedParts;
}
void CreateItemCommand::setParts(const QVector<PartMetaData> &parts)
{
    d_func()->parts = parts;
}
QVector<PartMetaData> CreateItemCommand::parts() const
{
    return d_func()->parts;
}

QDataStream &operator<<(QDataStream &stream, const CreateItemCommand &command)
{
    return stream << command.d_func()->mergeMode
                  << command.d_func()->collection
                  << command.d_func()->itemSize
                  << command.d_func()->mimeType
                  << command.d_func()->gid
                  << command.d_func()->remoteId
                  << command.d_func()->remoteRev
                  << command.d_func()->dateTime
                  << command.d_func()->flags
                  << command.d_func()->addedFlags
                  << command.d_func()->removedFlags
                  << command.d_func()->tags
                  << command.d_func()->addedTags
                  << command.d_func()->removedTags
                  << command.d_func()->removedParts
                  << command.d_func()->parts;
}

QDataStream &operator>>(QDataStream &stream, CreateItemCommand &command)
{
    return stream >> command.d_func()->mergeMode
                  >> command.d_func()->collection
                  >> command.d_func()->itemSize
                  >> command.d_func()->mimeType
                  >> command.d_func()->gid
                  >> command.d_func()->remoteId
                  >> command.d_func()->remoteRev
                  >> command.d_func()->dateTime
                  >> command.d_func()->flags
                  >> command.d_func()->addedFlags
                  >> command.d_func()->removedFlags
                  >> command.d_func()->tags
                  >> command.d_func()->addedTags
                  >> command.d_func()->removedTags
                  >> command.d_func()->removedParts
                  >> command.d_func()->parts;
}




/******************************************************************************/




CreateItemResponse::CreateItemResponse()
    : Response(new ResponsePrivate(Command::CreateItem))
{
}




/******************************************************************************/




class CopyItemsCommandPrivate : public CommandPrivate
{
public:
    CopyItemsCommandPrivate()
        : CommandPrivate(Command::CopyItems)
    {}
    CopyItemsCommandPrivate(const Scope &items, const Scope &dest)
        : CommandPrivate(Command::CopyItems)
        , items(items)
        , dest(dest)
    {}

    Scope items;
    Scope dest;
};

CopyItemsCommand::CopyItemsCommand()
    : Command(new CopyItemsCommandPrivate)
{
}

CopyItemsCommand::CopyItemsCommand(const Scope &items, const Scope &dest)
    : Command(new CopyItemsCommandPrivate(items, dest))
{
}

Scope CopyItemsCommand::items() const
{
    return d_func()->items;
}

Scope CopyItemsCommand::destination() const
{
    return d_func()->dest;
}

QDataStream &operator<<(QDataStream &stream, const CopyItemsCommand &command)
{
    return stream << command.d_func()->items
                  << command.d_func()->dest;
}

QDataStream &operator>>(QDataStream &stream, CopyItemsCommand &command)
{
    return stream >> command.d_func()->items
                  >> command.d_func()->dest;
}




/******************************************************************************/




CopyItemsResponse::CopyItemsResponse()
    : Response(new ResponsePrivate(Command::CopyItems))
{
}



/******************************************************************************/




class DeleteItemsCommandPrivate : public CommandPrivate
{
public:
    DeleteItemsCommandPrivate()
        : CommandPrivate(Command::DeleteItems)
    {}
    DeleteItemsCommandPrivate(const Scope &items)
        : CommandPrivate(Command::DeleteItems)
        , items(items)
    {}

    Scope items;
};

DeleteItemsCommand::DeleteItemsCommand()
    : Command(new DeleteItemsCommandPrivate)
{
}

DeleteItemsCommand::DeleteItemsCommand(const Scope &items)
    : Command(new DeleteItemsCommandPrivate(items))
{
}

Scope DeleteItemsCommand::items() const
{
    return d_func()->items;
}

QDataStream &operator<<(QDataStream &stream, const DeleteItemsCommand &command)
{
    return stream << command.d_func()->items;
}

QDataStream &operator>>(QDataStream &stream, DeleteItemsCommand &command)
{
    return stream >> command.d_func()->items;
}




/******************************************************************************/




DeleteItemsResponse::DeleteItemsResponse()
    : Response(new ResponsePrivate(Command::DeleteItems))
{
}




/******************************************************************************/





class FetchRelationsCommandPrivate : public CommandPrivate
{
public:
    FetchRelationsCommandPrivate()
        : CommandPrivate(Command::FetchRelations)
        , left(-1)
        , right(-1)
        , side(-1)
    {}

    qint64 left;
    qint64 right;
    qint64 side;
    QString type;
    QString resource;
};

FetchRelationsCommand::FetchRelationsCommand()
    : Command(new FetchRelationsCommandPrivate)
{
}

void FetchRelationsCommand::setLeft(qint64 left)
{
    d_func()->left = left;
}
qint64 FetchRelationsCommand::left() const
{
    return d_func()->left;
}

void FetchRelationsCommand::setRight(qint64 right)
{
    d_func()->right = right;
}
qint64 FetchRelationsCommand::right() const
{
    return d_func()->right;
}

void FetchRelationsCommand::setSide(qint64 side)
{
    d_func()->side = side;
}
qint64 FetchRelationsCommand::side() const
{
    return d_func()->side;
}

void FetchRelationsCommand::setType(const QString &type)
{
    d_func()->type = type;
}
QString FetchRelationsCommand::type() const
{
    return d_func()->type;
}

void FetchRelationsCommand::setResource(const QString &resource)
{
    d_func()->resource = resource;
}
QString FetchRelationsCommand::resource() const
{
    return d_func()->resource;
}

QDataStream &operator<<(QDataStream &stream, const FetchRelationsCommand &command)
{
    return stream << command.d_func()->left
                  << command.d_func()->right
                  << command.d_func()->side
                  << command.d_func()->type
                  << command.d_func()->resource;
}

QDataStream &operator>>(QDataStream &stream, FetchRelationsCommand &command)
{
    return stream >> command.d_func()->left
                  >> command.d_func()->right
                  >> command.d_func()->side
                  >> command.d_func()->type
                  >> command.d_func()->resource;
}




/*****************************************************************************/




class FetchRelationsResponsePrivate : public ResponsePrivate
{
public:
    FetchRelationsResponsePrivate(qint64 left = -1, qint64 right = -1, const QString &type = QString())
        : ResponsePrivate(Command::FetchRelations)
        , left(left)
        , right(right)
        , type(type)
    {}

    qint64 left;
    qint64 right;
    QString type;
    QString remoteId;
};

FetchRelationsResponse::FetchRelationsResponse()
    : Response(new FetchRelationsResponsePrivate)
{
}

FetchRelationsResponse::FetchRelationsResponse(qint64 left, qint64 right, const QString &type)
    : Response(new FetchRelationsResponsePrivate(left, right, type))
{
}

qint64 FetchRelationsResponse::left() const
{
    return d_func()->left;
}
qint64 FetchRelationsResponse::right() const
{
    return d_func()->right;
}
QString FetchRelationsResponse::type() const
{
    return d_func()->type;
}
void FetchRelationsResponse::setRemoteId(const QString &remoteId)
{
    d_func()->remoteId = remoteId;
}
QString FetchRelationsResponse::remoteId() const
{
    return d_func()->remoteId;
}

QDataStream &operator<<(QDataStream &stream, const FetchRelationsResponse &command)
{
    return stream << command.d_func()->left
                  << command.d_func()->right
                  << command.d_func()->type
                  << command.d_func()->remoteId;
}

QDataStream &operator>>(QDataStream &stream, FetchRelationsResponse &command)
{
    return stream >> command.d_func()->left
                  >> command.d_func()->right
                  >> command.d_func()->type
                  >> command.d_func()->remoteId;
}




/******************************************************************************/



QDataStream &operator<<(QDataStream &stream, const FetchItemsCommand &command)
{
    return stream << command.mScope
                  << command.mFetchScope;
}

QDataStream &operator>>(QDataStream &stream, FetchItemsCommand &command)
{
    return stream >> command.mScope
                  >> command.mFetchScope;
}


QDataStream &operator<<(QDataStream &stream, const FetchItemsResponse &command)
{
    return stream << command.mId
                  << command.mRevision
                  << command.mCollectionId
                  << command.mRemoteId
                  << command.mRemoteRev
                  << command.mGid
                  << command.mSize
                  << command.mMimeType
                  << command.mTime
                  << command.mFlags
                  << command.mTags
                  << command.mVirtRefs
                  << command.mRelations
                  << command.mAncestors
                  << command.mParts
                  << command.mCachedParts;
}

QDataStream &operator>>(QDataStream &stream, FetchItemsResponse &command)
{
    return stream >> command.mId
                  >> command.mRevision
                  >> command.mCollectionId
                  >> command.mRemoteId
                  >> command.mRemoteRev
                  >> command.mGid
                  >> command.mSize
                  >> command.mMimeType
                  >> command.mTime
                  >> command.mFlags
                  >> command.mTags
                  >> command.mVirtRefs
                  >> command.mRelations
                  >> command.mAncestors
                  >> command.mParts
                  >> command.mCachedParts;
}


QDataStream &operator<<(QDataStream &stream, const LinkItemsCommand &command)
{
    return stream << command.mAction
                  << command.mScope
                  << command.mDest;
}

QDataStream &operator>>(QDataStream &stream, LinkItemsCommand &command)
{
    return stream >> command.mAction
                  >> command.mScope
                  >> command.mDest;
}


QDataStream &operator<<(QDataStream &stream, const ModifyItemsCommand &command)
{
    stream << command.mScope
           << command.mOldRevision
           << command.mModifiedParts
           << command.mDirty
           << command.mInvalidate
           << command.mNoResponse
           << command.mNotify;

    if (command.mModifiedParts & ModifyItemsCommand::Flags) {
        stream << command.mFlags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::AddedFlags) {
        stream << command.mAddedFlags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemovedFlags) {
        stream << command.mRemovedFlags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::Tags) {
        stream << command.mTags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::AddedTags) {
        stream << command.mAddedTags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemovedTags) {
        stream << command.mRemovedTags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemoteID) {
        stream << command.mRemoteId;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemoteRevision) {
        stream << command.mRemoteRev;
    }
    if (command.mModifiedParts & ModifyItemsCommand::GID) {
        stream << command.mGid;
    }
    if (command.mModifiedParts & ModifyItemsCommand::Size) {
        stream << command.mSize;
    }
    if (command.mModifiedParts & ModifyItemsCommand::Parts) {
        stream << command.mParts;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemovedParts) {
        stream << command.mRemovedParts;
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, ModifyItemsCommand &command)
{
    stream >> command.mScope
           >> command.mOldRevision
           >> command.mModifiedParts
           >> command.mDirty
           >> command.mInvalidate
           >> command.mNoResponse
           >> command.mNotify;

    if (command.mModifiedParts & ModifyItemsCommand::Flags) {
        stream >> command.mFlags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::AddedFlags) {
        stream >> command.mAddedFlags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemovedFlags) {
        stream >> command.mRemovedFlags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::Tags) {
        stream >> command.mTags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::AddedTags) {
        stream >> command.mAddedTags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemovedTags) {
        stream >> command.mRemovedTags;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemoteID) {
        stream >> command.mRemoteId;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemoteRevision) {
        stream >> command.mRemoteRev;
    }
    if (command.mModifiedParts & ModifyItemsCommand::GID) {
        stream >> command.mGid;
    }
    if (command.mModifiedParts & ModifyItemsCommand::Size) {
        stream >> command.mSize;
    }
    if (command.mModifiedParts & ModifyItemsCommand::Parts) {
        stream >> command.mParts;
    }
    if (command.mModifiedParts & ModifyItemsCommand::RemovedParts) {
        stream >> command.mRemovedParts;
    }
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const ModifyItemsResponse &command)
{
    return stream << command.mId
                  << command.mNewRevision;
}

QDataStream &operator>>(QDataStream &stream, ModifyItemsResponse &command)
{
    return stream >> command.mId
                  >> command.mNewRevision;
}

QDataStream &operator<<(QDataStream &stream, const MoveItemsCommand &command)
{
    return stream << command.mScope
                  << command.mDest;
}

QDataStream &operator>>(QDataStream &stream, MoveItemsCommand &command)
{
    return stream >> command.mScope
                  >> command.mDest;
}


QDataStream &operator<<(QDataStream &stream, const CreateCollectionCommand &command)
{
    return stream << command.mParent
                  << command.mName
                  << command.mRemoteId
                  << command.mRemoteRev
                  << command.mMimeTypes
                  << command.mCachePolicy
                  << command.mAttributes
                  << command.mEnabled
                  << command.mSync
                  << command.mDisplay
                  << command.mIndex
                  << command.mVirtual;
}

QDataStream &operator>>(QDataStream &stream, CreateCollectionCommand &command)
{
    return stream >> command.mParent
                  >> command.mName
                  >> command.mRemoteId
                  >> command.mRemoteRev
                  >> command.mMimeTypes
                  >> command.mCachePolicy
                  >> command.mAttributes
                  >> command.mEnabled
                  >> command.mSync
                  >> command.mDisplay
                  >> command.mIndex
                  >> command.mVirtual;
}


QDataStream &operator<<(QDataStream &stream, const CopyCollectionCommand &command)
{
    return stream << command.mScope
                  << command.mDest;
}

QDataStream &operator>>(QDataStream &stream, CopyCollectionCommand &command)
{
    return stream >> command.mScope
                  >> command.mDest;
}


QDataStream &operator<<(QDataStream &stream, const DeleteCollectionCommand &command)
{
    return stream << command.mScope;
}

QDataStream &operator>>(QDataStream &stream, DeleteCollectionCommand &command)
{
    return stream >> command.mScope;
}


QDataStream &operator<<(QDataStream &stream, const FetchCollectionsCommand &command)
{
    return stream << command.mScope
                  << command.mResource
                  << command.mMimeTypes
                  << command.mDepth
                  << command.mAncestorsDepth
                  << command.mAncestorsAttributes
                  << command.mEnabled
                  << command.mSync
                  << command.mDisplay
                  << command.mIndex
                  << command.mStats;
}

QDataStream &operator>>(QDataStream &stream, FetchCollectionsCommand &command)
{
    return stream >> command.mScope
                  >> command.mResource
                  >> command.mMimeTypes
                  >> command.mDepth
                  >> command.mAncestorsDepth
                  >> command.mAncestorsAttributes
                  >> command.mEnabled
                  >> command.mSync
                  >> command.mDisplay
                  >> command.mIndex
                  >> command.mStats;
}


QDataStream &operator<<(QDataStream &stream, const FetchCollectionsResponse &command)
{
    return stream << command.mId
                  << command.mParentId
                  << command.mName
                  << command.mMimeType
                  << command.mRemoteId
                  << command.mRemoteRev
                  << command.mResource
                  << command.mStats
                  << command.mSearchQuery
                  << command.mSearchCols
                  << command.mAncestors
                  << command.mCachePolicy
                  << command.mAttributes
                  << command.mDisplay
                  << command.mSync
                  << command.mIndex
                  << command.mIsVirtual
                  << command.mReferenced
                  << command.mEnabled;
}

QDataStream &operator>>(QDataStream &stream, FetchCollectionsResponse &command)
{
    return stream >> command.mId
                  >> command.mParentId
                  >> command.mName
                  >> command.mMimeType
                  >> command.mRemoteId
                  >> command.mRemoteRev
                  >> command.mResource
                  >> command.mStats
                  >> command.mSearchQuery
                  >> command.mSearchCols
                  >> command.mAncestors
                  >> command.mCachePolicy
                  >> command.mAttributes
                  >> command.mDisplay
                  >> command.mSync
                  >> command.mIndex
                  >> command.mIsVirtual
                  >> command.mReferenced
                  >> command.mEnabled;
}


QDataStream &operator<<(QDataStream &stream, const ModifyCollectionCommand &command)
{
    stream << command.mScope
           << command.mModifiedParts;

    if (command.mModifiedParts & ModifyCollectionCommand::Name) {
        stream << command.mName;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::RemoteID) {
        stream << command.mRemoteId;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::RemoteRevision) {
        stream << command.mRemoteRev;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::ParentID) {
        stream << command.mParentId;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::MimeTypes) {
        stream << command.mMimeTypes;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::CachePolicy) {
        stream << command.mCachePolicy;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::PersistentSearch) {
        stream << command.mPersistentSearchQuery
               << command.mPersistentSearchCols
               << command.mPersistentSearchRemote
               << command.mPersistentSearchRecursive;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::RemovedAttributes) {
        stream << command.mRemovedAttributes;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::Attributes) {
        stream << command.mAttributes;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::ListPreferences) {
        stream << command.mEnabled
               << command.mSync
               << command.mDisplay
               << command.mIndex;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::Referenced) {
        stream << command.mReferenced;
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, ModifyCollectionCommand &command)
{
    stream >> command.mScope
           >> command.mModifiedParts;

    if (command.mModifiedParts & ModifyCollectionCommand::Name) {
        stream >> command.mName;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::RemoteID) {
        stream >> command.mRemoteId;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::RemoteRevision) {
        stream >> command.mRemoteRev;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::ParentID) {
        stream >> command.mParentId;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::MimeTypes) {
        stream >> command.mMimeTypes;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::CachePolicy) {
        stream >> command.mCachePolicy;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::PersistentSearch) {
        stream >> command.mPersistentSearchQuery
               >> command.mPersistentSearchCols
               >> command.mPersistentSearchRemote
               >> command.mPersistentSearchRecursive;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::RemovedAttributes) {
        stream >> command.mRemovedAttributes;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::Attributes) {
        stream >> command.mAttributes;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::ListPreferences) {
        stream >> command.mEnabled
               >> command.mSync
               >> command.mDisplay
               >> command.mIndex;
    }
    if (command.mModifiedParts & ModifyCollectionCommand::Referenced) {
        stream >> command.mReferenced;
    }
    return stream;
}


QDataStream &operator<<(QDataStream &stream, const MoveCollectionCommand &command)
{
    return stream << command.mCol
                  << command.mDest;
}

QDataStream &operator>>(QDataStream &stream, MoveCollectionCommand &command)
{
    return stream >> command.mCol
                  >> command.mDest;
}


QDataStream &operator<<(QDataStream &stream, const SelectCollectionCommand &command)
{
    return stream << command.mScope;
}

QDataStream &operator>>(QDataStream &stream, SelectCollectionCommand &command)
{
    return stream >> command.mScope;
}



QDataStream &operator<<(QDataStream &stream, const FetchCollectionStatsCommand &command)
{
    return stream << command.mScope;
}

QDataStream &operator>>(QDataStream &stream, FetchCollectionStatsCommand &command)
{
    return stream >> command.mScope;
}


QDataStream &operator<<(QDataStream &stream, const FetchCollectionStatsResponse &command)
{
    return stream << command.mCount
                  << command.mUnseen
                  << command.mSize;
}

QDataStream &operator>>(QDataStream &stream, FetchCollectionStatsResponse &command)
{
    return stream >> command.mCount
                  >> command.mUnseen
                  >> command.mSize;
}


QDataStream &operator<<(QDataStream &stream, const SearchCommand &command)
{
    return stream << command.mMimeTypes
                  << command.mCollections
                  << command.mQuery
                  << command.mFetchScope
                  << command.mRecursive
                  << command.mRemote;
}

QDataStream &operator>>(QDataStream &stream, SearchCommand &command)
{
    return stream >> command.mMimeTypes
                  >> command.mCollections
                  >> command.mQuery
                  >> command.mFetchScope
                  >> command.mRecursive
                  >> command.mRemote;
}


QDataStream &operator<<(QDataStream &stream, const SearchResultCommand &command)
{
    return stream << command.mSearchId
                  << command.mCollectionId
                  << command.mResult;
}

QDataStream &operator>>(QDataStream &stream, SearchResultCommand &command)
{
    return stream >> command.mSearchId
                  >> command.mCollectionId
                  >> command.mResult;
}


QDataStream &operator<<(QDataStream &stream, const StoreSearchCommand &command)
{
    return stream << command.mName
                  << command.mQuery
                  << command.mMimeTypes
                  << command.mQueryCols
                  << command.mRemote
                  << command.mRecursive;
}

QDataStream &operator>>(QDataStream &stream, StoreSearchCommand &command)
{
    return stream >> command.mName
                  >> command.mQuery
                  >> command.mMimeTypes
                  >> command.mQueryCols
                  >> command.mRemote
                  >> command.mRecursive;
}


QDataStream &operator<<(QDataStream &stream, const CreateTagCommand &command)
{
    return stream << command.mGid
                  << command.mRemoteId
                  << command.mType
                  << command.mAttributes
                  << command.mParentId
                  << command.mMerge;
}

QDataStream &operator>>(QDataStream &stream, CreateTagCommand &command)
{
    return stream >> command.mGid
                  >> command.mRemoteId
                  >> command.mType
                  >> command.mAttributes
                  >> command.mParentId
                  >> command.mMerge;
}


QDataStream &operator<<(QDataStream &stream, const DeleteTagCommand &command)
{
    return stream << command.mScope;
}

QDataStream &operator>>(QDataStream &stream, DeleteTagCommand &command)
{
    return stream >> command.mScope;
}


QDataStream &operator<<(QDataStream &stream, const FetchTagsCommand &command)
{
    return stream << command.mScope;
}

QDataStream &operator>>(QDataStream &stream, FetchTagsCommand &command)
{
    return stream >> command.mScope;
}


QDataStream &operator<<(QDataStream &stream, const FetchTagsResponse &command)
{
    return stream << command.mId
                  << command.mParentId
                  << command.mGid
                  << command.mType
                  << command.mRemoteId
                  << command.mAttributes;
}

QDataStream &operator>>(QDataStream &stream, FetchTagsResponse &command)
{
    return stream >> command.mId
                  >> command.mParentId
                  >> command.mGid
                  >> command.mType
                  >> command.mRemoteId
                  >> command.mAttributes;
}


QDataStream &operator<<(QDataStream &stream, const ModifyTagCommand &command)
{
    stream << command.mTagId
           << command.mModifiedParts;
    if (command.mModifiedParts & ModifyTagCommand::ParentId) {
        stream << command.mParentId;
    }
    if (command.mModifiedParts & ModifyTagCommand::Type) {
        stream << command.mType;
    }
    if (command.mModifiedParts & ModifyTagCommand::RemoteId) {
        stream << command.mRemoteId;
    }
    if (command.mModifiedParts & ModifyTagCommand::RemovedAttributes) {
        stream << command.mRemovedAttributes;
    }
    if (command.mModifiedParts & ModifyTagCommand::Attributes) {
        stream << command.mAttributes;
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, ModifyTagCommand &command)
{
    stream >> command.mTagId
           >> command.mModifiedParts;
    if (command.mModifiedParts & ModifyTagCommand::ParentId) {
        stream >> command.mParentId;
    }
    if (command.mModifiedParts & ModifyTagCommand::Type) {
        stream >> command.mType;
    }
    if (command.mModifiedParts & ModifyTagCommand::RemoteId) {
        stream >> command.mRemoteId;
    }
    if (command.mModifiedParts & ModifyTagCommand::RemovedAttributes) {
        stream >> command.mRemovedAttributes;
    }
    if (command.mModifiedParts & ModifyTagCommand::Attributes) {
        stream >> command.mAttributes;
    }
    return stream;
}







QDataStream &operator<<(QDataStream &stream, const ModifyRelationCommand &command)
{
    return stream << command.mLeft
                  << command.mRight
                  << command.mType
                  << command.mRemoteId;
}

QDataStream &operator>>(QDataStream &stream, ModifyRelationCommand &command)
{
    return stream >> command.mLeft
                  >> command.mRight
                  >> command.mType
                  >> command.mRemoteId;
}


QDataStream &operator<<(QDataStream &stream, const RemoveRelationsCommand &command)
{
    return stream << command.mLeft
                  << command.mRight
                  << command.mType;
}

QDataStream &operator>>(QDataStream &stream, RemoveRelationsCommand &command)
{
    return stream >> command.mLeft
                  >> command.mRight
                  >> command.mType;
}


QDataStream &operator<<(QDataStream &stream, const SelectResourceCommand &command)
{
    return stream << command.mResourceId;
}

QDataStream &operator>>(QDataStream &stream, SelectResourceCommand &command)
{
    return stream >> command.mResourceId;
}


QDataStream &operator<<(QDataStream &stream, const StreamPayloadCommand &command)
{
    return stream << command.mPayloadName
                  << command.mExpectedSize
                  << command.mExternalFile;
}

QDataStream &operator>>(QDataStream &stream, StreamPayloadCommand &command)
{
    return stream >> command.mPayloadName
                  >> command.mExpectedSize
                  >> command.mExternalFile;
}


QDataStream &operator<<(QDataStream &stream, const StreamPayloadResponse &command)
{
    return stream << command.mIsExternal
                  << command.mData;
}

QDataStream &operator>>(QDataStream &stream, StreamPayloadResponse &command)
{
    return stream >> command.mIsExternal
                  >> command.mData;
}

} // namespace Protocol
} // namespace Akonadi