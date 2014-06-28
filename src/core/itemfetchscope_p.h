/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef ITEMFETCHSCOPE_P_H
#define ITEMFETCHSCOPE_P_H

#include <QtCore/QSet>
#include <QtCore/QString>
#include <QDateTime>
#include "itemfetchscope.h"

namespace Akonadi {

/**
 * @internal
 */
class ItemFetchScopePrivate : public QSharedData
{
public:
    ItemFetchScopePrivate()
        : mAncestorDepth(ItemFetchScope::None)
        , mFullPayload(false)
        , mAllAttributes(false)
        , mCacheOnly(false)
        , mCheckCachedPayloadPartsOnly(false)
        , mFetchMtime(true)
        , mIgnoreRetrievalErrors(false)
        , mFetchRid(true)
        , mFetchGid(false)
        , mFetchTags(false)
        , mFetchVRefs(false)
    {
    }

    ItemFetchScopePrivate(const ItemFetchScopePrivate &other)
        : QSharedData(other)
    {
        mPayloadParts = other.mPayloadParts;
        mAttributes = other.mAttributes;
        mAncestorDepth = other.mAncestorDepth;
        mFullPayload = other.mFullPayload;
        mAllAttributes = other.mAllAttributes;
        mCacheOnly = other.mCacheOnly;
        mCheckCachedPayloadPartsOnly = other.mCheckCachedPayloadPartsOnly;
        mFetchMtime = other.mFetchMtime;
        mIgnoreRetrievalErrors = other.mIgnoreRetrievalErrors;
        mChangedSince = other.mChangedSince;
        mFetchRid = other.mFetchRid;
        mFetchGid = other.mFetchGid;
        mFetchTags = other.mFetchTags;
        mFetchVRefs = other.mFetchVRefs;
    }

public:
    QSet<QByteArray> mPayloadParts;
    QSet<QByteArray> mAttributes;
    ItemFetchScope::AncestorRetrieval mAncestorDepth;
    bool mFullPayload;
    bool mAllAttributes;
    bool mCacheOnly;
    bool mCheckCachedPayloadPartsOnly;
    bool mFetchMtime;
    bool mIgnoreRetrievalErrors;
    QDateTime mChangedSince;
    bool mFetchRid;
    bool mFetchGid;
    bool mFetchTags;
    bool mFetchVRefs;
};

}

#endif
