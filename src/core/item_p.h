/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEM_P_H
#define AKONADI_ITEM_P_H

#include <QDateTime>

#include "itempayloadinternals_p.h"
#include "itemchangelog_p.h"
#include "tag.h"

#include <memory>
#include <algorithm>
#include <cassert>
#include <vector>

namespace Akonadi
{

namespace _detail
{

template <typename T>
class clone_ptr
{
    std::unique_ptr<T> t;
public:
    explicit clone_ptr() = default;
    explicit clone_ptr(T *t)
        : t(t)
    {}

    clone_ptr(const clone_ptr &other)
        : t(other.t ? other.t->clone() : nullptr)
    {
    }

    clone_ptr(clone_ptr &&) noexcept = default;

    ~clone_ptr() = default;

    clone_ptr &operator=(const clone_ptr &other)
    {
        if (this != &other) {
            clone_ptr copy(other);
            swap(copy);
        }
        return *this;
    }

    clone_ptr &operator=(clone_ptr &&) noexcept = default;

    void swap(clone_ptr &other)
    {
        using std::swap;
        swap(t, other.t);
    }

    T *operator->() const
    {
        return get();
    }

    T &operator*() const
    {
        assert(get() != nullptr);
        return *get();
    }

    T *get() const
    {
        return t.get();
    }

    T *release()
    {
        return t.release();
    }

    void reset(T *other = nullptr)
    {
        t.reset(other);
    }

    explicit operator bool() const noexcept
    {
        return get() != nullptr;
    }
};

template <typename T>
inline void swap(clone_ptr<T> &lhs, clone_ptr<T> &rhs) noexcept
{
    lhs.swap(rhs);
}

struct TypedPayload {
    clone_ptr<Internal::PayloadBase> payload;
    int sharedPointerId;
    int metaTypeId;
};

struct BySharedPointerAndMetaTypeID {
    const int spid;
    const int mtid;
    BySharedPointerAndMetaTypeID(int spid, int mtid)
        : spid(spid)
        , mtid(mtid)
    {
    }
    bool operator()(const TypedPayload &tp) const
    {
        return (mtid == -1 || mtid == tp.metaTypeId)
               && (spid == -1 || spid == tp.sharedPointerId);
    }
};

}

} // namespace Akonadi

namespace std
{
template <>
inline void swap<Akonadi::_detail::TypedPayload>(Akonadi::_detail::TypedPayload &lhs,
    Akonadi::_detail::TypedPayload &rhs) noexcept
{
    lhs.payload.swap(rhs.payload);
    swap(lhs.sharedPointerId, rhs.sharedPointerId);
    swap(lhs.metaTypeId, rhs.metaTypeId);
}
}

namespace Akonadi
{
typedef std::vector<_detail::TypedPayload> PayloadContainer;
}

namespace QtPrivate {
// disable Q_FOREACH on PayloadContainer (b/c it likes to take copies and clone_ptr doesn't like that)
template <>
class QForeachContainer<Akonadi::PayloadContainer>
{
};
}

namespace Akonadi
{

/**
 * @internal
 */
class ItemPrivate : public QSharedData
{
public:
    explicit ItemPrivate(Item::Id id = -1)
        : QSharedData()
        , mRevision(-1)
        , mId(id)
        , mPayloads()
        , mCollectionId(-1)
        , mSize(0)
        , mModificationTime()
        , mFlagsOverwritten(false)
        , mTagsOverwritten(false)
        , mSizeChanged(false)
        , mClearPayload(false)
        , mConversionInProgress(false)
    {
    }

