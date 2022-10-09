/*
 *  SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *  SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "datastream_p_p.h"
#include "imapset_p.h"
#include "protocol_p.h"
#include "scope_p.h"

#include <type_traits>
#include <typeinfo>

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>

#include <cassert>

// clazy:excludeall=function-args-by-value

#undef AKONADI_DECLARE_PRIVATE
#define AKONADI_DECLARE_PRIVATE(Class)                                                                                                                         \
    inline Class##Private *Class::d_func()                                                                                                                     \
    {                                                                                                                                                          \
        return reinterpret_cast<Class##Private *>(d_ptr.data());                                                                                               \
    }                                                                                                                                                          \
    inline const Class##Private *Class::d_func() const                                                                                                         \
    {                                                                                                                                                          \
        return reinterpret_cast<const Class##Private *>(d_ptr.constData());                                                                                    \
    }

namespace Akonadi
{
namespace Protocol
{
QDebug operator<<(QDebug _dbg, Command::Type type)
{
    QDebug dbg(_dbg.noquote());

    switch (type) {
    case Command::Invalid:
        return dbg << "Invalid";

    case Command::Hello:
        return dbg << "Hello";
    case Command::Login:
        return dbg << "Login";
    case Command::Logout:
        return dbg << "Logout";

    case Command::Transaction:
        return dbg << "Transaction";

    case Command::CreateItem:
        return dbg << "CreateItem";
    case Command::CopyItems:
        return dbg << "CopyItems";
    case Command::DeleteItems:
        return dbg << "DeleteItems";
    case Command::FetchItems:
        return dbg << "FetchItems";
    case Command::LinkItems:
        return dbg << "LinkItems";
    case Command::ModifyItems:
        return dbg << "ModifyItems";
    case Command::MoveItems:
        return dbg << "MoveItems";

    case Command::CreateCollection:
        return dbg << "CreateCollection";
    case Command::CopyCollection:
        return dbg << "CopyCollection";
    case Command::DeleteCollection:
        return dbg << "DeleteCollection";
    case Command::FetchCollections:
        return dbg << "FetchCollections";
    case Command::FetchCollectionStats:
        return dbg << "FetchCollectionStats";
    case Command::ModifyCollection:
        return dbg << "ModifyCollection";
    case Command::MoveCollection:
        return dbg << "MoveCollection";

    case Command::Search:
        return dbg << "Search";
    case Command::SearchResult:
        return dbg << "SearchResult";
    case Command::StoreSearch:
        return dbg << "StoreSearch";

    case Command::CreateTag:
        return dbg << "CreateTag";
    case Command::DeleteTag:
        return dbg << "DeleteTag";
    case Command::FetchTags:
        return dbg << "FetchTags";
    case Command::ModifyTag:
        return dbg << "ModifyTag";

    case Command::FetchRelations:
        return dbg << "FetchRelations";
    case Command::ModifyRelation:
        return dbg << "ModifyRelation";
    case Command::RemoveRelations:
        return dbg << "RemoveRelations";

    case Command::SelectResource:
        return dbg << "SelectResource";

    case Command::StreamPayload:
        return dbg << "StreamPayload";
    case Command::ItemChangeNotification:
        return dbg << "ItemChangeNotification";
    case Command::CollectionChangeNotification:
        return dbg << "CollectionChangeNotification";
    case Command::TagChangeNotification:
        return dbg << "TagChangeNotification";
    case Command::RelationChangeNotification:
        return dbg << "RelationChangeNotification";
    case Command::SubscriptionChangeNotification:
        return dbg << "SubscriptionChangeNotification";
    case Command::DebugChangeNotification:
        return dbg << "DebugChangeNotification";
    case Command::CreateSubscription:
        return dbg << "CreateSubscription";
    case Command::ModifySubscription:
        return dbg << "ModifySubscription";

    case Command::_ResponseBit:
        Q_ASSERT(false);
        return dbg << static_cast<int>(type);
    }

    Q_ASSERT(false);
    return dbg << static_cast<int>(type);
}

template<typename T>
DataStream &operator<<(DataStream &stream, const QSharedPointer<T> &ptr)
{
    Protocol::serialize(stream, ptr);
    return stream;
}

template<typename T>
DataStream &operator>>(DataStream &stream, QSharedPointer<T> &ptr)
{
    ptr = Protocol::deserialize(stream.device()).staticCast<T>();
    return stream;
}

/******************************************************************************/

