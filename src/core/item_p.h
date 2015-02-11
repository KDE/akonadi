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

#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QVarLengthArray>

#include "entity_p.h"
#include "itempayloadinternals_p.h"
#include "tag.h"

#include <memory>
#include <algorithm>
#include <cassert>
#include <vector>

namespace Akonadi {

namespace _detail {

template <typename T>
class clone_ptr {
    T *t;
public:
    clone_ptr()
        : t(0)
    {
    }
    explicit clone_ptr(T *t)
        : t(t)
    {
    }
    clone_ptr(const clone_ptr &other)
        : t(other.t ? other.t->clone() : 0)
    {
    }
    ~clone_ptr()
    {
        delete t;
    }
    clone_ptr &operator=(const clone_ptr &other)
    {
        if (this != &other) {
            clone_ptr copy(other);
            swap(copy);
        }
        return *this;
    }
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
        assert(get() != 0);
        return *get();
    }
    T *get() const
    {
        return t;
    }
    T *release()
    {
        T *const r = t;
        t = 0;
        return r;
    }
    void reset(T *other = 0)
    {
        delete t;
        t = other;
    }

private:
    struct _save_bool
    {
        void f()
        {
        };
    };
    typedef void (_save_bool::*save_bool)();
public:
    operator save_bool() const
    {
        return get() ? &_save_bool::f : 0;
    }
};

template <typename T>
inline void swap(clone_ptr<T> &lhs, clone_ptr<T> &rhs)
{
    lhs.swap(rhs);
}

template <typename T, size_t N>
class VarLengthArray {
    QVarLengthArray<T, N> impl; // ###should be replaced by self-written container that doesn't waste so much space
public:
    typedef T value_type;
    typedef T *iterator;
    typedef const T *const_iterator;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;

    explicit VarLengthArray(int size = 0)
        : impl(size)
    {
    }
    // compiler-generated dtor, copy ctor, copy assignment are ok
    // swap() makes little sense

    void push_back(const T &t)
    {
        impl.append(t);
    }
    int capacity() const
    {
        return impl.capacity();
    }
    void clear()
    {
        impl.clear();
    }
    size_t size() const
    {
        return impl.count();
    }
    bool empty() const
    {
        return impl.isEmpty();
    }
    void pop_back()
    {
        return impl.removeLast();
    }
    void reserve(size_t n)
    {
        impl.reserve(n);
    }
    void resize(size_t n)
    {
        impl.resize(n);
    }

    iterator begin()
    {
        return impl.data();
    }
    iterator end()
    {
        return impl.data() + impl.size();
    }
    const_iterator begin() const
    {
        return impl.data();
    }
    const_iterator end() const
    {
        return impl.data() + impl.size();
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    const_iterator cend() const
    {
        return end();
    }

    reference front()
    {
        return *impl.data();
    }
    reference back()
    {
        return *(impl.data() + impl.size());
    }
    const_reference front() const
    {
        return *impl.data();
    }
    const_reference back() const
    {
        return *(impl.data() + impl.size());
    }

    reference operator[](size_t n)
    {
        return impl[n];
    }
    const_reference operator[](size_t n) const
    {
        return impl[n];
    }
};

struct TypedPayload
{
    clone_ptr<PayloadBase> payload;
    int sharedPointerId;
    int metaTypeId;
};

struct BySharedPointerAndMetaTypeID : std::unary_function<TypedPayload, bool>
{
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
               && (spid == -1 || spid == tp.sharedPointerId) ;
    }
};

}

} // namespace Akonadi

namespace std {
template <>
inline void swap<Akonadi::_detail::TypedPayload>(Akonadi::_detail::TypedPayload &lhs, Akonadi::_detail::TypedPayload &rhs)
{
    lhs.payload.swap(rhs.payload);
    swap(lhs.sharedPointerId, rhs.sharedPointerId);
    swap(lhs.metaTypeId, rhs.metaTypeId);
}
}

namespace Akonadi {
//typedef _detail::VarLengthArray<_detail::TypedPayload,2> PayloadContainer;
typedef std::vector<_detail::TypedPayload> PayloadContainer;
}

// disable Q_FOREACH on PayloadContainer (b/c it likes to take copies and clone_ptr doesn't like that)
template <>
class QForeachContainer<Akonadi::PayloadContainer>
{
};

