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

using namespace Akonadi;
using namespace Akonadi::Protocol;

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


QDataStream &operator<<(QDataStream &stream, const FetchScope &scope)
{
    return stream << scope.mRequestedParts
                  << scope.mRequestedPayloads
                  << scope.mChangedSince
                  << scope.mTagFetchScope
                  << scope.mAncestorDepth
                  << scope.mFetchFlags;
}

QDataStream &operator>>(QDataStream &stream, FetchScope &scope)
{
    return stream >> scope.mRequestedParts
                  >> scope.mRequestedPayloads
                  >> scope.mChangedSince
                  >> scope.mTagFetchScope
                  >> scope.mAncestorDepth
                  >> scope.mFetchFlags;
}


QDataStream &operator<<(QDataStream &stream, const PartMetaData &part)
{
    return stream << part.mName
                  << part.mSize
                  << part.mVersion;
}

QDataStream &operator>>(QDataStream &stream, PartMetaData &part)
{
    return stream >> part.mName
                  >> part.mSize
                  >> part.mVersion;
}


QDataStream &operator<<(QDataStream &stream, const CachePolicy &policy)
{
    return stream << policy.mInherit
                  << policy.mInterval
                  << policy.mCacheTimeout
                  << policy.mSyncOnDemand
                  << policy.mLocalParts;
}

QDataStream &operator>>(QDataStream &stream, CachePolicy &policy)
{
    return stream >> policy.mInherit
                  >> policy.mInterval
                  >> policy.mCacheTimeout
                  >> policy.mSyncOnDemand
                  >> policy.mLocalParts;
}

QDataStream &operator<<(QDataStream &stream, const Ancestor &ancestor)
{
    return stream << ancestor.mId
                  << ancestor.mRemoteId
                  << ancestor.mAttributes;
}

QDataStream &operator>>(QDataStream &stream, Ancestor &ancestor)
{
    return stream >> ancestor.mId
                  >> ancestor.mRemoteId
                  >> ancestor.mAttributes;
}

QDataStream &operator<<(QDataStream &stream, const Command &command)
{
    return stream << command.mCommandType;
}

QDataStream &operator>>(QDataStream &stream, Command &command)
{
    return stream >> command.mCommandType;
}


QDataStream &operator<<(QDataStream &stream, const Response &command)
{
    return stream << command.mErrorCode
                  << command.mErrorMsg;
}

QDataStream &operator>>(QDataStream &stream, Response &command)
{
    return stream >> command.mErrorCode
                  >> command.mErrorMsg;
}


QDataStream &operator<<(QDataStream &stream, const HelloResponse &command)
{
    return stream << command.mServer
                  << command.mMessage
                  << command.mVersion;
}

QDataStream &operator>>(QDataStream &stream, HelloResponse &command)
{
    return stream >> command.mServer
                  >> command.mMessage
                  >> command.mVersion;
}


QDataStream &operator<<(QDataStream &stream, const LoginCommand &command)
{
    return stream << command.mSession;
}

QDataStream &operator>>(QDataStream &stream, LoginCommand &command)
{
    return stream >> command.mSession;
}


QDataStream &operator<<(QDataStream &stream, const TransactionCommand &command)
{
    return stream << command.mMode;
}

QDataStream &operator>>(QDataStream &stream, TransactionCommand &command)
{
    return stream >> command.mMode;
}


QDataStream &operator<<(QDataStream &stream, const CreateItemCommand &command)
{
    return stream << command.mMergeMode
                  << command.mCollection
                  << command.mItemSize
                  << command.mMimeType
                  << command.mGid
                  << command.mRemoteId
                  << command.mRemoteRev
                  << command.mDateTime
                  << command.mFlags
                  << command.mAddedFlags
                  << command.mRemovedFlags
                  << command.mTags
                  << command.mAddedTags
                  << command.mRemovedTags
                  << command.mRemovedParts
                  << command.mParts;
}

QDataStream &operator>>(QDataStream &stream, CreateItemCommand &command)
{
    return stream >> command.mMergeMode
                  >> command.mCollection
                  >> command.mItemSize
                  >> command.mMimeType
                  >> command.mGid
                  >> command.mRemoteId
                  >> command.mRemoteRev
                  >> command.mDateTime
                  >> command.mFlags
                  >> command.mAddedFlags
                  >> command.mRemovedFlags
                  >> command.mTags
                  >> command.mAddedTags
                  >> command.mRemovedTags
                  >> command.mRemovedParts
                  >> command.mParts;
}


QDataStream &operator<<(QDataStream &stream, const CopyItemsCommand &command)
{
    return stream << command.mScope
                  << command.mDest;
}

QDataStream &operator>>(QDataStream &stream, CopyItemsCommand &command)
{
    return stream >> command.mScope
                  >> command.mDest;
}


QDataStream &operator<<(QDataStream &stream, const DeleteItemsCommand &command)
{
    return stream << command.mScope;
}

QDataStream &operator>>(QDataStream &stream, DeleteItemsCommand &command)
{
    return stream >> command.mScope;
}


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
    return stream << command.mId
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


QDataStream &operator<<(QDataStream &stream, const FetchRelationsCommand &command)
{
    return stream << command.mLeft
                  << command.mRight
                  << command.mSide
                  << command.mType
                  << command.mResource;
}

QDataStream &operator>>(QDataStream &stream, FetchRelationsCommand &command)
{
    return stream >> command.mLeft
                  >> command.mRight
                  >> command.mSide
                  >> command.mType
                  >> command.mResource;
}



QDataStream &operator<<(QDataStream &stream, const FetchRelationsResponse &command)
{
    return stream << command.mLeft
                  << command.mRight
                  << command.mType
                  << command.mRemoteId;
}

QDataStream &operator>>(QDataStream &stream, FetchRelationsResponse &command)
{
    return stream >> command.mLeft
                  >> command.mRight
                  >> command.mType
                  >> command.mRemoteId;
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