Command::Command(quint8 type)
    : mType(type)
{
}

bool Command::operator==(const Command &other) const
{
    return mType == other.mType;
}

void Command::toJson(QJsonObject &json) const
{
    json[QStringLiteral("response")] = static_cast<bool>(mType & Command::_ResponseBit);

#define case_label(x)                                                                                                                                          \
    case Command::x: {                                                                                                                                         \
        json[QStringLiteral("type")] = QStringLiteral(#x);                                                                                                     \
    } break;

    switch (mType & ~Command::_ResponseBit) {
        case_label(Invalid) case_label(Hello)

            case_label(Login) case_label(Logout)

                case_label(Transaction)

                    case_label(CreateItem) case_label(CopyItems) case_label(DeleteItems) case_label(FetchItems) case_label(LinkItems) case_label(ModifyItems)
                        case_label(MoveItems)

                            case_label(CreateCollection) case_label(CopyCollection) case_label(DeleteCollection) case_label(FetchCollections)
                                case_label(FetchCollectionStats) case_label(ModifyCollection) case_label(MoveCollection)

                                    case_label(Search) case_label(SearchResult) case_label(StoreSearch)

                                        case_label(CreateTag) case_label(DeleteTag) case_label(FetchTags) case_label(ModifyTag)

                                            case_label(FetchRelations) case_label(ModifyRelation) case_label(RemoveRelations)

                                                case_label(SelectResource)

                                                    case_label(StreamPayload) case_label(CreateSubscription) case_label(ModifySubscription)

                                                        case_label(DebugChangeNotification) case_label(ItemChangeNotification)
                                                            case_label(CollectionChangeNotification) case_label(TagChangeNotification)
                                                                case_label(RelationChangeNotification) case_label(SubscriptionChangeNotification)
    }
#undef case_label
}

DataStream &operator<<(DataStream &stream, const Command &cmd)
{
    return stream << cmd.mType;
}

DataStream &operator>>(DataStream &stream, Command &cmd)
{
    return stream >> cmd.mType;
}

QDebug operator<<(QDebug dbg, const Command &cmd)
{
    return dbg.noquote() << ((cmd.mType & Command::_ResponseBit) ? "Response:" : "Command:") << static_cast<Command::Type>(cmd.mType & ~Command::_ResponseBit)
                         << "\n";
}

void toJson(const Akonadi::Protocol::Command *command, QJsonObject &json)
{
#define case_notificationlabel(x, class)                                                                                                                       \
    case Command::x: {                                                                                                                                         \
        static_cast<const Akonadi::Protocol::class *>(command)->toJson(json);                                                                                  \
    } break;
#define case_commandlabel(x, cmd, resp)                                                                                                                        \
    case Command::x: {                                                                                                                                         \
        static_cast<const Akonadi::Protocol::cmd *>(command)->toJson(json);                                                                                    \
    } break;                                                                                                                                                   \
    case Command::x | Command::_ResponseBit: {                                                                                                                 \
        static_cast<const Akonadi::Protocol::resp *>(command)->toJson(json);                                                                                   \
    } break;

    switch (command->mType) {
    case Command::Invalid:
        break;

    case Command::Hello | Command::_ResponseBit: {
        static_cast<const Akonadi::Protocol::HelloResponse *>(command)->toJson(json);
    } break;
        case_commandlabel(Login, LoginCommand, LoginResponse) case_commandlabel(Logout, LogoutCommand, LogoutResponse)

            case_commandlabel(Transaction, TransactionCommand, TransactionResponse)

                case_commandlabel(CreateItem, CreateItemCommand, CreateItemResponse) case_commandlabel(CopyItems, CopyItemsCommand, CopyItemsResponse)
                    case_commandlabel(DeleteItems, DeleteItemsCommand, DeleteItemsResponse) case_commandlabel(FetchItems, FetchItemsCommand, FetchItemsResponse)
                        case_commandlabel(LinkItems, LinkItemsCommand, LinkItemsResponse) case_commandlabel(
                            ModifyItems,
                            ModifyItemsCommand,
                            ModifyItemsResponse) case_commandlabel(MoveItems, MoveItemsCommand, MoveItemsResponse)

                            case_commandlabel(CreateCollection, CreateCollectionCommand, CreateCollectionResponse) case_commandlabel(
                                CopyCollection,
                                CopyCollectionCommand,
                                CopyCollectionResponse) case_commandlabel(DeleteCollection, DeleteCollectionCommand, DeleteCollectionResponse)
                                case_commandlabel(FetchCollections, FetchCollectionsCommand, FetchCollectionsResponse) case_commandlabel(
                                    FetchCollectionStats,
                                    FetchCollectionStatsCommand,
                                    FetchCollectionStatsResponse) case_commandlabel(ModifyCollection, ModifyCollectionCommand, ModifyCollectionResponse)
                                    case_commandlabel(MoveCollection, MoveCollectionCommand, MoveCollectionResponse)

                                        case_commandlabel(Search, SearchCommand, SearchResponse) case_commandlabel(
                                            SearchResult,
                                            SearchResultCommand,
                                            SearchResultResponse) case_commandlabel(StoreSearch, StoreSearchCommand, StoreSearchResponse)

                                            case_commandlabel(CreateTag, CreateTagCommand, CreateTagResponse)
                                                case_commandlabel(DeleteTag, DeleteTagCommand, DeleteTagResponse)
                                                    case_commandlabel(FetchTags, FetchTagsCommand, FetchTagsResponse)
                                                        case_commandlabel(ModifyTag, ModifyTagCommand, ModifyTagResponse)

                                                            case_commandlabel(FetchRelations, FetchRelationsCommand, FetchRelationsResponse)
                                                                case_commandlabel(ModifyRelation, ModifyRelationCommand, ModifyRelationResponse)
                                                                    case_commandlabel(RemoveRelations, RemoveRelationsCommand, RemoveRelationsResponse)

                                                                        case_commandlabel(SelectResource, SelectResourceCommand, SelectResourceResponse)

                                                                            case_commandlabel(StreamPayload, StreamPayloadCommand, StreamPayloadResponse)
                                                                                case_commandlabel(
                                                                                    CreateSubscription,
                                                                                    CreateSubscriptionCommand,
                                                                                    CreateSubscriptionResponse) case_commandlabel(ModifySubscription,
                                                                                                                                  ModifySubscriptionCommand,
                                                                                                                                  ModifySubscriptionResponse)

                                                                                    case_notificationlabel(DebugChangeNotification, DebugChangeNotification)
                                                                                        case_notificationlabel(ItemChangeNotification, ItemChangeNotification)
                                                                                            case_notificationlabel(CollectionChangeNotification,
                                                                                                                   CollectionChangeNotification)
                                                                                                case_notificationlabel(TagChangeNotification,
                                                                                                                       TagChangeNotification)
                                                                                                    case_notificationlabel(RelationChangeNotification,
                                                                                                                           RelationChangeNotification)
                                                                                                        case_notificationlabel(SubscriptionChangeNotification,
                                                                                                                               SubscriptionChangeNotification)
    }
#undef case_notificationlabel
#undef case_commandlabel
}

/******************************************************************************/

Response::Response()
    : Command(Command::Invalid | Command::_ResponseBit)
    , mErrorCode(0)
{
}

Response::Response(Command::Type type)
    : Command(type | Command::_ResponseBit)
    , mErrorCode(0)
{
}

bool Response::operator==(const Response &other) const
{
    return *static_cast<const Command *>(this) == static_cast<const Command &>(other) && mErrorCode == other.mErrorCode && mErrorMsg == other.mErrorMsg;
}

void Response::toJson(QJsonObject &json) const
{
    static_cast<const Command *>(this)->toJson(json);
    if (isError()) {
        QJsonObject error;
        error[QStringLiteral("code")] = errorCode();
        error[QStringLiteral("message")] = errorMessage();
        json[QStringLiteral("error")] = error;
    } else {
        json[QStringLiteral("error")] = false;
    }
}

DataStream &operator<<(DataStream &stream, const Response &cmd)
{
    return stream << static_cast<const Command &>(cmd) << cmd.mErrorCode << cmd.mErrorMsg;
}

DataStream &operator>>(DataStream &stream, Response &cmd)
{
    return stream >> static_cast<Command &>(cmd) >> cmd.mErrorCode >> cmd.mErrorMsg;
}

QDebug operator<<(QDebug dbg, const Response &resp)
{
    return dbg.noquote() << static_cast<const Command &>(resp) << "Error code:" << resp.mErrorCode << "\n"
                         << "Error msg:" << resp.mErrorMsg << "\n";
}

/******************************************************************************/

class FactoryPrivate
{
public:
    typedef CommandPtr (*CommandFactoryFunc)();
    typedef ResponsePtr (*ResponseFactoryFunc)();

    FactoryPrivate()
    {
        // Session management
        registerType<Command::Hello, Command /* invalid */, HelloResponse>();
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
        registerType<Command::ItemChangeNotification, ItemChangeNotification, Response /* invalid */>();
        registerType<Command::CollectionChangeNotification, CollectionChangeNotification, Response /* invalid */>();
        registerType<Command::TagChangeNotification, TagChangeNotification, Response /* invalid */>();
        registerType<Command::RelationChangeNotification, RelationChangeNotification, Response /* invalid */>();
        registerType<Command::SubscriptionChangeNotification, SubscriptionChangeNotification, Response /* invalid */>();
        registerType<Command::DebugChangeNotification, DebugChangeNotification, Response /* invalid */>();
        registerType<Command::CreateSubscription, CreateSubscriptionCommand, CreateSubscriptionResponse>();
        registerType<Command::ModifySubscription, ModifySubscriptionCommand, ModifySubscriptionResponse>();
    }

    // clang has problem resolving the right qHash() overload for Command::Type,
    // so use its underlying integer type instead
    QHash<std::underlying_type<Command::Type>::type, QPair<CommandFactoryFunc, ResponseFactoryFunc>> registrar;

private:
    template<typename T>
    static CommandPtr commandFactoryFunc()
    {
        return QSharedPointer<T>::create();
    }
    template<typename T>
    static ResponsePtr responseFactoryFunc()
    {
        return QSharedPointer<T>::create();
    }

    template<Command::Type T, typename CmdType, typename RespType>
    void registerType()
    {
        CommandFactoryFunc cmdFunc = &commandFactoryFunc<CmdType>;
        ResponseFactoryFunc respFunc = &responseFactoryFunc<RespType>;
        registrar.insert(T, qMakePair(cmdFunc, respFunc));
    }
};

Q_GLOBAL_STATIC(FactoryPrivate, sFactoryPrivate) // NOLINT(readability-redundant-member-init)

CommandPtr Factory::command(Command::Type type)
{
    auto iter = sFactoryPrivate->registrar.constFind(type);
    if (iter == sFactoryPrivate->registrar.constEnd()) {
        return QSharedPointer<Command>::create();
    }
    return iter->first();
}

ResponsePtr Factory::response(Command::Type type)
{
    auto iter = sFactoryPrivate->registrar.constFind(type);
    if (iter == sFactoryPrivate->registrar.constEnd()) {
        return QSharedPointer<Response>::create();
    }
    return iter->second();
}

/******************************************************************************/

/******************************************************************************/

bool ItemFetchScope::operator==(const ItemFetchScope &other) const
{
    return mRequestedParts == other.mRequestedParts && mChangedSince == other.mChangedSince && mAncestorDepth == other.mAncestorDepth && mFlags == other.mFlags;
}

QVector<QByteArray> ItemFetchScope::requestedPayloads() const
{
    QVector<QByteArray> rv;
    std::copy_if(mRequestedParts.begin(), mRequestedParts.end(), std::back_inserter(rv), [](const QByteArray &ba) {
        return ba.startsWith("PLD:");
    });
    return rv;
}

void ItemFetchScope::setFetch(FetchFlags attributes, bool fetch)
{
    if (fetch) {
        mFlags |= attributes;
        if (attributes & FullPayload) {
            if (!mRequestedParts.contains(AKONADI_PARAM_PLD_RFC822)) {
                mRequestedParts << AKONADI_PARAM_PLD_RFC822;
            }
        }
    } else {
        mFlags &= ~attributes;
    }
}

bool ItemFetchScope::fetch(FetchFlags flags) const
{
    if (flags == None) {
        return mFlags == None;
    } else {
        return mFlags & flags;
    }
}

void ItemFetchScope::toJson(QJsonObject &json) const
{
    json[QStringLiteral("flags")] = static_cast<int>(mFlags);
    json[QStringLiteral("ChangedSince")] = mChangedSince.toString();
    json[QStringLiteral("AncestorDepth")] = static_cast<std::underlying_type<AncestorDepth>::type>(mAncestorDepth);

    QJsonArray requestedPartsArray;
    for (const auto &part : std::as_const(mRequestedParts)) {
        requestedPartsArray.append(QString::fromUtf8(part));
    }
    json[QStringLiteral("RequestedParts")] = requestedPartsArray;
}

QDebug operator<<(QDebug dbg, ItemFetchScope::AncestorDepth depth)
{
    switch (depth) {
    case ItemFetchScope::NoAncestor:
        return dbg << "No ancestor";
    case ItemFetchScope::ParentAncestor:
        return dbg << "Parent ancestor";
    case ItemFetchScope::AllAncestors:
        return dbg << "All ancestors";
    }
    Q_UNREACHABLE();
}

DataStream &operator<<(DataStream &stream, const ItemFetchScope &scope)
{
    return stream << scope.mRequestedParts << scope.mChangedSince << scope.mAncestorDepth << scope.mFlags;
}

DataStream &operator>>(DataStream &stream, ItemFetchScope &scope)
{
    return stream >> scope.mRequestedParts >> scope.mChangedSince >> scope.mAncestorDepth >> scope.mFlags;
}

QDebug operator<<(QDebug dbg, const ItemFetchScope &scope)
{
    return dbg.noquote() << "FetchScope(\n"
                         << "Fetch Flags:" << scope.mFlags << "\n"
                         << "Changed Since:" << scope.mChangedSince << "\n"
                         << "Ancestor Depth:" << scope.mAncestorDepth << "\n"
                         << "Requested Parts:" << scope.mRequestedParts << ")\n";
}

/******************************************************************************/

ScopeContext::ScopeContext(Type type, qint64 id)
{
    if (type == ScopeContext::Tag) {
        mTagCtx = id;
    } else if (type == ScopeContext::Collection) {
        mColCtx = id;
    }
}

ScopeContext::ScopeContext(Type type, const QString &ctx)
{
    if (type == ScopeContext::Tag) {
        mTagCtx = ctx;
    } else if (type == ScopeContext::Collection) {
        mColCtx = ctx;
    }
}

bool ScopeContext::operator==(const ScopeContext &other) const
{
    return mColCtx == other.mColCtx && mTagCtx == other.mTagCtx;
}

void ScopeContext::toJson(QJsonObject &json) const
{
    if (isEmpty()) {
        json[QStringLiteral("scopeContext")] = false;
    } else if (hasContextId(ScopeContext::Tag)) {
        json[QStringLiteral("scopeContext")] = QStringLiteral("tag");
        json[QStringLiteral("TagID")] = contextId(ScopeContext::Tag);
    } else if (hasContextId(ScopeContext::Collection)) {
        json[QStringLiteral("scopeContext")] = QStringLiteral("collection");
        json[QStringLiteral("ColID")] = contextId(ScopeContext::Collection);
    } else if (hasContextRID(ScopeContext::Tag)) {
        json[QStringLiteral("scopeContext")] = QStringLiteral("tagrid");
        json[QStringLiteral("TagRID")] = contextRID(ScopeContext::Tag);
    } else if (hasContextRID(ScopeContext::Collection)) {
        json[QStringLiteral("scopeContext")] = QStringLiteral("colrid");
        json[QStringLiteral("ColRID")] = contextRID(ScopeContext::Collection);
    }
}

DataStream &operator<<(DataStream &stream, const ScopeContext &context)
{
    // We don't have a custom generic DataStream streaming operator for QVariant
    // because it's very hard, esp. without access to QVariant private
    // stuff, so we have to decompose it manually here.
    QVariant::Type vType = context.mColCtx.type();
    stream << vType;
    if (vType == QVariant::LongLong) {
        stream << context.mColCtx.toLongLong();
    } else if (vType == QVariant::String) {
        stream << context.mColCtx.toString();
    }

    vType = context.mTagCtx.type();
    stream << vType;
    if (vType == QVariant::LongLong) {
        stream << context.mTagCtx.toLongLong();
    } else if (vType == QVariant::String) {
        stream << context.mTagCtx.toString();
    }

    return stream;
}

DataStream &operator>>(DataStream &stream, ScopeContext &context)
{
    QVariant::Type vType;
    qint64 id;
    QString rid;

    for (ScopeContext::Type type : {ScopeContext::Collection, ScopeContext::Tag}) {
        stream >> vType;
        if (vType == QVariant::LongLong) {
            stream >> id;
            context.setContext(type, id);
        } else if (vType == QVariant::String) {
            stream >> rid;
            context.setContext(type, rid);
        }
    }

    return stream;
}

QDebug operator<<(QDebug _dbg, const ScopeContext &ctx)
{
    QDebug dbg(_dbg.noquote());
    dbg << "ScopeContext(";
    if (ctx.isEmpty()) {
        dbg << "empty";
    } else if (ctx.hasContextId(ScopeContext::Tag)) {
        dbg << "Tag ID:" << ctx.contextId(ScopeContext::Tag);
    } else if (ctx.hasContextId(ScopeContext::Collection)) {
        dbg << "Col ID:" << ctx.contextId(ScopeContext::Collection);
    } else if (ctx.hasContextRID(ScopeContext::Tag)) {
        dbg << "Tag RID:" << ctx.contextRID(ScopeContext::Tag);
    } else if (ctx.hasContextRID(ScopeContext::Collection)) {
        dbg << "Col RID:" << ctx.contextRID(ScopeContext::Collection);
    }
    return dbg << ")\n";
}

/******************************************************************************/

ChangeNotification::ChangeNotification(Command::Type type)
    : Command(type)
{
}

bool ChangeNotification::operator==(const ChangeNotification &other) const
{
    return static_cast<const Command &>(*this) == other && mSessionId == other.mSessionId;
    // metadata are not compared
}

QList<qint64> ChangeNotification::itemsToUids(const QVector<FetchItemsResponse> &items)
{
    QList<qint64> rv;
    rv.reserve(items.size());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(rv), [](const FetchItemsResponse &item) {
        return item.id();
    });
    return rv;
}