namespace Akonadi {

/**
 * @internal
 */
class ItemPrivate : public EntityPrivate
{
public:
    explicit ItemPrivate(Item::Id id = -1)
        : EntityPrivate(id)
        , mLegacyPayload()
        , mPayloads()
        , mConversionInProgress(false)
        , mRevision(-1)
        , mCollectionId(-1)
        , mSize(0)
        , mModificationTime()
        , mFlagsOverwritten(false)
        , mTagsOverwritten(false)
        , mSizeChanged(false)
        , mClearPayload(false)
    {
    }

#if 0
    ItemPrivate(const ItemPrivate &other)
        : EntityPrivate(other)
    {
        mFlags = other.mFlags;
        mRevision = other.mRevision;
        mSize = other.mSize;
        mModificationTime = other.mModificationTime;
        mMimeType = other.mMimeType;
        mLegacyPayload = other.mLegacyPayload;
        mPayloads = other.mPayloads;
        mConversionInProgress = false;
        mAddedFlags = other.mAddedFlags;
        mDeletedFlags = other.mDeletedFlags;
        mFlagsOverwritten = other.mFlagsOverwritten;
        mSizeChanged = other.mSizeChanged;
        mCollectionId = other.mCollectionId;
        mClearPayload = other.mClearPayload;
    }
#endif

    ~ItemPrivate()
    {
    }

    void resetChangeLog() Q_DECL_OVERRIDE
    {
        mFlagsOverwritten = false;
        mAddedFlags.clear();
        mDeletedFlags.clear();
        mSizeChanged = false;
        mTagsOverwritten = false;
        mAddedTags.clear();
        mDeletedTags.clear();
    }

    EntityPrivate *clone() const Q_DECL_OVERRIDE
    {
        return new ItemPrivate(*this);
    }

    bool hasMetaTypeId(int mtid) const
    {
        return std::find_if(mPayloads.begin(), mPayloads.end(),
                            _detail::BySharedPointerAndMetaTypeID(-1, mtid))
               != mPayloads.end();
    }

    PayloadBase *payloadBaseImpl(int spid, int mtid) const
    {
        const PayloadContainer::const_iterator it
            = std::find_if(mPayloads.begin(), mPayloads.end(),
                           _detail::BySharedPointerAndMetaTypeID(spid, mtid));
        return it == mPayloads.end() ? 0 : it->payload.get() ;
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
        for (PayloadContainer::iterator
             dst = mPayloads.begin() + oldSize,
             src = oPayloads.begin(), end = oPayloads.end() ; src != end ; ++src) {
            if (matcher(*src)) {
                swap(*dst, *src);
                ++dst;
            }
        }
        return numMatching > 0 ;
    }

#if 0
    std::auto_ptr<PayloadBase> takePayloadBaseImpl(int spid, int mtid)
    {
        PayloadContainer::iterator it
            = std::find_if(mPayloads.begin(), mPayloads.end(),
                           _detail::BySharedPointerAndMetaTypeID(spid, mtid));
        if (it == mPayloads.end()) {
            return std::auto_ptr<PayloadBase>();
        }
        std::rotate(it, it + 1, mPayloads.end());
        std::auto_ptr<PayloadBase> result(it->payload.release());
        mPayloads.pop_back();
        return result;
    }
#endif

    void setPayloadBaseImpl(int spid, int mtid, std::unique_ptr<PayloadBase> &p, bool add) const   /*sic!*/
    {

        if (!add) {
            mLegacyPayload.reset();
        }

        if (!p.get()) {
            if (!add) {
                mPayloads.clear();
            }
            return;
        }

        // if !add, delete all payload variants
        // (they're conversions of each other)
        mPayloads.resize(add ? mPayloads.size() + 1 : 1);
        _detail::TypedPayload &tp = mPayloads.back();
        tp.payload.reset(p.release());
        tp.sharedPointerId = spid;
        tp.metaTypeId = mtid;
    }

    void setLegacyPayloadBaseImpl(std::unique_ptr<PayloadBase> p);
    void tryEnsureLegacyPayload() const;

    mutable _detail::clone_ptr<PayloadBase> mLegacyPayload;
    mutable PayloadContainer mPayloads;
    mutable bool mConversionInProgress;
    int mRevision;
    Item::Flags mFlags;
    Tag::List mTags;
    Entity::Id mCollectionId;
    Collection::List mVirtualReferences;
    qint64 mSize;
    QDateTime mModificationTime;
    QString mMimeType;
    QString mGid;
    Item::Flags mAddedFlags;
    Item::Flags mDeletedFlags;
    Tag::List mAddedTags;
    Tag::List mDeletedTags;
    QSet<QByteArray> mCachedPayloadParts;
    bool mFlagsOverwritten : 1;
    bool mTagsOverwritten : 1;
    bool mSizeChanged : 1;
    bool mClearPayload : 1;
};

}

#endif