    ItemPrivate(const ItemPrivate &other)
        : QSharedData(other)
    {
        mId = other.mId;
        mRemoteId = other.mRemoteId;
        mRemoteRevision = other.mRemoteRevision;
        mPayloadPath = other.mPayloadPath;
        if (other.mParent) {
            mParent.reset(new Collection(*(other.mParent)));
        }
        mFlags = other.mFlags;
        mRevision = other.mRevision;
        mTags = other.mTags;
        mRelations = other.mRelations;
        mSize = other.mSize;
        mModificationTime = other.mModificationTime;
        mMimeType = other.mMimeType;
        mPayloads = other.mPayloads;
        mFlagsOverwritten = other.mFlagsOverwritten;
        mSizeChanged = other.mSizeChanged;
        mCollectionId = other.mCollectionId;
        mClearPayload = other.mClearPayload;
        mVirtualReferences = other.mVirtualReferences;
        mGid = other.mGid;
        mCachedPayloadParts = other.mCachedPayloadParts;
        mTagsOverwritten = other.mTagsOverwritten;
        mConversionInProgress = false;

        ItemChangeLog *changelog = ItemChangeLog::instance();
        changelog->addedFlags(this) = changelog->addedFlags(&other);
        changelog->deletedFlags(this) = changelog->deletedFlags(&other);
        changelog->addedTags(this) = changelog->addedTags(&other);
        changelog->deletedTags(this) = changelog->deletedTags(&other);
        changelog->attributeStorage(this) = changelog->attributeStorage(&other);
    }

    ~ItemPrivate()
    {
        ItemChangeLog::instance()->removeItem(this);
    }

    void resetChangeLog()
    {
        mFlagsOverwritten = false;
        mSizeChanged = false;
        mTagsOverwritten = false;
        ItemChangeLog::instance()->clearItemChangelog(this);
    }

    bool hasMetaTypeId(int mtid) const
    {
        return std::any_of(mPayloads.cbegin(), mPayloads.cend(), _detail::BySharedPointerAndMetaTypeID(-1, mtid));
    }

    Internal::PayloadBase *payloadBaseImpl(int spid, int mtid) const
    {
        auto it = std::find_if(mPayloads.cbegin(), mPayloads.cend(),
                               _detail::BySharedPointerAndMetaTypeID(spid, mtid));
        return it == mPayloads.cend() ? nullptr : it->payload.get();
    }

    bool movePayloadFrom(ItemPrivate *other, int mtid) const    /*sic!*/
    {
        assert(other);
        const size_t oldSize = mPayloads.size();
        PayloadContainer &oPayloads = other->mPayloads;
        const _detail::BySharedPointerAndMetaTypeID matcher(-1, mtid);
        const size_t numMatching = std::count_if(oPayloads.begin(), oPayloads.end(), matcher);
        mPayloads.resize(oldSize + numMatching);
        using namespace std; // for swap()
        for (auto dst = mPayloads.begin() + oldSize, src = oPayloads.begin(), end = oPayloads.end(); src != end; ++src) {
            if (matcher(*src)) {
                swap(*dst, *src);
                ++dst;
            }
        }
        return numMatching > 0;
    }

    void setPayloadBaseImpl(int spid, int mtid, std::unique_ptr<Internal::PayloadBase> &p, bool add) const   /*sic!*/
    {
        if (!p.get()) {
            if (!add) {
                mPayloads.clear();
            }
            return;
        }

        // if !add, delete all payload variants
        // (they're conversions of each other)
        mPayloadPath.clear();
        mPayloads.resize(add ? mPayloads.size() + 1 : 1);
        _detail::TypedPayload &tp = mPayloads.back();
        tp.payload.reset(p.release());
        tp.sharedPointerId = spid;
        tp.metaTypeId = mtid;
    }


    // Utilise the 4-bytes padding from QSharedData
    int mRevision;
    Item::Id mId;
    QString mRemoteId;
    QString mRemoteRevision;
    mutable QString mPayloadPath;
    mutable QScopedPointer<Collection> mParent;
    mutable PayloadContainer mPayloads;
    Item::Flags mFlags;
    Tag::List mTags;
    Relation::List mRelations;
    Item::Id mCollectionId;
    Collection::List mVirtualReferences;
    // TODO: Maybe just use uint? Would save us another 8 bytes after reordering
    qint64 mSize;
    QDateTime mModificationTime;
    QString mMimeType;
    QString mGid;
    QSet<QByteArray> mCachedPayloadParts;
    bool mFlagsOverwritten : 1;
    bool mTagsOverwritten : 1;
    bool mSizeChanged : 1;
    bool mClearPayload : 1;
    mutable bool mConversionInProgress;
    // 6 bytes padding here
};

}

#endif