bool ChangeNotification::isRemove() const
{
    switch (type()) {
    case Command::Invalid:
        return false;
    case Command::ItemChangeNotification:
        return static_cast<const class ItemChangeNotification *>(this)->operation() == ItemChangeNotification::Remove;
    case Command::CollectionChangeNotification:
        return static_cast<const class CollectionChangeNotification *>(this)->operation() == CollectionChangeNotification::Remove;
    case Command::TagChangeNotification:
        return static_cast<const class TagChangeNotification *>(this)->operation() == TagChangeNotification::Remove;
    case Command::RelationChangeNotification:
        return static_cast<const class RelationChangeNotification *>(this)->operation() == RelationChangeNotification::Remove;
    case Command::SubscriptionChangeNotification:
        return static_cast<const class SubscriptionChangeNotification *>(this)->operation() == SubscriptionChangeNotification::Remove;
    case Command::DebugChangeNotification:
        return false;
    default:
        Q_ASSERT_X(false, __FUNCTION__, "Unknown ChangeNotification type");
    }

    return false;
}

bool ChangeNotification::isMove() const
{
    switch (type()) {
    case Command::Invalid:
        return false;
    case Command::ItemChangeNotification:
        return static_cast<const class ItemChangeNotification *>(this)->operation() == ItemChangeNotification::Move;
    case Command::CollectionChangeNotification:
        return static_cast<const class CollectionChangeNotification *>(this)->operation() == CollectionChangeNotification::Move;
    case Command::TagChangeNotification:
    case Command::RelationChangeNotification:
    case Command::SubscriptionChangeNotification:
    case Command::DebugChangeNotification:
        return false;
    default:
        Q_ASSERT_X(false, __FUNCTION__, "Unknown ChangeNotification type");
    }

    return false;
}

bool ChangeNotification::appendAndCompress(ChangeNotificationList &list, const ChangeNotificationPtr &msg)
{
    // It is likely that compressible notifications are within the last few notifications, so avoid searching a list that is potentially huge
    static const int maxCompressionSearchLength = 10;
    int searchCounter = 0;
    // There are often multiple Collection Modify notifications in the queue,
    // so we optimize for this case.

    if (msg->type() == Command::CollectionChangeNotification) {
        const auto &cmsg = Protocol::cmdCast<class CollectionChangeNotification>(msg);
        if (cmsg.operation() == CollectionChangeNotification::Modify) {
            // We are iterating from end, since there's higher probability of finding
            // matching notification
            for (auto iter = list.end(), begin = list.begin(); iter != begin;) {
                --iter;
                if ((*iter)->type() == Protocol::Command::CollectionChangeNotification) {
                    auto &it = Protocol::cmdCast<class CollectionChangeNotification>(*iter);
                    const auto &msgCol = cmsg.collection();
                    const auto &itCol = it.collection();
                    if (msgCol.id() == itCol.id() && msgCol.remoteId() == itCol.remoteId() && msgCol.remoteRevision() == itCol.remoteRevision()
                        && msgCol.resource() == itCol.resource() && cmsg.destinationResource() == it.destinationResource()
                        && cmsg.parentCollection() == it.parentCollection() && cmsg.parentDestCollection() == it.parentDestCollection()) {
                        // both are modifications, merge them together and drop the new one
                        if (cmsg.operation() == CollectionChangeNotification::Modify && it.operation() == CollectionChangeNotification::Modify) {
                            const auto parts = it.changedParts();
                            it.setChangedParts(parts + cmsg.changedParts());
                            return false;
                        }

                        // we found Add notification, which means we can drop this modification
                        if (it.operation() == CollectionChangeNotification::Add) {
                            return false;
                        }
                    }
                }
                searchCounter++;
                if (searchCounter >= maxCompressionSearchLength) {
                    break;
                }
            }
        }
    }

    // All other cases are just append, as the compression becomes too expensive in large batches
    list.append(msg);
    return true;
}

void ChangeNotification::toJson(QJsonObject &json) const
{
    static_cast<const Command *>(this)->toJson(json);
    json[QStringLiteral("session")] = QString::fromUtf8(mSessionId);

    QJsonArray metadata;
    for (const auto &m : std::as_const(mMetaData)) {
        metadata.append(QString::fromUtf8(m));
    }
    json[QStringLiteral("metadata")] = metadata;
}

DataStream &operator<<(DataStream &stream, const ChangeNotification &ntf)
{
    return stream << static_cast<const Command &>(ntf) << ntf.mSessionId;
}

DataStream &operator>>(DataStream &stream, ChangeNotification &ntf)
{
    return stream >> static_cast<Command &>(ntf) >> ntf.mSessionId;
}

QDebug operator<<(QDebug dbg, const ChangeNotification &ntf)
{
    return dbg.noquote() << static_cast<const Command &>(ntf) << "Session:" << ntf.mSessionId << "\n"
                         << "MetaData:" << ntf.mMetaData << "\n";
}

DataStream &operator>>(DataStream &stream, ChangeNotification::Relation &relation)
{
    return stream >> relation.type >> relation.leftId >> relation.rightId;
}

DataStream &operator<<(DataStream &stream, const ChangeNotification::Relation &relation)
{
    return stream << relation.type << relation.leftId << relation.rightId;
}

QDebug operator<<(QDebug _dbg, const ChangeNotification::Relation &rel)
{
    QDebug dbg(_dbg.noquote());
    return dbg << "Left: " << rel.leftId << ", Right:" << rel.rightId << ", Type: " << rel.type;
}

} // namespace Protocol
} // namespace Akonadi

// Helpers for the generated code
namespace Akonadi
{
namespace Protocol
{
template<typename Value, template<typename> class Container>
inline bool containerComparator(const Container<Value> &c1, const Container<Value> &c2)
{
    return c1 == c2;
}

template<typename T, template<typename> class Container>
inline bool containerComparator(const Container<QSharedPointer<T>> &c1, const Container<QSharedPointer<T>> &c2)
{
    if (c1.size() != c2.size()) {
        return false;
    }

    for (auto it1 = c1.cbegin(), it2 = c2.cbegin(), end1 = c1.cend(); it1 != end1; ++it1, ++it2) {
        if (**it1 != **it2) {
            return false;
        }
    }
    return true;
}

} // namespace Protocol
} // namespace Akonadi

/******************************************************************************/

// Here comes the generated protocol implementation
#include "protocol_gen.cpp"

/******************************************************************************/
